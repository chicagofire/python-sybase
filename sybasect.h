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
#include <bkpublic.h>
#include "Python.h"
#include "structmember.h"

#undef WANT_THREADS

#ifdef WANT_THREADS
#define SY_BEGIN_THREADS Py_BEGIN_ALLOW_THREADS
#define SY_END_THREADS Py_END_ALLOW_THREADS
#else
#define SY_BEGIN_THREADS {
#define SY_END_THREADS }
#endif

/* 
Need to turn this on for sybase v>12. Don't know to detected this
from the sybase includes.

#define HAS_ARRAY_INSERT
*/

enum { OPTION_BOOL, OPTION_INT, OPTION_STRING, OPTION_CMD,
       OPTION_NUMERIC, OPTION_UNKNOWN };

typedef struct CS_CONTEXTObj {
    PyObject_HEAD
    CS_CONTEXT *ctx;
    PyObject *servermsg_cb;
    PyObject *clientmsg_cb;
    int debug;
    struct CS_CONTEXTObj *next;
} CS_CONTEXTObj;

typedef struct CS_CONNECTIONObj {
    PyObject_HEAD
    CS_CONTEXTObj *ctx;
    CS_CONNECTION *conn;
    int strip;
    int debug;
    struct CS_CONNECTIONObj *next;
} CS_CONNECTIONObj;

typedef struct {
    PyObject_HEAD
    CS_CONNECTIONObj *conn;
    CS_BLKDESC *blk;
    CS_INT direction;
    int debug;
} CS_BLKDESCObj;

typedef struct {
    PyObject_HEAD
    CS_CONNECTIONObj *conn;
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
    CS_IODESC iodesc;
} CS_IODESCObj;

typedef struct {
    PyObject_HEAD
    int strip;
    CS_DATAFMT fmt;
    char *buff;
    CS_INT *copied;
    CS_SMALLINT *indicator;
} DataBufObj;

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
extern PyTypeObject CS_BLKDESCType;
extern PyTypeObject CS_COMMANDType;
extern PyTypeObject CS_DATAFMTType;
extern PyTypeObject CS_IODESCType;
extern PyTypeObject CS_CLIENTMSGType;
extern PyTypeObject CS_SERVERMSGType;
extern PyTypeObject DataBufType;
extern PyTypeObject NumericType;

int first_tuple_int(PyObject *args, int *int_arg);

enum { CSVER, ACTION, CANCEL, RESULT, RESINFO, CMD, CURSOR, CURSOROPT,
       BULK, BULKDIR, BULKPROPS, DYNAMIC, PROPS, DIRSERV, SECURITY, NETIO,
       OPTION, DATEDAY, DATEFMT, DATAFMT, LEVEL, TYPE, STATUS, STATUSFMT,
       CBTYPE, };

char *value_str(int type, int value);

int Numeric_Check(PyObject *obj);
int DataBuf_Check(PyObject *obj);
int CS_DATAFMT_Check(PyObject *obj);
int CS_IODESC_Check(PyObject *obj);

#define NUMERIC_LEN (CS_MAX_PREC + 1)

void numeric_datafmt(CS_DATAFMT *fmt, int precision, int scale);
void char_datafmt(CS_DATAFMT *fmt);
void int_datafmt(CS_DATAFMT *fmt);
void float_datafmt(CS_DATAFMT *fmt);

CS_CONTEXT *global_ctx(void);

PyObject *ctx_find_object(CS_CONTEXT *cs_ctx);
PyObject *ctx_alloc(CS_INT version);
PyObject *ctx_global(CS_INT version);

PyObject *conn_alloc(CS_CONTEXTObj *ctx);
PyObject *conn_find_object(CS_CONNECTION *conn);

PyObject *bulk_alloc(CS_CONNECTIONObj *conn, int version);

PyObject *cmd_alloc(CS_CONNECTIONObj *conn);
PyObject *cmd_eed(CS_CONNECTIONObj *conn, CS_COMMAND *eed);

PyObject *clientmsg_alloc(void);
PyObject *servermsg_alloc(void);

extern char datafmt_new__doc__[];
PyObject *datafmt_new(PyObject *module, PyObject *args);
PyObject *datafmt_alloc(CS_DATAFMT *datafmt, int strip);

extern char iodesc_new__doc__[];
PyObject *iodesc_new(PyObject *module, PyObject *args);
PyObject *iodesc_alloc(CS_IODESC *iodesc);
PyObject *buffer_alloc(PyObject *obj);

NumericObj *numeric_alloc(CS_NUMERIC *num);
int numeric_as_string(PyObject *obj, char *text);
extern char numeric_new__doc__[];
PyObject *NumericType_new(PyObject *module, PyObject *args);
