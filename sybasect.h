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
#include "pythread.h"
#include "structmember.h"

#ifdef HAVE_FREETDS
#include "freetds.h"
#endif

#ifdef WANT_THREADS
#define SY_DECLARE_LOCK PyThread_type_lock lock
#define SY_THREAD_STATE PyThreadState *_save;
#define SY_LOCK_CLEAR(self) self->lock = NULL
#define SY_LOCK_ALLOC(self) \
    self->lock = PyThread_allocate_lock(); \
    if (self->lock == NULL) { \
	return NULL; \
    }
#define SY_LOCK_FREE(self) \
    if (self->lock != NULL) { \
	PyThread_free_lock(self->lock); \
    }
#define SY_LOCK_ACQUIRE(owner) \
    if (owner->lock != NULL) { \
	PyThread_acquire_lock(owner->lock, WAIT_LOCK); \
    }
#define SY_LOCK_RELEASE(owner) \
    if (owner->lock != NULL) { \
	PyThread_release_lock(owner->lock); \
    }
#define SY_BEGIN_THREADS Py_UNBLOCK_THREADS
#define SY_END_THREADS Py_BLOCK_THREADS
#else
#define SY_DECLARE_LOCK
#define SY_THREAD_STATE
#define SY_LOCK_CLEAR(self)
#define SY_LOCK_ALLOC(self)
#define SY_LOCK_FREE(self)
#define SY_LOCK_ACQUIRE(owner)
#define SY_LOCK_RELEASE(owner)
#define SY_BEGIN_THREADS
#define SY_END_THREADS
#endif

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
       OPTION_NUMERIC, OPTION_LOCALE, OPTION_UNKNOWN };

void debug_msg(char *fmt, ...);

typedef struct CS_CONTEXTObj {
    PyObject_HEAD
    CS_CONTEXT *ctx;
    PyObject *servermsg_cb;
    PyObject *clientmsg_cb;
    int is_global;
    int debug;
    int serial;
    SY_DECLARE_LOCK;
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
    int serial;
    SY_DECLARE_LOCK;
    struct CS_CONNECTIONObj *next;
} CS_CONNECTIONObj;

extern PyTypeObject CS_CONNECTIONType;
PyObject *conn_alloc(CS_CONTEXTObj *ctx, int enable_lock);
PyObject *conn_find_object(CS_CONNECTION *conn);

typedef struct {
    PyObject_HEAD
    CS_CONNECTIONObj *conn;
    CS_BLKDESC *blk;
    CS_INT direction;
    int debug;
    int serial;
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
    int serial;
} CS_COMMANDObj;

extern PyTypeObject CS_COMMANDType;
PyObject *cmd_alloc(CS_CONNECTIONObj *conn);
PyObject *cmd_eed(CS_CONNECTIONObj *conn, CS_COMMAND *eed);

typedef struct {
    PyObject_HEAD
    CS_DATAFMT fmt;
    int strip;
    int serial;
} CS_DATAFMTObj;

extern PyTypeObject CS_DATAFMTType;
#define CS_DATAFMT_Check(obj) (obj->ob_type == &CS_DATAFMTType)
void datetime_datafmt(CS_DATAFMT *fmt, int type);
void money_datafmt(CS_DATAFMT *fmt, int type);
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
    int serial;
} CS_IODESCObj;

extern PyTypeObject CS_IODESCType;
#define CS_IODESC_Check(obj) (obj->ob_type == &CS_IODESCType)
extern char iodesc_new__doc__[];
PyObject *iodesc_new(PyObject *module, PyObject *args);
PyObject *iodesc_alloc(CS_IODESC *iodesc);

typedef struct {
    PyObject_HEAD
    CS_CONTEXTObj *ctx;
    int debug;
    CS_LOCALE *locale;
    int serial;
} CS_LOCALEObj;

extern PyTypeObject CS_LOCALEType;
#define CS_LOCALE_Check(obj) (obj->ob_type == &CS_LOCALEType)
PyObject *locale_alloc(CS_CONTEXTObj *ctx);

typedef struct {
    PyObject_HEAD
    int strip;
    CS_DATAFMT fmt;
    char *buff;
    CS_INT *copied;
    CS_SMALLINT *indicator;
    int serial;
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
int numeric_from_value(CS_NUMERIC *num, int precision, int scale, PyObject *obj);
int numeric_as_string(PyObject *obj, char *text);
extern char NumericType_new__doc__[];
PyObject *NumericType_new(PyObject *module, PyObject *args);
void copy_reg_numeric(PyObject *dict);
extern char pickle_numeric__doc__[];
PyObject *pickle_numeric(PyObject *module, PyObject *args);

typedef union {
    CS_MONEY money;
    CS_MONEY4 money4;
} MoneyUnion;

typedef struct {
    PyObject_HEAD
    int type;
    MoneyUnion v;
} MoneyObj;

extern PyTypeObject MoneyType;
#define Money_Check(obj) (obj->ob_type == &MoneyType)
MoneyObj *money_alloc(MoneyUnion *num, int type);
int money_from_value(MoneyUnion *money, int type, PyObject *obj);
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
    int serial;
} CS_CLIENTMSGObj;

typedef struct {
    PyObject_HEAD
    CS_SERVERMSG msg;
    int serial;
} CS_SERVERMSGObj;

extern PyTypeObject CS_CLIENTMSGType;
extern PyTypeObject CS_SERVERMSGType;
PyObject *clientmsg_alloc(void);
PyObject *servermsg_alloc(void);

int first_tuple_int(PyObject *args, int *int_arg);

enum { VAL_ACTION, VAL_BULK, VAL_BULKDIR, VAL_BULKPROPS, VAL_CANCEL,
       VAL_CBTYPE, VAL_CMD, VAL_CONSTAT, VAL_CSDATES, VAL_CSVER,
       VAL_CURSOR, VAL_CURSOROPT, VAL_CURSTAT, VAL_DATAFMT, VAL_DATEDAY,
       VAL_DATEFMT, VAL_DIRSERV, VAL_DTINFO, VAL_DYNAMIC, VAL_LEVEL,
       VAL_LOCVAL, VAL_NETIO, VAL_OPTION, VAL_PROPS, VAL_RESINFO,
       VAL_RESULT, VAL_SECURITY, VAL_STATUS, VAL_STATUSFMT, VAL_TYPE, };

char *value_str(int type, int value);
char *mask_str(int type, int value);

#define NUMERIC_LEN (CS_MAX_PREC + 1)
#define MONEY_LEN   NUMERIC_LEN
