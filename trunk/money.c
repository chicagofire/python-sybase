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

PyTypeObject MoneyType;

static struct PyMethodDef Money_methods[] = {
    { NULL }			/* sentinel */
};

int money_assign(PyObject *obj, int type, void *buff)
{
    CS_DATAFMT src_fmt;
    CS_DATAFMT dest_fmt;
    CS_RETCODE conv_result;
    CS_INT money_len;

    if (((MoneyObj*)obj)->type == type) {
	if (type == CS_MONEY_TYPE)
	    *(CS_MONEY*)buff = ((MoneyObj*)obj)->v.money;
	else
	    *(CS_MONEY4*)buff = ((MoneyObj*)obj)->v.money4;
	return CS_SUCCEED;
    }
    money_datafmt(&src_fmt, ((MoneyObj*)obj)->type);
    money_datafmt(&dest_fmt, type);

    PyErr_Clear();
    conv_result = cs_convert(global_ctx(),
			     &src_fmt, &((MoneyObj*)obj)->v,
			     &dest_fmt, buff, &money_len);
    if (PyErr_Occurred())
	return CS_FAIL;
    if (conv_result != CS_SUCCEED)
	PyErr_SetString(PyExc_TypeError, "money conversion failed");
    return conv_result;
}

int money_as_string(PyObject *obj, char *text)
{
    CS_DATAFMT money_fmt;
    CS_DATAFMT char_fmt;
    CS_INT char_len;

    money_datafmt(&money_fmt, ((MoneyObj*)obj)->type);
    char_datafmt(&char_fmt);

    return cs_convert(global_ctx(), &money_fmt, &((MoneyObj*)obj)->v,
		      &char_fmt, text, &char_len);
}

MoneyObj *money_alloc(MoneyUnion *num, int type)
{
    MoneyObj *self;

    self = PyObject_NEW(MoneyObj, &MoneyType);
    if (self == NULL)
	return NULL;

    SY_LEAK_REG(self);
    self->type = type;
    if (type == CS_MONEY_TYPE)
	self->v.money = num->money;
    else
	self->v.money4 = num->money4;
    return self;
}

MoneyObj *money_from_int(long num, int type)
{
    CS_RETCODE conv_result;
    CS_DATAFMT int_fmt;
    CS_INT int_value;
    CS_DATAFMT money_fmt;
    MoneyUnion money_value;
    CS_INT money_len;

    int_datafmt(&int_fmt);
    money_datafmt(&money_fmt, type);
    int_value = num;

    PyErr_Clear();
    conv_result = cs_convert(global_ctx(), &int_fmt, &int_value,
			     &money_fmt, &money_value, &money_len);
    if (PyErr_Occurred())
	return NULL;
    if (conv_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "money from int conversion failed");
	return NULL;
    }

    return money_alloc(&money_value, type);
}

MoneyObj *Money_FromInt(PyObject *obj, int type)
{
    return money_from_int(PyInt_AsLong(obj), type);
}

MoneyObj *Money_FromString(PyObject *obj, int type)
{
    CS_RETCODE conv_result;
    CS_DATAFMT char_fmt;
    CS_DATAFMT money_fmt;
    MoneyUnion money_value;
    CS_INT money_len;
    char *str = PyString_AsString(obj);
    
    money_datafmt(&money_fmt, type);
    char_datafmt(&char_fmt);
    char_fmt.maxlength = strlen(str);

    PyErr_Clear();
    conv_result = cs_convert(global_ctx(), &char_fmt, str,
			     &money_fmt, &money_value, &money_len);
    if (PyErr_Occurred())
	return NULL;
    if (conv_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError,
			"money from string conversion failed");
	return NULL;
    }

    return money_alloc(&money_value, type);
}

