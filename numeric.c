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

#include "sybasect.h"

#define maxv(a,b) ((a) > (b) ? (a) : (b))

PyTypeObject NumericType;

static struct PyMethodDef Numeric_methods[] = {
    { NULL }			/* sentinel */
};

CS_CONTEXT *global_ctx()
{
    static CS_CONTEXT *ctx;

    if (ctx == NULL)
	cs_ctx_global(CS_VERSION_100, &ctx);
    return ctx;
}

/* Does obj reference a Numeric object?
 */
int Numeric_Check(PyObject *obj)
{
    return obj->ob_type == &NumericType;
}

int numeric_as_string(PyObject *obj, char *text)
{
    CS_DATAFMT numeric_fmt;
    CS_DATAFMT char_fmt;
    CS_INT char_len;

    numeric_datafmt(&numeric_fmt, CS_SRC_VALUE, CS_SRC_VALUE);
    char_datafmt(&char_fmt);
    return cs_convert(global_ctx(), &numeric_fmt, &((NumericObj*)obj)->num,
		      &char_fmt, text, &char_len);
}

NumericObj *numeric_alloc(CS_NUMERIC *num)
{
    NumericObj *self;

    self = PyObject_NEW(NumericObj, &NumericType);
    if (self == NULL)
	return NULL;

    memcpy(&self->num, num, sizeof(self->num));
    return self;
}

NumericObj *numeric_from_int(long num, int precision, int scale)
{
    CS_DATAFMT int_fmt;
    CS_INT int_value;
    CS_DATAFMT numeric_fmt;
    CS_NUMERIC numeric_value;
    CS_INT numeric_len;

    int_datafmt(&int_fmt);
    if (precision < 0)
	precision = 16;
    if (scale < 0)
	scale = 0;
    numeric_datafmt(&numeric_fmt, precision, scale);
    int_value = num;
    if (cs_convert(global_ctx(), &int_fmt, &int_value,
		   &numeric_fmt, &numeric_value, &numeric_len) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "numeric from int conversion failed");
	return NULL;
    }
    return numeric_alloc(&numeric_value);
}

NumericObj *Numeric_FromInt(PyObject *obj, int precision, int scale)
{
    return numeric_from_int(PyInt_AsLong(obj), precision, scale);
}

NumericObj *Numeric_FromString(PyObject *obj, int precision, int scale)
{
    CS_DATAFMT char_fmt;
    CS_DATAFMT numeric_fmt;
    CS_NUMERIC numeric_value;
    CS_INT numeric_len;
    char *str = PyString_AsString(obj);
    char *dp = strchr(str, '.');
    int len = strlen(str);
    
    char_datafmt(&char_fmt);
    if (precision < 0) {
	precision = len;
	if (precision > CS_MAX_PREC)
	    precision = CS_MAX_PREC;
    }
    if (scale < 0) {
	if (dp) {
	    int decimals = len - (dp - str) - 1;
	    if (decimals > CS_MAX_SCALE)
		decimals = CS_MAX_SCALE;
	    scale = decimals;
	} else
	    scale = 0;
    }
    numeric_datafmt(&numeric_fmt, precision, scale);
    if (cs_convert(global_ctx(), &char_fmt, str,
		   &numeric_fmt, &numeric_value, &numeric_len) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "numeric from string conversion failed");
	return NULL;
    }
    return numeric_alloc(&numeric_value);
}

NumericObj *Numeric_FromLong(PyObject *obj, int precision, int scale)
{
    CS_DATAFMT char_fmt;
    CS_DATAFMT numeric_fmt;
    CS_NUMERIC numeric_value;
    CS_INT numeric_len;
    CS_RETCODE conv_result;
    PyObject *strobj = PyObject_Str(obj);
    char *str;

    if (strobj == NULL)
	return NULL;
    str = PyString_AsString(strobj);
    char_datafmt(&char_fmt);
    char_fmt.maxlength = strlen(str) - 1;
    if (precision < 0)
	precision = strlen(str) - 1;
    if (precision > CS_MAX_PREC)
	precision = CS_MAX_PREC;
    if (scale < 0)
	scale = 0;
    numeric_datafmt(&numeric_fmt, precision, scale);
    conv_result = cs_convert(global_ctx(), &char_fmt, str,
			     &numeric_fmt, &numeric_value, &numeric_len);
    Py_DECREF(strobj);
    if (conv_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "numeric from long conversion failed");
	return NULL;
    }
    return numeric_alloc(&numeric_value);
}

