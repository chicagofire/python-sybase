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

static char CS_COMMAND_ct_bind__doc__[] = 
"ct_bind(int, datafmt) -> status, buf";

static PyObject *CS_COMMAND_ct_bind(CS_COMMANDObj *self, PyObject *args)
{
    CS_INT item;
    CS_DATAFMTObj *datafmt;
    DataBufObj *databuf;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "iO!", &item, &CS_DATAFMTType, &datafmt))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    databuf = (DataBufObj *)databuf_alloc((PyObject*)datafmt);
    if (databuf == NULL)
	return NULL;

    SY_BEGIN_THREADS;
    status = ct_bind(self->cmd, item, &datafmt->fmt,
		     databuf->buff, databuf->copied, databuf->indicator);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_bind(%d) -> %s\n",
		(int)item, value_str(STATUS, status));
    return Py_BuildValue("iN", status, databuf);
}

static char CS_COMMAND_ct_cancel__doc__[] = 
"ct_cancel(type) -> status";

static PyObject *CS_COMMAND_ct_cancel(CS_COMMANDObj *self, PyObject *args)
{
    int type;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "i", &type))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    SY_BEGIN_THREADS;
    status = ct_cancel(NULL, self->cmd, type);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_cancel(%s) -> %s\n",
		value_str(CANCEL, type), value_str(STATUS, status));

    return PyInt_FromLong(status);
}

static char CS_COMMAND_ct_cmd_drop__doc__[] = 
"ct_cmd_drop() -> status";

static PyObject *CS_COMMAND_ct_cmd_drop(CS_COMMANDObj *self, PyObject *args)
{
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    SY_BEGIN_THREADS;
    status = ct_cmd_drop(self->cmd);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_cmd_drop() -> %s\n", value_str(STATUS, status));

    if (status == CS_SUCCEED)
	self->cmd = NULL;

    return PyInt_FromLong(status);
}

static char CS_COMMAND_ct_command__doc__[] = 
"ct_command(CS_LANG_CMD, sql [,option]) -> status\n"
"ct_command(CS_MSG_CMD, int) -> status\n"
"ct_command(CS_PACKAGE_CMD, name) -> status\n"
"ct_command(CS_RPC_CMD, name [,option]) -> status\n"
"ct_command(CS_SEND_DATA_CMD) -> status";

static PyObject *CS_COMMAND_ct_command(CS_COMMANDObj *self, PyObject *args)
{
    int type;
    char *databuf;
    CS_INT num;
    CS_INT option = CS_UNUSED;
    CS_RETCODE status;
    char *type_str = NULL;

    if (!first_tuple_int(args, &type))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    switch (type) {
    case CS_LANG_CMD:
	/* ct_command(CS_LANG_CMD, sql [,option]) -> status */
	type_str = "CS_LANG_CMD";
    case CS_RPC_CMD:
	/* ct_command(CS_RPC_CMD, name [,option]) -> status */
	if (type_str == NULL)
	    type_str = "CS_RPC_CMD";
	if (!PyArg_ParseTuple(args, "is|i", &type, &databuf, &option))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_command(self->cmd, type, databuf, CS_NULLTERM, option);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_command(%s, %s, %s) -> %s\n",
		    type_str, databuf, value_str(CMD, option),
		    value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_MSG_CMD:
	/* ct_command(CS_MSG_CMD, int) -> status */
	if (!PyArg_ParseTuple(args, "ii", &type, &num))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_command(self->cmd, type, (CS_VOID*)&num, CS_UNUSED, CS_UNUSED);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_command(CS_MSG_CMD, %d) -> %s\n",
		    (int)num, value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_PACKAGE_CMD:
	/* ct_command(CS_PACKAGE_CMD, name) -> status */
	if (!PyArg_ParseTuple(args, "is", &type, &databuf))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_command(self->cmd, type, databuf, CS_NULLTERM, CS_UNUSED);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_command(CS_PACKAGE_CMD, %s) -> %s\n",
		    databuf, value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_SEND_DATA_CMD:
	/* ct_command(CS_SEND_DATA_CMD) -> status */
	if (!PyArg_ParseTuple(args, "i", &type))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_command(self->cmd, type, NULL, CS_UNUSED, CS_COLUMN_DATA);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_command(CS_SEND_DATA_CMD) -> %s\n",
		    value_str(STATUS, status));
	return PyInt_FromLong(status);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown type");
	return NULL;
    }
}

