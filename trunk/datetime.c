/**********************************************************************
Copyright 1999 by Object Craft

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Object Craft not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

OBJECT CRAFT DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL OBJECT CRAFT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
**********************************************************************/

#include "mssqldb.h"

PyTypeObject DateTimeType;

static void datetime_crack(DateTimeObj *self)
{
    if (self->type == CS_DATETIME_TYPE)
	cs_dt_crack(global_ctx(), self->type, &self->v.datetime, &date_rec);
    else
	cs_dt_crack(global_ctx(), self->type, &self->v.datetime4, &date_rec);
    self->cracked = 1;
}

static struct PyMethodDef DateTime_methods[] = {
    { NULL }			/* sentinel */
};

int datetime_as_string(PyObject *obj, char *text)
{
    CS_DATAFMT datetime_fmt;
    CS_DATAFMT char_fmt;
    CS_INT char_len;
    int type;
    void *data;

    type = ((DateTimeObj*)self)->type;
    datetime_datafmt(&datetime_fmt, type);
    char_datafmt(&char_fmt);

    if (type == CS_DATETIME_TYPE)
	data = &((DateTimeObj*)self)->v.datetime;
    else
	data = &((DateTimeObj*)self)->v.datetime4;
    return cs_convert(global_ctx(),
		      &datetime_fmt, data,
		      &char_fmt, text, &char_len);
}

PyObject *datetime_alloc(int type, void *value)
{
    DateTimeObj *self;

    self = PyObject_NEW(DateTimeObj, &DateTimeType);
    if (self == NULL)
	return NULL;

    self->type = type;
    if (type == CS_DATETIME_TYPE)
	memcpy(&self->v.datetime, value, sizeof(self->v.datetime));
    else
	memcpy(&self->v.datetime4, value, sizeof(self->v.datetime4));
    memset(&self->daterec, 0, sizeof(self->daterec));
    self->cracked = 0;
    return (PyObject*)self;
}

PyObject *DateTime_FromString(PyObject *obj)
{
    CS_DATAFMT datetime_fmt;
    CS_DATAFMT char_fmt;
    CS_DATETIME datetime;
    CS_INT datetime_len;
    char *str = PyString_AsString(obj);
    
    datetime_datafmt(&datetime_fmt, CS_DATETIME_TYPE);
    char_datafmt(&char_fmt);

    PyErr_Clear();
    if (cs_convert(global_ctx(),
		   &char_fmt, str,
		   &datetime_fmt, &datetime, &datetime_len) <= 0) {
	PyErr_SetString(PyExc_TypeError, "datetime from string conversion failed");
	return NULL;
    }
    if (PyErr_Occurred())
	return NULL;

    return datetime_alloc(CS_DATETIME_TYPE, &datetime);
}

static void DateTime_dealloc(DateTimeObj *self)
{
    PyMem_DEL(self);
}

static PyObject *DateTime_repr(DateTimeObj *self)
{
    char text[DATETIME_LEN];

    PyErr_Clear();
    datetime_as_string((PyObject*)self, text);
    if (PyErr_Occurred())
	return NULL;

    return PyString_FromString(text);
}

static PyObject *DateTime_int(DateTimeObj *v)
{
    CS_DATAFMT datetime_fmt;
    CS_DATAFMT int_fmt;
    CS_INT int_value;
    CS_INT int_len;
    void *value;

    datetime_datafmt(&datetime_fmt, self->type);
    int_datafmt(&int_fmt);

    if (type == CS_DATETIME_TYPE)
	value = &self->v.datetime;
    else
	value = &self->v.datetime4;

    PyErr_Clear();
    if (cs_convert(global_ctx(), &datetime_fmt, value,
		   &int_fmt, &int_value, &int_len) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "int conversion failed");
	return NULL;
    }
    if (PyErr_Occurred())
	return NULL;

    return PyInt_FromLong(int_value);
}

