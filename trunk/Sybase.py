#
# Copyright 2001 by Object Craft P/L, Melbourne, Australia.
#
# LICENCE - see LICENCE file distributed with this software for details.
#
try:
    import DateTime
    use_datetime = 1
except ImportError:
    try:
        import mx.DateTime
        DateTime = mx.DateTime
        use_datetime = 1
    except ImportError:
        use_datetime = 0
import sys
import time
import string
import threading
from sybasect import *

set_debug(sys.stderr)

__version__ = '0.36pre1'

# DB-API values
apilevel = '2.0'                        # DB API level supported

threadsafety = 2                        # Threads may share the module
                                        # and connections.


paramstyle = 'named'                    # Named style, 
                                        # e.g. '...WHERE name=@name'

# DB-API exceptions
#
# StandardError
# |__Warning
# |__Error
#    |__InterfaceError
#    |__DatabaseError
#       |__DataError
#       |__OperationalError
#       |__IntegrityError
#       |__InternalError
#       |__ProgrammingError
#       |__NotSupportedError

class Warning(StandardError):
    pass

class Error(StandardError):
    def append(self, other):
        self.args = (self.args[0] + other.args[0],)

class InterfaceError(Error):
    pass

class DatabaseError(Error):
    pass

class DataError(DatabaseError):
    pass

class OperationalError(DatabaseError):
    pass

class IntegrityError(DatabaseError):
    pass

class InternalError(DatabaseError):
    pass

class ProgrammingError(DatabaseError):
    pass

class NotSupportedError(DatabaseError):
    pass

class DBAPITypeObject:
    def __init__(self, *values):
	self.values = values

    def __cmp__(self, other):
	if other in self.values:
	    return 0
	if other < self.values:
	    return 1
	else:
	    return -1

STRING = DBAPITypeObject(CS_LONGCHAR_TYPE, CS_VARCHAR_TYPE,
                         CS_TEXT_TYPE, CS_CHAR_TYPE)
BINARY = DBAPITypeObject(CS_IMAGE_TYPE, CS_LONGBINARY_TYPE,
                         CS_VARBINARY_TYPE, CS_BINARY_TYPE)
NUMBER = DBAPITypeObject(CS_BIT_TYPE, CS_TINYINT_TYPE,
                         CS_SMALLINT_TYPE, CS_INT_TYPE,
                         CS_MONEY_TYPE, CS_REAL_TYPE, CS_FLOAT_TYPE,
                         CS_DECIMAL_TYPE, CS_NUMERIC_TYPE)
DATETIME = DBAPITypeObject(CS_DATETIME4_TYPE, CS_DATETIME_TYPE)
ROWID = DBAPITypeObject(CS_DECIMAL_TYPE, CS_NUMERIC_TYPE)

def Date(year, month, day):
    return datetime('%s-%s-%s' % (year, month, day))

def Timestamp(year, month, day, hour, minute, second):
    return datetime('%s-%s-%s %d:%d:%d' % (year, month, day,
                                           hour, minute, second))

def DateFromTicks(ticks):
    return apply(Date, time.localtime(ticks)[:3])

def TimestampFromTicks(ticks):
    return apply(Timestamp, time.localtime(ticks)[:6])

def Binary(str):
    return str

def _fmt_server(msg):
    parts = []
    for label, name in (('Msg', 'msgnumber'),
                        ('Level', 'severity'),
                        ('State', 'state'),
                        ('Procedure', 'proc'),
                        ('Line', 'line')):
        value = getattr(msg, name)
        if value:
            parts.append('%s %s' % (label, value))
    return '%s\n%s' % (string.join(parts, ', '), msg.text)

def _fmt_client(msg):
    return 'Layer: %s, Origin: %s\n' \
           '%s' % (CS_LAYER(msg.msgnumber), CS_ORIGIN(msg.msgnumber),
                   msg.msgstring)

def _cslib_cb(ctx, msg):
    raise Error(_fmt_client(msg))

def _clientmsg_cb(ctx, conn, msg):
    raise DatabaseError(_fmt_client(msg))

def _servermsg_cb(ctx, conn, msg):
    if msg.msgnumber not in (5701, 5703):
        raise DatabaseError(_fmt_server(msg))

def _row_bind(cmd, count = 1):
    '''Bind buffers for count rows of column data.
    '''
    status, num_cols = cmd.ct_res_info(CS_NUMDATA)
    if status != CS_SUCCEED:
        raise Error('ct_res_info')
    bufs = []
    for i in range(num_cols):
        status, fmt = cmd.ct_describe(i + 1)
        if status != CS_SUCCEED:
            raise Error('ct_describe')
        fmt.count = count
        status, buf = cmd.ct_bind(i + 1, fmt)
        if status != CS_SUCCEED:
            raise Error('ct_bind')
        bufs.append(buf)
    return bufs

