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

int first_tuple_int(PyObject *args, int *int_arg)
{
    PyObject *obj;

    if (!PyTuple_Check(args)) {
	PyErr_SetString(PyExc_SystemError, "argument is not a tuple");
	return 0;
    }
    obj = PyTuple_GetItem(args, 0);
    if (obj == NULL)
	return 0;
    *int_arg = PyInt_AsLong(obj);
    return !PyErr_Occurred();
}

static char *module = "sybasect";

static char sybasect_cs_ctx_alloc__doc__[] =
"cs_ctx_alloc([version]) -> ctx\n"
"\n"
"Allocate a new Sybase library context object.";

static PyObject *sybasect_cs_ctx_alloc(PyObject *module, PyObject *args)
{
    int version = CS_VERSION_100;

    if (!PyArg_ParseTuple(args, "|i", &version))
	return NULL;
    return ctx_alloc(version);
}

static char sybasect_cs_ctx_global__doc__[] =
"cs_ctx_global([version]) -> ctx\n"
"\n"
"Allocate or return global Sybase library context object.";

static PyObject *sybasect_cs_ctx_global(PyObject *module, PyObject *args)
{
    int version = CS_VERSION_100;

    if (!PyArg_ParseTuple(args, "|i", &version))
	return NULL;
    return ctx_global(version);
}

static char sybasect_Buffer__doc__[] =
"Buffer(obj) -> buffer\n"
"\n"
"Allocate a buffer to store data described by datafmt or object type.";

static PyObject *sybasect_Buffer(PyObject *module, PyObject *args)
{
    PyObject *obj;

    if (!PyArg_ParseTuple(args, "O", &obj))
	return NULL;

    return buffer_alloc(obj);
}

/* List of methods defined in the module */

static struct PyMethodDef sybasect_methods[] = {
    { "cs_ctx_alloc", (PyCFunction)sybasect_cs_ctx_alloc, METH_VARARGS, sybasect_cs_ctx_alloc__doc__ },
    { "cs_ctx_global", (PyCFunction)sybasect_cs_ctx_global, METH_VARARGS, sybasect_cs_ctx_global__doc__ },
    { "Buffer", (PyCFunction)sybasect_Buffer, METH_VARARGS, sybasect_Buffer__doc__ },
    { "numeric", (PyCFunction)NumericType_new, METH_VARARGS },
    { NULL }			/* sentinel */
};

/* Initialization function for the module (*must* be called
   initsybasect) */

static char sybasect_module_documentation[] = 
"Thin wrapper on top of the Sybase CT library - intended to be used\n"
"by the Sybase.py module.";

static int dict_add_int(PyObject *dict, char *key, int value)
{
    int err;
    PyObject *obj = PyInt_FromLong(value);
    if (obj == NULL)
	return 0;

    err = PyDict_SetItemString(dict, key, obj);
    Py_DECREF(obj);
    return !err;
}

#define SYVAL(t, v) { t, #v, v }

typedef struct {
    int type;
    char *name;
    int value;
} value_desc;

