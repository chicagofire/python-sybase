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
OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

#include "sybasect.h"
#include "time.h"

static struct PyMethodDef DataBuf_methods[] = {
    { NULL }			/* sentinel */
};

static PyObject *allocate_buffers(DataBufObj *self)
{
    int i;

    self->buff = malloc(self->fmt.maxlength * self->fmt.count);
    if (self->buff == NULL)
	return PyErr_NoMemory();
    self->copied = malloc(sizeof(*self->copied) * self->fmt.count);
    if (self->copied == NULL)
	return PyErr_NoMemory();
    self->indicator = malloc(sizeof(*self->indicator) * self->fmt.count);
    if (self->indicator == NULL)
	return PyErr_NoMemory();

    for (i = 0; i < self->fmt.count; i++)
	self->indicator[i] = CS_NULLDATA;

    return (PyObject*)self;
}

static int DataBuf_ass_item(DataBufObj *self, int i, PyObject *v);

PyObject *databuf_alloc(PyObject *obj)
{
    DataBufObj *self;

    self = PyObject_NEW(DataBufObj, &DataBufType);
    if (self == NULL)
	return NULL;

    SY_LEAK_REG(self);
    self->buff = NULL;
    self->copied = NULL;
    self->indicator = NULL;

    if (CS_DATAFMT_Check(obj)) {
	self->strip = ((CS_DATAFMTObj*)obj)->strip;
	self->fmt = ((CS_DATAFMTObj*)obj)->fmt;
	if (self->fmt.count == 0)
	    self->fmt.count = 1;

	if (allocate_buffers(self) == NULL) {
	    Py_DECREF(self);
	    return NULL;
	}
    } else {
	if (PyInt_Check(obj) || PyLong_Check(obj) || obj == Py_None)
	    int_datafmt(&self->fmt);
	else if (PyFloat_Check(obj))
	    float_datafmt(&self->fmt);
	else if (Numeric_Check(obj))
	    numeric_datafmt(&self->fmt, CS_SRC_VALUE, CS_SRC_VALUE);
	else if (DateTime_Check(obj))
	    datetime_datafmt(&self->fmt, ((DateTimeObj*)obj)->type);
	else if (Money_Check(obj))
	    money_datafmt(&self->fmt, ((MoneyObj*)obj)->type);
	else if (PyString_Check(obj)) {
	    char_datafmt(&self->fmt);
	    self->fmt.maxlength = PyString_Size(obj) + 1;
	} else {
	    PyErr_SetString(PyExc_TypeError, "unsupported parameter type");
	    Py_DECREF(self);
	    return NULL;
	}
	self->fmt.status = CS_INPUTVALUE;
	self->fmt.count = 1;

	if (allocate_buffers(self) == NULL
	    || DataBuf_ass_item(self, 0, obj) < 0) {
	    Py_DECREF(self);
	    return NULL;
	}
    }

    return (PyObject*)self;
}

static void DataBuf_dealloc(DataBufObj *self)
{
    SY_LEAK_UNREG(self);

    if (self->buff != NULL)
	free(self->buff);
    if (self->copied != NULL)
	free(self->copied);
    if (self->indicator != NULL)
	free(self->indicator);

    PyMem_DEL(self);
}

/* Code to handle accessing DataBuf objects as sequence objects */
static int DataBuf_length(DataBufObj *self)
{
    return self->fmt.count;
}

static PyObject *DataBuf_concat(DataBufObj *self, PyObject *bb)
{
    PyErr_SetString(PyExc_TypeError, "buffer concat not supported");
    return NULL;
}

static PyObject *DataBuf_repeat(DataBufObj *self, int n)
{
    PyErr_SetString(PyExc_TypeError, "buffer repeat not supported");
    return NULL;
}