def _extract_row(bufs, n):
    '''Extract a row tuple from buffers.
    '''
    row = [None] * len(bufs)
    col = 0
    for buf in bufs:
        val = buf[n]
        if use_datetime and type(val) is DateTimeType:
            row[col] = DateTime.DateTime(val.year, val.month + 1, val.day,
                                         val.hour, val.minute,
                                         val.second + val.msecond / 1000.0)
        else:
            row[col] = val
        col = col + 1
    return tuple(row)
        
def _fetch_rows(cmd, bufs):
    '''Fetch rows into bufs.

    When bound to buffers for a single row, return a row tuple.
    When bound to multiple row buffers, return a list of row
    tuples.
    '''
    status, rows_read = cmd.ct_fetch()
    if status == CS_SUCCEED:
        pass
    elif status == CS_END_DATA:
        return None
    elif status in (CS_ROW_FAIL, CS_FAIL, CS_CANCELED):
        raise Error('ct_fetch')
    if bufs[0].count > 1:
        rows = []
        for i in xrange(rows_read):
            rows.append(_extract_row(bufs, i))
        return rows
    else:
        return _extract_row(bufs, 0)

# Setup global library context
status, _ctx = cs_ctx_alloc()
if status != CS_SUCCEED:
    raise InternalError('cs_ctx_alloc failed')
if _ctx.ct_init() != CS_SUCCEED:
    raise Error('ct_init')
_ctx.cs_config(CS_SET, CS_MESSAGE_CB, _cslib_cb)
_ctx.ct_callback(CS_SET, CS_CLIENTMSG_CB, _clientmsg_cb)
_ctx.ct_callback(CS_SET, CS_SERVERMSG_CB, _servermsg_cb)
set_global_ctx(_ctx)
if _ctx.ct_config(CS_SET, CS_NETIO, CS_SYNC_IO) != CS_SUCCEED:
    raise Error('ct_config')

_CUR_IDLE = 0                           # prepared command
_CUR_FETCHING = 1                       # fetching rows
_CUR_END_RESULT = 2                     # fetching rows
_CUR_CLOSED = 3                         # cursor closed