static value_desc sybase_args[] = {
    SYVAL(CSVER, CS_VERSION_100),
    SYVAL(CSVER, CS_VERSION_110),

    SYVAL(ACTION, CS_CACHE),
    SYVAL(ACTION, CS_CLEAR),
    SYVAL(ACTION, CS_GET),
    SYVAL(ACTION, CS_INIT),
    SYVAL(ACTION, CS_MSGLIMIT),
    SYVAL(ACTION, CS_SEND),
    SYVAL(ACTION, CS_SET),
    SYVAL(ACTION, CS_STATUS),
    SYVAL(ACTION, CS_SUPPORTED),

    SYVAL(CANCEL, CS_CANCEL_ALL),
    SYVAL(CANCEL, CS_CANCEL_ATTN),
    SYVAL(CANCEL, CS_CANCEL_CURRENT),

    SYVAL(RESULT, CS_CMD_DONE),
    SYVAL(RESULT, CS_CMD_FAIL),
    SYVAL(RESULT, CS_CMD_SUCCEED),
    SYVAL(RESULT, CS_COMPUTEFMT_RESULT),
    SYVAL(RESULT, CS_COMPUTE_RESULT),
    SYVAL(RESULT, CS_CURSOR_RESULT),
    SYVAL(RESULT, CS_DESCRIBE_RESULT),
    SYVAL(RESULT, CS_MSG_RESULT),
    SYVAL(RESULT, CS_PARAM_RESULT),
    SYVAL(RESULT, CS_ROWFMT_RESULT),
    SYVAL(RESULT, CS_ROW_RESULT),
    SYVAL(RESULT, CS_STATUS_RESULT),

    SYVAL(RESINFO, CS_ROW_COUNT),
    SYVAL(RESINFO, CS_CMD_NUMBER),
    SYVAL(RESINFO, CS_NUM_COMPUTES),
    SYVAL(RESINFO, CS_NUMDATA),
    SYVAL(RESINFO, CS_ORDERBY_COLS),
    SYVAL(RESINFO, CS_NUMORDERCOLS),
    SYVAL(RESINFO, CS_MSGTYPE),
    SYVAL(RESINFO, CS_BROWSE_INFO),
    SYVAL(RESINFO, CS_TRANS_STATE),

    SYVAL(CMD, CS_LANG_CMD),
    SYVAL(CMD, CS_RPC_CMD),
    SYVAL(CMD, CS_MSG_CMD),
    SYVAL(CMD, CS_SEND_DATA_CMD),
    SYVAL(CMD, CS_PACKAGE_CMD),
    SYVAL(CMD, CS_SEND_BULK_CMD),

    SYVAL(CURSOR, CS_CURSOR_DECLARE),
    SYVAL(CURSOR, CS_CURSOR_OPEN),
    SYVAL(CURSOR, CS_CURSOR_ROWS),
    SYVAL(CURSOR, CS_CURSOR_UPDATE),
    SYVAL(CURSOR, CS_CURSOR_DELETE),
    SYVAL(CURSOR, CS_CURSOR_CLOSE),
    SYVAL(CURSOR, CS_CURSOR_OPTION),
    SYVAL(CURSOR, CS_CURSOR_DEALLOC),

    SYVAL(CURSOROPT, CS_FOR_UPDATE),
    SYVAL(CURSOROPT, CS_READ_ONLY),
    SYVAL(CURSOROPT, CS_DYNAMIC),
#ifdef CS_RESTORE_OPEN
    SYVAL(CURSOROPT, CS_RESTORE_OPEN),
#endif
#ifdef CS_MORE
    SYVAL(CURSOROPT, CS_MORE),
#endif
#ifdef CS_END
    SYVAL(CURSOROPT, CS_END),
#endif

    SYVAL(BULK, CS_BLK_ALL),
    SYVAL(BULK, CS_BLK_BATCH),
    SYVAL(BULK, CS_BLK_CANCEL),

    SYVAL(BULKDIR, CS_BLK_IN),
    SYVAL(BULKDIR, CS_BLK_OUT),

    SYVAL(BULKPROPS, BLK_IDENTITY),
    SYVAL(BULKPROPS, BLK_SENSITIVITY_LBL),
    SYVAL(BULKPROPS, BLK_NOAPI_CHK),
    SYVAL(BULKPROPS, BLK_SLICENUM),
    SYVAL(BULKPROPS, BLK_IDSTARTNUM),
#ifdef HAS_ARRAY_INSERT
    SYVAL(BULKPROPS, ARRAY_INSERT),
#endif

    SYVAL(DYNAMIC, CS_PREPARE),
    SYVAL(DYNAMIC, CS_EXECUTE),
    SYVAL(DYNAMIC, CS_EXEC_IMMEDIATE),
    SYVAL(DYNAMIC, CS_DESCRIBE_INPUT),
    SYVAL(DYNAMIC, CS_DESCRIBE_OUTPUT),
    SYVAL(DYNAMIC, CS_DYN_CURSOR_DECLARE),

    SYVAL(DYNAMIC, CS_DEALLOC),

    SYVAL(PROPS, CS_USERNAME),
    SYVAL(PROPS, CS_PASSWORD),
    SYVAL(PROPS, CS_APPNAME),
    SYVAL(PROPS, CS_HOSTNAME),
    SYVAL(PROPS, CS_LOGIN_STATUS),
    SYVAL(PROPS, CS_TDS_VERSION),
    SYVAL(PROPS, CS_CHARSETCNV),
    SYVAL(PROPS, CS_PACKETSIZE),
    SYVAL(PROPS, CS_NETIO),
    SYVAL(PROPS, CS_TEXTLIMIT),
    SYVAL(PROPS, CS_HIDDEN_KEYS),
    SYVAL(PROPS, CS_VERSION),
    SYVAL(PROPS, CS_LOGIN_TIMEOUT),
    SYVAL(PROPS, CS_TIMEOUT),
    SYVAL(PROPS, CS_MAX_CONNECT),
    SYVAL(PROPS, CS_EXPOSE_FMTS),
    SYVAL(PROPS, CS_EXTRA_INF),
    SYVAL(PROPS, CS_TRANSACTION_NAME),
    SYVAL(PROPS, CS_ANSI_BINDS),
    SYVAL(PROPS, CS_BULK_LOGIN),
    SYVAL(PROPS, CS_EED_CMD),
    SYVAL(PROPS, CS_DIAG_TIMEOUT),
    SYVAL(PROPS, CS_DISABLE_POLL),
    SYVAL(PROPS, CS_SEC_ENCRYPTION),
    SYVAL(PROPS, CS_SEC_CHALLENGE),
    SYVAL(PROPS, CS_SEC_NEGOTIATE),
    SYVAL(PROPS, CS_ENDPOINT),
    SYVAL(PROPS, CS_NO_TRUNCATE),
    SYVAL(PROPS, CS_CON_STATUS),
    SYVAL(PROPS, CS_VER_STRING),
    SYVAL(PROPS, CS_ASYNC_NOTIFS),
    SYVAL(PROPS, CS_SERVERNAME),
    SYVAL(PROPS, CS_SEC_APPDEFINED),
    SYVAL(PROPS, CS_NOCHARSETCNV_REQD),
    SYVAL(PROPS, CS_EXTERNAL_CONFIG),
    SYVAL(PROPS, CS_CONFIG_FILE),
    SYVAL(PROPS, CS_CONFIG_BY_SERVERNAME),

    SYVAL(DIRSERV, CS_DS_CHAIN),
    SYVAL(DIRSERV, CS_DS_EXPANDALIAS),
    SYVAL(DIRSERV, CS_DS_COPY),
    SYVAL(DIRSERV, CS_DS_SIZELIMIT),
    SYVAL(DIRSERV, CS_DS_TIMELIMIT),
    SYVAL(DIRSERV, CS_DS_PRINCIPAL),
    SYVAL(DIRSERV, CS_DS_SEARCH),
    SYVAL(DIRSERV, CS_DS_DITBASE),
    SYVAL(DIRSERV, CS_DS_FAILOVER),
    SYVAL(DIRSERV, CS_DS_PROVIDER),
#ifdef CS_RETRY_COUNT
    SYVAL(DIRSERV, CS_RETRY_COUNT),
#endif
#ifdef CS_LOOP_DELAY
    SYVAL(DIRSERV, CS_LOOP_DELAY),
#endif
#ifdef CS_DS_PASSWORD
    SYVAL(DIRSERV, CS_DS_PASSWORD),
#endif

    SYVAL(SECURITY, CS_SEC_NETWORKAUTH),
    SYVAL(SECURITY, CS_SEC_DELEGATION),
    SYVAL(SECURITY, CS_SEC_MUTUALAUTH),
    SYVAL(SECURITY, CS_SEC_INTEGRITY),
    SYVAL(SECURITY, CS_SEC_CONFIDENTIALITY),
    SYVAL(SECURITY, CS_SEC_CREDTIMEOUT),
    SYVAL(SECURITY, CS_SEC_SESSTIMEOUT),
    SYVAL(SECURITY, CS_SEC_DETECTREPLAY),
    SYVAL(SECURITY, CS_SEC_DETECTSEQ),
    SYVAL(SECURITY, CS_SEC_DATAORIGIN),
    SYVAL(SECURITY, CS_SEC_MECHANISM),
    SYVAL(SECURITY, CS_SEC_CHANBIND),
    SYVAL(SECURITY, CS_SEC_SERVERPRINCIPAL),
    SYVAL(SECURITY, CS_SEC_KEYTAB),

    SYVAL(NETIO, CS_SYNC_IO),
    SYVAL(NETIO, CS_ASYNC_IO),
    SYVAL(NETIO, CS_DEFER_IO),

    SYVAL(OPTION, CS_OPT_DATEFIRST),
    SYVAL(OPTION, CS_OPT_TEXTSIZE),
    SYVAL(OPTION, CS_OPT_STATS_TIME),
    SYVAL(OPTION, CS_OPT_STATS_IO),
    SYVAL(OPTION, CS_OPT_ROWCOUNT),
    SYVAL(OPTION, CS_OPT_NATLANG),
    SYVAL(OPTION, CS_OPT_DATEFORMAT),
    SYVAL(OPTION, CS_OPT_ISOLATION),
    SYVAL(OPTION, CS_OPT_AUTHON),
    SYVAL(OPTION, CS_OPT_CHARSET),
    SYVAL(OPTION, CS_OPT_SHOWPLAN),
    SYVAL(OPTION, CS_OPT_NOEXEC),
    SYVAL(OPTION, CS_OPT_ARITHIGNORE),
    SYVAL(OPTION, CS_OPT_TRUNCIGNORE),
    SYVAL(OPTION, CS_OPT_ARITHABORT),
    SYVAL(OPTION, CS_OPT_PARSEONLY),
    SYVAL(OPTION, CS_OPT_GETDATA),
    SYVAL(OPTION, CS_OPT_NOCOUNT),
    SYVAL(OPTION, CS_OPT_FORCEPLAN),
    SYVAL(OPTION, CS_OPT_FORMATONLY),
    SYVAL(OPTION, CS_OPT_CHAINXACTS),
    SYVAL(OPTION, CS_OPT_CURCLOSEONXACT),
    SYVAL(OPTION, CS_OPT_FIPSFLAG),
    SYVAL(OPTION, CS_OPT_RESTREES),
    SYVAL(OPTION, CS_OPT_IDENTITYON),
    SYVAL(OPTION, CS_OPT_CURREAD),
    SYVAL(OPTION, CS_OPT_CURWRITE),
    SYVAL(OPTION, CS_OPT_IDENTITYOFF),
    SYVAL(OPTION, CS_OPT_AUTHOFF),
    SYVAL(OPTION, CS_OPT_ANSINULL),
    SYVAL(OPTION, CS_OPT_QUOTED_IDENT),
    SYVAL(OPTION, CS_OPT_ANSIPERM),
    SYVAL(OPTION, CS_OPT_STR_RTRUNC),

    SYVAL(DATEDAY, CS_OPT_MONDAY),
    SYVAL(DATEDAY, CS_OPT_TUESDAY),
    SYVAL(DATEDAY, CS_OPT_WEDNESDAY),
    SYVAL(DATEDAY, CS_OPT_THURSDAY),
    SYVAL(DATEDAY, CS_OPT_FRIDAY),
    SYVAL(DATEDAY, CS_OPT_SATURDAY),
    SYVAL(DATEDAY, CS_OPT_SUNDAY),

    SYVAL(DATEFMT, CS_OPT_FMTMDY),
    SYVAL(DATEFMT, CS_OPT_FMTDMY),
    SYVAL(DATEFMT, CS_OPT_FMTYMD),
    SYVAL(DATEFMT, CS_OPT_FMTYDM),
    SYVAL(DATEFMT, CS_OPT_FMTMYD),
    SYVAL(DATEFMT, CS_OPT_FMTDYM),

    SYVAL(STATUSFMT, CS_HIDDEN),
    SYVAL(STATUSFMT, CS_KEY),
    SYVAL(STATUSFMT, CS_VERSION_KEY),
    SYVAL(STATUSFMT, CS_NODATA),
    SYVAL(STATUSFMT, CS_UPDATABLE),
    SYVAL(STATUSFMT, CS_CANBENULL),
    SYVAL(STATUSFMT, CS_DESCIN),
    SYVAL(STATUSFMT, CS_DESCOUT),
    SYVAL(STATUSFMT, CS_INPUTVALUE),
    SYVAL(STATUSFMT, CS_UPDATECOL),
    SYVAL(STATUSFMT, CS_RETURN),
#ifdef CS_RETURN_CANBENULL
    SYVAL(STATUSFMT, CS_RETURN_CANBENULL),
#endif
    SYVAL(STATUSFMT, CS_TIMESTAMP),
    SYVAL(STATUSFMT, CS_NODEFAULT),
    SYVAL(STATUSFMT, CS_IDENTITY),

    SYVAL(LEVEL, CS_OPT_LEVEL0),
    SYVAL(LEVEL, CS_OPT_LEVEL1),
    SYVAL(LEVEL, CS_OPT_LEVEL3),

    SYVAL(TYPE, CS_CHAR_TYPE),
    SYVAL(TYPE, CS_BINARY_TYPE),
    SYVAL(TYPE, CS_LONGCHAR_TYPE),
    SYVAL(TYPE, CS_LONGBINARY_TYPE),
    SYVAL(TYPE, CS_TEXT_TYPE),
    SYVAL(TYPE, CS_IMAGE_TYPE),
    SYVAL(TYPE, CS_TINYINT_TYPE),
    SYVAL(TYPE, CS_SMALLINT_TYPE),
    SYVAL(TYPE, CS_INT_TYPE),
    SYVAL(TYPE, CS_REAL_TYPE),
    SYVAL(TYPE, CS_FLOAT_TYPE),
    SYVAL(TYPE, CS_BIT_TYPE),
    SYVAL(TYPE, CS_DATETIME_TYPE),
    SYVAL(TYPE, CS_DATETIME4_TYPE),
    SYVAL(TYPE, CS_MONEY_TYPE),
    SYVAL(TYPE, CS_MONEY4_TYPE),
    SYVAL(TYPE, CS_NUMERIC_TYPE),
    SYVAL(TYPE, CS_DECIMAL_TYPE),
    SYVAL(TYPE, CS_VARCHAR_TYPE),
    SYVAL(TYPE, CS_VARBINARY_TYPE),
    SYVAL(TYPE, CS_LONG_TYPE),
    SYVAL(TYPE, CS_SENSITIVITY_TYPE),
    SYVAL(TYPE, CS_BOUNDARY_TYPE),
    SYVAL(TYPE, CS_VOID_TYPE),
    SYVAL(TYPE, CS_USHORT_TYPE),

    SYVAL(TYPE, CS_CLIENTMSG_TYPE),
    SYVAL(TYPE, CS_SERVERMSG_TYPE),
    SYVAL(TYPE, CS_ALLMSG_TYPE),

    SYVAL(STATUS, CS_SUCCEED), 
    SYVAL(STATUS, CS_FAIL),
    SYVAL(STATUS, CS_MEM_ERROR),
    SYVAL(STATUS, CS_PENDING),
    SYVAL(STATUS, CS_QUIET),
    SYVAL(STATUS, CS_BUSY),
    SYVAL(STATUS, CS_INTERRUPT),
    SYVAL(STATUS, CS_BLK_HAS_TEXT),
    SYVAL(STATUS, CS_CONTINUE),
    SYVAL(STATUS, CS_FATAL),
    SYVAL(STATUS, CS_CANCELED),
    SYVAL(STATUS, CS_ROW_FAIL),
    SYVAL(STATUS, CS_END_DATA),
    SYVAL(STATUS, CS_END_RESULTS),
    SYVAL(STATUS, CS_END_ITEM),
    SYVAL(STATUS, CS_NOMSG),

    SYVAL(OPTION, CS_RECOMPILE),
    SYVAL(OPTION, CS_NO_RECOMPILE),
    SYVAL(OPTION, CS_BULK_INIT),
    SYVAL(OPTION, CS_BULK_CONT),
    SYVAL(OPTION, CS_BULK_DATA),
    SYVAL(OPTION, CS_COLUMN_DATA),

    SYVAL(DATAFMT, CS_FMT_UNUSED),
    SYVAL(DATAFMT, CS_FMT_NULLTERM),
    SYVAL(DATAFMT, CS_FMT_PADNULL),
    SYVAL(DATAFMT, CS_FMT_PADBLANK),
    SYVAL(DATAFMT, CS_FMT_JUSTIFY_RT),
    SYVAL(DATAFMT, CS_FMT_STRIPBLANKS),
    SYVAL(DATAFMT, CS_FMT_SAFESTR),

    SYVAL(OPTION, CS_FORCE_CLOSE),
    SYVAL(OPTION, CS_INPUTVALUE),
    SYVAL(OPTION, CS_UNUSED),
    {0,0,0}
};

