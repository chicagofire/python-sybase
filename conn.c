/******************************************************************
Copyright 2000 by Object Craft P/L, Melbourne, Australia.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Object Craft
is not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

OBJECT CRAFT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL OBJECT CRAFT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

#include "sybasect.h"

static CS_CONNECTIONObj *conn_list;

static void conn_add_object(CS_CONNECTIONObj *conn)
{
    conn->next = conn_list;
    conn_list = conn;
}

static void conn_del_object(CS_CONNECTIONObj *conn)
{
    CS_CONNECTIONObj *scan, *prev;

    for (prev = NULL, scan = conn_list; scan != NULL; scan = scan->next) {
	if (scan == conn) {
	    if (prev == NULL)
		conn_list = scan->next;
	    else
		prev->next = scan->next;
	}
    }
}

PyObject *conn_find_object(CS_CONNECTION *cs_conn)
{
    CS_CONNECTIONObj *conn;

    for (conn = conn_list; conn != NULL; conn = conn->next)
	if (conn->conn == cs_conn)
	    return (PyObject*)conn;
    return NULL;
}

static char CS_CONNECTION_ct_diag__doc__[] = 
"ct_diag(CS_INIT) -> status\n"
"ct_diag(CS_MSGLIMIT, type, num) -> status\n"
"ct_diag(CS_CLEAR, type) -> status\n"
"ct_diag(CS_GET, type, index) -> status, msg\n"
"ct_diag(CS_STATUS, type) -> status, num\n"
"ct_diag(CS_EED_CMD, type, index) -> status, cmd";

static PyObject *CS_CONNECTION_ct_diag(CS_CONNECTIONObj *self, PyObject *args)
{
    int operation, type, index, num;
    CS_VOID *buffer;
    PyObject *cmd, *msg;
    CS_RETCODE status;
    CS_COMMAND *eed;

    if (!first_tuple_int(args, &operation))
	return NULL;

    if (self->conn == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_CONNECTION has been dropped");
	return NULL;
    }

    switch (operation) {
    case CS_INIT:
	/* ct_diag(CS_INIT) -> status */
	if (!PyArg_ParseTuple(args, "i", &operation))
	    return NULL;

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = ct_diag(self->conn, operation, CS_UNUSED, CS_UNUSED, NULL);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_diag(CS_INIT) -> %s\n",
		    value_str(STATUS, status));
	if (PyErr_Occurred())
	    return NULL;

	return PyInt_FromLong(status);

    case CS_MSGLIMIT:
	/* ct_diag(CS_MSGLIMIT, type, num) -> status */
	if (!PyArg_ParseTuple(args, "iii", &operation, &type, &num))
	    return NULL;

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = ct_diag(self->conn, operation, type, CS_UNUSED, &num);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_diag(CS_MSGLIMIT, %s, %d) -> %s\n",
		    value_str(TYPE, type), num, value_str(STATUS, status));
	if (PyErr_Occurred())
	    return NULL;

	return PyInt_FromLong(status);

    case CS_CLEAR:
	/* ct_diag(CS_CLEAR, type) -> status */
	if (!PyArg_ParseTuple(args, "ii", &operation, &type))
	    return NULL;

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = ct_diag(self->conn, operation, type, CS_UNUSED, NULL);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_diag(CS_CLEAR, %s) -> %s\n",
		    value_str(TYPE, type), value_str(STATUS, status));
	if (PyErr_Occurred())
	    return NULL;

	return PyInt_FromLong(status);

    case CS_GET:
	/* ct_diag(CS_GET, type, index) -> status, msg */
	if (!PyArg_ParseTuple(args, "iii", &operation, &type, &index))
	    return NULL;
	if (type == CS_CLIENTMSG_TYPE) {
	    if ((msg = clientmsg_alloc()) == NULL)
		return NULL;
	    buffer = &((CS_CLIENTMSGObj*)msg)->msg;
	} else if (type == CS_SERVERMSG_TYPE) {
	    if ((msg = servermsg_alloc()) == NULL)
		return NULL;
	    buffer = &((CS_SERVERMSGObj*)msg)->msg;
	} else {
	    PyErr_SetString(PyExc_TypeError, "unsupported message type");
	    return NULL;
	}

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = ct_diag(self->conn, operation, type, index, buffer);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_diag(CS_GET, %s, %d) -> %s\n",
		    value_str(TYPE, type), index, value_str(STATUS, status));
	if (PyErr_Occurred())
	    return NULL;

	if (status != CS_SUCCEED)
	    return Py_BuildValue("iO", status, Py_None);
	return Py_BuildValue("iN", CS_SUCCEED, msg);

    case CS_STATUS:
	/* ct_diag(CS_STATUS, type) -> status, num */
	if (!PyArg_ParseTuple(args, "ii", &operation, &type))
	    return NULL;
	num = 0;

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = ct_diag(self->conn, operation, type, CS_UNUSED, &num);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_diag(CS_STATUS, %s) -> %s, %d\n",
		    value_str(TYPE, type), value_str(STATUS, status), num);
	if (PyErr_Occurred())
	    return NULL;

	return Py_BuildValue("ii", status, num);

    case CS_EED_CMD:
	/* ct_diag(CS_EED_CMD, type, index) -> status, cmd */
	if (!PyArg_ParseTuple(args, "iii", &operation, &type, &index))
	    return NULL;

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = ct_diag(self->conn, operation, type, index, &eed);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_diag(CS_EED_CMD, %s, %d) -> %s\n",
		    value_str(TYPE, type), index, value_str(STATUS, status));
	if (PyErr_Occurred())
	    return NULL;

	cmd = cmd_eed(self, eed);
	if (cmd == NULL)
	    return NULL;
	return Py_BuildValue("iN", status, cmd);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown operation");
	return NULL;
    }
}

