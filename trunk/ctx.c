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

static char CS_CONTEXT_ct_init__doc__[] = 
"ct_init() -> status";

static PyObject *CS_CONTEXT_ct_init(CS_CONTEXTObj *self, PyObject *args)
{
    int version;
    CS_RETCODE status;

    version = CS_VERSION_100;
    if (!PyArg_ParseTuple(args, "|i", &version))
	return NULL;

    Py_BEGIN_ALLOW_THREADS;
    status = ct_init(self->ctx, version);
    Py_END_ALLOW_THREADS;

    return PyInt_FromLong(status);
}

static int property_type(int property)
{
    switch (property) {
    case CS_LOGIN_TIMEOUT:
    case CS_MAX_CONNECT:
    case CS_NETIO:
    case CS_NO_TRUNCATE:
    case CS_TEXTLIMIT:
    case CS_TIMEOUT:
	return OPTION_INT;
    case CS_VER_STRING:
    case CS_VERSION:
	return OPTION_STRING;
    default:
	return OPTION_UNKNOWN;
    }
}

static char CS_CONTEXT_ct_config__doc__[] = 
"ct_config(CS_SET, property, value) -> status\n"
"ct_config(CS_GET, property) -> status, value\n"
"ct_config(CS_CLEAR, property) -> status\n";

static PyObject *CS_CONTEXT_ct_config(CS_CONTEXTObj *self, PyObject *args)
{
    int action, property;
    PyObject *obj = NULL;
    CS_RETCODE status;
    int int_value;
    char *str_value;
    char str_buff[10240];
    CS_INT buff_len;

    if (!first_tuple_int(args, &action))
	return NULL;

    switch (action) {
    case CS_SET:
	/* ct_config(CS_SET, property, value) -> status */
	if (!PyArg_ParseTuple(args, "iiO", &action, &property, &obj))
	    return NULL;
	switch (property_type(property)) {
	case OPTION_INT:
	    int_value = PyInt_AsLong(obj);
	    if (PyErr_Occurred())
		return NULL;
	    Py_BEGIN_ALLOW_THREADS;
	    status = ct_config(self->ctx, CS_SET, property,
			       &int_value, CS_UNUSED, NULL);
	    Py_END_ALLOW_THREADS;
	    return PyInt_FromLong(status);

	case OPTION_STRING:
	    str_value = PyString_AsString(obj);
	    if (PyErr_Occurred())
		return NULL;
	    Py_BEGIN_ALLOW_THREADS;
	    status = ct_config(self->ctx, CS_SET, property,
			       str_value, CS_NULLTERM, NULL);
	    Py_END_ALLOW_THREADS;
	    return PyInt_FromLong(status);

	default:
	    PyErr_SetString(PyExc_TypeError, "unknown property value");
	    return NULL;
	}
	break;

    case CS_GET:
	/* ct_config(CS_GET, property) -> status, value */
	if (!PyArg_ParseTuple(args, "ii", &action, &property))
	    return NULL;

	switch (property_type(property)) {
	case OPTION_INT:
	    Py_BEGIN_ALLOW_THREADS;
	    status = ct_config(self->ctx, CS_GET, property,
			       &int_value, CS_UNUSED, NULL);
	    Py_END_ALLOW_THREADS;
	    return Py_BuildValue("ii", status, int_value);

	case OPTION_STRING:
	    Py_BEGIN_ALLOW_THREADS;
	    status = ct_config(self->ctx, CS_GET, property,
			       str_buff, sizeof(str_buff), &buff_len);
	    Py_END_ALLOW_THREADS;
	    if (buff_len > sizeof(str_buff))
		buff_len = sizeof(str_buff);
	    return Py_BuildValue("is#", status, str_buff, buff_len);

	default:
	    PyErr_SetString(PyExc_TypeError, "unknown property value");
	    return NULL;
	}
	break;

    case CS_CLEAR:
	/* ct_config(CS_CLEAR, property) -> status */
	if (!PyArg_ParseTuple(args, "ii", &action, &property))
	    return NULL;

	Py_BEGIN_ALLOW_THREADS;
	status = ct_config(self->ctx, CS_CLEAR, property,
			   NULL, CS_UNUSED, NULL);
	Py_END_ALLOW_THREADS;
	return PyInt_FromLong(status);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown action");
	return NULL;
    }
}

static char CS_CONTEXT_ct_con_alloc__doc__[] = 
"";

static PyObject *CS_CONTEXT_ct_con_alloc(CS_CONTEXTObj *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
	return NULL;
    return con_alloc(self);
}

static char CS_CONTEXT_cs_diag__doc__[] = 
"cs_diag(CS_INIT) -> status\n"
"cs_diag(CS_MSGLIMIT, type, num) -> status\n"
"cs_diag(CS_CLEAR, type) -> status\n"
"cs_diag(CS_GET, type, index) -> status, msg\n"
"cs_diag(CS_STATUS, type) -> status, num\n";

