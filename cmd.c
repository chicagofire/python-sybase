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

#include "_sybase.h"

static char CS_COMMAND_ct_send__doc__[] = 
"ct_send() -> status";

static PyObject *CS_COMMAND_ct_send(CS_COMMANDObj *self, PyObject *args)
{
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    Py_BEGIN_ALLOW_THREADS;
    status = ct_send(self->cmd);
    Py_END_ALLOW_THREADS;

    if (self->debug)
	fprintf(stderr, "ct_send() -> %s\n", value_str(STATUS, status));
    return PyInt_FromLong(status);
}

static char CS_COMMAND_ct_results__doc__[] = 
"ct_results() -> status, result";

static PyObject *CS_COMMAND_ct_results(CS_COMMANDObj *self, PyObject *args)
{
    CS_RETCODE status;
    CS_INT result = 0;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    Py_BEGIN_ALLOW_THREADS;
    status = ct_results(self->cmd, &result);
    Py_END_ALLOW_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_results() -> %s, %s\n",
		value_str(STATUS, status), value_str(RESULT, result));

    return Py_BuildValue("ii", status, result);
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

	Py_BEGIN_ALLOW_THREADS;
	status = ct_dynamic(self->cmd, type,
			    id, CS_NULLTERM, buff, CS_NULLTERM);
	Py_END_ALLOW_THREADS;
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

	Py_BEGIN_ALLOW_THREADS;
	status = ct_dynamic(self->cmd, type,
			    id, CS_NULLTERM, NULL, CS_UNUSED);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_dynamic(%s, %s) -> %s\n",
		    cmd_str, id, value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_EXEC_IMMEDIATE:
	/* ct_dynamic(CS_EXEC_IMMEDIATE, sql) -> status */
	if (!PyArg_ParseTuple(args, "is", &type, &buff))
	    return NULL;

	Py_BEGIN_ALLOW_THREADS;
	status = ct_dynamic(self->cmd, type,
			    NULL, CS_UNUSED, buff, CS_NULLTERM);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_dynamic(CS_EXEC_IMMEDIATE, %s) -> %s\n",
		    buff, value_str(STATUS, status));
	return PyInt_FromLong(status);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown dynamic command");
	return NULL;
    }
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

    switch (type) {
    case CS_BROWSE_INFO:
	/* ct_res_info(CS_BROWSE_INFO) -> status, bool */
	Py_BEGIN_ALLOW_THREADS;
	status = ct_res_info(self->cmd, type, &bool_val, CS_UNUSED, NULL);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_res_info(CS_BROWSE_INFO) -> %s, %d\n",
		    value_str(STATUS, status), bool_val);
	return Py_BuildValue("ii", status, bool_val);

    case CS_MSGTYPE:
	/* ct_res_info(CS_MSGTYPE) -> status, int */
	Py_BEGIN_ALLOW_THREADS;
	status = ct_res_info(self->cmd, type, &ushort_val, CS_UNUSED, NULL);
	Py_END_ALLOW_THREADS;
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
	Py_BEGIN_ALLOW_THREADS;
	status = ct_res_info(self->cmd, type, &int_val, CS_UNUSED, NULL);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_res_info(%s) -> %s, %d\n",
		    type_str, value_str(STATUS, status), int_val);
	return Py_BuildValue("ii", status, int_val);

    case CS_ORDERBY_COLS:
	/* ct_res_info(CS_ORDERBY_COLS) -> status, list of int */
	Py_BEGIN_ALLOW_THREADS;
	status = ct_res_info(self->cmd, CS_NUMORDERCOLS, &int_val, CS_UNUSED, NULL);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_res_info(CS_NUMORDERCOLS) -> %s, %d\n",
		    value_str(STATUS, status), int_val);
	if (status != CS_SUCCEED)
	    return Py_BuildValue("iO", status, Py_None);

	if (int_val <= 0)
	    return Py_BuildValue("i[]", status);

	col_nums = malloc(sizeof(*col_nums) * int_val);
	if (col_nums == NULL)
	    return PyErr_NoMemory();

	Py_BEGIN_ALLOW_THREADS;
	status = ct_res_info(self->cmd, CS_ORDERBY_COLS,
			     col_nums, sizeof(*col_nums) * int_val, NULL);
	Py_END_ALLOW_THREADS;
	if (self->debug) {
	    int i;

	    fprintf(stderr, "ct_res_info(CS_ORDERBY_COLS) -> %s, [",
		    value_str(STATUS, status));
	    for (i = 0; i < int_val; i++) {
		if (i > 0)
		    fprintf(stderr, ",");
		fprintf(stderr, "%d", col_nums[i]);
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

    memset(&datafmt, 0, sizeof(datafmt));
    Py_BEGIN_ALLOW_THREADS;
    status = ct_describe(self->cmd, num, &datafmt);
    Py_END_ALLOW_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_describe(%d) -> %s\n",
		num, value_str(STATUS, status));

    if (status != CS_SUCCEED)
	return Py_BuildValue("iO", status, Py_None);

    switch (datafmt.datatype) {
    case CS_LONGCHAR_TYPE:
    case CS_VARCHAR_TYPE:
    case CS_TEXT_TYPE:
	datafmt.datatype = CS_CHAR_TYPE;
    case CS_CHAR_TYPE:
	if (datafmt.maxlength > 64 * 1024)
	    datafmt.maxlength = 64 * 1024;
	break;

    case CS_IMAGE_TYPE:
    case CS_LONGBINARY_TYPE:
    case CS_VARBINARY_TYPE:
	datafmt.datatype = CS_BINARY_TYPE;
    case CS_BINARY_TYPE:
	if (datafmt.maxlength > 64 * 1024)
	    datafmt.maxlength = 64 * 1024;
	break;

    case CS_BIT_TYPE:
    case CS_TINYINT_TYPE:
    case CS_SMALLINT_TYPE:
	datafmt.datatype = CS_INT_TYPE;
	datafmt.maxlength = 4;
    case CS_INT_TYPE:
	break;

    case CS_DECIMAL_TYPE:
	datafmt.datatype = CS_NUMERIC_TYPE;
    case CS_NUMERIC_TYPE:
	break;

    case CS_MONEY_TYPE:
    case CS_MONEY4_TYPE:
    case CS_REAL_TYPE:
	datafmt.datatype = CS_FLOAT_TYPE;
	datafmt.maxlength = 8;
    case CS_FLOAT_TYPE:
	break;

    case CS_DATETIME4_TYPE:
	datafmt.datatype = CS_DATETIME_TYPE;
	datafmt.maxlength = 8;
    case CS_DATETIME_TYPE:
	break;

    default:
	break;
    }

    fmt = datafmt_alloc(&datafmt, self->strip);
    if (fmt == NULL)
	return NULL;

    return Py_BuildValue("iN", status, fmt);
}