static char CS_CONNECTION_ct_cancel__doc__[] = 
"ct_cancel(type) -> status";

static PyObject *CS_CONNECTION_ct_cancel(CS_CONNECTIONObj *self, PyObject *args)
{
    int type;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "i", &type))
	return NULL;

    if (self->conn == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_CONNECTION has been dropped");
	return NULL;
    }

    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = ct_cancel(self->conn, NULL, type);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_cancel(%s) -> %s\n",
		value_str(CANCEL, type), value_str(STATUS, status));
    if (PyErr_Occurred())
	return NULL;

    return PyInt_FromLong(status);
}

static char CS_CONNECTION_ct_connect__doc__[] = 
"ct_connect(str = None) - > status";

static PyObject *CS_CONNECTION_ct_connect(CS_CONNECTIONObj *self, PyObject *args)
{
    CS_RETCODE status;
    char *dsn = NULL;

    if (!PyArg_ParseTuple(args, "|s", &dsn))
	return NULL;

    if (self->conn == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_CONNECTION has been dropped");
	return NULL;
    }

    PyErr_Clear();
    if (dsn == NULL) {
	SY_BEGIN_THREADS;
	status = ct_connect(self->conn, NULL, 0);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_connect(NULL) -> %s\n",
		    value_str(STATUS, status));
    } else {
	SY_BEGIN_THREADS;
	status = ct_connect(self->conn, dsn, CS_NULLTERM);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_connect(%s) -> %s\n",
		    dsn, value_str(STATUS, status));
    }
    if (PyErr_Occurred())
	return NULL;

    return PyInt_FromLong(status);
}

static char CS_CONNECTION_ct_cmd_alloc__doc__[] = 
"ct_cmd_alloc() -> status, cmd";

static PyObject *CS_CONNECTION_ct_cmd_alloc(CS_CONNECTIONObj *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    if (self->conn == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_CONNECTION has been dropped");
	return NULL;
    }

    return cmd_alloc(self);
}

static char CS_CONNECTION_blk_alloc__doc__[] = 
"";

static PyObject *CS_CONNECTION_blk_alloc(CS_CONNECTIONObj *self, PyObject *args)
{
    int version = BLK_VERSION_100;

    if (!PyArg_ParseTuple(args, "|i", &version))
	return NULL;

    if (self->conn == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_CONNECTION has been dropped");
	return NULL;
    }

    return bulk_alloc(self, version);
}