static PyObject *DataBuf_item(DataBufObj *self, int i)
{
    void *item;

    if (i < 0 || i >= self->fmt.count)
	PyErr_SetString(PyExc_IndexError, "buffer index out of range");

    item = self->buff + self->fmt.maxlength * i;

    if (self->indicator[i] == CS_NULLDATA) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    switch (self->fmt.datatype) {
    case CS_LONGCHAR_TYPE:
    case CS_VARCHAR_TYPE:
    case CS_TEXT_TYPE:
    case CS_IMAGE_TYPE:
    case CS_LONGBINARY_TYPE:
    case CS_VARBINARY_TYPE:
    case CS_BINARY_TYPE:
	return PyString_FromStringAndSize(item, self->copied[i]);

    case CS_CHAR_TYPE:
	if (self->strip) {
	    int end;

	    for (end = self->copied[i] - 1; end >= 0; end--)
		if (((char*)item)[end] != ' ')
		    break;
	    return PyString_FromStringAndSize(item, end + 1);
	} else
	    return PyString_FromStringAndSize(item, self->copied[i]);

    case CS_BIT_TYPE:
	return PyInt_FromLong(*(CS_BIT*)item);

    case CS_TINYINT_TYPE:
	return PyInt_FromLong(*(CS_TINYINT*)item);

    case CS_SMALLINT_TYPE:
	return PyInt_FromLong(*(CS_SMALLINT*)item);

    case CS_INT_TYPE:
	return PyInt_FromLong(*(CS_INT*)item);

    case CS_MONEY_TYPE:
	return (PyObject*)money_alloc(item, CS_MONEY_TYPE);

    case CS_MONEY4_TYPE:
	return (PyObject*)money_alloc(item, CS_MONEY4_TYPE);

    case CS_REAL_TYPE:
	return PyFloat_FromDouble(*(CS_REAL*)item);

    case CS_FLOAT_TYPE:
	return PyFloat_FromDouble(*(CS_FLOAT*)item);

    case CS_DATETIME4_TYPE:
        return datetime_alloc(item, CS_DATETIME4_TYPE);

    case CS_DATETIME_TYPE:
        return datetime_alloc(item, CS_DATETIME_TYPE);

    case CS_DECIMAL_TYPE:
    case CS_NUMERIC_TYPE:
	return (PyObject*)numeric_alloc(item);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown data format");
	return NULL;
    }
}

static PyObject *DataBuf_slice(DataBufObj *self, int ilow, int ihigh)
{
    PyErr_SetString(PyExc_TypeError, "buffer slice not supported");
    return NULL;
}

/* Assign to the i-th element of self */
static int DataBuf_ass_item(DataBufObj *self, int i, PyObject *v)
{
    void *item;
    PyObject *obj = NULL;
    int size;

    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError, "buffer delete not supported");
	return -1;
    }
    if (i < 0 || i >= self->fmt.count) {
	PyErr_SetString(PyExc_IndexError, "buffer index out of range");
	return -1;
    }
    if (v == Py_None) {
	self->indicator[i] = CS_NULLDATA;
	return 0;
    }

    item = self->buff + self->fmt.maxlength * i;
    
    switch (self->fmt.datatype) {
    case CS_LONGCHAR_TYPE:
    case CS_VARCHAR_TYPE:
    case CS_TEXT_TYPE:
    case CS_IMAGE_TYPE:
    case CS_LONGBINARY_TYPE:
    case CS_VARBINARY_TYPE:
    case CS_BINARY_TYPE:
	size = PyString_Size(v);
	if (size > self->fmt.maxlength) {
	    PyErr_SetString(PyExc_TypeError, "string too long for buffer");
	    Py_XDECREF(obj);
	    return -1;
	}
	memmove(item, PyString_AsString(v), size);
	if (size < self->fmt.maxlength)
	    ((char*)item)[size] = '\0';
	self->copied[i] = size;
	break;

    case CS_CHAR_TYPE:
	if (!PyString_Check(v)) {
	    obj = PyObject_Str(v);
	    if (obj == NULL)
		return -1;
	    v = obj;
	}
	size = PyString_Size(v);
	if (size > self->fmt.maxlength) {
	    PyErr_SetString(PyExc_TypeError, "string too long for buffer");
	    Py_XDECREF(obj);
	    return -1;
	}
	memmove(item, PyString_AsString(v), size);
	if (size < self->fmt.maxlength)
	    ((char*)item)[size] = '\0';
	self->copied[i] = size;
	break;

    case CS_BIT_TYPE:
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "integer expected");
	    return -1;
	}
	*(CS_BIT*)item = (CS_BIT)PyInt_AsLong(v);
	self->copied[i] = self->fmt.maxlength;
	break;

    case CS_TINYINT_TYPE:
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "integer expected");
	    return -1;
	}
	*(CS_TINYINT*)item = (CS_TINYINT)PyInt_AsLong(v);
	self->copied[i] = self->fmt.maxlength;
	break;

    case CS_SMALLINT_TYPE:
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "integer expected");
	    return -1;
	}
	*(CS_SMALLINT*)item = (CS_SMALLINT)PyInt_AsLong(v);
	self->copied[i] = self->fmt.maxlength;
	break;

    case CS_INT_TYPE:
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "integer expected");
	    return -1;
	}
	*(CS_INT*)item = PyInt_AsLong(v);
	self->copied[i] = self->fmt.maxlength;
	break;

    case CS_MONEY_TYPE:
	if (!money_from_value((MoneyUnion*)item, CS_MONEY_TYPE, v))
	    return -1;
	self->copied[i] = self->fmt.maxlength;
	break;

    case CS_MONEY4_TYPE:
	if (!money_from_value((MoneyUnion*)item, CS_MONEY4_TYPE, v))
	    return -1;
	self->copied[i] = self->fmt.maxlength;
	break;

    case CS_FLOAT_TYPE:
	if (!PyFloat_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "float expected");
	    return -1;
	}
	*(CS_FLOAT*)item = PyFloat_AsDouble(v);
	self->copied[i] = self->fmt.maxlength;
	break;

    case CS_REAL_TYPE:
	if (!PyFloat_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "float expected");
	    return -1;
	}
	*(CS_REAL*)item = (CS_REAL)PyFloat_AsDouble(v);
	self->copied[i] = self->fmt.maxlength;
	break;

    case CS_DATETIME_TYPE:
        if (!DateTime_Check(v)) {
            PyErr_SetString(PyExc_TypeError, "datetime expected");
            return -1;
        }
	if (datetime_assign(v, CS_DATETIME_TYPE, item) != CS_SUCCEED)
	    return -1;
	self->copied[i] = self->fmt.maxlength;
        break;

    case CS_DATETIME4_TYPE:
        if (!DateTime_Check(v)) {
            PyErr_SetString(PyExc_TypeError, "datetime expected");
            return -1;
        }
	if (datetime_assign(v, CS_DATETIME4_TYPE, item) != CS_SUCCEED)
	    return -1;
	self->copied[i] = self->fmt.maxlength;
        break;

    case CS_DECIMAL_TYPE:
    case CS_NUMERIC_TYPE:
	if (!numeric_from_value((CS_NUMERIC*)item, -1, -1, v))
	    return -1;
	self->copied[i] = self->fmt.maxlength;
	break;

    default:
	PyErr_SetString(PyExc_TypeError, "unknown data format");
	return -1;
    }

    Py_XDECREF(obj);
    self->indicator[i] = 0;
    return 0;
}