static char CS_COMMAND_ct_cancel__doc__[] = 
"ct_cancel(type) -> status";

static PyObject *CS_COMMAND_ct_cancel(CS_COMMANDObj *self, PyObject *args)
{
    int type;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "i", &type))
	return NULL;

    Py_BEGIN_ALLOW_THREADS;
    status = ct_cancel(NULL, self->cmd, type);
    Py_END_ALLOW_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_cancel(%s) -> %s\n",
		value_str(CANCEL, type), value_str(STATUS, status));

    return PyInt_FromLong(status);
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

	Py_BEGIN_ALLOW_THREADS;
	status = ct_cursor(self->cmd, type,
			   name, CS_NULLTERM, text, CS_NULLTERM, option);
	Py_END_ALLOW_THREADS;
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

	Py_BEGIN_ALLOW_THREADS;
	status = ct_cursor(self->cmd, type,
			   NULL, CS_UNUSED, NULL, CS_UNUSED, option);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_cursor(%s, %s) -> %s\n",
		    type_str, value_str(CURSOROPT, option),
		    value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_CURSOR_ROWS:
	/* ct_cursor(CS_CURSOR_ROWS, int) -> status */
	if (!PyArg_ParseTuple(args, "ii", &type, &option))
	    return NULL;

	Py_BEGIN_ALLOW_THREADS;
	status = ct_cursor(self->cmd, type,
			   NULL, CS_UNUSED, NULL, CS_UNUSED, option);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_cursor(CS_CURSOR_ROWS, %s) -> %s\n",
		    value_str(CURSOROPT, option), value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_CURSOR_DELETE:
	/* ct_cursor(CS_CURSOR_DELETE, table) -> status */
	if (!PyArg_ParseTuple(args, "is", &type, &name))
	    return NULL;

	Py_BEGIN_ALLOW_THREADS;
	status = ct_cursor(self->cmd, type,
			   name, CS_NULLTERM, NULL, CS_UNUSED, CS_UNUSED);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_cursor(CS_CURSOR_DELETE, %s) -> %s\n",
		    name, value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_CURSOR_DEALLOC:
	/* ct_cursor(CS_CURSOR_DEALLOC) -> status */
	if (!PyArg_ParseTuple(args, "i", &type))
	    return NULL;

	Py_BEGIN_ALLOW_THREADS;
	status = ct_cursor(self->cmd, type,
			   NULL, CS_UNUSED, NULL, CS_UNUSED, CS_UNUSED);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_cursor(CS_CURSOR_DEALLOC) -> %s\n",
		    value_str(STATUS, status));
	return PyInt_FromLong(status);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown type");
	return NULL;
    }
}