static char CS_CONNECTION_ct_close__doc__[] = 
"ct_close([option]) - > status";

static PyObject *CS_CONNECTION_ct_close(CS_CONNECTIONObj *self, PyObject *args)
{
    CS_RETCODE status;
    int option = CS_UNUSED;

    if (!PyArg_ParseTuple(args, "|i", &option))
	return NULL;
    
    if (self->conn == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_CONNECTION has been dropped");
	return NULL;
    }

    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = ct_close(self->conn, option);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_close(%s) -> %s\n",
		value_str(OPTION, option), value_str(STATUS, status));
    if (PyErr_Occurred())
	return NULL;
    return PyInt_FromLong(status);
}

static char CS_CONNECTION_ct_con_drop__doc__[] = 
"ct_con_drop() - > status";

static PyObject *CS_CONNECTION_ct_con_drop(CS_CONNECTIONObj *self, PyObject *args)
{
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;
    
    if (self->conn == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_CONNECTION has been dropped");
	return NULL;
    }

    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = ct_con_drop(self->conn);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_con_drop() -> %s\n", value_str(STATUS, status));
    if (PyErr_Occurred())
	return NULL;

    return PyInt_FromLong(status);
}

static int property_type(int property)
{
    switch (property) {
    case CS_ANSI_BINDS:
    case CS_ASYNC_NOTIFS:
    case CS_BULK_LOGIN:
    case CS_CHARSETCNV:
    case CS_CONFIG_BY_SERVERNAME:
    case CS_DIAG_TIMEOUT:
    case CS_DISABLE_POLL:
    case CS_DS_COPY:
    case CS_DS_EXPANDALIAS:
    case CS_DS_FAILOVER:
    case CS_EXPOSE_FMTS:
    case CS_EXTERNAL_CONFIG:
    case CS_EXTRA_INF:
    case CS_HIDDEN_KEYS:
    case CS_LOGIN_STATUS:
    case CS_NOCHARSETCNV_REQD:
    case CS_SEC_APPDEFINED:
    case CS_SEC_CHALLENGE:
    case CS_SEC_CHANBIND:
    case CS_SEC_CONFIDENTIALITY:
    case CS_SEC_DATAORIGIN:
    case CS_SEC_DELEGATION:
    case CS_SEC_DETECTREPLAY:
    case CS_SEC_DETECTSEQ:
    case CS_SEC_ENCRYPTION:
    case CS_SEC_INTEGRITY:
    case CS_SEC_MUTUALAUTH:
    case CS_SEC_NEGOTIATE:
    case CS_SEC_NETWORKAUTH:
	return OPTION_BOOL;
    case CS_CON_STATUS:
#ifdef CS_LOOP_DELAY
    case CS_LOOP_DELAY:
#endif
#ifdef CS_RETRY_COUNT
    case CS_RETRY_COUNT:
#endif
    case CS_NETIO:
    case CS_TEXTLIMIT:
    case CS_DS_SEARCH:
    case CS_DS_SIZELIMIT:
    case CS_DS_TIMELIMIT:
    case CS_ENDPOINT:
    case CS_PACKETSIZE:
    case CS_SEC_CREDTIMEOUT:
    case CS_SEC_SESSTIMEOUT:
	return OPTION_INT;
    case CS_APPNAME:
    case CS_HOSTNAME:
    case CS_PASSWORD:
    case CS_SERVERNAME:
    case CS_USERNAME:
    case CS_TDS_VERSION:
    case CS_DS_DITBASE:
#ifdef CS_DS_PASSWORD
    case CS_DS_PASSWORD:
#endif
    case CS_DS_PRINCIPAL:
    case CS_DS_PROVIDER:
    case CS_SEC_KEYTAB:
    case CS_SEC_MECHANISM:
    case CS_SEC_SERVERPRINCIPAL:
    case CS_TRANSACTION_NAME:
	return OPTION_STRING;
    case CS_EED_CMD:
	return OPTION_CMD;
    default:
	return OPTION_UNKNOWN;
    }
}