static int DataBuf_ass_slice(PyListObject *self, int ilow, int ihigh, PyObject *v)
{
    PyErr_SetString(PyExc_TypeError, "buffer slice not supported");
    /* XXXX Replace ilow..ihigh slice of self with v */
    return -1;
}

static PySequenceMethods DataBuf_as_sequence = {
    (inquiry)DataBuf_length,	/*sq_length*/
    (binaryfunc)DataBuf_concat,	/*sq_concat*/
    (intargfunc)DataBuf_repeat,	/*sq_repeat*/
    (intargfunc)DataBuf_item,	/*sq_item*/
    (intintargfunc)DataBuf_slice, /*sq_slice*/
    (intobjargproc)DataBuf_ass_item, /*sq_ass_item*/
    (intintobjargproc)DataBuf_ass_slice, /*sq_ass_slice*/
};

/* Code to access structure members by accessing attributes */

#define OFF(x) offsetof(DataBufObj, x)

static struct memberlist DataBuf_memberlist[] = {
    { "name", T_STRING, OFF(fmt.name) }, /* faked */
    { "datatype", T_INT, OFF(fmt.datatype), RO },
    { "format", T_INT, OFF(fmt.format) },
    { "maxlength", T_INT, OFF(fmt.maxlength), RO },
    { "scale", T_INT, OFF(fmt.scale), RO },
    { "precision", T_INT, OFF(fmt.precision), RO },
    { "status", T_INT, OFF(fmt.status) },
    { "count", T_INT, OFF(fmt.count), RO },
    { "usertype", T_INT, OFF(fmt.usertype), RO },
    { "strip", T_INT, OFF(strip) },
    { NULL }			/* Sentinel */
};

static PyObject *DataBuf_getattr(DataBufObj *self, char *name)
{
    PyObject *rv;

    if (strcmp(name, "name") == 0)
	return PyString_FromStringAndSize(self->fmt.name, self->fmt.namelen);

    rv = PyMember_Get((char *)self, DataBuf_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(DataBuf_methods, (PyObject *)self, name);
}

static int DataBuf_setattr(DataBufObj *self, char *name, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
	return -1;
    }
    if (strcmp(name, "name") == 0) {
	int size;

	if (!PyString_Check(v)) {
	    PyErr_BadArgument();
	    return -1;
	}
	size = PyString_Size(v);
	if (size > sizeof(self->fmt.name)) {
	    PyErr_SetString(PyExc_TypeError, "name too long");
	    return -1;
	}
	strncpy(self->fmt.name, PyString_AsString(v), sizeof(self->fmt.name));
	self->fmt.namelen = size;
	return 0;
    }
    return PyMember_Set((char *)self, DataBuf_memberlist, name, v);
}

static char DataBufType__doc__[] = 
"";

PyTypeObject DataBufType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "DataBufType",		/*tp_name*/
    sizeof(DataBufObj),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)DataBuf_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)DataBuf_getattr, /*tp_getattr*/
    (setattrfunc)DataBuf_setattr, /*tp_setattr*/
    (cmpfunc)0,			/*tp_compare*/
    (reprfunc)0,		/*tp_repr*/
    0,				/*tp_as_number*/
    &DataBuf_as_sequence,	/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L, 0L, 0L, 0L,
    DataBufType__doc__		/* Documentation string */
};
