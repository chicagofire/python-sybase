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

PyObject *locale_alloc(CS_CONTEXTObj *ctx)
{
    CS_LOCALEObj *self;
    CS_RETCODE status;
    CS_LOCALE *loc;

    self = PyObject_NEW(CS_LOCALEObj, &CS_LOCALEType);
    if (self == NULL)
	return NULL;

    SY_LEAK_REG(self);
    self->locale = NULL;
    self->debug = ctx->debug;

    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = cs_loc_alloc(ctx->ctx, &loc);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "cs_loc_alloc() -> %s\n", value_str(STATUS, status));
    if (PyErr_Occurred()) {
	Py_DECREF(self);
	return NULL;
    }

    if (status != CS_SUCCEED) {
	Py_DECREF(self);
	return Py_BuildValue("iO", status, Py_None);
    }

    self->ctx = ctx;
    Py_INCREF(self->ctx);
    self->locale = loc;

    return Py_BuildValue("iN", CS_SUCCEED, self);
}

static void CS_LOCALE_dealloc(CS_LOCALEObj *self)
{
    SY_LEAK_UNREG(self);

    if (self->locale) {
	/* should check return == CS_SUCCEED, but we can't handle failure
	   here */
	CS_RETCODE status;

	status = cs_loc_drop(self->ctx->ctx, self->locale);
	if (self->debug)
	    fprintf(stderr, "cs_loc_drop() -> %s\n",
		    value_str(STATUS, status));
    }
    Py_XDECREF(self->ctx);
    PyMem_DEL(self);
}

static int csdate_type(int type)
{
    switch (type) {
    case CS_12HOUR:
	return OPTION_BOOL;
    case CS_DT_CONVFMT:
	return OPTION_INT;
    case CS_MONTH:
    case CS_SHORTMONTH:
    case CS_DAYNAME:
    case CS_DATEORDER:
	return OPTION_STRING;
    default:
	return OPTION_UNKNOWN;
    }
}

static char CS_LOCALE_cs_dt_info__doc__[] = 
"cs_dt_info(CS_SET, type, value) -> status\n"
"cs_dt_info(CS_GET, type [, item]) -> status, value";

static PyObject *CS_LOCALE_cs_dt_info(CS_LOCALEObj *self, PyObject *args)
{
    int action, type;
    PyObject *obj = NULL;
    CS_RETCODE status;
    CS_INT int_value, out_len, item, buff_len;
    CS_BOOL bool_value;
    char str_buff[10240];

    if (!first_tuple_int(args, &action))
	return NULL;

    switch (action) {
    case CS_SET:
	/* cs_dt_info(CS_SET, type, value) -> status */
	if (!PyArg_ParseTuple(args, "iiO", &action, &type, &obj))
	    return NULL;
	int_value = PyInt_AsLong(obj);
	if (PyErr_Occurred())
	    return NULL;

	SY_BEGIN_THREADS;
	status = cs_dt_info(self->ctx->ctx, CS_SET, self->locale,
			    type, CS_UNUSED,
			    &int_value, sizeof(int_value), &out_len);
	SY_END_THREADS;
	if (self->debug) {
	    if (type == CS_DT_CONVFMT)
		fprintf(stderr, "cs_dt_info(CS_SET, %s, %s) -> %s\n",
			value_str(DTINFO, type), value_str(CSDATES, int_value),
			value_str(STATUS, status));
	    else
		fprintf(stderr, "cs_dt_info(CS_SET, %s, %d) -> %s\n",
			value_str(DTINFO, type), (int)int_value,
			value_str(STATUS, status));
	}

	return PyInt_FromLong(status);

    case CS_GET:
	/* cs_dt_info(CS_GET, type [, item]) -> status, value */
	item = CS_UNUSED;
	if (!PyArg_ParseTuple(args, "ii|i", &action, &type, &item))
	    return NULL;

	switch (csdate_type(type)) {
	case OPTION_BOOL:
	    SY_BEGIN_THREADS;
	    status = cs_dt_info(self->ctx->ctx, CS_GET, self->locale,
				type, CS_UNUSED,
				&bool_value, sizeof(bool_value), &out_len);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "cs_dt_info(CS_GET, %s) -> %s, %d\n",
			value_str(DTINFO, type),
			value_str(STATUS, status), (int)bool_value);
	    return Py_BuildValue("ii", status, bool_value);

	case OPTION_INT:
	    SY_BEGIN_THREADS;
	    status = cs_dt_info(self->ctx->ctx, CS_GET, self->locale,
				type, CS_UNUSED,
				&int_value, sizeof(int_value), &out_len);
	    SY_END_THREADS;
	    if (self->debug) {
		if (type == CS_DT_CONVFMT)
		    fprintf(stderr, "cs_dt_info(CS_GET, %s) -> %s, %s\n",
			    value_str(DTINFO, type),
			    value_str(STATUS, status),
			    value_str(CSDATES, int_value));
		else
		    fprintf(stderr, "cs_dt_info(CS_GET, %s) -> %s, %d\n",
			    value_str(DTINFO, type),
			    value_str(STATUS, status), (int)int_value);
	    }
	    return Py_BuildValue("ii", status, int_value);

	case OPTION_STRING:
	    SY_BEGIN_THREADS;
	    status = cs_dt_info(self->ctx->ctx, CS_GET, self->locale,
				type, item,
				str_buff, sizeof(str_buff), &buff_len);
	    SY_END_THREADS;
	    if (buff_len > sizeof(str_buff))
		buff_len = sizeof(str_buff);
	    if (self->debug)
		fprintf(stderr, "cs_dt_info(CS_GET, %s, %d) -> %s, '%.*s'\n",
			value_str(DTINFO, type), (int)item,
			value_str(STATUS, status), (int)buff_len, str_buff);
	    return Py_BuildValue("is#", status, str_buff, buff_len);

	case OPTION_UNKNOWN:
	    PyErr_SetString(PyExc_TypeError, "unknown option type");
	    return NULL;

	default:
	    PyErr_SetString(PyExc_TypeError, "unhandled property value");
	    return NULL;
	}

    default:
	PyErr_SetString(PyExc_TypeError, "unknown action");
	return NULL;
    }
}