static char CS_CONNECTION_ct_con_props__doc__[] = 
"ct_con_props(CS_SET, property, value) -> status\n"
"ct_con_props(CS_GET, property) -> status, value\n"
"ct_con_props(CS_CLEAR, property) -> status\n";

static PyObject *CS_CONNECTION_ct_con_props(CS_CONNECTIONObj *self, PyObject *args)
{
    int action, property;
    PyObject *obj = NULL;
    CS_RETCODE status;
    CS_INT int_value;
    CS_BOOL bool_value;
    char *str_value;
    char str_buff[10240];
    CS_INT buff_len;

    if (!first_tuple_int(args, &action))
	return NULL;

    if (self->conn == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_CONNECTION has been dropped");
	return NULL;
    }

    switch (action) {
    case CS_SET:
	/* ct_con_props(CS_SET, property, value) -> status */
	if (!PyArg_ParseTuple(args, "iiO", &action, &property, &obj))
	    return NULL;

	switch (property_type(property)) {
	case OPTION_BOOL:
	    bool_value = PyInt_AsLong(obj);
	    if (PyErr_Occurred())
		return NULL;

	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_con_props(self->conn, CS_SET, property,
				  &bool_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "ct_con_props(CS_SET, %s, %d) -> %s\n",
			value_str(PROPS, property), (int)bool_value,
			value_str(STATUS, status));
	    if (PyErr_Occurred())
		return NULL;

	    return PyInt_FromLong(status);

	case OPTION_INT:
	    int_value = PyInt_AsLong(obj);
	    if (PyErr_Occurred())
		return NULL;

	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_con_props(self->conn, CS_SET, property,
				  &int_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "ct_con_props(CS_SET, %s, %d) -> %s\n",
			value_str(PROPS, property), (int)int_value,
			value_str(STATUS, status));
	    if (PyErr_Occurred())
		return NULL;

	    return PyInt_FromLong(status);

	case OPTION_STRING:
	    str_value = PyString_AsString(obj);
	    if (PyErr_Occurred())
		return NULL;

	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_con_props(self->conn, CS_SET, property,
				  str_value, CS_NULLTERM, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "ct_con_props(CS_SET, %s, '%s') -> %s\n",
			value_str(PROPS, property), str_value,
			value_str(STATUS, status));
	    if (PyErr_Occurred())
		return NULL;

	    return PyInt_FromLong(status);

	default:
	    PyErr_SetString(PyExc_TypeError, "unhandled property value");
	    return NULL;
	}
	break;

    case CS_GET:
	/* ct_con_props(CS_GET, property) -> status, value */
	if (!PyArg_ParseTuple(args, "ii", &action, &property))
	    return NULL;

	switch (property_type(property)) {
	case OPTION_BOOL:
	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_con_props(self->conn, CS_GET, property,
				  &bool_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "ct_con_props(CS_GET, %s) -> %s, %d\n",
			value_str(PROPS, property),
			value_str(STATUS, status), (int)bool_value);
	    if (PyErr_Occurred())
		return NULL;

	    return Py_BuildValue("ii", status, bool_value);

	case OPTION_INT:
	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_con_props(self->conn, CS_GET, property,
				  &int_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug) {
		if (property == CS_CON_STATUS || property == CS_LOGIN_STATUS)
		    fprintf(stderr, "ct_con_props(CS_GET, %s) -> %s, %s\n",
			    value_str(PROPS, property),
			    value_str(STATUS, status),
			    mask_str(CONSTAT, (int)int_value));
		else
		    fprintf(stderr, "ct_con_props(CS_GET, %s) -> %s, %d\n",
			    value_str(PROPS, property),
			    value_str(STATUS, status), (int)int_value);
	    }
	    if (PyErr_Occurred())
		return NULL;

	    return Py_BuildValue("ii", status, int_value);

	case OPTION_STRING:
	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_con_props(self->conn, CS_GET, property,
				  str_buff, sizeof(str_buff), &buff_len);
	    SY_END_THREADS;
	    if (buff_len > sizeof(str_buff))
		buff_len = sizeof(str_buff);
	    if (self->debug)
		fprintf(stderr, "ct_con_props(CS_GET, %s) -> %s, '%.*s'\n",
			value_str(PROPS, property),
			value_str(STATUS, status), (int)buff_len, str_buff);
	    if (PyErr_Occurred())
		return NULL;

	    return Py_BuildValue("is#", status, str_buff, buff_len);

	case OPTION_CMD:
	    PyErr_SetString(PyExc_TypeError, "EED not supported yet");
	    return NULL;

	case OPTION_UNKNOWN:
	    PyErr_SetString(PyExc_TypeError, "unknown property value");
	    return NULL;

	default:
	    PyErr_SetString(PyExc_TypeError, "unhandled property value");
	    return NULL;
	}
	break;

    case CS_CLEAR:
	/* ct_con_props(CS_CLEAR, property) -> status */
	if (!PyArg_ParseTuple(args, "ii", &action, &property))
	    return NULL;

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = ct_con_props(self->conn, CS_CLEAR, property,
			      NULL, CS_UNUSED, NULL);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_con_props(CS_CLEAR, %s) -> %s\n",
		    value_str(PROPS, property), value_str(STATUS, status));
	if (PyErr_Occurred())
	    return NULL;

	return PyInt_FromLong(status);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown action");
	return NULL;
    }
}