# state      event    action                 next       
# ---------------------------------------------------------------------
# idle       execute  prepare,params,results fetching
#                                            end_result
# fetching   fetchone fetch                  fetching 
#                                            end_result
# end_result nextset  results                fetching
#                                            idle
#            fetchone                        end_result
class Cursor:
    def __init__(self, owner):
        '''Implements DB-API Cursor object
        '''
        self.description = None         # DB-API
        self.rowcount = -1              # DB-API
        self.arraysize = 1              # DB-API
        self._owner = owner
        self._lock_count = 0
        self._do_locking = owner._do_locking
        self._open()

    def _open(self):
        if self._do_locking:
            self._lock()
        try:
            status, self._cmd = self._owner._conn.ct_cmd_alloc()
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_cmd_alloc')
            self._state = _CUR_IDLE
        finally:
            if self._do_locking:
                self._unlock()

    def _close(self):
        if self._state == _CUR_CLOSED:
            return
        if self._do_locking:
            self._lock()
        try:
            if self._state != _CUR_IDLE:
                status = self._cmd.ct_cancel(CS_CANCEL_ALL)
                if status == CS_SUCCEED:
                    if self._do_locking:
                        self._unlock()
            self._cmd = None
            self._state = _CUR_CLOSED
        finally:
            if self._do_locking:
                self._unlock()

    def __del__(self):
        if self._state not in (_CUR_IDLE, _CUR_CLOSED):
            self._owner._conn.ct_cancel(CS_CANCEL_ALL)
        if self._lock_count:
            # By the time we get called the threading module might
            # have killed the thread the lock was created in ---
            # oops.
            count, owner = self._owner._connlock._release_save()
            self._owner._connlock._acquire_restore((count, threading.currentThread()))
            while self._lock_count:
                self._unlock()

    def _lock(self):
        if not self._do_locking:
            return
        self._owner._lock()
        self._lock_count = self._lock_count + 1

    def _unlock(self):
        if not self._do_locking:
            return
        self._owner._unlock()
        self._lock_count = self._lock_count - 1

    def _raise_error(self, exc, text):
        if self._state not in (_CUR_IDLE, _CUR_CLOSED):
            if self._owner._conn.ct_cancel(CS_CANCEL_ALL) == CS_SUCCEED:
                self._state = _CUR_IDLE
                if self._do_locking:
                    self._unlock()
        raise exc(text)

    def callproc(self, name, params = ()):
        '''DB-API Cursor.callproc()
        '''
        if self._do_locking:
            self._lock()
        try:
            if self._state == _CUR_CLOSED:
                self._raise_error(ProgrammingError, 'cursor is closed')
            if self._state != _CUR_IDLE:
                self._close()
                self._open()
            # At the start of a command acquire an extra lock -
            # when the cursor is idle again the extra lock will be
            # released.
            if self._do_locking:
                self._lock()
            status = self._cmd.ct_command(CS_RPC_CMD, name)
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_command')
            if type(params) is type({}):
                for name, value in params.items():
                    buf = DataBuf(value)
                    buf.name = name
                    status = self._cmd.ct_param(buf)
                    if status != CS_SUCCEED:
                        self._raise_error(Error, 'ct_param')
            else:
                for value in params:
                    buf = DataBuf(value)
                    status = self._cmd.ct_param(buf)
                    if status != CS_SUCCEED:
                        self._raise_error(Error, 'ct_param')
            status = self._cmd.ct_send()
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_send')
            self._state = _CUR_FETCHING
            self._start_results()
        finally:
            if self._do_locking:
                self._unlock()

    def close(self):
        '''DB-API Cursor.close()
        '''
        if self._state == _CUR_CLOSED:
            self._raise_error(Error, 'cursor is closed')
        self._close()

    def execute(self, sql, params = {}):
        '''DB-API Cursor.execute()
        '''
        if self._do_locking:
            self._lock()
        try:
            if self._state == _CUR_CLOSED:
                self._raise_error(Error, 'cursor is closed')
            if self._state != _CUR_IDLE:
                self._close()
                self._open()
            # At the start of a command acquire an extra lock - when
            # the cursor is idle again the extra lock will be
            # released.
            if self._do_locking:
                self._lock()
            self._cmd.ct_command(CS_LANG_CMD, sql)
            for name, value in params.items():
                buf = DataBuf(value)
                buf.name = name
                status = self._cmd.ct_param(buf)
                if status != CS_SUCCEED:
                    self._raise_error(Error, 'ct_param')
            self._cmd.ct_send()
            self._start_results()
        finally:
            if self._do_locking:
                self._unlock()

    def executemany(self, sql, params_seq = []):
        '''DB-API Cursor.executemany()
        '''
        if self._do_locking:
            self._lock()
        try:
            for params in params_seq:
                self.execute(sql, params)
                if self._state != _CUR_IDLE:
                    self._raise_error(ProgrammingError, 'fetchable results on cursor')
        finally:
            if self._do_locking:
                self._unlock()

    def fetchone(self):
        '''DB-API Cursor.fetchone()
        '''
        if self._do_locking:
            self._lock()
        try:
            if self._state == _CUR_IDLE:
                self._raise_error(ProgrammingError, 'no result set pending')
            if self._state == _CUR_CLOSED:
                self._raise_error(ProgrammingError, 'cursor is closed')
            if self._state == _CUR_FETCHING:
                if self._array_pos >= len(self._array):
                    try:
                        self._array_pos = 0
                        _array = _fetch_rows(self._cmd, self._bufs)
                        if _array is None:
                            self._array = []
                        elif self._bufs[0].count == 1:
                            self._array = [_array]
                        else:
                            self._array = _array
                    except Error:
                        status = self._cmd.ct_cancel(CS_CANCEL_ALL)
                        if status == CS_SUCCEED:
                            self._state = _CUR_IDLE
                            if self._do_locking:
                                self._unlock()
                        raise
                if self._array_pos < len(self._array):
                    row = self._array[self._array_pos]
                    self._array_pos = self._array_pos + 1
                    return row
                self._fetch_rowcount()
            self._state = _CUR_END_RESULT
        finally:
            if self._do_locking:
                self._unlock()

    def fetchmany(self, num = -1):
        '''DB-API Cursor.fetchmany()
        '''
        if self._do_locking:
            self._lock()
        try:
            if self._state == _CUR_IDLE:
                self._raise_error(ProgrammingError, 'no result set pending')
            if self._state == _CUR_CLOSED:
                self._raise_error(ProgrammingError, 'cursor is closed')
            if self._state == _CUR_FETCHING:
                if num == -1:
                    num = self.arraysize
                if num != self._bufs[0].count:
                    rows = []
                    for i in xrange(num):
                        row = self.fetchone()
                        if not row:
                            break
                        rows.append(row)
                    return rows
                elif self._array and self._array_pos < len(self._array):
                    rows = self._array[self._array_pos:]
                else:
                    try:
                        rows = _fetch_rows(self._cmd, self._bufs)
                        if rows is None:
                            rows = []
                        elif self._bufs[0].count == 1:
                            rows = [rows]
                    except Error:
                        status = self._cmd.ct_cancel(CS_CANCEL_ALL)
                        if status == CS_SUCCEED:
                            self._state = _CUR_IDLE
                            if self._do_locking:
                                self._unlock()
                        raise
                self._array = []
                self._array_pos = 0
                if rows:
                    return rows
                self._fetch_rowcount()
            self._state = _CUR_END_RESULT
            return []
        finally:
            if self._do_locking:
                self._unlock()

    def fetchall(self):
        '''DB-API Cursor.fetchall()
        '''
        if self._do_locking:
            self._lock()
        try:
            rows = []
            while 1:
                row = self.fetchone()
                if not row:
                    break
                rows.append(row)
            return rows
        finally:
            if self._do_locking:
                self._unlock()

    def nextset(self):
        '''DB-API Cursor.nextset()
        '''
        if self._do_locking:
            self._lock()
        try:
            if self._state == _CUR_CLOSED:
                self._raise_error(ProgrammingError, 'cursor is closed')
            if self._state == _CUR_IDLE:
                return
            if self._state == _CUR_FETCHING:
                status = self._cmd.ct_cancel(CS_CANCEL_CURRENT)
                if status != CS_SUCCEED:
                    self._raise_error(Error, 'ct_cancel')
            self._start_results()
            return self._state != _CUR_IDLE or None
        finally:
            if self._do_locking:
                self._unlock()

    def setinputsizes(self):
        '''DB-API Cursor.setinputsizes()
        '''
        pass

    def setoutputsize(self, size, column = None):
        '''DB-API Cursor.setoutputsize()
        '''
        pass

    def _fetch_rowcount(self):
        status, result = self._cmd.ct_results()
        if status == CS_END_RESULTS:
            self._state = _CUR_IDLE
            if self._do_locking:
                self._unlock()
            return
        elif status != CS_SUCCEED:
            self._raise_error(Error, 'ct_results')
        elif result in (CS_CMD_DONE, CS_CMD_SUCCEED):
            status, self.rowcount = self._cmd.ct_res_info(CS_ROW_COUNT)
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_res_info')
        else:
            self._raise_error(Error, 'ct_results')

    def _start_results(self):
        self._array = []
        self._array_pos = 0
        while 1:
            status, result = self._cmd.ct_results()
            if status == CS_END_RESULTS:
                self._state = _CUR_IDLE
                if self._do_locking:
                    self._unlock()
                return
            elif status != CS_SUCCEED:
                self._raise_error(Error, 'ct_results')
            if result in (CS_COMPUTE_RESULT, CS_CURSOR_RESULT,
                          CS_PARAM_RESULT, CS_ROW_RESULT, CS_STATUS_RESULT):
                if self.arraysize > 0:
                    bufs = self._bufs = _row_bind(self._cmd, self.arraysize)
                else:
                    bufs = self._bufs = _row_bind(self._cmd)
                desc = []
                for buf in bufs:
                    desc.append((buf.name, buf.datatype, 0,
                                 buf.maxlength, buf.precision, buf.scale,
                                 buf.status & CS_CANBENULL))
                self.description = desc
                self._state = _CUR_FETCHING
                return
            elif result in (CS_CMD_DONE, CS_CMD_SUCCEED):
                status, self.rowcount = self._cmd.ct_res_info(CS_ROW_COUNT)
                if status != CS_SUCCEED:
                    self._raise_error(Error, 'ct_res_info')
            else:
                self._raise_error(Error, 'ct_results')