NumericObj *Numeric_FromFloat(PyObject *obj, int precision, int scale)
{
    CS_DATAFMT float_fmt;
    CS_FLOAT float_value;
    CS_DATAFMT numeric_fmt;
    CS_NUMERIC numeric_value;
    CS_INT numeric_len;

    float_datafmt(&float_fmt);
    if (precision < 0)
	precision = CS_MAX_PREC;
    if (scale < 0)
	scale = 12;
    numeric_datafmt(&numeric_fmt, precision, scale);
    float_value = PyFloat_AsDouble(obj);
    if (cs_convert(global_ctx(), &float_fmt, &float_value,
		   &numeric_fmt, &numeric_value, &numeric_len) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "numeric from float conversion failed");
	return NULL;
    }
    return numeric_alloc(&numeric_value);
}

NumericObj *Numeric_FromNumeric(PyObject *obj, int precision, int scale)
{
    CS_DATAFMT src_numeric_fmt;
    CS_DATAFMT numeric_fmt;
    CS_NUMERIC numeric_value;
    CS_INT numeric_len;

    if ((precision < 0 || precision == ((NumericObj*)obj)->num.precision)
	&& (scale < 0 || scale == ((NumericObj*)obj)->num.scale)) {
	Py_INCREF(obj);
	return (NumericObj*)obj;
    }
    numeric_datafmt(&src_numeric_fmt, CS_SRC_VALUE, CS_SRC_VALUE);
    if (precision < 0)
	precision = ((NumericObj*)obj)->num.precision;
    if (scale < 0)
	scale = ((NumericObj*)obj)->num.scale;
    numeric_datafmt(&numeric_fmt, precision, scale);
    if (cs_convert(global_ctx(), &src_numeric_fmt, &((NumericObj*)obj)->num,
		   &numeric_fmt, &numeric_value, &numeric_len) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "numeric conversion failed");
	return NULL;
    }
    return numeric_alloc(&numeric_value);
}

static void Numeric_dealloc(NumericObj *self)
{
    PyMem_DEL(self);
}

static int Numeric_print(NumericObj *self, FILE *fp, int flags)
{
    char text[NUMERIC_LEN];

    numeric_as_string((PyObject*)self, text);
    fputs(text, fp);
    return 0;
}

static int Numeric_compare(NumericObj *v, NumericObj *w)
{
    CS_INT result;

    if (cs_cmp(global_ctx(), CS_NUMERIC_TYPE,
	       &v->num, &w->num, &result) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "compare failed");
	return 0;
    }
    return result;
}

static PyObject *Numeric_repr(NumericObj *self)
{
    char text[NUMERIC_LEN];

    numeric_as_string((PyObject*)self, text);
    return PyString_FromString(text);
}

static PyObject *Numeric_long(NumericObj *v);

/* Implement a hash function such that:
 * hash(100) == hash(Sybase.numeric(100))
 * hash(100200300400500L) == hash(Sybase.numeric(100200300400500L))
 */
static long Numeric_hash(NumericObj *self)
{
    long hash;
    CS_DATAFMT numeric_fmt;
    CS_DATAFMT int_fmt;
    CS_INT int_value;
    CS_INT int_len;
    CS_RETCODE conv_result;
    PyObject *long_value;

    /* Only use own hash for numbers with decimal places
     */
    if (self->num.scale > 0) {
	int i;

	hash = 0;
	for (i = 0; i < sizeof(self->num.array); i++)
	    hash = hash * 31 + self->num.array[i];
	return (hash == -1) ? -2 : hash;
    }
    /* Try as int
     */
    numeric_datafmt(&numeric_fmt, CS_SRC_VALUE, CS_SRC_VALUE);
    int_datafmt(&int_fmt);
    conv_result = cs_convert(global_ctx(), &numeric_fmt, &self->num,
			     &int_fmt, &int_value, &int_len);
    if (conv_result == CS_SUCCEED)
	return (int_value == -1) ? -2 : int_value;
    /* XXX clear_cs_messages(); */
    /* Try as long
     */
    long_value = Numeric_long(self);
    if (long_value != NULL) {
	hash = PyObject_Hash(long_value);
	Py_DECREF(long_value);
	return hash;
    }
    return -1;
}

/* Code to access Numeric objects as numbers */
static PyObject * Numeric_add(NumericObj *v, NumericObj *w)
{
    CS_NUMERIC result;

    result.precision = maxv(v->num.precision, w->num.precision) + 1;
    if (result.precision > CS_MAX_PREC)
	result.precision = CS_MAX_PREC;
    result.scale = maxv(v->num.scale, w->num.scale);
    if (cs_calc(global_ctx(), CS_ADD, CS_NUMERIC_TYPE,
		&v->num, &w->num, &result) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "numeric add failed");
	return NULL;
    }
    return (PyObject*)numeric_alloc(&result);
}