static PyObject *DateTime_long(DateTimeObj *v)
{
    char *end;
    char text[DATETIME_LEN];

    PyErr_Clear();
    datetime_as_string((PyObject*)v, text);
    if (PyErr_Occurred())
	return NULL;

    return PyLong_FromString(text, &end, 10);
}

static PyObject *DateTime_float(DateTimeObj *v)
{
    CS_DATAFMT datetime_fmt;
    CS_DATAFMT float_fmt;
    CS_FLOAT float_value;
    CS_INT float_len;
    void *value;

    datetime_datafmt(&datetime_fmt, v->type);
    float_datafmt(&float_fmt);

    if (type == CS_DATETIME_TYPE)
	value = &self->v.datetime;
    else
	value = &self->v.datetime4;

    PyErr_Clear();
    if (cs_convert(global_ctx(), &datetime_fmt, value,
		   &float_fmt, &float_value, &float_len) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "float conversion failed");
	return NULL;
    }
    if (PyErr_Occurred())
	return NULL;

    return PyFloat_FromDouble(float_value);
}

static PyNumberMethods DateTime_as_number = {
    (binaryfunc)0,		/*nb_add*/
    (binaryfunc)0,		/*nb_subtract*/
    (binaryfunc)0,		/*nb_multiply*/
    (binaryfunc)0,		/*nb_divide*/
    (binaryfunc)0,		/*nb_remainder*/
    (binaryfunc)0,		/*nb_divmod*/
    (ternaryfunc)0,		/*nb_power*/
    (unaryfunc)0,		/*nb_negative*/
    (unaryfunc)0,		/*nb_positive*/
    (unaryfunc)0,		/*nb_absolute*/
    (inquiry)0,			/*nb_nonzero*/
    (unaryfunc)0,		/*nb_invert*/
    (binaryfunc)0,		/*nb_lshift*/
    (binaryfunc)0,		/*nb_rshift*/
    (binaryfunc)0,		/*nb_and*/
    (binaryfunc)0,		/*nb_xor*/
    (binaryfunc)0,		/*nb_or*/
    (coercion)0,		/*nb_coerce*/
    (unaryfunc)DateTime_int,	/*nb_int*/
    (unaryfunc)DateTime_long,	/*nb_long*/
    (unaryfunc)DateTime_float,	/*nb_float*/
    (unaryfunc)0,		/*nb_oct*/
    (unaryfunc)0,		/*nb_hex*/
};

#define OFFSET(x) offsetof(DateTimeObj, x)

static struct memberlist DateTime_memberlist[] = {
    { "type",      T_INT, OFFSET(type), RO },
    { "year",      T_INT, OFFSET(daterec.dateyear) },
    { "month",     T_INT, OFFSET(daterec.datemonth) },
    { "day",       T_INT, OFFSET(daterec.datedmonth) },
    { "dayofyear", T_INT, OFFSET(daterec.datedyear) },
    { "weekday",   T_INT, OFFSET(daterec.datedweek) },
    { "hour",      T_INT, OFFSET(daterec.datehour) },
    { "minute",    T_INT, OFFSET(daterec.dateminute) },
    { "second",    T_INT, OFFSET(daterec.datesecond) },
    { "msecond",   T_INT, OFFSET(daterec.datemsecond) },
    { "tzone",     T_INT, OFFSET(daterec.datetzone) },
    { NULL }
};

static PyObject *DateTime_getattr(DateTimeObj *self, char *name)
{
    PyObject *rv;

    if (!self->cracked && strcmp(name, "type") != 0) {
	PyErr_Clear();
	datetime_crack(self);
	if (PyErr_Occurred())
	    return NULL;
    }

    rv = PyMember_Get((char*)self, DateTime_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(DateTime_methods, (PyObject *)self, name);
}

static int DateTime_setattr(DateTimeObj *self, char *name, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
	return -1;
    }
    return PyMember_Set((char*)self, DateTime_memberlist, name, v);
}