static int option_type(int option)
{
    switch (option) {
    case CS_OPT_ANSINULL:
    case CS_OPT_ANSIPERM:
    case CS_OPT_ARITHABORT:
    case CS_OPT_ARITHIGNORE:
    case CS_OPT_CHAINXACTS:
    case CS_OPT_CURCLOSEONXACT:
    case CS_OPT_FIPSFLAG:
    case CS_OPT_FORCEPLAN:
    case CS_OPT_FORMATONLY:
    case CS_OPT_GETDATA:
    case CS_OPT_NOCOUNT:
    case CS_OPT_NOEXEC:
    case CS_OPT_PARSEONLY:
    case CS_OPT_QUOTED_IDENT:
    case CS_OPT_RESTREES:
    case CS_OPT_SHOWPLAN:
    case CS_OPT_STATS_IO:
    case CS_OPT_STATS_TIME:
    case CS_OPT_STR_RTRUNC:
    case CS_OPT_TRUNCIGNORE:
	return OPTION_BOOL;
    case CS_OPT_DATEFIRST:
    case CS_OPT_DATEFORMAT:
    case CS_OPT_ISOLATION:
    case CS_OPT_ROWCOUNT:
    case CS_OPT_TEXTSIZE:
	return OPTION_INT;
    case CS_OPT_AUTHOFF:
    case CS_OPT_AUTHON:
    case CS_OPT_CURREAD:
    case CS_OPT_CURWRITE:
    case CS_OPT_IDENTITYOFF:
    case CS_OPT_IDENTITYON:
	return OPTION_STRING;
    default:
	return OPTION_UNKNOWN;
    }
}

static char CS_CONNECTION_ct_options__doc__[] = 
"ct_options(CS_SET, option, value) -> status\n"
"ct_options(CS_GET, option) -> status, value\n"
"ct_options(CS_CLEAR, option) -> status\n";