static char CS_COMMAND_ct_cursor__doc__[] = 
"ct_cursor(CS_CURSOR_DECLARE, cursor_id, sql [,options]) -> status\n"
"ct_cursor(CS_CURSOR_UPDATE, table, sql [,options]) -> status\n"
"ct_cursor(CS_CURSOR_OPTION [,options]) -> status\n"
"ct_cursor(CS_CURSOR_ROWS, int) -> status\n"
"ct_cursor(CS_CURSOR_OPEN [,options]) -> status\n"
"ct_cursor(CS_CURSOR_DELETE, table) -> status\n"
"ct_cursor(CS_CURSOR_CLOSE [,options]) -> status\n"
"ct_cursor(CS_CURSOR_DEALLOC) -> status\n";

static PyObject *CS_COMMAND_ct_cursor(CS_COMMANDObj *self, PyObject *args)
{
    int type;
    char *name, *text;
    CS_INT option = CS_UNUSED;
    CS_RETCODE status;
    char *type_str = NULL;

    if (!first_tuple_int(args, &type))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    switch (type) {
    case CS_CURSOR_DECLARE:
	/* ct_cursor(CS_CURSOR_DECLARE, cursor_id, sql [,options]) -> status */
	type_str = "CS_CURSOR_DECLARE";
    case CS_CURSOR_UPDATE:
	/* ct_cursor(CS_CURSOR_UPDATE, table, sql [,options]) -> status */
	if (type_str == NULL)
	    type_str = "CS_CURSOR_UPDATE";
	if (!PyArg_ParseTuple(args, "iss|i", &type, &name, &text, &option))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_cursor(self->cmd, type,
			   name, CS_NULLTERM, text, CS_NULLTERM, option);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_cursor(%s, %s, %s, %s) -> %s\n",
		    type_str, name, text, value_str(CURSOROPT, option),
		    value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_CURSOR_OPTION:
	/* ct_cursor(CS_CURSOR_OPTION [,options]) -> status */
	type_str = "CS_CURSOR_OPTION";
    case CS_CURSOR_OPEN:
	/* ct_cursor(CS_CURSOR_OPEN [,options]) -> status */
	if (type_str == NULL)
	    type_str = "CS_CURSOR_OPEN";
    case CS_CURSOR_CLOSE:
	/* ct_cursor(CS_CURSOR_CLOSE [,options]) -> status */
	if (type_str == NULL)
	    type_str = "CS_CURSOR_CLOSE";
	if (!PyArg_ParseTuple(args, "i|i", &type, &option))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_cursor(self->cmd, type,
			   NULL, CS_UNUSED, NULL, CS_UNUSED, option);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_cursor(%s, %s) -> %s\n",
		    type_str, value_str(CURSOROPT, option),
		    value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_CURSOR_ROWS:
	/* ct_cursor(CS_CURSOR_ROWS, int) -> status */
	if (!PyArg_ParseTuple(args, "ii", &type, &option))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_cursor(self->cmd, type,
			   NULL, CS_UNUSED, NULL, CS_UNUSED, option);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_cursor(CS_CURSOR_ROWS, %s) -> %s\n",
		    value_str(CURSOROPT, option), value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_CURSOR_DELETE:
	/* ct_cursor(CS_CURSOR_DELETE, table) -> status */
	if (!PyArg_ParseTuple(args, "is", &type, &name))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_cursor(self->cmd, type,
			   name, CS_NULLTERM, NULL, CS_UNUSED, CS_UNUSED);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_cursor(CS_CURSOR_DELETE, %s) -> %s\n",
		    name, value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_CURSOR_DEALLOC:
	/* ct_cursor(CS_CURSOR_DEALLOC) -> status */
	if (!PyArg_ParseTuple(args, "i", &type))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_cursor(self->cmd, type,
			   NULL, CS_UNUSED, NULL, CS_UNUSED, CS_UNUSED);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_cursor(CS_CURSOR_DEALLOC) -> %s\n",
		    value_str(STATUS, status));
	return PyInt_FromLong(status);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown type");
	return NULL;
    }
}

static char CS_COMMAND_ct_data_info__doc__[] = 
"ct_data_info(CS_SET, iodesc) -> status\n"
"ct_data_info(CS_GET, num) -> status, iodesc";