static char DateTimeType__doc__[] = 
"";

PyTypeObject DateTimeType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "DateTimeType",		/*tp_name*/
    sizeof(DateTimeObj),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)DateTime_dealloc,/*tp_dealloc*/
    (printfunc)DateTime_print,	/*tp_print*/
    (getattrfunc)DateTime_getattr, /*tp_getattr*/
    (setattrfunc)DateTime_setattr, /*tp_setattr*/
    (cmpfunc)0,			/*tp_compare*/
    (reprfunc)DateTime_repr,	/*tp_repr*/
    &DateTime_as_number,	/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    (hashfunc)0,		/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    DateTimeType__doc__		/* Documentation string */
};

char datetime_new__doc__[] =
"datetime(type, s) -> DateTime\n"
"\n"
"Create a datetime object.";

/* Implement the mssqldb.datetime() method
 */
PyObject *DateTimeType_new(PyObject *module, PyObject *args)
{
    CS_DATAFMT datetime_fmt;
    CS_DATAFMT char_fmt;
    int type;
    char *str;
    CS_DATETIME datetime;

    if (!PyArg_ParseTuple(args, "is", &type, &str))
	return NULL;

    datetime_datafmt(&datetime_fmt, type);
    char_datafmt(&char_fmt);

    PyErr_Clear();
    if (cs_convert(global_ctx(),
		   &char_fmt, str,
		   &datetime_fmt, &datetime, &datetime_len) <= 0) {
	PyErr_SetString(PyExc_TypeError, "datetime from string conversion failed");
	return NULL;
    }
    if (PyErr_Occurred())
	return NULL;

    return datetime_alloc(type, &datetime);
}

/* Used in unpickler
 */
static PyObject *datetime_constructor = NULL;

/* Register the datetime type with the copy_reg module.  This allows
 * Python to (un)pickle datetime objects.  The equivalent Python code
 * is this:
 *
 * def pickle_datetime(dt):
 *     return datetime, (dt.type, str(dt))
 * 
 * copy_reg.pickle(type(datetime(1)), pickle_datetime, datetime)
 */
char pickle_datetime__doc__[] =
"pickle_datetime(dt) -> datetime, (dt.type, str(dt))\n"
"\n"
"Used to pickle the datetime data type.";

/* DateTime pickling function
 */
PyObject *pickle_datetime(PyObject *module, PyObject *args)
{
    DateTimeObj *obj = NULL;
    PyObject *values = NULL,
	*tuple = NULL;
    char text[DATETIME_LEN];

    if (!PyArg_ParseTuple(args, "O!", &DateTimeType, &obj))
	goto error;
    datetime_as_string((PyObject*)obj, text);
    if ((values = Py_BuildValue("(is)", obj->type, text)) == NULL)
	goto error;
    tuple = Py_BuildValue("(OO)", datetime_constructor, values);

error:
    Py_XDECREF(values);
    return tuple;
}

/* Register DateTime type pickler
 */
void copy_reg_datetime(PyObject *dict)
{
    PyObject *module = NULL,
	*pickle_func = NULL,
	*pickler = NULL,
	*obj = NULL;

    module = PyImport_ImportModule("copy_reg");
    if (module == NULL)
	goto error;
    if ((pickle_func = PyObject_GetAttrString(module, "pickle")) == NULL)
	goto error;
    if ((datetime_constructor = PyDict_GetItemString(dict, "datetime")) == NULL)
	goto error;
    if ((pickler = PyDict_GetItemString(dict, "pickle_datetime")) == NULL)
	goto error;
    obj = PyObject_CallFunction(pickle_func, "OOO",
				&DateTimeType, pickler, datetime_constructor);

error:
    Py_XDECREF(obj);
    Py_XDECREF(pickle_func);
    Py_XDECREF(module);
}
