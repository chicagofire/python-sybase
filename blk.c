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

static char CS_BLKDESC_blk_bind__doc__[] = 
"blk_bind(int, buffer) -> status";

static PyObject *CS_BLKDESC_blk_bind(CS_BLKDESCObj *self, PyObject *args)
{
    int colnum;
    DataBufObj *buffer;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "iO!", &colnum, &DataBufType, &buffer))
	return NULL;

    if (self->blk == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_BLKDESC has been dropped");
	return NULL;
    }

    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = blk_bind(self->blk, colnum, &buffer->fmt,
		      buffer->buff, buffer->copied, buffer->indicator);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "blk_bind(%d) -> %s\n",
		colnum, value_str(STATUS, status));
    if (PyErr_Occurred())
        return NULL;

    return PyInt_FromLong(status);
}

static char CS_BLKDESC_blk_describe__doc__[] = 
"blk_describe(int) -> status, datafmt";

static PyObject *CS_BLKDESC_blk_describe(CS_BLKDESCObj *self, PyObject *args)
{
    int colnum;
    CS_DATAFMT datafmt;
    CS_RETCODE status;
    PyObject *fmt;

    if (!PyArg_ParseTuple(args, "i", &colnum))
	return NULL;

    if (self->blk == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_BLKDESC has been dropped");
	return NULL;
    }

    memset(&datafmt, 0, sizeof(datafmt));

    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = blk_describe(self->blk, colnum, &datafmt);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "blk_describe(%d) -> %s\n",
		colnum, value_str(STATUS, status));
    if (PyErr_Occurred())
        return NULL;

    if (status != CS_SUCCEED)
	return Py_BuildValue("iO", status, Py_None);

    fmt = datafmt_alloc(&datafmt, 0);
    if (fmt == NULL)
	return NULL;

    return Py_BuildValue("iN", status, fmt);
}

static char CS_BLKDESC_blk_done__doc__[] = 
"blk_done(type) -> status, outrow";

static PyObject *CS_BLKDESC_blk_done(CS_BLKDESCObj *self, PyObject *args)
{
    int type;
    CS_RETCODE status;
    CS_INT outrow;

    if (!PyArg_ParseTuple(args, "i", &type))
	return NULL;

    if (self->blk == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_BLKDESC has been dropped");
	return NULL;
    }

    /* blk_done(type) -> status, outrow */
    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = blk_done(self->blk, type, &outrow);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "blk_done(%s) -> %s, %d\n",
		value_str(BULK, type), value_str(STATUS, status), (int)outrow);
    if (PyErr_Occurred())
        return NULL;

    return Py_BuildValue("ii", status, outrow);
}

static char CS_BLKDESC_blk_drop__doc__[] = 
"blk_drop() -> status";

static PyObject *CS_BLKDESC_blk_drop(CS_BLKDESCObj *self, PyObject *args)
{
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    if (self->blk == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_BLKDESC has been dropped");
	return NULL;
    }

    /* blk_drop() -> status */
    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = blk_drop(self->blk);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "blk_drop() -> %s\n", value_str(STATUS, status));
    if (status == CS_SUCCEED)
	self->blk = NULL;
    if (PyErr_Occurred())
        return NULL;

    return PyInt_FromLong(status);
}

static char CS_BLKDESC_blk_init__doc__[] = 
"blk_init(direction, table) -> status";

static PyObject *CS_BLKDESC_blk_init(CS_BLKDESCObj *self, PyObject *args)
{
    int direction;
    char *table;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "is", &direction, &table))
	return NULL;

    if (self->blk == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_BLKDESC has been dropped");
	return NULL;
    }

    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = blk_init(self->blk, direction, table, CS_NULLTERM);
    SY_END_THREADS;
    self->direction = direction;
    if (self->debug)
	fprintf(stderr, "blk_init(%s, %s) -> %s\n",
		value_str(BULKDIR, direction), table,
		value_str(STATUS, status));
    if (PyErr_Occurred())
        return NULL;

    return PyInt_FromLong(status);
}