static PyObject *Numeric_sub(NumericObj *v, NumericObj *w)
{
    CS_NUMERIC result;

    result.precision = maxv(v->num.precision, w->num.precision) + 1;
    if (result.precision > CS_MAX_PREC)
	result.precision = CS_MAX_PREC;
    result.scale = maxv(v->num.scale, w->num.scale);
    if (cs_calc(global_ctx(), CS_SUB, CS_NUMERIC_TYPE,
		&v->num, &w->num, &result) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "numeric sub failed");
	return NULL;
    }
    return (PyObject*)numeric_alloc(&result);
}

static PyObject *Numeric_mul(NumericObj *v, NumericObj *w)
{
    CS_NUMERIC result;

    result.precision = v->num.precision + w->num.precision;
    if (result.precision > CS_MAX_PREC)
	result.precision = CS_MAX_PREC;
    result.scale = v->num.scale + w->num.scale;
    if (result.scale > CS_MAX_SCALE)
	result.scale = CS_MAX_SCALE;
    if (cs_calc(global_ctx(), CS_MULT, CS_NUMERIC_TYPE,
		&v->num, &w->num, &result) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "numeric mul failed");
	return NULL;
    }
    return (PyObject*)numeric_alloc(&result);
}

static PyObject *Numeric_div(NumericObj *v, NumericObj *w)
{
    CS_NUMERIC result;

    result.precision = v->num.precision + w->num.precision;
    if (result.precision > CS_MAX_PREC)
	result.precision = CS_MAX_PREC;
    result.scale = v->num.scale + w->num.scale;
    if (result.scale > CS_MAX_SCALE)
	result.scale = CS_MAX_SCALE;
    if (cs_calc(global_ctx(), CS_DIV, CS_NUMERIC_TYPE,
		&v->num, &w->num, &result) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "numeric div failed");
	return NULL;
    }
    return (PyObject*)numeric_alloc(&result);
}

static NumericObj *numeric_minusone(void)
{
    static NumericObj *minusone;
    if (minusone == NULL)
	minusone = numeric_from_int(-1, -1, -1);
    return minusone;
}

static NumericObj *numeric_zero(void)
{
    static NumericObj *zero;
    if (zero == NULL)
	zero = numeric_from_int(0, -1, -1);
    return zero;
}

static PyObject *Numeric_neg(NumericObj *v)
{
    return Numeric_mul(v, numeric_minusone());
}

static PyObject *Numeric_pos(NumericObj *v)
{
    Py_INCREF(v);
    return (PyObject*)v;
}

static PyObject *Numeric_abs(NumericObj *v)
{
    if (Numeric_compare(v, numeric_zero()) < 0)
	return Numeric_mul(v, numeric_minusone());
    else {
	Py_INCREF(v);
	return (PyObject*)v;
    }
}

static int Numeric_nonzero(NumericObj *v)
{
    return Numeric_compare(v, numeric_zero()) != 0;
}

static int Numeric_coerce(PyObject **pv, PyObject **pw)
{
    NumericObj *num = NULL;
    if (PyInt_Check(*pw))
	num = Numeric_FromInt(*pw, -1, -1);
    else if (PyLong_Check(*pw))
	num = Numeric_FromLong(*pw, -1, -1);
    else if (PyFloat_Check(*pw))
	num = Numeric_FromFloat(*pw, -1, -1);
    if (num) {
	*pw = (PyObject*)num;
	Py_INCREF(*pv);
	return 0;
    }
    return 1;
}

static PyObject *Numeric_int(NumericObj *v)
{
    CS_DATAFMT numeric_fmt;
    CS_DATAFMT int_fmt;
    CS_INT int_value;
    CS_INT int_len;

    numeric_datafmt(&numeric_fmt, CS_SRC_VALUE, CS_SRC_VALUE);
    int_datafmt(&int_fmt);
    if (cs_convert(global_ctx(), &numeric_fmt, &v->num,
		   &int_fmt, &int_value, &int_len) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "int conversion failed");
	return NULL;
    }
    return PyInt_FromLong(int_value);
}

static PyObject *Numeric_long(NumericObj *v)
{
    char *end;
    char text[NUMERIC_LEN];

    numeric_as_string((PyObject*)v, text);
    return PyLong_FromString(text, &end, 10);
}