static char CS_COMMAND_ct_bind__doc__[] = 
"ct_bind(int, datafmt) -> status, buffer";

static PyObject *CS_COMMAND_ct_bind(CS_COMMANDObj *self, PyObject *args)
{
    CS_INT item;
    CS_DATAFMTObj *datafmt;
    BufferObj *buffer;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "iO!", &item, &CS_DATAFMTType, &datafmt))
	return NULL;

    buffer = (BufferObj *)buffer_alloc(datafmt);
    if (buffer == NULL)
	return NULL;

    Py_BEGIN_ALLOW_THREADS;
    status = ct_bind(self->cmd, item, &datafmt->fmt,
		     buffer->buff, buffer->copied, buffer->indicator);
    Py_END_ALLOW_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_bind(%d) -> %s\n", item, value_str(STATUS, status));
    return Py_BuildValue("iN", status, buffer);
}

static char CS_COMMAND_ct_fetch__doc__[] = 
"ct_fetch() -> result, rows_read";

static PyObject *CS_COMMAND_ct_fetch(CS_COMMANDObj *self, PyObject *args)
{
    CS_RETCODE status;
    CS_INT rows_read = 0;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    Py_BEGIN_ALLOW_THREADS;
    status = ct_fetch(self->cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &rows_read);
    Py_END_ALLOW_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_fetch() -> %s, %d\n",
		value_str(STATUS, status), rows_read);

    return Py_BuildValue("ii", status, rows_read);
}

static char CS_COMMAND_ct_param__doc__[] = 
"ct_param(buffer) -> status";

static PyObject *CS_COMMAND_ct_param(CS_COMMANDObj *self, PyObject *args)
{
    CS_DATAFMTObj *datafmt;
    BufferObj *buffer;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "O!", &BufferType, &buffer))
	return NULL;

    Py_BEGIN_ALLOW_THREADS;
    status = ct_param(self->cmd, &buffer->fmt,
		      buffer->buff, buffer->copied[0], buffer->indicator[0]);
    Py_END_ALLOW_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_param() -> %s\n", value_str(STATUS, status));
    return PyInt_FromLong(status);
}

static char CS_COMMAND_ct_setparam__doc__[] = 
"ct_setparam(buffer) -> status";

static PyObject *CS_COMMAND_ct_setparam(CS_COMMANDObj *self, PyObject *args)
{
    CS_DATAFMTObj *datafmt;
    BufferObj *buffer;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "O!", &BufferType, &buffer))
	return NULL;

    Py_BEGIN_ALLOW_THREADS;
    status = ct_param(self->cmd, &buffer->fmt,
		      buffer->buff, buffer->copied[0], buffer->indicator[0]);
    Py_END_ALLOW_THREADS;
    if (self->debug)
	fprintf(stderr, "ct_setparam() -> %s\n", value_str(STATUS, status));
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
    char *buffer;
    CS_INT num;
    CS_INT option = CS_UNUSED;
    CS_RETCODE status;
    char *type_str = NULL;

    if (!first_tuple_int(args, &type))
	return NULL;

    switch (type) {
    case CS_LANG_CMD:
	/* ct_command(CS_LANG_CMD, sql [,option]) -> status */
	type_str = "CS_LANG_CMD";
    case CS_RPC_CMD:
	/* ct_command(CS_RPC_CMD, name [,option]) -> status */
	if (type_str == NULL)
	    type_str = "CS_RPC_CMD";
	if (!PyArg_ParseTuple(args, "is|i", &type, &buffer, &option))
	    return NULL;

	Py_BEGIN_ALLOW_THREADS;
	status = ct_command(self->cmd, type, buffer, CS_NULLTERM, option);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_command(%s, %s, %s) -> %s\n",
		    type_str, buffer, value_str(CMD, option),
		    value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_MSG_CMD:
	/* ct_command(CS_MSG_CMD, int) -> status */
	if (!PyArg_ParseTuple(args, "ii", &type, &num))
	    return NULL;

	Py_BEGIN_ALLOW_THREADS;
	status = ct_command(self->cmd, type, (CS_VOID*)&num, CS_UNUSED, CS_UNUSED);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_command(CS_MSG_CMD, %d) -> %s\n",
		    num, value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_PACKAGE_CMD:
	/* ct_command(CS_PACKAGE_CMD, name) -> status */
	if (!PyArg_ParseTuple(args, "is", &type, &buffer))
	    return NULL;

	Py_BEGIN_ALLOW_THREADS;
	status = ct_command(self->cmd, type, buffer, CS_NULLTERM, CS_UNUSED);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_command(CS_PACKAGE_CMD, %s) -> %s\n",
		    buffer, value_str(STATUS, status));
	return PyInt_FromLong(status);

    case CS_SEND_DATA_CMD:
	/* ct_command(CS_SEND_DATA_CMD) -> status */
	if (!PyArg_ParseTuple(args, "i", &type))
	    return NULL;

	Py_BEGIN_ALLOW_THREADS;
	status = ct_command(self->cmd, type, NULL, CS_UNUSED, CS_UNUSED);
	Py_END_ALLOW_THREADS;
	if (self->debug)
	    fprintf(stderr, "ct_command(CS_SEND_DATA_CMD) -> %s\n",
		    value_str(STATUS, status));
	return PyInt_FromLong(status);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown type");
	return NULL;
    }
}

