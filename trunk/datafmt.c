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

void numeric_datafmt(CS_DATAFMT *fmt, int precision, int scale)
{
    fmt->datatype = CS_NUMERIC_TYPE;
    fmt->maxlength = sizeof(CS_NUMERIC);
    fmt->locale = NULL;
    fmt->format = CS_FMT_NULLTERM;
    fmt->precision = (precision < 0) ? CS_SRC_VALUE : precision;
    fmt->scale = (scale < 0) ? CS_SRC_VALUE : scale;
}

void char_datafmt(CS_DATAFMT *fmt)
{
    fmt->datatype = CS_CHAR_TYPE;
    fmt->maxlength = NUMERIC_LEN;
    fmt->locale = NULL;
    fmt->format = CS_FMT_NULLTERM;
    fmt->scale = 0;
    fmt->precision = 0;
}

void int_datafmt(CS_DATAFMT *fmt)
{
    fmt->datatype = CS_INT_TYPE;
    fmt->maxlength = sizeof(CS_INT);
    fmt->locale = NULL;
    fmt->format = CS_FMT_UNUSED;
    fmt->scale = 0;
    fmt->precision = 0;
}

void float_datafmt(CS_DATAFMT *fmt)
{
    fmt->datatype = CS_FLOAT_TYPE;
    fmt->maxlength = sizeof(CS_FLOAT);
    fmt->locale = NULL;
    fmt->format = CS_FMT_UNUSED;
    fmt->scale = 0;
    fmt->precision = 0;
}

static struct PyMethodDef CS_DATAFMT_methods[] = {
    { NULL }			/* sentinel */
};

PyObject *datafmt_alloc(CS_DATAFMT *datafmt, int strip)
{
    CS_DATAFMTObj *self;

    self = PyObject_NEW(CS_DATAFMTObj, &CS_DATAFMTType);
    if (self == NULL)
	return NULL;
    self->strip = strip;
    self->fmt = *datafmt;
    return (PyObject*)self;
}

char datafmt_new__doc__[] =
"CS_DATAFMT() -> fmt\n"
"\n"
"Allocate a new CS_DATAFMT object.";

PyObject *datafmt_new(PyObject *module, PyObject *args)
{
    CS_DATAFMTObj *self;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    self = PyObject_NEW(CS_DATAFMTObj, &CS_DATAFMTType);
    if (self == NULL)
	return NULL;

    memset(&self->fmt, 0, sizeof(self->fmt));
    self->strip = 0;
    char_datafmt(&self->fmt);
    self->fmt.maxlength = 1;
    return (PyObject*)self;
}

static void CS_DATAFMT_dealloc(CS_DATAFMTObj *self)
{
    PyMem_DEL(self);
}

/* Code to access structure members by accessing attributes */

#define OFF(x) offsetof(CS_DATAFMTObj, x)

static struct memberlist CS_DATAFMT_memberlist[] = {
    { "name", T_STRING, OFF(fmt.name) }, /* faked */
    { "datatype", T_INT, OFF(fmt.datatype) },
    { "format", T_INT, OFF(fmt.format) },
    { "maxlength", T_INT, OFF(fmt.maxlength) },
    { "scale", T_INT, OFF(fmt.scale) },
    { "precision", T_INT, OFF(fmt.precision) },
    { "status", T_INT, OFF(fmt.status) },
    { "count", T_INT, OFF(fmt.count) },
    { "usertype", T_INT, OFF(fmt.usertype) },
    { "strip", T_INT, OFF(strip) },
    { NULL }			/* Sentinel */
};

static PyObject *CS_DATAFMT_getattr(CS_DATAFMTObj *self, char *name)
{
    PyObject *rv;

    if (strcmp(name, "name") == 0)
	return PyString_FromStringAndSize(self->fmt.name, self->fmt.namelen);

    rv = PyMember_Get((char *)self, CS_DATAFMT_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(CS_DATAFMT_methods, (PyObject *)self, name);
}

static int CS_DATAFMT_setattr(CS_DATAFMTObj *self, char *name, PyObject *v)
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
    return PyMember_Set((char *)self, CS_DATAFMT_memberlist, name, v);
}

static char CS_DATAFMTType__doc__[] = 
"";

PyTypeObject CS_DATAFMTType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "CS_DATAFMT",		/*tp_name*/
    sizeof(CS_DATAFMTObj),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)CS_DATAFMT_dealloc,/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)CS_DATAFMT_getattr, /*tp_getattr*/
    (setattrfunc)CS_DATAFMT_setattr, /*tp_setattr*/
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
    CS_DATAFMTType__doc__	/* Documentation string */
};

int CS_DATAFMT_Check(PyObject *obj)
{
    return obj->ob_type == &CS_DATAFMTType;
}
