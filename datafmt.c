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
	if (size > CS_MAX_NAME) {
	    PyErr_SetString(PyExc_TypeError, "name too long");
	    return -1;
	}
	strcpy(self->fmt.name, PyString_AsString(v));
	self->fmt.namelen = size;
	return 0;
    }
    return PyMember_Set((char *)self, CS_DATAFMT_memberlist, name, v);
}

static char CS_DATAFMTType__doc__[] = 
"";

PyTypeObject CS_DATAFMTType = {
    PyObject_HEAD_INIT(&PyType_Type)
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