static int property_type(int property)
{
    switch (property) {
    case BLK_IDENTITY:
#ifdef BLK_NOAPI_CHK
    case BLK_NOAPI_CHK:
#endif
#ifdef BLK_SENSITIVITY_LBL
    case BLK_SENSITIVITY_LBL:
#endif
#ifdef HAS_ARRAY_INSERT
    case ARRAY_INSERT:
#endif

	return OPTION_BOOL;
#ifdef BLK_SLICENUM
    case BLK_SLICENUM:
	return OPTION_INT;
#endif
#ifdef BLK_IDSTARTNUM
    case BLK_IDSTARTNUM:
	return OPTION_NUMERIC;
#endif
    default:
	return OPTION_UNKNOWN;
    }
}

static char CS_BLKDESC_blk_props__doc__[] = 
"blk_props(CS_SET, property, value) -> status\n"
"blk_props(CS_GET, property) -> status, value\n"
"blk_props(CS_CLEAR, property) -> status\n";

static PyObject *CS_BLKDESC_blk_props(CS_BLKDESCObj *self, PyObject *args)
{
    int action, property;
    PyObject *obj = NULL;
    CS_RETCODE status;
    CS_INT int_value;
    CS_BOOL bool_value;
    CS_NUMERIC numeric_value;

    if (!first_tuple_int(args, &action))
	return NULL;

    if (self->blk == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_BLKDESC has been dropped");
	return NULL;
    }

    switch (action) {
    case CS_SET:
	/* blk_props(CS_SET, property, value) -> status */
	if (!PyArg_ParseTuple(args, "iiO", &action, &property, &obj))
	    return NULL;

	switch (property_type(property)) {
	case OPTION_BOOL:
	    bool_value = PyInt_AsLong(obj);
	    if (PyErr_Occurred())
		return NULL;

	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = blk_props(self->blk, CS_SET, property,
			       &bool_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "blk_props(CS_SET, %s, %d) -> %s\n",
			value_str(BULKPROPS, property), (int)bool_value,
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
	    status = blk_props(self->blk, CS_SET, property,
			       &int_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "blk_props(CS_SET, %s, %d) -> %s\n",
			value_str(BULKPROPS, property), (int)int_value,
			value_str(STATUS, status));
	    if (PyErr_Occurred())
		return NULL;

	    return PyInt_FromLong(status);

	case OPTION_NUMERIC:
	    if (!Numeric_Check(obj)) {
		PyErr_SetString(PyExc_TypeError, "numeric value expected");
		return NULL;
	    }

	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = blk_props(self->blk, CS_SET, property,
			       &((NumericObj*)obj)->num, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug) {
		char text[NUMERIC_LEN];

		numeric_as_string(obj, text);
		fprintf(stderr, "blk_props(CS_SET, %s, %s) -> %s\n",
			value_str(BULKPROPS, property), text,
			value_str(STATUS, status));
	    }
	    if (PyErr_Occurred())
		return NULL;

	    return PyInt_FromLong(status);

	default:
	    PyErr_SetString(PyExc_TypeError, "unhandled property value");
	    return NULL;
	}
	break;

    case CS_GET:
	/* blk_props(CS_GET, property) -> status, value */
	if (!PyArg_ParseTuple(args, "ii", &action, &property))
	    return NULL;

	switch (property_type(property)) {
	case OPTION_BOOL:
	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = blk_props(self->blk, CS_GET, property,
			       &bool_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "blk_props(CS_GET, %s) -> %s, %d\n",
			value_str(BULKPROPS, property),
			value_str(STATUS, status), (int)bool_value);
	    if (PyErr_Occurred())
		return NULL;

	    return Py_BuildValue("ii", status, bool_value);

	case OPTION_INT:
	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = blk_props(self->blk, CS_GET, property,
			       &int_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    if (self->debug)
		fprintf(stderr, "blk_props(CS_GET, %s) -> %s, %d\n",
			value_str(BULKPROPS, property),
			value_str(STATUS, status), (int)int_value);
	    if (PyErr_Occurred())
		return NULL;

	    return Py_BuildValue("ii", status, int_value);

	case OPTION_NUMERIC:
	    PyErr_Clear();
	    SY_BEGIN_THREADS;
	    status = blk_props(self->blk, CS_GET, property,
			       &numeric_value, CS_UNUSED, NULL);
	    SY_END_THREADS;
	    obj = (PyObject*)numeric_alloc(&numeric_value);
	    if (obj == NULL)
		return NULL;
	    if (self->debug) {
		char text[NUMERIC_LEN];

		numeric_as_string(obj, text);
		fprintf(stderr, "blk_props(CS_GET, %s) -> %s, %s\n",
			value_str(BULKPROPS, property),
			value_str(STATUS, status), text);
	    }
	    if (PyErr_Occurred()) {
		Py_DECREF(obj);
		return NULL;
	    }

	    return Py_BuildValue("iN", status, obj);

	case OPTION_UNKNOWN:
	    PyErr_SetString(PyExc_TypeError, "unknown property value");
	    return NULL;

	default:
	    PyErr_SetString(PyExc_TypeError, "unhandled property value");
	    return NULL;
	}
	break;

    case CS_CLEAR:
	/* blk_props(CS_CLEAR, property) -> status */
	if (!PyArg_ParseTuple(args, "ii", &action, &property))
	    return NULL;

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = blk_props(self->blk, CS_CLEAR, property,
			   NULL, CS_UNUSED, NULL);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "blk_props(CS_CLEAR, %s) -> %s\n",
		    value_str(BULKPROPS, property),
		    value_str(STATUS, status));
	if (PyErr_Occurred())
	    return NULL;

	return PyInt_FromLong(status);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown action");
	return NULL;
    }
}

static char CS_BLKDESC_blk_rowxfer__doc__[] = 
"blk_rowxfer() -> status";

static PyObject *CS_BLKDESC_blk_rowxfer(CS_BLKDESCObj *self, PyObject *args)
{
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    if (self->blk == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_BLKDESC has been dropped");
	return NULL;
    }

    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = blk_rowxfer(self->blk);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "blk_rowxfer() -> %s\n", value_str(STATUS, status));
    if (PyErr_Occurred())
	return NULL;

    return PyInt_FromLong(status);
}