static PyObject *CS_COMMAND_ct_data_info(CS_COMMANDObj *self, PyObject *args)
{
    int action;
    CS_INT num;
    CS_IODESC iodesc;
    CS_IODESCObj *desc;
    CS_RETCODE status;

    if (!first_tuple_int(args, &action))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    switch (action) {
    case CS_SET:
	/* ct_data_info(CS_SET, int, iodesc) -> status */
	if (!PyArg_ParseTuple(args, "iO!",
			      &action, &CS_IODESCType, &desc))
	    return NULL;
	status = ct_data_info(self->cmd, CS_SET, CS_UNUSED, &desc->iodesc);
	if (self->debug)
	    fprintf(stderr, "ct_data_info(CS_SET) -> %s\n",
		    value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_GET:
	/* ct_data_info(CS_GET, int) -> status, iodesc */
	if (!PyArg_ParseTuple(args, "ii", &action, &num))
	    return NULL;
	memset(&iodesc, 0, sizeof(iodesc));
	status = ct_data_info(self->cmd, CS_GET, num, &iodesc);
	if (self->debug)
	    fprintf(stderr, "ct_data_info(CS_GET, %d) -> %s\n",
		    (int)num, value_str(STATUS, status));
	if (status != CS_SUCCEED)
	    return Py_BuildValue("iO", status, Py_None);
	desc = (CS_IODESCObj*)iodesc_alloc(&iodesc);
	if (desc == NULL)
	    return NULL;
	return Py_BuildValue("iN", status, desc);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown action");
	return NULL;
    }
}

static char CS_COMMAND_ct_describe__doc__[] = 
"ct_describe(int) -> status, datafmt";

static PyObject *CS_COMMAND_ct_describe(CS_COMMANDObj *self, PyObject *args)
{
    CS_INT num;
    CS_DATAFMT datafmt;
    PyObject *fmt;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "i", &num))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    memset(&datafmt, 0, sizeof(datafmt));
    SY_BEGIN_THREADS;
    status = ct_describe(self->cmd, num, &datafmt);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_describe(%d) -> %s\n",
		(int)num, value_str(STATUS, status));

    if (status != CS_SUCCEED)
	return Py_BuildValue("iO", status, Py_None);

    fmt = datafmt_alloc(&datafmt, self->strip);
    if (fmt == NULL)
	return NULL;

    return Py_BuildValue("iN", status, fmt);
}

static char CS_COMMAND_ct_dynamic__doc__[] = 
"ct_dynamic(CS_CURSOR_DECLARE, dyn_id, cursor_id) -> status\n"
"ct_dynamic(CS_DEALLOC, dyn_id) -> status\n"
"ct_dynamic(CS_DESCRIBE_INPUT, dyn_id) -> status\n"
"ct_dynamic(CS_DESCRIBE_OUTPUT, dyn_id) -> status\n"
"ct_dynamic(CS_EXECUTE, dyn_id) -> status\n"
"ct_dynamic(CS_EXEC_IMMEDIATE, sql) -> status\n"
"ct_dynamic(CS_PREPARE, dyn_id, sql) -> status";

static PyObject *CS_COMMAND_ct_dynamic(CS_COMMANDObj *self, PyObject *args)
{
    int type;
    char *id, *buff;
    char *cmd_str = NULL;
    CS_RETCODE status;

    if (!first_tuple_int(args, &type))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    switch (type) {
    case CS_CURSOR_DECLARE:
	/* ct_dynamic(CS_CURSOR_DECLARE, dyn_id, cursor_id) -> status */
	cmd_str = "CS_CURSOR_DECLARE";
    case CS_PREPARE:
	/* ct_dynamic(CS_PREPARE, dyn_id, sql) -> status */
	if (cmd_str == NULL)
	    cmd_str = "CS_PREPARE";
	if (!PyArg_ParseTuple(args, "iss", &type, &id, &buff))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_dynamic(self->cmd, type,
			    id, CS_NULLTERM, buff, CS_NULLTERM);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_dynamic(%s, %s, %s) -> %s\n",
		    cmd_str, id, buff, value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_DEALLOC:
	/* ct_dynamic(CS_DEALLOC, dyn_id) -> status */
	cmd_str = "CS_DEALLOC";
    case CS_DESCRIBE_INPUT:
	/* ct_dynamic(CS_DESCRIBE_INPUT, dyn_id) -> status */
	if (cmd_str == NULL)
	    cmd_str = "CS_DESCRIBE_INPUT";
    case CS_DESCRIBE_OUTPUT:
	/* ct_dynamic(CS_DESCRIBE_OUTPUT, dyn_id) -> status */
	if (cmd_str == NULL)
	    cmd_str = "CS_DESCRIBE_OUTPUT";
    case CS_EXECUTE:
	/* ct_dynamic(CS_EXECUTE, dyn_id) -> status */
	if (cmd_str == NULL)
	    cmd_str = "CS_EXECUTE";
	if (!PyArg_ParseTuple(args, "is", &type, &id))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_dynamic(self->cmd, type,
			    id, CS_NULLTERM, NULL, CS_UNUSED);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_dynamic(%s, %s) -> %s\n",
		    cmd_str, id, value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_EXEC_IMMEDIATE:
	/* ct_dynamic(CS_EXEC_IMMEDIATE, sql) -> status */
	if (!PyArg_ParseTuple(args, "is", &type, &buff))
	    return NULL;

	SY_BEGIN_THREADS;
	status = ct_dynamic(self->cmd, type,
			    NULL, CS_UNUSED, buff, CS_NULLTERM);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_dynamic(CS_EXEC_IMMEDIATE, %s) -> %s\n",
		    buff, value_str(STATUS, status));
	return PyInt_FromLong(status);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown dynamic command");
	return NULL;
    }
}

