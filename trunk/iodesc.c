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

static struct PyMethodDef CS_IODESC_methods[] = {
    { NULL }			/* sentinel */
};

PyObject *iodesc_alloc(CS_IODESC *iodesc)
{
    CS_IODESCObj *self;

    self = PyObject_NEW(CS_IODESCObj, &CS_IODESCType);
    if (self == NULL)
	return NULL;
    self->iodesc = *iodesc;
    return (PyObject*)self;
}

char iodesc_new__doc__[] =
"CS_IODESC() -> iodesc\n"
"\n"
"Allocate a new CS_IODESC object.";

PyObject *iodesc_new(PyObject *module, PyObject *args)
{
    CS_IODESCObj *self;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    self = PyObject_NEW(CS_IODESCObj, &CS_IODESCType);
    if (self == NULL)
	return NULL;

    memset(&self->iodesc, 0, sizeof(self->iodesc));
    return (PyObject*)self;
}

static void CS_IODESC_dealloc(CS_IODESCObj *self)
{
    PyMem_DEL(self);
}

/* Code to access structure members by accessing attributes */

#define OFF(x) offsetof(CS_IODESCObj, x)

static struct memberlist CS_IODESC_memberlist[] = {
    { "iotype",        T_INT,    OFF(iodesc.iotype) },
    { "datatype",      T_INT,    OFF(iodesc.datatype) },
    { "usertype",      T_INT,    OFF(iodesc.usertype) },
    { "total_txtlen",  T_INT,    OFF(iodesc.total_txtlen) },
    { "offset",        T_INT,    OFF(iodesc.offset) },
    { "log_on_update", T_INT,    OFF(iodesc.log_on_update) },
    { "name",          T_STRING, OFF(iodesc.name) }, /* faked */
    { "timestamp",     T_STRING, OFF(iodesc.timestamp) }, /* faked */
    { "textptr",       T_STRING, OFF(iodesc.textptr) }, /* faked */
    { NULL }			/* Sentinel */
};

static PyObject *CS_IODESC_getattr(CS_IODESCObj *self, char *name)
{
    PyObject *rv;

    if (strcmp(name, "name") == 0)
	return PyString_FromStringAndSize(self->iodesc.name,
					  self->iodesc.namelen);
    if (strcmp(name, "timestamp") == 0)
	return PyString_FromStringAndSize(self->iodesc.timestamp,
					  self->iodesc.timestamplen);
    if (strcmp(name, "textptr") == 0)
	return PyString_FromStringAndSize(self->iodesc.textptr,
					  self->iodesc.textptrlen);

    rv = PyMember_Get((char *)self, CS_IODESC_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(CS_IODESC_methods, (PyObject *)self, name);
}

static int CS_IODESC_setattr(CS_IODESCObj *self, char *name, PyObject *v)
{
    void *ptr = NULL;
    CS_INT *len_ptr = NULL;
    int max_len = 0;

    if (v == NULL) {
	PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
	return -1;
    }
    if (strcmp(name, "name") == 0) {
	ptr = self->iodesc.name;
	len_ptr = &self->iodesc.namelen;
	max_len = sizeof(self->iodesc.name);
    } else if (strcmp(name, "timestamp") == 0) {
	ptr = self->iodesc.timestamp;
	len_ptr = &self->iodesc.timestamplen;
	max_len = sizeof(self->iodesc.timestamp);
    } else if (strcmp(name, "textptr") == 0) {
	ptr = self->iodesc.textptr;
	len_ptr = &self->iodesc.textptrlen;
	max_len = sizeof(self->iodesc.textptr);
    }
    if (ptr != NULL) {
	int size;

	if (!PyString_Check(v)) {
	    PyErr_BadArgument();
	    return -1;
	}
	size = PyString_Size(v);
	if (size > max_len) {
	    PyErr_SetString(PyExc_TypeError, "too long");
	    return -1;
	}
	memmove(ptr, PyString_AsString(v), size);
	*len_ptr = size;
	return 0;
    }
    return PyMember_Set((char *)self, CS_IODESC_memberlist, name, v);
}

static char CS_IODESCType__doc__[] = 
"";

PyTypeObject CS_IODESCType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "CS_IODESC",		/*tp_name*/
    sizeof(CS_IODESCObj),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)CS_IODESC_dealloc,/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)CS_IODESC_getattr, /*tp_getattr*/
    (setattrfunc)CS_IODESC_setattr, /*tp_setattr*/
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
    CS_IODESCType__doc__	/* Documentation string */
};

int CS_IODESC_Check(PyObject *obj)
{
    return obj->ob_type == &CS_IODESCType;
}