static struct PyMethodDef CS_COMMAND_methods[] = {
    { "ct_bind", (PyCFunction)CS_COMMAND_ct_bind, METH_VARARGS, CS_COMMAND_ct_bind__doc__ },
    { "ct_cancel", (PyCFunction)CS_COMMAND_ct_cancel, METH_VARARGS, CS_COMMAND_ct_cancel__doc__ },
    { "ct_command", (PyCFunction)CS_COMMAND_ct_command, METH_VARARGS, CS_COMMAND_ct_command__doc__ },
    { "ct_cursor", (PyCFunction)CS_COMMAND_ct_cursor, METH_VARARGS, CS_COMMAND_ct_cursor__doc__ },
    { "ct_describe", (PyCFunction)CS_COMMAND_ct_describe, METH_VARARGS, CS_COMMAND_ct_describe__doc__ },
    { "ct_dynamic", (PyCFunction)CS_COMMAND_ct_dynamic, METH_VARARGS, CS_COMMAND_ct_dynamic__doc__ },
    { "ct_fetch", (PyCFunction)CS_COMMAND_ct_fetch, METH_VARARGS, CS_COMMAND_ct_fetch__doc__ },
    { "ct_param", (PyCFunction)CS_COMMAND_ct_param, METH_VARARGS, CS_COMMAND_ct_param__doc__ },
    { "ct_res_info", (PyCFunction)CS_COMMAND_ct_res_info, METH_VARARGS, CS_COMMAND_ct_res_info__doc__ },
    { "ct_results", (PyCFunction)CS_COMMAND_ct_results, METH_VARARGS, CS_COMMAND_ct_results__doc__ },
    { "ct_send", (PyCFunction)CS_COMMAND_ct_send, METH_VARARGS, CS_COMMAND_ct_send__doc__ },
    { "ct_setparam", (PyCFunction)CS_COMMAND_ct_setparam, METH_VARARGS, CS_COMMAND_ct_setparam__doc__ },
    { NULL }			/* sentinel */
};

PyObject *cmd_alloc(CS_CONNECTIONObj *con)
{
    CS_COMMANDObj *self;
    CS_RETCODE status;
    CS_COMMAND *cmd;

    self = PyObject_NEW(CS_COMMANDObj, &CS_COMMANDType);
    if (self == NULL)
	return NULL;
    self->is_eed = 0;
    self->cmd = NULL;
    self->con = NULL;
    self->strip = con->strip;
    self->debug = con->debug;

    Py_BEGIN_ALLOW_THREADS;
    status = ct_cmd_alloc(con->con, &cmd);
    Py_END_ALLOW_THREADS;
    if (status != CS_SUCCEED) {
	Py_DECREF(self);
	return Py_BuildValue("iO", status, Py_None);
    }

    self->cmd = cmd;
    self->con = con;
    Py_INCREF(self->con);
    return Py_BuildValue("iN", CS_SUCCEED, self);
}

PyObject *cmd_eed(CS_CONNECTIONObj *con, CS_COMMAND *eed)
{
    CS_COMMANDObj *self;

    self = PyObject_NEW(CS_COMMANDObj, &CS_COMMANDType);
    if (self == NULL)
	return NULL;

    self->is_eed = 1;
    self->cmd = eed;
    self->con = con;
    Py_INCREF(self->con);

    return (PyObject*)self;
}

static void CS_COMMAND_dealloc(CS_COMMANDObj *self)
{
    if (!self->is_eed && self->cmd) {
	/* should check return == CS_SUCCEED, but we can't handle failure
	   here */
	Py_BEGIN_ALLOW_THREADS;
	ct_cmd_drop(self->cmd);
	Py_END_ALLOW_THREADS;
    }
    Py_XDECREF(self->con);
    PyMem_DEL(self);
}

#define OFF(x) offsetof(CS_COMMANDObj, x)

static struct memberlist CS_COMMAND_memberlist[] = {
    { "is_eed", T_INT, OFF(is_eed), RO },
    { "con", T_OBJECT, OFF(con), RO },
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
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "CS_COMMAND",		/*tp_name*/
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