static char CS_COMMAND_ct_fetch__doc__[] = 
"ct_fetch() -> result, rows_read";

static PyObject *CS_COMMAND_ct_fetch(CS_COMMANDObj *self, PyObject *args)
{
    CS_RETCODE status;
    CS_INT rows_read = 0;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    SY_BEGIN_THREADS;
    status = ct_fetch(self->cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &rows_read);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_fetch() -> %s, %d\n",
		value_str(STATUS, status), (int)rows_read);

    return Py_BuildValue("ii", status, rows_read);
}

static char CS_COMMAND_ct_get_data__doc__[] = 
"ct_get_data(num, buf) -> result, len";

static PyObject *CS_COMMAND_ct_get_data(CS_COMMANDObj *self, PyObject *args)
{
    DataBufObj *databuf;
    int num;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "iO!", &num, &DataBufType, &databuf))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    SY_BEGIN_THREADS;
    status = ct_get_data(self->cmd, (CS_INT)num,
			 databuf->buff, databuf->fmt.maxlength,
			 &databuf->copied[0]);
    databuf->indicator[0] = 0;
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_get_data(%d) -> %s, %d\n",
		num, value_str(STATUS, status), (int)databuf->copied[0]);

    return Py_BuildValue("ii", status, databuf->copied[0]);
}

static char CS_COMMAND_ct_param__doc__[] = 
"ct_param(buf) -> status";

static PyObject *CS_COMMAND_ct_param(CS_COMMANDObj *self, PyObject *args)
{
    PyObject *obj;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "O", &obj))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    /* FIXME: Need to handle CS_UPDATECOL variant */
    if (DataBuf_Check(obj)) {
	DataBufObj *databuf = (DataBufObj *)obj;

	SY_BEGIN_THREADS;
	status = ct_param(self->cmd, &databuf->fmt,
			  databuf->buff, databuf->copied[0],
			  databuf->indicator[0]);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_param(buf) -> %s\n",
		    value_str(STATUS, status));
    } else if (CS_DATAFMT_Check(obj)) {
	CS_DATAFMTObj *datafmt = (CS_DATAFMTObj *)obj;

	SY_BEGIN_THREADS;
	status = ct_param(self->cmd, &datafmt->fmt,
			  NULL, CS_UNUSED, CS_UNUSED);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_param(fmt) -> %s\n",
		    value_str(STATUS, status));
    } else {
	PyErr_SetString(PyExc_TypeError, "expect CS_DATAFMT or DataBuf");
	return NULL;
	
    }
    return PyInt_FromLong(status);
}

static PyObject *build_int_list(CS_INT *values, int len)
{
    int i;			/* iterate over table columns */
    PyObject *list;		/* list containing all columns */

    list = PyList_New(len);
    if (list == NULL)
	return NULL;

    for (i = 0; i < len; i++) {
	PyObject *num;

	num = PyInt_FromLong(values[i]);
	if (num == NULL) {
	    Py_DECREF(list);
	    return NULL;
	}
	if (PyList_SetItem(list, i, num) != 0) {
	    Py_DECREF(list);
	    return NULL;
	}
    }

    return list;
}