#ifdef HAVE_BLK_ROWXFER_MULT
static char CS_BLKDESC_blk_rowxfer_mult__doc__[] = 
"blk_rowxfer_mult([row_count]) -> status, row_count";

static PyObject *CS_BLKDESC_blk_rowxfer_mult(CS_BLKDESCObj *self, PyObject *args)
{
    int orig_count = 0;
    CS_INT row_count;
    CS_RETCODE status;

    if (!PyArg_ParseTuple(args, "|i", &orig_count))
	return NULL;
    row_count = orig_count;

    if (self->blk == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_BLKDESC has been dropped");
	return NULL;
    }

    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = blk_rowxfer_mult(self->blk, &row_count);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "blk_rowxfer_mult(%d) -> %s, %d\n",
		orig_count, value_str(STATUS, status), (int)row_count);
    if (PyErr_Occurred())
	return NULL;

    return Py_BuildValue("ii", status, row_count);
}
#endif

static char CS_BLKDESC_blk_textxfer__doc__[] = 
"blk_textxfer(str) -> status\n"
"blk_textxfer() -> status, str";

static PyObject *CS_BLKDESC_blk_textxfer(CS_BLKDESCObj *self, PyObject *args)
{
    CS_RETCODE status;

    if (self->blk == NULL) {
	PyErr_SetString(PyExc_TypeError, "CS_BLKDESC has been dropped");
	return NULL;
    }

    if (self->direction == CS_BLK_IN) {
	char *buff;
	int buff_len;

	if (!PyArg_ParseTuple(args, "s#", &buff, &buff_len))
	    return NULL;

	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = blk_textxfer(self->blk, buff, buff_len, NULL);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "blk_textxfer() -> %s\n",
		    value_str(STATUS, status));
	if (PyErr_Occurred())
	    return NULL;

	return PyInt_FromLong(status);
    } else {
	char buff[32 * 1024];
	CS_INT outlen;

	if (!PyArg_ParseTuple(args, ""))
	    return NULL;

	outlen = 0;
	PyErr_Clear();
	SY_BEGIN_THREADS;
	status = blk_textxfer(self->blk, buff, sizeof(buff), &outlen);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "blk_textxfer() -> %s, %d\n",
		    value_str(STATUS, status), (int)outlen);
	if (PyErr_Occurred())
	    return NULL;

	return Py_BuildValue("is#", status, buff, outlen);
    }
}