char *value_str(int type, int value)
{
    value_desc *desc;
    char *name = NULL;
    static char num_str[16];

    for (desc = sybase_args; desc->name != NULL; desc++)
	if (desc->value == value) {
	    if (desc->type == type)
		return desc->name;
	    name = desc->name;
	}
    if (name != NULL)
	return name;
    sprintf(num_str, "%d", value);
    return num_str;
}

void initsybasect(void)
{
    PyObject *m, *d;
    value_desc *desc;

    /* Initialize the type of the new type objects here; doing it here
     * is required for portability to Windows without requiring C++. */
    CS_BLKDESCType.ob_type = &PyType_Type;
    CS_COMMANDType.ob_type = &PyType_Type;
    CS_CONNECTIONType.ob_type = &PyType_Type;
    CS_CONTEXTType.ob_type = &PyType_Type;
    CS_DATAFMTType.ob_type = &PyType_Type;
    CS_CLIENTMSGType.ob_type = &PyType_Type;
    CS_SERVERMSGType.ob_type = &PyType_Type;
    NumericType.ob_type = &PyType_Type;
    BufferType.ob_type = &PyType_Type;

    /* Create the module and add the functions */
    m = Py_InitModule4(module, sybasect_methods,
		       sybasect_module_documentation,
		       (PyObject*)NULL, PYTHON_API_VERSION);

    /* Add some symbolic constants to the module */
    d = PyModule_GetDict(m);
    /* Add constants here */
    for (desc = sybase_args; desc->name != NULL; desc++)
	if (!dict_add_int(d, desc->name, desc->value))
	    break;

    /* Check for errors */
    if (PyErr_Occurred()) {
	char msg[128];

	sprintf(msg, "%s: import failed", module);
	Py_FatalError(msg);
    }
}