static char CS_COMMAND_ct_res_info__doc__[] = 
"ct_res_info(CS_BROWSE_INFO) -> status, bool\n"
"ct_res_info(CS_CMD_NUMBER) -> status, int\n"
"ct_res_info(CS_MSGTYPE) -> status, int\n"
"ct_res_info(CS_NUM_COMPUTES) -> status, int\n"
"ct_res_info(CS_NUMDATA) -> status, int\n"
"ct_res_info(CS_NUMORDER_COLS) -> status, int\n"
"ct_res_info(CS_ORDERBY_COLS) -> status, list of int\n"
"ct_res_info(CS_ROW_COUNT) -> status, int\n"
"ct_res_info(CS_TRANS_STATE) -> status, int";

static PyObject *CS_COMMAND_ct_res_info(CS_COMMANDObj *self, PyObject *args)
{
    int type;
    CS_RETCODE status;
    CS_INT *col_nums;
    CS_INT int_val;
    CS_BOOL bool_val;
    CS_USHORT ushort_val;
    PyObject *list;
    char *type_str = NULL;

    if (!PyArg_ParseTuple(args, "i", &type))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    switch (type) {
    case CS_BROWSE_INFO:
	/* ct_res_info(CS_BROWSE_INFO) -> status, bool */
	SY_BEGIN_THREADS;
	status = ct_res_info(self->cmd, type, &bool_val, CS_UNUSED, NULL);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_res_info(CS_BROWSE_INFO) -> %s, %d\n",
		    value_str(STATUS, status), (int)bool_val);
	return Py_BuildValue("ii", status, bool_val);

    case CS_MSGTYPE:
	/* ct_res_info(CS_MSGTYPE) -> status, int */
	SY_BEGIN_THREADS;
	status = ct_res_info(self->cmd, type, &ushort_val, CS_UNUSED, NULL);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_res_info(CS_MSGTYPE) -> %s, %d\n",
		    value_str(STATUS, status), ushort_val);
	return Py_BuildValue("ii", status, ushort_val);

    case CS_CMD_NUMBER:
	/* ct_res_info(CS_CMD_NUMBER) -> status, int */
	type_str = "CS_CMD_NUMBER";
    case CS_NUM_COMPUTES:
	/* ct_res_info(CS_NUM_COMPUTES) -> status, int */
	if (type_str == NULL)
	    type_str = "CS_NUM_COMPUTES";
    case CS_NUMDATA:
	/* ct_res_info(CS_NUMDATA) -> status, int */
	if (type_str == NULL)
	    type_str = "CS_NUMDATA";
    case CS_NUMORDERCOLS:
	/* ct_res_info(CS_NUMORDER_COLS) -> status, int */
	if (type_str == NULL)
	    type_str = "CS_NUMORDER_COLS";
    case CS_ROW_COUNT:
	/* ct_res_info(CS_ROW_COUNT) -> status, int */
	if (type_str == NULL)
	    type_str = "CS_ROW_COUNT";
    case CS_TRANS_STATE:
	/* ct_res_info(CS_TRANS_STATE) -> status, int */
	if (type_str == NULL)
	    type_str = "CS_TRANS_STATE";
	SY_BEGIN_THREADS;
	status = ct_res_info(self->cmd, type, &int_val, CS_UNUSED, NULL);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_res_info(%s) -> %s, %d\n",
		    type_str, value_str(STATUS, status), (int)int_val);
	return Py_BuildValue("ii", status, int_val);

    case CS_ORDERBY_COLS:
	/* ct_res_info(CS_ORDERBY_COLS) -> status, list of int */
	SY_BEGIN_THREADS;
	status = ct_res_info(self->cmd, CS_NUMORDERCOLS, &int_val, CS_UNUSED, NULL);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_res_info(CS_NUMORDERCOLS) -> %s, %d\n",
		    value_str(STATUS, status), (int)int_val);
	if (status != CS_SUCCEED)
	    return Py_BuildValue("iO", status, Py_None);

	if (int_val <= 0)
	    return Py_BuildValue("i[]", status);

	col_nums = malloc(sizeof(*col_nums) * int_val);
	if (col_nums == NULL)
	    return PyErr_NoMemory();

	SY_BEGIN_THREADS;
	status = ct_res_info(self->cmd, CS_ORDERBY_COLS,
			     col_nums, sizeof(*col_nums) * int_val, NULL);
	SY_END_THREADS;
	if (self->debug) {
	    int i;

	    fprintf(stderr, "ct_res_info(CS_ORDERBY_COLS) -> %s, [",
		    value_str(STATUS, status));
	    for (i = 0; i < int_val; i++) {
		if (i > 0)
		    fprintf(stderr, ",");
		fprintf(stderr, "%d", (int)col_nums[i]);
	    }
	    fprintf(stderr, "]\n");
	}
	list = build_int_list(col_nums, int_val);
	free(col_nums);
	return Py_BuildValue("iO", status, list);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown command");
	return NULL;
    }
}