static struct PyMethodDef CS_BLKDESC_methods[] = {
    { "blk_bind", (PyCFunction)CS_BLKDESC_blk_bind, METH_VARARGS, CS_BLKDESC_blk_bind__doc__ },
    { "blk_describe", (PyCFunction)CS_BLKDESC_blk_describe, METH_VARARGS, CS_BLKDESC_blk_describe__doc__ },
    { "blk_done", (PyCFunction)CS_BLKDESC_blk_done, METH_VARARGS, CS_BLKDESC_blk_done__doc__ },
    { "blk_drop", (PyCFunction)CS_BLKDESC_blk_drop, METH_VARARGS, CS_BLKDESC_blk_drop__doc__ },
    { "blk_init", (PyCFunction)CS_BLKDESC_blk_init, METH_VARARGS, CS_BLKDESC_blk_init__doc__ },
    { "blk_props", (PyCFunction)CS_BLKDESC_blk_props, METH_VARARGS, CS_BLKDESC_blk_props__doc__ },
    { "blk_rowxfer", (PyCFunction)CS_BLKDESC_blk_rowxfer, METH_VARARGS, CS_BLKDESC_blk_rowxfer__doc__ },
#ifdef HAVE_BLK_ROWXFER_MULT
    { "blk_rowxfer_mult", (PyCFunction)CS_BLKDESC_blk_rowxfer_mult, METH_VARARGS, CS_BLKDESC_blk_rowxfer_mult__doc__ },
#endif
    { "blk_textxfer", (PyCFunction)CS_BLKDESC_blk_textxfer, METH_VARARGS, CS_BLKDESC_blk_textxfer__doc__ },
    
    { NULL }			/* sentinel */
};

PyObject *bulk_alloc(CS_CONNECTIONObj *conn, int version)
{
    CS_BLKDESCObj *self;
    CS_RETCODE status;
    CS_BLKDESC *blk;

    self = PyObject_NEW(CS_BLKDESCObj, &CS_BLKDESCType);
    if (self == NULL)
	return NULL;

    SY_LEAK_REG(self);
    self->blk = NULL;
    self->conn = NULL;
    self->direction = 0;
    self->debug = conn->debug;

    PyErr_Clear();
    SY_BEGIN_THREADS;
    status = blk_alloc(conn->conn, version, &blk);
    SY_END_THREADS;
    if (self->debug)
	fprintf(stderr, "blk_alloc(%s) -> %s\n",
		value_str(BULK, version), value_str(STATUS, status));
    if (PyErr_Occurred()) {
	Py_DECREF(self);
	return NULL;
    }

    if (status != CS_SUCCEED) {
	Py_DECREF(self);
	return Py_BuildValue("iO", status, Py_None);
    }

    self->blk = blk;
    self->conn = conn;
    Py_INCREF(self->conn);
    return Py_BuildValue("iN", CS_SUCCEED, self);
}

static void CS_BLKDESC_dealloc(CS_BLKDESCObj *self)
{
    SY_LEAK_UNREG(self);
    if (self->blk) {
	/* should check return == CS_SUCCEED, but we can't handle failure
	   here */
	CS_RETCODE status;

	SY_BEGIN_THREADS;
	status = blk_drop(self->blk);
	SY_END_THREADS;
	if (self->debug)
	    fprintf(stderr, "blk_drop() -> %s\n", value_str(STATUS, status));
    }
    Py_XDECREF(self->conn);
    PyMem_DEL(self);
}

#define OFFSET(x) offsetof(CS_BLKDESCObj, x)

static struct memberlist CS_BLKDESC_memberlist[] = {
    { "conn",      T_OBJECT, OFFSET(conn),      RO },
    { "direction", T_INT,    OFFSET(direction), RO },
    { "debug",     T_INT,    OFFSET(debug) },
    { NULL }			/* Sentinel */
};

static PyObject *CS_BLKDESC_getattr(CS_BLKDESCObj *self, char *name)
{
    PyObject *rv;

    rv = PyMember_Get((char *)self, CS_BLKDESC_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(CS_BLKDESC_methods, (PyObject *)self, name);
}

static int CS_BLKDESC_setattr(CS_BLKDESCObj *self, char *name, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
	return -1;
    }
    return PyMember_Set((char *)self, CS_BLKDESC_memberlist, name, v);
}

static char CS_BLKDESCType__doc__[] = 
"Wrap the Sybase CS_BLKDESC structure and associated functionality.";

PyTypeObject CS_BLKDESCType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "BlkDescType",		/*tp_name*/
    sizeof(CS_BLKDESCObj),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)CS_BLKDESC_dealloc,/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)CS_BLKDESC_getattr, /*tp_getattr*/
    (setattrfunc)CS_BLKDESC_setattr, /*tp_setattr*/
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
    CS_BLKDESCType__doc__	/* documentation string */
};