static PyObject *Numeric_float(NumericObj *v)
{
    CS_DATAFMT numeric_fmt;
    CS_DATAFMT float_fmt;
    CS_FLOAT float_value;
    CS_INT float_len;

    numeric_datafmt(&numeric_fmt, CS_SRC_VALUE, CS_SRC_VALUE);
    float_datafmt(&float_fmt);
    if (cs_convert(global_ctx(), &numeric_fmt, &v->num,
		   &float_fmt, &float_value, &float_len) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "float conversion failed");
	return NULL;
    }
    return PyFloat_FromDouble(float_value);
}

static PyNumberMethods Numeric_as_number = {
    (binaryfunc)Numeric_add,	/*nb_add*/
    (binaryfunc)Numeric_sub,	/*nb_subtract*/
    (binaryfunc)Numeric_mul,	/*nb_multiply*/
    (binaryfunc)Numeric_div,	/*nb_divide*/
    (binaryfunc)0,		/*nb_remainder*/
    (binaryfunc)0,		/*nb_divmod*/
    (ternaryfunc)0,		/*nb_power*/
    (unaryfunc)Numeric_neg,	/*nb_negative*/
    (unaryfunc)Numeric_pos,	/*nb_positive*/
    (unaryfunc)Numeric_abs,	/*nb_absolute*/
    (inquiry)Numeric_nonzero,	/*nb_nonzero*/
    (unaryfunc)0,		/*nb_invert*/
    (binaryfunc)0,		/*nb_lshift*/
    (binaryfunc)0,		/*nb_rshift*/
    (binaryfunc)0,		/*nb_and*/
    (binaryfunc)0,		/*nb_xor*/
    (binaryfunc)0,		/*nb_or*/
    (coercion)Numeric_coerce,	/*nb_coerce*/
    (unaryfunc)Numeric_int,	/*nb_int*/
    (unaryfunc)Numeric_long,	/*nb_long*/
    (unaryfunc)Numeric_float,	/*nb_float*/
    (unaryfunc)0,		/*nb_oct*/
    (unaryfunc)0,		/*nb_hex*/
};

#define OFF(x) offsetof(NumericObj, x)

static struct memberlist Numeric_memberlist[] = {
    { "precision", T_BYTE, OFF(num.precision), RO },
    { "scale",     T_BYTE, OFF(num.scale), RO },
    { NULL }			/* Sentinel */
};

static PyObject *Numeric_getattr(NumericObj *self, char *name)
{
    PyObject *rv;
	
    rv = PyMember_Get((char*)self, Numeric_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(Numeric_methods, (PyObject *)self, name);
}

static int Numeric_setattr(NumericObj *self, char *name, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
	return -1;
    }
    return PyMember_Set((char*)self, Numeric_memberlist, name, v);
}

static char NumericType__doc__[] = 
"";

PyTypeObject NumericType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "Numeric",			/*tp_name*/
    sizeof(NumericObj),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)Numeric_dealloc,/*tp_dealloc*/
    (printfunc)Numeric_print,	/*tp_print*/
    (getattrfunc)Numeric_getattr, /*tp_getattr*/
    (setattrfunc)Numeric_setattr, /*tp_setattr*/
    (cmpfunc)Numeric_compare,	/*tp_compare*/
    (reprfunc)Numeric_repr,	/*tp_repr*/
    &Numeric_as_number,		/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    (hashfunc)Numeric_hash,	/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    /* Space for future expansion */
    0L,0L,0L,0L,
    NumericType__doc__		/* Documentation string */
};

char numeric_new__doc__[] =
"numeric(num, precision = -1, scale = -1) -> num\n"
"\n"
"Create a Sybase numeric object.";

/* Implement the Sybase.numeric() method
 */
PyObject *NumericType_new(PyObject *module, PyObject *args)
{
    NumericObj *self;
    int precision, scale;
    PyObject *obj;

    self = PyObject_NEW(NumericObj, &NumericType);
    if (self == NULL)
	return NULL;

    precision = -1;
    scale = -1;
    if (PyArg_ParseTuple(args, "O|ii", &obj, &precision, &scale)) {
	NumericObj *num = NULL;

	if (PyInt_Check(obj))
	    num = Numeric_FromInt(obj, precision, scale);
	else if (PyLong_Check(obj))
	    num = Numeric_FromLong(obj, precision, scale);
	else if (PyFloat_Check(obj))
	    num = Numeric_FromFloat(obj, precision, scale);
	else if (PyString_Check(obj))
	    num = Numeric_FromString(obj, precision, scale);
	else if (Numeric_Check(obj))
	    num = Numeric_FromNumeric(obj, precision, scale);
	else {
	    PyErr_SetString(PyExc_TypeError, "could not convert to Numeric");
	    return NULL;
	}
	if (num)
	    return (PyObject*)num;
    }

    Py_DECREF(self);
    return NULL;
}
