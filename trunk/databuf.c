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

static struct PyMethodDef Buffer_methods[] = {
    { NULL }			/* sentinel */
};

PyObject *buffer_alloc(CS_DATAFMTObj *datafmt)
{
    BufferObj *self;
    int i;

    self = PyObject_NEW(BufferObj, &BufferType);
    if (self == NULL)
	return NULL;
    self->strip = datafmt->strip;
    self->fmt = datafmt->fmt;
    self->buff = NULL;
    self->copied = NULL;
    self->indicator = NULL;

    if (self->fmt.count == 0)
	self->fmt.count = 1;

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

static void Buffer_dealloc(BufferObj *self)
{
    if (self->buff != NULL)
	free(self->buff);
    if (self->copied != NULL)
	free(self->copied);
    if (self->indicator != NULL)
	free(self->indicator);

    PyMem_DEL(self);
}

/* Code to handle accessing Buffer objects as sequence objects */
static int Buffer_length(BufferObj *self)
{
    return self->fmt.count;
}

static PyObject *Buffer_concat(BufferObj *self, PyObject *bb)
{
    PyErr_SetString(PyExc_TypeError, "buffer concat not supported");
    return NULL;
}

static PyObject *Buffer_repeat(BufferObj *self, int n)
{
    PyErr_SetString(PyExc_TypeError, "buffer repeat not supported");
    return NULL;
}

static PyObject *Buffer_item(BufferObj *self, int i)
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
    case CS_CHAR_TYPE:
	if (self->strip) {
	    int end;

	    for (end = self->copied[i] - 1; end >= 0; end--)
		if (((char*)item)[end] != ' ')
		    break;
	    return PyString_FromStringAndSize(item, end + 1);
	} else
	    return PyString_FromStringAndSize(item, self->copied[i]);

    case CS_INT_TYPE:
	return PyInt_FromLong(*(CS_INT*)item);

    case CS_FLOAT_TYPE:
	return PyFloat_FromDouble(*(CS_FLOAT*)item);

    case CS_DATETIME_TYPE:
	return Py_BuildValue("ii",
			     ((CS_DATETIME*)item)->dtdays,
			     ((CS_DATETIME*)item)->dttime);

    case CS_NUMERIC_TYPE:
	return (PyObject*)numeric_alloc(item);

    default:
	PyErr_SetString(PyExc_TypeError, "unknown data format");
	return NULL;
    }
}

static PyObject *Buffer_slice(BufferObj *self, int ilow, int ihigh)
{
    PyErr_SetString(PyExc_TypeError, "buffer slice not supported");
    return NULL;
}

/* Assign to the i-th element of self */
static int Buffer_ass_item(BufferObj *self, int i, PyObject *v)
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
    case CS_CHAR_TYPE:
	if (!PyString_Check(v)) {
	    obj = PyObject_Str(v);
	    if (obj == NULL)
		return -1;
	    v = obj;
	}
	size = PyString_Size(v);
	if (size + 1 > self->fmt.maxlength) {
	    PyErr_SetString(PyExc_TypeError, "string too long for buffer");
	    Py_XDECREF(obj);
	    return -1;
	}
	memmove(item, PyString_AsString(v), size);
	((char*)item)[size] = '\0';
	self->copied[i] = size;
	break;

    case CS_INT_TYPE:
	if (!PyInt_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "integer expected");
	    return -1;
	}
	*(CS_INT*)item = PyInt_AsLong(v);
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

    case CS_DATETIME_TYPE:
	if (!PyArg_ParseTuple(v, "ii",
			      &((CS_DATETIME*)item)->dtdays,
			      &((CS_DATETIME*)item)->dttime)) {
	    PyErr_SetString(PyExc_TypeError, "(int,int) expected");
	    return -1;
	}
	self->copied[i] = self->fmt.maxlength;
	break;

    case CS_NUMERIC_TYPE:
	if (!Numeric_Check(v)) {
	    PyErr_SetString(PyExc_TypeError, "numeric expected");
	    return -1;
	}
	*(CS_NUMERIC*)item = ((NumericObj*)v)->num;
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

static int Buffer_ass_slice(PyListObject *self, int ilow, int ihigh, PyObject *v)
{
    PyErr_SetString(PyExc_TypeError, "buffer slice not supported");
    /* XXXX Replace ilow..ihigh slice of self with v */
    return -1;
}

static PySequenceMethods Buffer_as_sequence = {
    (inquiry)Buffer_length,	/*sq_length*/
    (binaryfunc)Buffer_concat,	/*sq_concat*/
    (intargfunc)Buffer_repeat,	/*sq_repeat*/
    (intargfunc)Buffer_item,	/*sq_item*/
    (intintargfunc)Buffer_slice, /*sq_slice*/
    (intobjargproc)Buffer_ass_item, /*sq_ass_item*/
    (intintobjargproc)Buffer_ass_slice, /*sq_ass_slice*/
};

/* Code to access structure members by accessing attributes */

#define OFF(x) offsetof(BufferObj, x)

static struct memberlist Buffer_memberlist[] = {
    { "name", T_STRING, OFF(fmt.name), RO }, /* faked */
    { "datatype", T_INT, OFF(fmt.datatype), RO },
    { "format", T_INT, OFF(fmt.format), RO },
    { "maxlength", T_INT, OFF(fmt.maxlength), RO },
    { "scale", T_INT, OFF(fmt.scale), RO },
    { "precision", T_INT, OFF(fmt.precision), RO },
    { "status", T_INT, OFF(fmt.status), RO },
    { "count", T_INT, OFF(fmt.count), RO },
    { "usertype", T_INT, OFF(fmt.usertype), RO },
    { "strip", T_INT, OFF(strip) },
    { NULL }			/* Sentinel */
};

static PyObject *Buffer_getattr(BufferObj *self, char *name)
{
    PyObject *rv;

    if (strcmp(name, "name") == 0)
	return PyString_FromStringAndSize(self->fmt.name, self->fmt.namelen);

    rv = PyMember_Get((char *)self, Buffer_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(Buffer_methods, (PyObject *)self, name);
}

static int Buffer_setattr(BufferObj *self, char *name, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
	return -1;
    }
    return PyMember_Set((char *)self, Buffer_memberlist, name, v);
}

static char BufferType__doc__[] = 
"";

PyTypeObject BufferType = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,				/*ob_size*/
    "Buffer",			/*tp_name*/
    sizeof(BufferObj),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)Buffer_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)Buffer_getattr, /*tp_getattr*/
    (setattrfunc)Buffer_setattr, /*tp_setattr*/
    (cmpfunc)0,			/*tp_compare*/
    (reprfunc)0,		/*tp_repr*/
    0,				/*tp_as_number*/
    &Buffer_as_sequence,	/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L, 0L, 0L, 0L,
    BufferType__doc__		/* Documentation string */
};

