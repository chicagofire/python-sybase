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

#undef FIND_LEAKS

#ifdef FIND_LEAKS
void leak_reg(PyObject *obj);
void leak_unreg(PyObject *obj);
#define SY_LEAK_REG(o) leak_reg((PyObject*)o)
#define SY_LEAK_UNREG(o) leak_unreg((PyObject*)o)
#else
#define SY_LEAK_REG(o)
#define SY_LEAK_UNREG(o)
#endif

enum { OPTION_BOOL, OPTION_INT, OPTION_STRING, OPTION_CMD,
       OPTION_NUMERIC, OPTION_UNKNOWN };

typedef struct CS_CONTEXTObj {
    PyObject_HEAD
    CS_CONTEXT *ctx;
    PyObject *servermsg_cb;
    PyObject *clientmsg_cb;
    int is_global;
    int debug;
    struct CS_CONTEXTObj *next;
} CS_CONTEXTObj;

extern PyTypeObject CS_CONTEXTType;
CS_CONTEXT *global_ctx(void);
PyObject *ctx_find_object(CS_CONTEXT *cs_ctx);
PyObject *ctx_alloc(CS_INT version);
PyObject *ctx_global(CS_INT version);

typedef struct CS_CONNECTIONObj {
    PyObject_HEAD
    CS_CONTEXTObj *ctx;
    CS_CONNECTION *conn;
    int strip;
    int debug;
    struct CS_CONNECTIONObj *next;
} CS_CONNECTIONObj;

extern PyTypeObject CS_CONNECTIONType;
PyObject *conn_alloc(CS_CONTEXTObj *ctx);
PyObject *conn_find_object(CS_CONNECTION *conn);

typedef struct {
    PyObject_HEAD
    CS_CONNECTIONObj *conn;
    CS_BLKDESC *blk;
    CS_INT direction;
    int debug;
} CS_BLKDESCObj;

extern PyTypeObject CS_BLKDESCType;
PyObject *bulk_alloc(CS_CONNECTIONObj *conn, int version);

typedef struct {
    PyObject_HEAD
    CS_CONNECTIONObj *conn;
    CS_COMMAND *cmd;
    int is_eed;
    int strip;
    int debug;
} CS_COMMANDObj;

extern PyTypeObject CS_COMMANDType;
PyObject *cmd_alloc(CS_CONNECTIONObj *conn);
PyObject *cmd_eed(CS_CONNECTIONObj *conn, CS_COMMAND *eed);

typedef struct {
    PyObject_HEAD
    CS_DATAFMT fmt;
    int strip;
} CS_DATAFMTObj;

extern PyTypeObject CS_DATAFMTType;
#define CS_DATAFMT_Check(obj) (obj->ob_type == &CS_DATAFMTType)
void datetime_datafmt(CS_DATAFMT *fmt, int type);
void money_datafmt(CS_DATAFMT *fmt);
void numeric_datafmt(CS_DATAFMT *fmt, int precision, int scale);
void char_datafmt(CS_DATAFMT *fmt);
void int_datafmt(CS_DATAFMT *fmt);
void float_datafmt(CS_DATAFMT *fmt);
extern char datafmt_new__doc__[];
PyObject *datafmt_new(PyObject *module, PyObject *args);
PyObject *datafmt_alloc(CS_DATAFMT *datafmt, int strip);

typedef struct {
    PyObject_HEAD
    CS_IODESC iodesc;
} CS_IODESCObj;

extern PyTypeObject CS_IODESCType;
#define CS_IODESC_Check(obj) (obj->ob_type == &CS_IODESCType)
extern char iodesc_new__doc__[];
PyObject *iodesc_new(PyObject *module, PyObject *args);
PyObject *iodesc_alloc(CS_IODESC *iodesc);

typedef struct {
    PyObject_HEAD
    int strip;
    CS_DATAFMT fmt;
    char *buff;
    CS_INT *copied;
    CS_SMALLINT *indicator;
} DataBufObj;

extern PyTypeObject DataBufType;
#define DataBuf_Check(obj) (obj->ob_type == &DataBufType)
PyObject *databuf_alloc(PyObject *obj);

typedef struct {
    PyObject_HEAD
    CS_NUMERIC num;
} NumericObj;

extern PyTypeObject NumericType;
#define Numeric_Check(obj) (obj->ob_type == &NumericType)
NumericObj *numeric_alloc(CS_NUMERIC *num);
int numeric_as_string(PyObject *obj, char *text);
extern char NumericType_new__doc__[];
PyObject *NumericType_new(PyObject *module, PyObject *args);
void copy_reg_numeric(PyObject *dict);
extern char pickle_numeric__doc__[];
PyObject *pickle_numeric(PyObject *module, PyObject *args);

typedef struct {
    PyObject_HEAD
    CS_MONEY num;
} MoneyObj;

extern PyTypeObject MoneyType;
#define Money_Check(obj) (obj->ob_type == &MoneyType)
MoneyObj *money_alloc(CS_MONEY *num);
int money_as_string(PyObject *obj, char *text);
extern char MoneyType_new__doc__[];
PyObject *MoneyType_new(PyObject *module, PyObject *args);
void copy_reg_money(PyObject *dict);
extern char pickle_money__doc__[];
PyObject *pickle_money(PyObject *module, PyObject *args);

typedef struct {
    PyObject_HEAD
    int type;
    union {
	CS_DATETIME datetime;
	CS_DATETIME4 datetime4;
    } v;
    CS_DATEREC daterec;
    int cracked;
} DateTimeObj;

#define DATETIME_LEN 32

extern PyTypeObject DateTimeType;
#define DateTime_Check(obj) (obj->ob_type == &DateTimeType)
PyObject *datetime_alloc(void *value, int type);
int datetime_assign(PyObject *obj, int type, void *buff);
int datetime_as_string(PyObject *obj, char *text);
extern char DateTimeType_new__doc__[];
PyObject *DateTimeType_new(PyObject *module, PyObject *args);
void copy_reg_datetime(PyObject *dict);
extern char pickle_datetime__doc__[];
PyObject *pickle_datetime(PyObject *module, PyObject *args);

typedef struct {
    PyObject_HEAD
    CS_CLIENTMSG msg;
} CS_CLIENTMSGObj;

typedef struct {
    PyObject_HEAD
    CS_SERVERMSG msg;
} CS_SERVERMSGObj;

extern PyTypeObject CS_CLIENTMSGType;
extern PyTypeObject CS_SERVERMSGType;
PyObject *clientmsg_alloc(void);
PyObject *servermsg_alloc(void);

int first_tuple_int(PyObject *args, int *int_arg);

enum { CSVER, ACTION, CANCEL, RESULT, RESINFO, CMD, CURSOR, CURSOROPT,
       BULK, BULKDIR, BULKPROPS, DYNAMIC, PROPS, DIRSERV, SECURITY, NETIO,
       OPTION, DATEDAY, DATEFMT, DATAFMT, LEVEL, TYPE, STATUS, STATUSFMT,
       CBTYPE, CONSTAT, CURSTAT, };

char *value_str(int type, int value);
char *mask_str(int type, int value);

#define NUMERIC_LEN (CS_MAX_PREC + 1)
#define MONEY_LEN   NUMERIC_LEN