static char CS_LOCALE_cs_loc_drop__doc__[] = 
"cs_loc_drop() -> status";

static PyObject *CS_LOCALE_cs_loc_drop(CS_LOCALEObj *self, PyObject *args)
{
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    if (self->locale == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_LOCALE has been dropped");
	return NULL;
    }

    SY_BEGIN_THREADS;
    status = cs_loc_drop(self->ctx->ctx, self->locale);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "cs_loc_drop() -> %s\n", value_str(STATUS, status));

    if (status == CS_SUCCEED)
	self->locale = NULL;

    return PyInt_FromLong(status);
}

static char CS_LOCALE_cs_locale__doc__[] = 
"cs_locale(CS_SET, type, str) -> status\n"
"cs_locale(CS_GET, type) -> status, str\n";

static PyObject *CS_LOCALE_cs_locale(CS_LOCALEObj *self, PyObject *args)
{
    int action, type;
    CS_INT str_len;
    char *str;
    char buff[1024];
    CS_RETCODE status;

    if (!first_tuple_int(args, &action))
	return NULL;

    switch (action) {
    case CS_SET:
	/* cs_locale(CS_SET, type, str) -> status */
	if (!PyArg_ParseTuple(args, "iis", &action, &type, &str))
	    return NULL;

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = cs_locale(self->ctx->ctx, CS_SET, self->locale,
			   type, str, CS_NULLTERM, NULL);
	SY_END_THREADS;
	if (PyErr_Occurred())
	    return NULL;

	return PyInt_FromLong(status);

    case CS_GET:
	/* cs_locale(CS_GET, type) -> status, str */
	if (!PyArg_ParseTuple(args, "ii", &action, &type))
	    return NULL;

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = cs_locale(self->ctx->ctx, CS_GET, self->locale,
			   type, buff, sizeof(buff), &str_len);
	SY_END_THREADS;
	if (PyErr_Occurred())
	    return NULL;

	return Py_BuildValue("is", status, buff);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown type");
	return NULL;
    }
}

static struct PyMethodDef CS_LOCALE_methods[] = {
    { "cs_dt_info", (PyCFunction)CS_LOCALE_cs_dt_info, METH_VARARGS, CS_LOCALE_cs_dt_info__doc__ },
    { "cs_loc_drop", (PyCFunction)CS_LOCALE_cs_loc_drop, METH_VARARGS, CS_LOCALE_cs_loc_drop__doc__ },
    { "cs_locale", (PyCFunction)CS_LOCALE_cs_locale, METH_VARARGS, CS_LOCALE_cs_locale__doc__ },
    { NULL }
};

static PyObject *CS_LOCALE_getattr(CS_LOCALEObj *self, char *name)
{
    return Py_FindMethod(CS_LOCALE_methods, (PyObject *)self, name);
}

static char CS_LOCALEType__doc__[] = 
"";

PyTypeObject CS_LOCALEType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "LocaleType",		/*tp_name*/
    sizeof(CS_LOCALEObj),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)CS_LOCALE_dealloc,/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)CS_LOCALE_getattr, /*tp_getattr*/
    (setattrfunc)0,		/*tp_setattr*/
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
    CS_LOCALEType__doc__	/* Documentation string */
};
