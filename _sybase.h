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

#include <stdarg.h>
#include <ctpublic.h>
#include "Python.h"
#include "structmember.h"

enum { OPTION_BOOL, OPTION_INT, OPTION_STRING, OPTION_CMD, OPTION_UNKNOWN };

typedef struct {
    PyObject_HEAD
    CS_CONTEXT *ctx;
} CS_CONTEXTObj;

typedef struct {
    PyObject_HEAD
    CS_CONTEXTObj *ctx;
    CS_CONNECTION *con;
    int strip;
    int debug;
} CS_CONNECTIONObj;

typedef struct {
    PyObject_HEAD
    CS_CONNECTIONObj *con;
    CS_COMMAND *cmd;
    int is_eed;
    int strip;
    int debug;
} CS_COMMANDObj;

typedef struct {
    PyObject_HEAD
    CS_DATAFMT fmt;
    int strip;
} CS_DATAFMTObj;

typedef struct {
    PyObject_HEAD
    int strip;
    CS_DATAFMT fmt;
    char *buff;
    CS_INT *copied;
    CS_SMALLINT *indicator;
} BufferObj;

typedef struct {
    PyObject_HEAD
    CS_NUMERIC num;
} NumericObj;

typedef struct {
    PyObject_HEAD
    CS_CLIENTMSG msg;
} CS_CLIENTMSGObj;

typedef struct {
    PyObject_HEAD
    CS_SERVERMSG msg;
} CS_SERVERMSGObj;

extern PyTypeObject CS_CONTEXTType;
extern PyTypeObject CS_CONNECTIONType;
extern PyTypeObject CS_COMMANDType;
extern PyTypeObject CS_DATAFMTType;
extern PyTypeObject CS_CLIENTMSGType;
extern PyTypeObject CS_SERVERMSGType;
extern PyTypeObject BufferType;

int first_tuple_int(PyObject *args, int *int_arg);

enum { ACTION, CANCEL, RESULT, RESINFO, CMD, CURSOR, CURSOROPT, DYNAMIC,
       PROPS, DIRSERV, SECURITY, NETIO, OPTION, DATEDAY, DATEFMT, DATAFMT,
       LEVEL, TYPE, STATUS, STATUSFMT, };

char *value_str(int type, int value);

PyObject *ctx_alloc();
PyObject *ctx_global();
PyObject *con_alloc(CS_CONTEXTObj *ctx);
PyObject *cmd_alloc(CS_CONNECTIONObj *con);
PyObject *cmd_eed(CS_CONNECTIONObj *con, CS_COMMAND *eed);
PyObject *clientmsg_alloc();
PyObject *servermsg_alloc();
PyObject *datafmt_alloc(CS_DATAFMT *datafmt, int strip);
PyObject *buffer_alloc(CS_DATAFMTObj *fmt);
int Numeric_Check(PyObject *obj);
PyObject *NumericType_new(PyObject *module, PyObject *args);