MoneyObj *Money_FromLong(PyObject *obj, int type)
{
    CS_DATAFMT char_fmt;
    CS_DATAFMT money_fmt;
    MoneyUnion money_value;
    CS_INT money_len;
    CS_RETCODE conv_result;
    PyObject *strobj = PyObject_Str(obj);
    char *str;
    int num_digits;

    if (strobj == NULL)
	return NULL;
    str = PyString_AsString(strobj);
    num_digits = strlen(str);
    if (str[num_digits - 1] == 'L')
	num_digits--;
    char_datafmt(&char_fmt);
    char_fmt.maxlength = num_digits;
    money_datafmt(&money_fmt, type);

    PyErr_Clear();
    conv_result = cs_convert(global_ctx(), &char_fmt, str,
			     &money_fmt, &money_value, &money_len);
    Py_DECREF(strobj);
    if (PyErr_Occurred())
	return NULL;
    if (conv_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "money from long conversion failed");
	return NULL;
    }

    return money_alloc(&money_value, type);
}

MoneyObj *Money_FromFloat(PyObject *obj, int type)
{
    CS_RETCODE conv_result;
    CS_DATAFMT float_fmt;
    CS_FLOAT float_value;
    CS_DATAFMT money_fmt;
    MoneyUnion money_value;
    CS_INT money_len;

    float_datafmt(&float_fmt);
    money_datafmt(&money_fmt, type);
    float_value = PyFloat_AsDouble(obj);

    PyErr_Clear();
    conv_result = cs_convert(global_ctx(), &float_fmt, &float_value,
			     &money_fmt, &money_value, &money_len);
    if (PyErr_Occurred())
	return NULL;
    if (conv_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "money from float conversion failed");
	return NULL;
    }
    return money_alloc(&money_value, type);
}

MoneyObj *Money_FromMoney(PyObject *obj, int type)
{
    CS_DATAFMT src_fmt;
    CS_DATAFMT dest_fmt;
    CS_RETCODE conv_result;
    CS_INT money_len;
    MoneyUnion money_value;

    if (type == ((MoneyObj*)obj)->type) {
	Py_INCREF(obj);
	return (MoneyObj*)obj;
    }

    money_datafmt(&src_fmt, ((MoneyObj*)obj)->type);
    money_datafmt(&dest_fmt, type);

    PyErr_Clear();
    conv_result = cs_convert(global_ctx(),
			     &src_fmt, &((MoneyObj*)obj)->v,
			     &dest_fmt, &money_value, &money_len);
    if (PyErr_Occurred())
	return NULL;
    if (conv_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "money from money conversion failed");
	return NULL;
    }
    return money_alloc(&money_value, type);
}

static void Money_dealloc(MoneyObj *self)
{
    SY_LEAK_UNREG(self);
    PyMem_DEL(self);
}

static void money_promote(MoneyUnion *from, MoneyUnion *to)
{
    CS_DATAFMT src_fmt;
    CS_DATAFMT dest_fmt;
    CS_INT money_len;

    money_datafmt(&src_fmt, CS_MONEY4_TYPE);
    money_datafmt(&dest_fmt, CS_MONEY_TYPE);

    cs_convert(global_ctx(), &src_fmt, from, &dest_fmt, to, &money_len);
}

static int Money_compare(MoneyObj *v, MoneyObj *w)
{
    MoneyUnion tmp, *m1, *m2;
    int type;
    CS_RETCODE cmp_result;
    CS_INT result;

    m1 = &v->v;
    m2 = &w->v;
    if (v->type == w->type)
	type = v->type;
    else {
	type = CS_MONEY_TYPE;
	if (v->type == CS_MONEY4_TYPE) {
	    money_promote(&v->v, &tmp);
	    m1 = &tmp;
	} else {
	    money_promote(&w->v, &tmp);
	    m2 = &tmp;
	}
    }

    PyErr_Clear();
    cmp_result = cs_cmp(global_ctx(), type, m1, m2, &result);
    if (PyErr_Occurred())
	return 0;
    if (cmp_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "compare failed");
	return 0;
    }

    return result;
}