static char CS_COMMAND_ct_results__doc__[] = 
"ct_results() -> status, result";

static PyObject *CS_COMMAND_ct_results(CS_COMMANDObj *self, PyObject *args)
{
    CS_RETCODE status;
    CS_INT result = 0;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    SY_BEGIN_THREADS;
    status = ct_results(self->cmd, &result);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_results() -> %s, %s\n",
		value_str(STATUS, status), value_str(RESULT, result));

    return Py_BuildValue("ii", status, result);
}

static char CS_COMMAND_ct_send__doc__[] = 
"ct_send() -> status";

static PyObject *CS_COMMAND_ct_send(CS_COMMANDObj *self, PyObject *args)
{
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    SY_BEGIN_THREADS;
    status = ct_send(self->cmd);
    SY_END_THREADS;

    if (self->debug)
	fprintf(stderr, "ct_send() -> %s\n", value_str(STATUS, status));
    return PyInt_FromLong(status);
}

static char CS_COMMAND_ct_send_data__doc__[] = 
"ct_send_data(buf) -> status";

static PyObject *CS_COMMAND_ct_send_data(CS_COMMANDObj *self, PyObject *args)
{
    CS_RETCODE status;
    DataBufObj *databuf;

    if (!PyArg_ParseTuple(args, "O!", &DataBufType, &databuf))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    SY_BEGIN_THREADS;
    status = ct_send_data(self->cmd, databuf->buff, databuf->copied[0]);
    SY_END_THREADS;

    if (self->debug)
	fprintf(stderr, "ct_send_data() -> %s\n", value_str(STATUS, status));
    return PyInt_FromLong(status);
}

static char CS_COMMAND_ct_setparam__doc__[] = 
"ct_setparam(buf) -> status";

static PyObject *CS_COMMAND_ct_setparam(CS_COMMANDObj *self, PyObject *args)
{
    DataBufObj *databuf;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "O!", &DataBufType, &databuf))
	return NULL;

    if (self->cmd == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_COMMAND has been dropped");
	return NULL;
    }

    SY_BEGIN_THREADS;
    status = ct_setparam(self->cmd, &databuf->fmt,
			 databuf->buff, &databuf->copied[0],
			 &databuf->indicator[0]);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_setparam() -> %s\n", value_str(STATUS, status));
    return PyInt_FromLong(status);
}

static struct PyMethodDef CS_COMMAND_methods[] = {
    { "ct_bind", (PyCFunction)CS_COMMAND_ct_bind, METH_VARARGS, CS_COMMAND_ct_bind__doc__ },
    { "ct_cancel", (PyCFunction)CS_COMMAND_ct_cancel, METH_VARARGS, CS_COMMAND_ct_cancel__doc__ },
    { "ct_command", (PyCFunction)CS_COMMAND_ct_command, METH_VARARGS, CS_COMMAND_ct_command__doc__ },
    { "ct_cmd_drop", (PyCFunction)CS_COMMAND_ct_cmd_drop, METH_VARARGS, CS_COMMAND_ct_cmd_drop__doc__ },
    { "ct_cursor", (PyCFunction)CS_COMMAND_ct_cursor, METH_VARARGS, CS_COMMAND_ct_cursor__doc__ },
    { "ct_data_info", (PyCFunction)CS_COMMAND_ct_data_info, METH_VARARGS, CS_COMMAND_ct_data_info__doc__ },
    { "ct_describe", (PyCFunction)CS_COMMAND_ct_describe, METH_VARARGS, CS_COMMAND_ct_describe__doc__ },
    { "ct_dynamic", (PyCFunction)CS_COMMAND_ct_dynamic, METH_VARARGS, CS_COMMAND_ct_dynamic__doc__ },
    { "ct_fetch", (PyCFunction)CS_COMMAND_ct_fetch, METH_VARARGS, CS_COMMAND_ct_fetch__doc__ },
    { "ct_get_data", (PyCFunction)CS_COMMAND_ct_get_data, METH_VARARGS, CS_COMMAND_ct_get_data__doc__ },
    { "ct_param", (PyCFunction)CS_COMMAND_ct_param, METH_VARARGS, CS_COMMAND_ct_param__doc__ },
    { "ct_res_info", (PyCFunction)CS_COMMAND_ct_res_info, METH_VARARGS, CS_COMMAND_ct_res_info__doc__ },
    { "ct_results", (PyCFunction)CS_COMMAND_ct_results, METH_VARARGS, CS_COMMAND_ct_results__doc__ },
    { "ct_send", (PyCFunction)CS_COMMAND_ct_send, METH_VARARGS, CS_COMMAND_ct_send__doc__ },
    { "ct_send_data", (PyCFunction)CS_COMMAND_ct_send_data, METH_VARARGS, CS_COMMAND_ct_send_data__doc__ },
    { "ct_setparam", (PyCFunction)CS_COMMAND_ct_setparam, METH_VARARGS, CS_COMMAND_ct_setparam__doc__ },
    { NULL }			/* sentinel */
};