static PyObject *CS_CONNECTION_ct_options(CS_CONNECTIONObj *self, PyObject *args)
{
    int action, option;
    PyObject *obj = NULL;
    CS_RETCODE status;
    CS_INT int_value;
    CS_BOOL bool_value;
    char *str_value;
    char str_buff[10240];
    CS_INT buff_len;

    if (!first_tuple_int(args, &action))
	return NULL;

    if (self->conn == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_CONNECTION has been dropped");
	return NULL;
    }

    switch (action) {
    case CS_SET:
	/* ct_options(CS_SET, option, value) -> status */
	if (!PyArg_ParseTuple(args, "iiO", &action, &option, &obj))
	    return NULL;

	switch (option_type(option)) {
	case OPTION_BOOL:
	    bool_value = PyInt_AsLong(obj);
	    if (PyErr_Occurred())
		return NULL;

	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_options(self->conn, CS_SET, option,
				&bool_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "ct_options(CS_SET, %s, %d) -> %s\n",
			value_str(OPTION, option), (int)bool_value,
			value_str(STATUS, status));
	    if (PyErr_Occurred())
		return NULL;

	    return PyInt_FromLong(status);

	case OPTION_INT:
	    int_value = PyInt_AsLong(obj);
	    if (PyErr_Occurred())
		return NULL;

	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_options(self->conn, CS_SET, option,
				&int_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "ct_options(CS_SET, %s, %d) -> %s\n",
			value_str(OPTION, option), (int)int_value,
			value_str(STATUS, status));
	    if (PyErr_Occurred())
		return NULL;

	    return PyInt_FromLong(status);

	case OPTION_STRING:
	    str_value = PyString_AsString(obj);
	    if (PyErr_Occurred())
		return NULL;

	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_options(self->conn, CS_SET, option,
				str_value, CS_NULLTERM, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "ct_options(CS_SET, %s, '%s') -> %s\n",
			value_str(OPTION, option), str_value,
			value_str(STATUS, status));
	    if (PyErr_Occurred())
		return NULL;

	    return PyInt_FromLong(status);

	default:
	    PyErr_SetString(PyExc_TypeError, "unhandled option value");
	    return NULL;
	}
	break;

    case CS_GET:
	/* ct_options(CS_GET, option) -> status, value */
	if (!PyArg_ParseTuple(args, "ii", &action, &option))
	    return NULL;

	switch (option_type(option)) {
	case OPTION_BOOL:
	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_options(self->conn, CS_GET, option,
				&bool_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "ct_options(CS_GET, %s) -> %s, %d\n",
			value_str(OPTION, option),
			value_str(STATUS, status), (int)bool_value);
	    if (PyErr_Occurred())
		return NULL;

	    return Py_BuildValue("ii", status, bool_value);

	case OPTION_INT:
	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_options(self->conn, CS_GET, option,
				&int_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "ct_options(CS_GET, %s) -> %s, %d\n",
			value_str(OPTION, option),
			value_str(STATUS, status), (int)int_value);
	    if (PyErr_Occurred())
		return NULL;

	    return Py_BuildValue("ii", status, int_value);

	case OPTION_STRING:
	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = ct_options(self->conn, CS_GET, option,
				str_buff, sizeof(str_buff), &buff_len);
	    SY_END_THREADS;
	    if (buff_len > sizeof(str_buff))
		buff_len = sizeof(str_buff);
	    if (self->debug)
		fprintf(stderr, "ct_options(CS_GET, %s) -> %s, '%.*s'\n",
			value_str(OPTION, option),
			value_str(STATUS, status), (int)buff_len, str_buff);
	    if (PyErr_Occurred())
		return NULL;

	    return Py_BuildValue("is#", status, str_buff, buff_len);

	case OPTION_UNKNOWN:
	    PyErr_SetString(PyExc_TypeError, "unknown option value");
	    return NULL;

	default:
	    PyErr_SetString(PyExc_TypeError, "unhandled option value");
	    return NULL;
	}
	break;

    case CS_CLEAR:
	/* ct_options(CS_CLEAR, property) -> status */
	if (!PyArg_ParseTuple(args, "ii", &action, &option))
	    return NULL;

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = ct_options(self->conn, CS_CLEAR, option,
			    NULL, CS_UNUSED, NULL);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_options(CS_CLEAR, %s) -> %s\n",
		    value_str(OPTION, option), value_str(STATUS, status));
	if (PyErr_Occurred())
	    return NULL;

	return PyInt_FromLong(status);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown action");
	return NULL;
    }
}