static PyObject *Money_repr(MoneyObj *self)
{
    CS_RETCODE conv_result;
    char text[MONEY_LEN];

    PyErr_Clear();
    conv_result = money_as_string((PyObject*)self, text);
    if (PyErr_Occurred())
	return NULL;
    if (conv_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "money to string conversion failed");
	return NULL;
    }

    return PyString_FromString(text);
}

static PyObject *Money_long(MoneyObj *v);

static long Money_hash(MoneyObj *self)
{
    unsigned char *ptr;
    long hash;
    int i, len;

    if (self->type == CS_MONEY_TYPE)
	len = sizeof(self->v.money);
    else
	len = sizeof(self->v.money4);
    hash = 0;
    for (i = 0, ptr = (unsigned char*)&self->v; i < len; i++, ptr++)
	hash = hash * 31 + *ptr;
    return (hash == -1) ? -2 : hash;
}

static PyObject *Money_arithmetic(int op, MoneyObj *v, MoneyObj *w)
{
    MoneyUnion result, tmp, *m1, *m2;
    int type;

    m1 = &v->v;
    m2 = &w->v;
    if (v->type == w->type)
	type = v->type;
    else {
	type = CS_MONEY_TYPE;
	if (v->type == CS_MONEY4_TYPE) {
	    money_promote(&v->v, &tmp);
	    m1 = &tmp;
	} else {
	    money_promote(&w->v, &tmp);
	    m2 = &tmp;
	}
    }

    PyErr_Clear();
    if (cs_calc(global_ctx(), op, type, m1, m2, &result) != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "money arithmetic failed");
	return NULL;
    }
    if (PyErr_Occurred())
	return NULL;

    return (PyObject*)money_alloc(&result, type);
}

/* Code to access Money objects as numbers */
static PyObject *Money_add(MoneyObj *v, MoneyObj *w)
{
    return Money_arithmetic(CS_ADD, v, w);
}

static PyObject *Money_sub(MoneyObj *v, MoneyObj *w)
{
    return Money_arithmetic(CS_SUB, v, w);
}

static PyObject *Money_mul(MoneyObj *v, MoneyObj *w)
{
    return Money_arithmetic(CS_MULT, v, w);
}

static PyObject *Money_div(MoneyObj *v, MoneyObj *w)
{
    return Money_arithmetic(CS_DIV, v, w);
}

static MoneyObj *money_minusone(void)
{
    static MoneyObj *minusone;
    if (minusone == NULL)
	minusone = money_from_int(-1, CS_MONEY_TYPE);
    return minusone;
}

static MoneyObj *money_zero(void)
{
    static MoneyObj *zero;
    if (zero == NULL)
	zero = money_from_int(0, CS_MONEY_TYPE);
    return zero;
}

static PyObject *Money_neg(MoneyObj *v)
{
    return Money_mul(v, money_minusone());
}

static PyObject *Money_pos(MoneyObj *v)
{
    Py_INCREF(v);
    return (PyObject*)v;
}

static PyObject *Money_abs(MoneyObj *v)
{
    if (Money_compare(v, money_zero()) < 0)
	return Money_mul(v, money_minusone());
    if (PyErr_Occurred())
	return NULL;

    Py_INCREF(v);
    return (PyObject*)v;
}

static int Money_nonzero(MoneyObj *v)
{
    return Money_compare(v, money_zero()) != 0;
}

static int Money_coerce(PyObject **pv, PyObject **pw)
{
    MoneyObj *num = NULL;
    if (PyInt_Check(*pw))
	num = Money_FromInt(*pw, CS_MONEY_TYPE);
    else if (PyLong_Check(*pw))
	num = Money_FromLong(*pw, CS_MONEY_TYPE);
    else if (PyFloat_Check(*pw))
	num = Money_FromFloat(*pw, CS_MONEY_TYPE);
    if (num) {
	*pw = (PyObject*)num;
	Py_INCREF(*pv);
	return 0;
    }
    return 1;
}

