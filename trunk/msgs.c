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

static struct PyMethodDef CS_CLIENTMSG_methods[] = {
    { NULL }			/* sentinel */
};

PyObject *clientmsg_alloc()
{
    CS_CLIENTMSGObj *self;

    self = PyObject_NEW(CS_CLIENTMSGObj, &CS_CLIENTMSGType);
    if (self == NULL)
	return NULL;

    memset(&self->msg, 0, sizeof(self->msg));
    return (PyObject*)self;
}

static void CS_CLIENTMSG_dealloc(CS_CLIENTMSGObj *self)
{
    PyMem_DEL(self);
}

#define CLIENT_OFF(x) offsetof(CS_CLIENTMSG, x)

/* Adapted from Sybase cstypes.h */
#if defined (SYB_LP64) || defined (_AIX)
#define T_MSGNUM T_UINT
#else
#define T_MSGNUM T_LONG
#endif

static struct memberlist CS_CLIENTMSG_memberlist[] = {
    { "severity", T_INT, CLIENT_OFF(severity), RO },
    { "msgnumber", T_MSGNUM, CLIENT_OFF(msgnumber), RO },
    { "msgstring", T_STRING, CLIENT_OFF(msgstring), RO }, /* faked */
    { "osnumber", T_INT, CLIENT_OFF(osnumber), RO },
    { "osstring", T_STRING, CLIENT_OFF(osstring), RO }, /* faked */
    { "status", T_INT, CLIENT_OFF(status), RO },
    { "sqlstate", T_STRING, CLIENT_OFF(sqlstate), RO }, /* faked */
    { NULL }			/* Sentinel */
};

static PyObject *CS_CLIENTMSG_getattr(CS_CLIENTMSGObj *self, char *name)
{
    if (strcmp(name, "msgstring") == 0)
	return PyString_FromStringAndSize(self->msg.msgstring,
					  self->msg.msgstringlen);
    if (strcmp(name, "osstring") == 0)
	return PyString_FromStringAndSize(self->msg.osstring,
					  self->msg.osstringlen);
    if (strcmp(name, "sqlstate") == 0)
	return PyString_FromStringAndSize(self->msg.sqlstate,
					  self->msg.sqlstatelen);
    return PyMember_Get((char *)&self->msg, CS_CLIENTMSG_memberlist, name);
}

static int CS_CLIENTMSG_setattr(CS_CLIENTMSGObj *self, char *name, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
	return -1;
    }
    return PyMember_Set((char *)&self->msg, CS_CLIENTMSG_memberlist, name, v);
}

static char CS_CLIENTMSGType__doc__[] = 
"";

PyTypeObject CS_CLIENTMSGType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "CS_CLIENTMSG",		/*tp_name*/
    sizeof(CS_CLIENTMSGObj),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)CS_CLIENTMSG_dealloc,/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)CS_CLIENTMSG_getattr, /*tp_getattr*/
    (setattrfunc)CS_CLIENTMSG_setattr, /*tp_setattr*/
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
    CS_CLIENTMSGType__doc__	/* Documentation string */
};

static struct PyMethodDef CS_SERVERMSG_methods[] = {
    { NULL }			/* sentinel */
};

PyObject *servermsg_alloc()
{
    CS_SERVERMSGObj *self;

    self = PyObject_NEW(CS_SERVERMSGObj, &CS_SERVERMSGType);
    if (self == NULL)
	return NULL;

    memset(&self->msg, 0, sizeof(self->msg));
    return (PyObject*)self;
}

static void CS_SERVERMSG_dealloc(CS_SERVERMSGObj *self)
{
    PyMem_DEL(self);
}

#define SERV_OFF(x) offsetof(CS_SERVERMSG, x)

static struct memberlist CS_SERVERMSG_memberlist[] = {
    { "msgnumber", T_MSGNUM, SERV_OFF(msgnumber), RO },
    { "state", T_INT, SERV_OFF(state), RO },
    { "severity", T_INT, SERV_OFF(severity), RO },
    { "text", T_STRING, SERV_OFF(text), RO }, /* faked */
    { "svrname", T_STRING, SERV_OFF(svrname), RO }, /* faked */
    { "proc", T_STRING, SERV_OFF(proc), RO }, /* faked */
    { "line", T_INT, SERV_OFF(line), RO },
    { "status", T_INT, SERV_OFF(status), RO },
    { "sqlstate", T_STRING, SERV_OFF(sqlstate), RO }, /* faked */
    { NULL }			/* Sentinel */
};

static PyObject *CS_SERVERMSG_getattr(CS_SERVERMSGObj *self, char *name)
{
    if (strcmp(name, "text") == 0)
	return PyString_FromStringAndSize(self->msg.text,
					  self->msg.textlen);
    if (strcmp(name, "svrname") == 0)
	return PyString_FromStringAndSize(self->msg.svrname,
					  self->msg.svrnlen);
    if (strcmp(name, "proc") == 0)
	return PyString_FromStringAndSize(self->msg.proc,
					  self->msg.proclen);
    if (strcmp(name, "sqlstate") == 0)
	return PyString_FromStringAndSize(self->msg.sqlstate,
					  self->msg.sqlstatelen);
    return PyMember_Get((char *)&self->msg, CS_SERVERMSG_memberlist, name);
}

static int CS_SERVERMSG_setattr(CS_SERVERMSGObj *self, char *name, PyObject *v)
{
    if (v == NULL) {
	PyErr_SetString(PyExc_AttributeError, "Cannot delete attribute");
	return -1;
    }
    return PyMember_Set((char *)&self->msg, CS_SERVERMSG_memberlist, name, v);
}

static char CS_SERVERMSGType__doc__[] = 
"";

PyTypeObject CS_SERVERMSGType = {
    PyObject_HEAD_INIT(0)
    0,				/*ob_size*/
    "CS_SERVERMSG",		/*tp_name*/
    sizeof(CS_SERVERMSGObj),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    /* methods */
    (destructor)CS_SERVERMSG_dealloc,/*tp_dealloc*/
    (printfunc)0,		/*tp_print*/
    (getattrfunc)CS_SERVERMSG_getattr, /*tp_getattr*/
    (setattrfunc)CS_SERVERMSG_setattr, /*tp_setattr*/
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
    CS_SERVERMSGType__doc__	/* Documentation string */
};