static struct PyMethodDef CS_CONNECTION_methods[] = {
    { "ct_diag", (PyCFunction)CS_CONNECTION_ct_diag, METH_VARARGS, CS_CONNECTION_ct_diag__doc__ },
    { "ct_cancel", (PyCFunction)CS_CONNECTION_ct_cancel, METH_VARARGS, CS_CONNECTION_ct_cancel__doc__ },
    { "ct_connect", (PyCFunction)CS_CONNECTION_ct_connect, METH_VARARGS, CS_CONNECTION_ct_connect__doc__ },
    { "ct_cmd_alloc", (PyCFunction)CS_CONNECTION_ct_cmd_alloc, METH_VARARGS, CS_CONNECTION_ct_cmd_alloc__doc__ },
    { "blk_alloc", (PyCFunction)CS_CONNECTION_blk_alloc, METH_VARARGS, CS_CONNECTION_blk_alloc__doc__ },
    { "ct_close", (PyCFunction)CS_CONNECTION_ct_close, METH_VARARGS, CS_CONNECTION_ct_close__doc__ },
    { "ct_con_drop", (PyCFunction)CS_CONNECTION_ct_con_drop, METH_VARARGS, CS_CONNECTION_ct_con_drop__doc__ },
    { "ct_con_props", (PyCFunction)CS_CONNECTION_ct_con_props, METH_VARARGS, CS_CONNECTION_ct_con_props__doc__ },
    { "ct_options", (PyCFunction)CS_CONNECTION_ct_options, METH_VARARGS, CS_CONNECTION_ct_options__doc__ },
    { NULL }			/* sentinel */
};

PyObject *conn_alloc(CS_CONTEXTObj *ctx)
{
    CS_CONNECTIONObj *self;
    CS_RETCODE status;
    CS_CONNECTION *conn;

    self = PyObject_NEW(CS_CONNECTIONObj, &CS_CONNECTIONType);
    if (self == NULL)
	return NULL;

    SY_LEAK_REG(self);
    self->conn = NULL;
    self->ctx = NULL;
    self->strip = 0;
    self->debug = ctx->debug;

    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = ct_con_alloc(ctx->ctx, &conn);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_con_alloc() -> %s\n", value_str(STATUS, status));
    if (PyErr_Occurred()) {
	Py_DECREF(self);
	return NULL;
    }

    if (status != CS_SUCCEED) {
	Py_DECREF(self);
	return Py_BuildValue("iO", status, Py_None);
    }

    self->conn = conn;
    self->ctx = ctx;
    Py_INCREF(self->ctx);
    conn_add_object(self);
    return Py_BuildValue("iN", CS_SUCCEED, self);
}

static void CS_CONNECTION_dealloc(CS_CONNECTIONObj *self)
{
    SY_LEAK_UNREG(self);
    if (self->conn) {
	/* should check return == CS_SUCCEED, but we can't handle failure
	   here */
	CS_RETCODE status;

	status = ct_con_drop(self->conn);
	if (self->debug)
	    fprintf(stderr, "ct_con_drop() -> %s\n",
		    value_str(STATUS, status));
    }
    Py_XDECREF(self->ctx);
    conn_del_object(self);
    PyMem_DEL(self);
}

#define OFF(x) offsetof(CS_CONNECTIONObj, x)

static struct memberlist CS_CONNECTION_memberlist[] = {
    { "ctx", T_OBJECT, OFF(ctx), RO },
    { "strip", T_INT, OFF(strip) },
    { "debug", T_INT, OFF(debug) },
    { NULL }			/* Sentinel */
};

static PyObject *CS_CONNECTION_getattr(CS_CONNECTIONObj *self, char *name)
{
    PyObject *rv;

    rv = PyMember_Get((char *)self, CS_CONNECTION_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(CS_CONNECTION_methods, (PyObject *)self, name);
}

static int CS_CONNECTION_setattr(CS_CONNECTIONObj *self, char *name, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
	return -1;
    }
    return PyMember_Set((char *)self, CS_CONNECTION_memberlist, name, v);
}

static char CS_CONNECTIONType__doc__[] = 
"Wrap the Sybase CS_CONNECTION structure and associated functionality.";

PyTypeObject CS_CONNECTIONType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "ConnectionType",		/*tp_name*/
    sizeof(CS_CONNECTIONObj),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)CS_CONNECTION_dealloc,/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)CS_CONNECTION_getattr,	/*tp_getattr*/
    (setattrfunc)CS_CONNECTION_setattr,	/*tp_setattr*/
    (cmpfunc)0,			/*tp_compare*/
    (reprfunc)0,		/*tp_repr*/
    0,				/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L, 0L, 0L, 0L,
    CS_CONNECTIONType__doc__	/* Documentation string */
};