static PyObject *Money_int(MoneyObj *v)
{
    CS_RETCODE conv_result;
    CS_DATAFMT money_fmt;
    CS_DATAFMT int_fmt;
    CS_INT int_value;
    CS_INT int_len;

    money_datafmt(&money_fmt, v->type);
    int_datafmt(&int_fmt);

    PyErr_Clear();
    conv_result = cs_convert(global_ctx(), &money_fmt, &v->v,
			     &int_fmt, &int_value, &int_len);
    if (PyErr_Occurred())
	return NULL;
    if (conv_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "int conversion failed");
	return NULL;
    }

    return PyInt_FromLong(int_value);
}

static PyObject *Money_long(MoneyObj *v)
{
    CS_RETCODE conv_result;
    char *end;
    char text[MONEY_LEN];

    PyErr_Clear();
    conv_result = money_as_string((PyObject*)v, text);
    if (PyErr_Occurred())
	return NULL;
    if (conv_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "money to string conversion failed");
	return NULL;
    }

    return PyLong_FromString(text, &end, 10);
}

static PyObject *Money_float(MoneyObj *v)
{
    CS_RETCODE conv_result;
    CS_DATAFMT money_fmt;
    CS_DATAFMT float_fmt;
    CS_FLOAT float_value;
    CS_INT float_len;

    money_datafmt(&money_fmt, v->type);
    float_datafmt(&float_fmt);

    PyErr_Clear();
    conv_result = cs_convert(global_ctx(), &money_fmt, &v->v,
			     &float_fmt, &float_value, &float_len);
    if (PyErr_Occurred())
	return NULL;
    if (conv_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "float conversion failed");
	return NULL;
    }

    return PyFloat_FromDouble(float_value);
}

static PyNumberMethods Money_as_number = {
    (binaryfunc)Money_add,	/*nb_add*/
    (binaryfunc)Money_sub,	/*nb_subtract*/
    (binaryfunc)Money_mul,	/*nb_multiply*/
    (binaryfunc)Money_div,	/*nb_divide*/
    (binaryfunc)0,		/*nb_remainder*/
    (binaryfunc)0,		/*nb_divmod*/
    (ternaryfunc)0,		/*nb_power*/
    (unaryfunc)Money_neg,	/*nb_negative*/
    (unaryfunc)Money_pos,	/*nb_positive*/
    (unaryfunc)Money_abs,	/*nb_absolute*/
    (inquiry)Money_nonzero,	/*nb_nonzero*/
    (unaryfunc)0,		/*nb_invert*/
    (binaryfunc)0,		/*nb_lshift*/
    (binaryfunc)0,		/*nb_rshift*/
    (binaryfunc)0,		/*nb_and*/
    (binaryfunc)0,		/*nb_xor*/
    (binaryfunc)0,		/*nb_or*/
    (coercion)Money_coerce,	/*nb_coerce*/
    (unaryfunc)Money_int,	/*nb_int*/
    (unaryfunc)Money_long,	/*nb_long*/
    (unaryfunc)Money_float,	/*nb_float*/
    (unaryfunc)0,		/*nb_oct*/
    (unaryfunc)0,		/*nb_hex*/
};

#define OFFSET(x) offsetof(MoneyObj, x)

static struct memberlist Money_memberlist[] = {
    { "type", T_INT, OFFSET(type), RO },
    { NULL }
};

static PyObject *Money_getattr(MoneyObj *self, char *name)
{
    PyObject *rv;
	
    rv = PyMember_Get((char*)self, Money_memberlist, name);
    if (rv)
	return rv;
    PyErr_Clear();
    return Py_FindMethod(Money_methods, (PyObject *)self, name);
}

static int Money_setattr(MoneyObj *self, char *name, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
	return -1;
    }
    return PyMember_Set((char*)self, Money_memberlist, name, v);
}

static char MoneyType__doc__[] = 
"";