PyObject *cmd_alloc(CS_CONNECTIONObj *conn)
{
    CS_COMMANDObj *self;
    CS_RETCODE status;
    CS_COMMAND *cmd;

    self = PyObject_NEW(CS_COMMANDObj, &CS_COMMANDType);
    if (self == NULL)
	return NULL;
    SY_LEAK_REG(self);
    self->is_eed = 0;
    self->cmd = NULL;
    self->conn = NULL;
    self->strip = conn->strip;
    self->debug = conn->debug;

    SY_BEGIN_THREADS;
    status = ct_cmd_alloc(conn->conn, &cmd);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_cmd_alloc() -> %s\n", value_str(STATUS, status));
    if (status != CS_SUCCEED) {
	Py_DECREF(self);
	return Py_BuildValue("iO", status, Py_None);
    }

    self->cmd = cmd;
    self->conn = conn;
    Py_INCREF(self->conn);
    return Py_BuildValue("iN", CS_SUCCEED, self);
}

PyObject *cmd_eed(CS_CONNECTIONObj *conn, CS_COMMAND *eed)
{
    CS_COMMANDObj *self;

    self = PyObject_NEW(CS_COMMANDObj, &CS_COMMANDType);
    if (self == NULL)
	return NULL;

    SY_LEAK_REG(self);
    self->is_eed = 1;
    self->cmd = eed;
    self->conn = conn;
    Py_INCREF(self->conn);

    return (PyObject*)self;
}

static void CS_COMMAND_dealloc(CS_COMMANDObj *self)
{
    SY_LEAK_UNREG(self);
    if (!self->is_eed && self->cmd) {
	/* should check return == CS_SUCCEED, but we can't handle failure
	   here */
	CS_RETCODE status;

	status = ct_cmd_drop(self->cmd);
	if (self->debug)
	    fprintf(stderr, "ct_cmd_drop() -> %s\n",
		    value_str(STATUS, status));
    }
    Py_XDECREF(self->conn);
    PyMem_DEL(self);
}

#define OFF(x) offsetof(CS_COMMANDObj, x)

static struct memberlist CS_COMMAND_memberlist[] = {
    { "is_eed", T_INT, OFF(is_eed), RO },
    { "conn", T_OBJECT, OFF(conn), RO },
    { "strip", T_INT, OFF(strip) },
    { "debug", T_INT, OFF(debug) },
    { NULL }			/* Sentinel */
};

static PyObject *CS_COMMAND_getattr(CS_COMMANDObj *self, char *name)
{
    PyObject *rv;

    rv = PyMember_Get((char *)self, CS_COMMAND_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(CS_COMMAND_methods, (PyObject *)self, name);
}

static int CS_COMMAND_setattr(CS_COMMANDObj *self, char *name, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
	return -1;
    }
    return PyMember_Set((char *)self, CS_COMMAND_memberlist, name, v);
}

static char CS_COMMANDType__doc__[] = 
"";

PyTypeObject CS_COMMANDType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "CommandType",		/*tp_name*/
    sizeof(CS_COMMANDObj),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)CS_COMMAND_dealloc,/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)CS_COMMAND_getattr, /*tp_getattr*/
    (setattrfunc)CS_COMMAND_setattr, /*tp_setattr*/
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
    CS_COMMANDType__doc__	/* Documentation string */
};