static PyObject *CS_CONTEXT_cs_diag(CS_CONTEXTObj *self, PyObject *args)
{
    int operation, type, index, num;
    CS_VOID *buffer;
    PyObject *msg;
    CS_RETCODE status;

    if (!first_tuple_int(args, &operation))
	return NULL;

    switch (operation) {
    case CS_INIT:
	/* cs_diag(CS_INIT) -> status */
	if (!PyArg_ParseTuple(args, "i", &operation))
	    return NULL;
	Py_BEGIN_ALLOW_THREADS;
	status = cs_diag(self->ctx, operation, CS_UNUSED, CS_UNUSED, NULL);
	Py_END_ALLOW_THREADS;
	return PyInt_FromLong(status);

    case CS_MSGLIMIT:
	/* cs_diag(CS_MSGLIMIT, type, num) -> status */
	if (!PyArg_ParseTuple(args, "iii", &operation, &type, &num))
	    return NULL;
	Py_BEGIN_ALLOW_THREADS;
	status = cs_diag(self->ctx, operation, type, CS_UNUSED, &num);
	Py_END_ALLOW_THREADS;
	return PyInt_FromLong(status);

    case CS_CLEAR:
	/* cs_diag(CS_CLEAR, type) -> status */
	if (!PyArg_ParseTuple(args, "ii", &operation, &type))
	    return NULL;
	Py_BEGIN_ALLOW_THREADS;
	status = cs_diag(self->ctx, operation, type, CS_UNUSED, NULL);
	Py_END_ALLOW_THREADS;
	return PyInt_FromLong(status);

    case CS_GET:
	/* cs_diag(CS_GET, type, index) -> status, msg */
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
	Py_BEGIN_ALLOW_THREADS;
	status = cs_diag(self->ctx, operation, type, index, buffer);
	Py_END_ALLOW_THREADS;
	if (status != CS_SUCCEED)
	    return Py_BuildValue("iO", status, Py_None);
	return Py_BuildValue("iN", CS_SUCCEED, msg);

    case CS_STATUS:
	/* cs_diag(CS_STATUS, type) -> status, num */
	if (!PyArg_ParseTuple(args, "ii", &operation, &type))
	    return NULL;
	num = 0;
	Py_BEGIN_ALLOW_THREADS;
	status = cs_diag(self->ctx, operation, type, CS_UNUSED, &num);
	Py_END_ALLOW_THREADS;
	return Py_BuildValue("ii", status, num);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown operation");
	return NULL;
    }
}

static struct PyMethodDef CS_CONTEXT_methods[] = {
    { "ct_init", (PyCFunction)CS_CONTEXT_ct_init, METH_VARARGS, CS_CONTEXT_ct_init__doc__ },
    { "ct_config", (PyCFunction)CS_CONTEXT_ct_config, METH_VARARGS, CS_CONTEXT_ct_config__doc__ },
    { "ct_con_alloc", (PyCFunction)CS_CONTEXT_ct_con_alloc, METH_VARARGS, CS_CONTEXT_ct_con_alloc__doc__ },
    { "cs_diag", (PyCFunction)CS_CONTEXT_cs_diag, METH_VARARGS, CS_CONTEXT_cs_diag__doc__ },
    { NULL }			/* sentinel */
};

PyObject *ctx_alloc(CS_INT version)
{
    CS_CONTEXTObj *self;
    CS_RETCODE status;
    CS_CONTEXT *ctx;

    self = PyObject_NEW(CS_CONTEXTObj, &CS_CONTEXTType);
    if (self == NULL)
	return NULL;
    self->ctx = NULL;

    Py_BEGIN_ALLOW_THREADS;
    status = cs_ctx_alloc(version, &ctx);
    Py_END_ALLOW_THREADS;
    if (status != CS_SUCCEED) {
	Py_DECREF(self);
	return Py_BuildValue("iO", status, Py_None);
    }

    self->ctx = ctx;
    return Py_BuildValue("iN", CS_SUCCEED, self);
}

PyObject *ctx_global(CS_INT version)
{
    CS_CONTEXTObj *self;
    CS_RETCODE status;
    CS_CONTEXT *ctx;

    self = PyObject_NEW(CS_CONTEXTObj, &CS_CONTEXTType);
    if (self == NULL)
	return NULL;
    self->ctx = NULL;

    Py_BEGIN_ALLOW_THREADS;
    status = cs_ctx_global(version, &ctx);
    Py_END_ALLOW_THREADS;
    if (status != CS_SUCCEED) {
	Py_DECREF(self);
	return Py_BuildValue("iO", status, Py_None);
    }

    self->ctx = ctx;
    return Py_BuildValue("iN", CS_SUCCEED, self);
}

static void CS_CONTEXT_dealloc(CS_CONTEXTObj *self)
{
    if (self->ctx) {
	/* should check return == CS_SUCCEED, but we can't handle failure
	   here */
	Py_BEGIN_ALLOW_THREADS;
	cs_ctx_drop(self->ctx);
	Py_END_ALLOW_THREADS;
    }

    PyMem_DEL(self);
}

static PyObject *CS_CONTEXT_getattr(CS_CONTEXTObj *self, char *name)
{
    /* XXXX Add your own getattr code here */
    return Py_FindMethod(CS_CONTEXT_methods, (PyObject *)self, name);
}

static int CS_CONTEXT_setattr(CS_CONTEXTObj *self, char *name, PyObject *v)
{
    /* Set attribute 'name' to value 'v'. v==NULL means delete */

    /* XXXX Add your own setattr code here */
    return -1;
}

static char CS_CONTEXTType__doc__[] = 
"Wrap the Sybase CS_CONTEXT structure and associated functionality.";

PyTypeObject CS_CONTEXTType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "CS_CONTEXT",		/*tp_name*/
    sizeof(CS_CONTEXTObj),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)CS_CONTEXT_dealloc,/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)CS_CONTEXT_getattr, /*tp_getattr*/
    (setattrfunc)CS_CONTEXT_setattr, /*tp_setattr*/
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
    CS_CONTEXTType__doc__	/* Documentation string */
};