PyTypeObject MoneyType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "MoneyType",		/*tp_name*/
    sizeof(MoneyObj),		/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)Money_dealloc,	/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)Money_getattr, /*tp_getattr*/
    (setattrfunc)Money_setattr, /*tp_setattr*/
    (cmpfunc)Money_compare,	/*tp_compare*/
    (reprfunc)Money_repr,	/*tp_repr*/
    &Money_as_number,		/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    (hashfunc)Money_hash,	/*tp_hash*/
    (ternaryfunc)0,		/*tp_call*/
    (reprfunc)0,		/*tp_str*/

    0L,0L,0L,0L,
    MoneyType__doc__
};

char MoneyType_new__doc__[] =
"money(num [, type = CS_MONEY_TYPE]) -> num\n"
"\n"
"Create a Sybase money object.";

/* Implement the Sybase.money() method
 */
PyObject *MoneyType_new(PyObject *module, PyObject *args)
{
    MoneyObj *self;
    PyObject *obj;
    int type = CS_MONEY_TYPE;

    self = PyObject_NEW(MoneyObj, &MoneyType);
    if (self == NULL)
	return NULL;

    SY_LEAK_REG(self);
    if (PyArg_ParseTuple(args, "O|i", &obj, &type)) {
	MoneyObj *num = NULL;

	if (PyInt_Check(obj))
	    num = Money_FromInt(obj, type);
	else if (PyLong_Check(obj))
	    num = Money_FromLong(obj, type);
	else if (PyFloat_Check(obj))
	    num = Money_FromFloat(obj, type);
	else if (PyString_Check(obj))
	    num = Money_FromString(obj, type);
	else if (Money_Check(obj))
	    num = Money_FromMoney(obj, type);
	else {
	    PyErr_SetString(PyExc_TypeError, "could not convert to Money");
	    return NULL;
	}
	if (num)
	    return (PyObject*)num;
    }

    Py_DECREF(self);
    return NULL;
}

/* Used in unpickler
 */
static PyObject *money_constructor = NULL;

/* Register the money type with the copy_reg module.  This allows
 * Python to (un)pickle money objects.  The equivalent Python code
 * is this:
 *
 * def pickle_money(m):
 *     return money, (str(m), m.type)
 * 
 * copy_reg.pickle(type(money(1)), pickle_money, money)
 */
/* Money pickling function
 */
char pickle_money__doc__[] =
"pickle_money(m) -> money, (str(m), m.type)\n"
"\n"
"Used to pickle the money data type.";

PyObject *pickle_money(PyObject *module, PyObject *args)
{
    CS_RETCODE conv_result;
    MoneyObj *obj = NULL;
    PyObject *values = NULL,
	*tuple = NULL;
    char text[MONEY_LEN];

    if (!PyArg_ParseTuple(args, "O!", &MoneyType, &obj))
	goto error;

    PyErr_Clear();
    conv_result = money_as_string((PyObject*)obj, text);
    if (PyErr_Occurred())
	return NULL;
    if (conv_result != CS_SUCCEED) {
	PyErr_SetString(PyExc_TypeError, "money to string conversion failed");
	return NULL;
    }

    if ((values = Py_BuildValue("(si)", text, obj->type)) == NULL)
	goto error;
    tuple = Py_BuildValue("(OO)", money_constructor, values);

error:
    Py_XDECREF(values);
    return tuple;
}

/* Register Money type pickler
 */
void copy_reg_money(PyObject *dict)
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
    if ((money_constructor = PyDict_GetItemString(dict, "money")) == NULL)
	goto error;
    if ((pickler = PyDict_GetItemString(dict, "pickle_money")) == NULL)
	goto error;
    obj = PyObject_CallFunction(pickle_func, "OOO",
				&MoneyType, pickler, money_constructor);

error:
    Py_XDECREF(obj);
    Py_XDECREF(pickle_func);
    Py_XDECREF(module);
}