class Connection:
    def __init__(self, dsn, user, passwd, database = None,
                 strip = 0, auto_commit = 0, delay_connect = 0, locking = 1):
        '''DB-API Sybase.Connect()
        '''
        self._conn = self._cmd = None
        self.dsn = dsn
        self.user = user
        self.passwd = passwd
        self.database = database
        self.auto_commit = auto_commit
        self._do_locking = locking
        self._is_connected = 0
        self.arraysize = 32
        if locking:
            self._connlock = threading.RLock()

        # Do not lock in sybasect - we take care if locking in Python.
        status, conn = _ctx.ct_con_alloc(0)
        if status != CS_SUCCEED:
            raise Error('ct_con_alloc')
        self._conn = conn
        conn.strip = strip
        status = conn.ct_con_props(CS_SET, CS_USERNAME, user)
        if status != CS_SUCCEED:
            self._raise_error(Error, 'ct_con_props')
        status = conn.ct_con_props(CS_SET, CS_PASSWORD, passwd)
        if status != CS_SUCCEED:
            self._raise_error(Error, 'ct_con_props')
        if not delay_connect:
            self.connect()

    def _lock(self):
        if self._do_locking:
            self._connlock.acquire()

    def _unlock(self):
        if self._do_locking:
            self._connlock.release()

    def _raise_error(self, exc, text):
        if self._is_connected:
            self._conn.ct_cancel(CS_CANCEL_ALL)
        raise exc(text)

    def connect(self):
        conn = self._conn
        self._lock()
        try:
            status = conn.ct_connect(self.dsn)
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_connect')
            self._is_connected = 1
            status = conn.ct_options(CS_SET, CS_OPT_CHAINXACTS, not self.auto_commit)
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_options')
        finally:
            self._unlock()
        if self.database:
            self.execute('use %s' % self.database)
        self._dyn_num = 0

    def get_property(self, prop):
        conn = self._conn
        self._lock()
        try:
            status, value = conn.ct_con_props(CS_GET, prop)
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_con_props')
        finally:
            self._unlock()
        return value

    def set_property(self, prop, value):
        conn = self._conn
        self._lock()
        try:
            status = conn.ct_con_props(CS_SET, prop, value)
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_con_props')
        finally:
            self._unlock()

    def __del__(self):
        try:
            self.close()
        except:
            pass

    def close(self):
        '''DBI-API Connection.close()
        '''
        conn = self._conn
        self._lock()
        try:
            status, result = conn.ct_con_props(CS_GET, CS_CON_STATUS)
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_con_props')
            if not result & CS_CONSTAT_CONNECTED:
                self._raise_error(ProgrammingError, 'Connection is already closed')
            if self._cmd:
                self._cmd = None
            status = conn.ct_close(CS_FORCE_CLOSE)
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_close')
            self._is_connected = 0
        finally:
            self._unlock()

    def begin(self, name = None):
        '''Not in DB-API, but useful for Sybase
        '''
        if name:
            self.execute('begin transaction %s' % name)
        else:
            self.execute('begin transaction')

    def commit(self, name = None):
        '''DB-API Connection.commit()
        '''
        if name:
            self.execute('commit transaction %s' % name)
        else:
            self.execute('commit transaction')

    def rollback(self, name = None):
        '''DB-API Connection.rollback()
        '''
        if name:
            self.execute('rollback transaction %s' % name)
        else:
            self.execute('rollback transaction')

    def cursor(self):
        '''DB-API Connection.cursor()
        '''
        return Cursor(self)

    def execute(self, sql):
        '''Backwards compatibility
        '''
        conn = self._conn
        self._lock()
        try:
            status, cmd = self._conn.ct_cmd_alloc()
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_cmd_alloc')
            self._cmd = cmd
            status = cmd.ct_command(CS_LANG_CMD, sql)
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_command')
            status = cmd.ct_send()
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_send')
            result_list = self._fetch_results()
        finally:
            self._cmd = None
            self._unlock()
        return result_list

    def _fetch_results(self):
        result_list = []
        conn = self._conn
        cmd = self._cmd
        while 1:
            status, result = cmd.ct_results()
            if status == CS_END_RESULTS:
                return result_list
            elif status != CS_SUCCEED:
                self._raise_error(Error, 'ct_results')
            if result in (CS_COMPUTE_RESULT, CS_CURSOR_RESULT,
                          CS_PARAM_RESULT, CS_ROW_RESULT, CS_STATUS_RESULT):
                bufs = _row_bind(cmd, self.arraysize)
		logical_result = self._fetch_logical_result(bufs)
                result_list.append(logical_result)
            elif result not in (CS_CMD_DONE, CS_CMD_SUCCEED):
                self._raise_error(Error, 'ct_results')

    def _fetch_logical_result(self, bufs):
        cmd = self._cmd
        logical_result = []
        while 1:
            rows = _fetch_rows(cmd, bufs)
            if not rows:
                return logical_result
            logical_result.extend(rows)

def connect(dsn, user, passwd, database = None,
            strip = 0, auto_commit = 0, delay_connect = 0, locking = 1):
    return Connection(dsn, user, passwd, database,
                      strip, auto_commit, delay_connect, locking)
