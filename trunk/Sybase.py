#
# Copyright 2001 by Object Craft P/L, Melbourne, Australia.
#
# LICENCE - see LICENCE file distributed with this software for details.
#
import sys
import time
import string
import threading
from sybasect import *
from sybasect import __have_freetds__

set_debug(sys.stderr)

__version__ = '0.39pre1'

# DB-API values
apilevel = '2.0'                        # DB API level supported

threadsafety = 2                        # Threads may share the module
                                        # and connections.


paramstyle = 'named'                    # Named style, 
                                        # e.g. '...WHERE name=@name'

use_datetime = 0                        # Deprecated: date type

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
    def __init__(self, msg):
        '''take a sybasect.CS_SERVERMSG so break out the fields for use'''
        if type(msg) == ServerMsgType:
            str = _fmt_server(msg)
        elif type(msg) == ClientMsgType:
            str = _fmt_client(msg)
        else:
            # Assume string
            str = msg
            msg = None
        Error.__init__(self, str)
        self.msg = msg
            
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


class StoredProcedureError(ProgrammingError):
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


def OUTPUT(value):
    buf = DataBuf(value)
    buf.status = CS_RETURN
    return buf


def Date(year, month, day):
    return datetime('%s-%s-%s' % (year, month, day))


def Time(hour, minute, second):
    return datetime('%d:%d:%d' % (hour, minute, second))


def Timestamp(year, month, day, hour, minute, second):
    return datetime('%s-%s-%s %d:%d:%d' % (year, month, day,
                                           hour, minute, second))


def DateFromTicks(ticks):
    return apply(Date, time.localtime(ticks)[:3])


def TimeFromTicks(ticks):
    return apply(Time, time.localtime(ticks)[3:6])


def TimestampFromTicks(ticks):
    return apply(Timestamp, time.localtime(ticks)[:6])


def Binary(str):
    return str


_output_hooks = {}


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
    text = '%s\n%s' % (string.join(parts, ', '), msg.text)
    _ctx.debug_msg(text)
    return text


def _fmt_client(msg):
    text = 'Layer: %s, Origin: %s\n' \
           '%s' % (CS_LAYER(msg.msgnumber), CS_ORIGIN(msg.msgnumber),
                   msg.msgstring)
    _ctx.debug_msg(text)
    return text


def _cslib_cb(ctx, msg):
    raise Error(_fmt_client(msg), msg)


def _clientmsg_cb(ctx, conn, msg):
    raise DatabaseError(msg)


def _servermsg_cb(ctx, conn, msg):
    mn = msg.msgnumber
    if mn == 2601: ## Attempt to insert duplicate key row in object with unique index
        raise IntegrityError(msg)
    elif mn == 2812: ## Procedure not found
        raise StoredProcedureError(msg)
    elif mn in (0, 5701, 5703, 5704) or ((mn >= 6200) and (mn < 6300)):
        # Non-errors:
        #    0      PRINT
        # 5701      Changed db context
        # 5703      Changed language
        # 5704      Changed character set (Sybase)
        # 6200-6299 SHOWPLAN output (Sybase)
        hook = _output_hooks.get(conn)
        if hook:
            hook(conn, msg)
    else:
        raise DatabaseError(msg)


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
        if fmt.datatype == CS_VARBINARY_TYPE:
            fmt.datatype = CS_BINARY_TYPE
        if fmt.maxlength > 65536:
            fmt.maxlength = 65536
        status, buf = cmd.ct_bind(i + 1, fmt)
        if status != CS_SUCCEED:
            raise Error('ct_bind')
        bufs.append(buf)
    return bufs


def _column_value(val):
    return val


def _extract_row(bufs, n):
    '''Extract a row tuple from buffers.
    '''
    row = [None] * len(bufs)
    col = 0
    for buf in bufs:
        row[col] = _column_value(buf[n])
        col = col + 1
    return tuple(row)


def _fetch_rows(cmd, bufs, rows):
    '''Fetch rows into bufs.

    When bound to buffers for a single row, return a row tuple.
    When bound to multiple row buffers, return a list of row
    tuples.
    '''
    _ctx.debug_msg('_fetch_rows\n')
    status, rows_read = cmd.ct_fetch()
    if status == CS_SUCCEED:
        pass
    elif status == CS_END_DATA:
        return 0
    elif status in (CS_ROW_FAIL, CS_FAIL, CS_CANCELED):
        raise Error('ct_fetch')
    if bufs[0].count > 1:
        for i in xrange(rows_read):
            rows.append(_extract_row(bufs, i))
        return rows_read
    else:
        rows.append(_extract_row(bufs, 0))
        return 1


def _bufs_description(bufs):
    desc = []
    for buf in bufs:
        desc.append((buf.name, buf.datatype, 0,
                     buf.maxlength, buf.precision, buf.scale,
                     buf.status & CS_CANBENULL))
    return desc


# Setup global library context
status, _ctx = cs_ctx_alloc()
if status != CS_SUCCEED:
    raise InternalError('cs_ctx_alloc failed')
set_global_ctx(_ctx)
if _ctx.ct_init() != CS_SUCCEED:
    raise Error('ct_init')
_ctx.cs_config(CS_SET, CS_MESSAGE_CB, _cslib_cb)
_ctx.ct_callback(CS_SET, CS_CLIENTMSG_CB, _clientmsg_cb)
_ctx.ct_callback(CS_SET, CS_SERVERMSG_CB, _servermsg_cb)
if _ctx.ct_config(CS_SET, CS_NETIO, CS_SYNC_IO) != CS_SUCCEED:
    raise Error('ct_config')


class _FetchNow:

    def __init__(self, owner):
        self._owner = owner
        self._conn = owner._conn
        self._result_list = []
        self._description_list = []
        self._rownum = 0
        self.rowcount = -1
        status, self._cmd = self._conn.ct_cmd_alloc()
        if status != CS_SUCCEED:
            self._raise_error(Error, 'ct_cmd_alloc')

    def start(self, arraysize):
        self._arraysize = arraysize
        status = self._cmd.ct_send()
        if status != CS_SUCCEED:
            self._raise_error(Error, 'ct_send')
        while 1:
            try:
                status, result = self._cmd.ct_results()
            except Exception, e:
                status = self._cmd.ct_cancel(CS_CANCEL_ALL)
                if status != CS_SUCCEED:
                    self._raise_error(Error, 'ct_cancel')
                raise e
            if status == CS_END_RESULTS:
                if self._description_list:
                    return self._description_list[0]
                return None
            elif status != CS_SUCCEED:
                self._raise_error(Error, 'ct_results')
            if result == CS_ROW_RESULT:
                self._row_result()
            elif result == CS_STATUS_RESULT:
                self._status_result()
            elif result == CS_PARAM_RESULT:
                self._param_result()
            elif result == CS_COMPUTE_RESULT:
                self._compute_result()
            elif result not in (CS_CMD_DONE, CS_CMD_SUCCEED):
                self._raise_error(Error, 'ct_results')

    def _is_idle(self):
        return 1

    def _raise_error(self, exc, text):
        status = self._cmd.ct_cancel(CS_CANCEL_ALL)
        if status != CS_SUCCEED:
            self._raise_error(Error, 'ct_cancel')
        raise exc(text)

    def _read_result(self):
        bufs = _row_bind(self._cmd, self._arraysize)
        self._description_list.append(_bufs_description(bufs))
        logical_result = []
        while _fetch_rows(self._cmd, bufs, logical_result):
            pass
        self._result_list.append(logical_result)

    _row_result = _read_result
    _status_result = _read_result
    _param_result = _read_result
    _compute_result = _read_result

    def result_list(self):
        return self._result_list

    def fetchone(self):
        rownum = self._rownum
        if self._result_list and rownum < len(self._result_list[0]):
            self._rownum = self._rownum + 1
            return self._result_list[0][rownum]

    def fetchmany(self, num):
        if self._result_list:
            rownum = self._rownum
            rows = self._result_list[0][rownum: rownum + num]
            self._rownum = rownum + num
            return rows
        return []

    def fetchall(self):
        if self._result_list:
            rows = self._result_list[0][self._rownum:]
            self._rownum = len(self._result_list[0])
            return rows
        return []

    def nextset(self):
        if self._result_list:
            del self._result_list[0]
            del self._description_list[0]
            self._rownum = 0
        if self._result_list:
            return self._description_list[0]
        return None


class _FetchNowParams(_FetchNow):

    def start(self, arraysize, params):
        self._params = params
        return _FetchNow.start(self, arraysize)

    def _param_result(self):
        bufs = _row_bind(self._cmd, 1)
        while 1:
            status, rows_read = self._cmd.ct_fetch()
            if status == CS_SUCCEED:
                pass
            elif status == CS_END_DATA:
                break
            elif status in (CS_ROW_FAIL, CS_FAIL, CS_CANCELED):
                self._raise_error(Error, 'ct_fetch')
            pos = -1
            for buf in bufs:
                if type(self._params) is type({}):
                    self._params[buf.name] = _column_value(buf[0])
                else:
                    while 1:
                        pos += 1
                        param = self._params[pos]
                        if (type(param) is DataBufType
                            and param.status & CS_RETURN):
                            break
                    self._params[pos] = _column_value(buf[0])

    def _status_result(self):
        bufs = _row_bind(self._cmd, 1)
        status_result = []
        while _fetch_rows(self._cmd, bufs, status_result):
            pass
        if len(status_result) == 1:
            row = status_result[0]
            if len(row) == 1:
                self.return_status = row[0]


_LAZY_IDLE = 0                          # prepared command
_LAZY_FETCHING = 1                      # fetching rows
_LAZY_END_RESULT = 2                    # fetching rows
_LAZY_CLOSED = 3                        # cursor closed
_state_names = { _LAZY_IDLE: '_LAZY_IDLE',
                 _LAZY_FETCHING: '_LAZY_FETCHING',
                 _LAZY_END_RESULT: '_LAZY_END_RESULT',
                 _LAZY_CLOSED: '_LAZY_CLOSED' }


class _FetchLazy:

    def __init__(self, owner):
        self._owner = owner
        self._conn = owner._conn
        self._cmd = None
        self._lock_count = 0
        self._state = _LAZY_IDLE
        self.rowcount = -1
        self._open()

    def _set_state(self, state):
        _ctx.debug_msg('_set_state: %s\n' % _state_names[state])
        self._state = state

    def _lock(self):
        self._lock_count = self._lock_count + 1
        _ctx.debug_msg('_lock: count -> %d\n' % self._lock_count)
        self._owner._lock()

    def _unlock(self):
        self._lock_count = self._lock_count - 1
        _ctx.debug_msg('_unlock: count -> %d\n' % self._lock_count)
        self._owner._unlock()

    def _open(self):
        self._lock()
        try:
            status, self._cmd = self._owner._conn.ct_cmd_alloc()
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_cmd_alloc')
            self._lock()
            self._set_state(_LAZY_IDLE)
        finally:
            self._unlock()

    def _close(self):
        if self._state == _LAZY_CLOSED:
            return
        self._lock()
        try:
            if self._state != _LAZY_IDLE:
                status = self._cmd.ct_cancel(CS_CANCEL_ALL)
                if status == CS_SUCCEED:
                    self._unlock()
            self._cmd = None
            self._set_state(_LAZY_CLOSED)
        finally:
            self._unlock()

    def __del__(self):
        if self._state not in (_LAZY_IDLE, _LAZY_CLOSED):
            if self._owner._is_connected:
                self._owner._conn.ct_cancel(CS_CANCEL_ALL)
        if self._lock_count:
            # By the time we get called the threading module might
            # have killed the thread the lock was created in ---
            # oops.
            if self._owner._do_locking:
                count, owner = self._owner._connlock._release_save()
                self._owner._connlock._acquire_restore((count, threading.currentThread()))
            while self._lock_count:
                self._unlock()

    def _raise_exception(self, exc):
        if self._state not in (_LAZY_IDLE, _LAZY_CLOSED):
            if self._cmd.ct_cancel(CS_CANCEL_ALL) == CS_SUCCEED:
                self._set_state(_LAZY_IDLE)
                self._unlock()
        raise exc

    def _raise_error(self, exc, text):
        if self._state not in (_LAZY_IDLE, _LAZY_CLOSED):
            if self._cmd.ct_cancel(CS_CANCEL_ALL) == CS_SUCCEED:
                self._set_state(_LAZY_IDLE)
                self._unlock()
        raise exc(text)

    def _is_idle(self):
        return self._state == _LAZY_IDLE

    def start(self, arraysize):
        self._arraysize = arraysize
        self._set_state(_LAZY_FETCHING)
        status = self._cmd.ct_send()
        if status != CS_SUCCEED:
            self._raise_error(Error, 'ct_send')
        return self._start_results()

    def fetchone(self):
        self._lock()
        try:
            if self._state == _LAZY_IDLE:
                self._raise_error(ProgrammingError, 'no result set pending')
            if self._state == _LAZY_CLOSED:
                self._raise_error(ProgrammingError, 'cursor is closed')
            if self._state == _LAZY_FETCHING:
                if self._array_pos >= len(self._array):
                    try:
                        self._array_pos = 0
                        self._array = []
                        _fetch_rows(self._cmd, self._bufs, self._array)
                    except Error:
                        status = self._cmd.ct_cancel(CS_CANCEL_ALL)
                        if status == CS_SUCCEED:
                            self._set_state(_LAZY_IDLE)
                            self._unlock()
                        raise
                if self._array_pos < len(self._array):
                    row = self._array[self._array_pos]
                    self._array_pos = self._array_pos + 1
                    return row
                self._fetch_rowcount()
            self._set_state(_LAZY_END_RESULT)
        finally:
            self._unlock()

    def fetchmany(self, num):
        self._lock()
        try:
            if self._state == _LAZY_IDLE:
                self._raise_error(ProgrammingError, 'no result set pending')
            if self._state == _LAZY_CLOSED:
                self._raise_error(ProgrammingError, 'cursor is closed')
            if self._state == _LAZY_FETCHING:
                if num == -1:
                    num = self._arraysize
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
                        rows = []
                        _fetch_rows(self._cmd, self._bufs, rows)
                    except Error:
                        status = self._cmd.ct_cancel(CS_CANCEL_ALL)
                        if status == CS_SUCCEED:
                            self._set_state(_LAZY_IDLE)
                            self._unlock()
                        raise
                self._array = []
                self._array_pos = 0
                if rows:
                    return rows
                self._fetch_rowcount()
            self._set_state(_LAZY_END_RESULT)
            return []
        finally:
            self._unlock()

    def fetchall(self):
        self._lock()
        try:
            rows = []
            while 1:
                new_rows = self.fetchmany(-1)
                if not new_rows:
                    break
                rows.extend(new_rows)
            return rows
        finally:
            self._unlock()

    def nextset(self):
        self.rowcount = 0
        self._lock()
        try:
            if self._state == _LAZY_CLOSED:
                self._raise_error(ProgrammingError, 'cursor is closed')
            if self._state == _LAZY_IDLE:
                return []
            if self._state == _LAZY_FETCHING:
                status = self._cmd.ct_cancel(CS_CANCEL_CURRENT)
                if status != CS_SUCCEED:
                    self._raise_error(Error, 'ct_cancel')
            return self._start_results()
        finally:
            self._unlock()

    def _start_results(self):
        _ctx.debug_msg('_start_results\n')
        self._array = []
        self._array_pos = 0
        while 1:
            try:
                status, result = self._cmd.ct_results()
            except Exception, e:
                self._raise_exception(e)
            if status == CS_END_RESULTS:
                if self._state != _LAZY_END_RESULT:
                    self._unlock()
                self._set_state(_LAZY_IDLE)
                return None
            elif status != CS_SUCCEED:
                self._raise_error(Error, 'ct_results')
            if result in (CS_COMPUTE_RESULT, CS_CURSOR_RESULT,
                          CS_PARAM_RESULT, CS_ROW_RESULT, CS_STATUS_RESULT):
                if self._arraysize > 0:
                    bufs = self._bufs = _row_bind(self._cmd, self._arraysize)
                else:
                    bufs = self._bufs = _row_bind(self._cmd)
                self._set_state(_LAZY_FETCHING)
                return _bufs_description(bufs)
            elif result in (CS_CMD_DONE, CS_CMD_SUCCEED):
                status, self.rowcount = self._cmd.ct_res_info(CS_ROW_COUNT)
                if status != CS_SUCCEED:
                    self._raise_error(Error, 'ct_res_info')
            else:
                self._raise_error(Error, 'ct_results')

    def _fetch_rowcount(self):
        _ctx.debug_msg('_fetch_rowcount\n')
        while 1:
            status, result = self._cmd.ct_results()
            if status == CS_END_RESULTS:
                self._set_state(_LAZY_IDLE)
                self._unlock()
                return
            elif status != CS_SUCCEED:
                self._raise_error(Error, 'ct_results')
            if result == CS_PARAM_RESULT:
                bufs = _row_bind(self._cmd)
                while 1:
                    rows = []
                    _fetch_rows(self._cmd, bufs, rows)
                    if not rows:
                        break
            elif result in (CS_CMD_DONE, CS_CMD_SUCCEED):
                status, self.rowcount = self._cmd.ct_res_info(CS_ROW_COUNT)
                if status != CS_SUCCEED:
                    self._raise_error(Error, 'ct_res_info')
            else:
                self._raise_error(Error, 'ct_results')


class CmdCursor:

    def __init__(self, owner):
        '''Implements DB-API Cursor object
        '''
        self.description = None         # DB-API
        self.rowcount = -1              # DB-API
        self.arraysize = 1              # DB-API
        self._owner = owner
        self._fetcher = None
        self._closed = 0

    def _lock(self):
        self._owner._lock()

    def _unlock(self):
        self._owner._unlock()

    def _checkready(self):
        if self._closed:
            raise ProgrammingError('cursor is closed')
        if not self._fetcher:
            raise ProgrammingError('query has not been executed')

    def callproc(self, name, params = ()):
        '''DB-API Cursor.callproc()
        '''
        _ctx.debug_msg('Cursor.callproc\n')
        if self._closed:
            raise ProgrammingError('cursor is closed')
        self._lock()
        try:
            # Discard any previous results
            self._fetcher = None
            self.return_status = None
            self.rowcount = -1

            # Prepare to retrieve new results.
            fetcher = self._fetcher = _FetchNowParams(self._owner)
            cmd = fetcher._cmd
            status = cmd.ct_command(CS_RPC_CMD, name)
            if status != CS_SUCCEED:
                fetcher._raise_error(Error, 'ct_command')
            # Send parameters.
            if type(params) is type({}):
                out_params = {}
                for name, value in params.items():
                    out_params[name] = value
                    if isinstance(value, DataBufType):
                        buf = value
                    else:
                        buf = DataBuf(value)
                    buf.name = name
                    status = cmd.ct_param(buf)
                    if status != CS_SUCCEED:
                        fetcher._raise_error(Error, 'ct_param')
            else:
                out_params = []
                for value in params:
                    out_params.append(value)
                    if isinstance(value, DataBufType):
                        buf = value
                    else:
                        buf = DataBuf(value)
                    status = cmd.ct_param(buf)
                    if status != CS_SUCCEED:
                        fetcher._raise_error(Error, 'ct_param')
            # Start retreiving results.
            self.description = fetcher.start(self.arraysize, out_params)
            self.return_status = fetcher.return_status
            self.rowcount = len(out_params)
            return out_params
        finally:
            self._unlock()

    def close(self):
        '''DB-API Cursor.close()
        '''
        if self._closed:
            raise ProgrammingError('cursor is closed')
        self._fetcher = None
        self.rowcount = -1
        self._closed = 1

    def execute(self, sql, params = {}):
        '''DB-API Cursor.execute()
        '''
        _ctx.debug_msg('Cursor.execute\n')
        if self._closed:
            raise ProgrammingError('cursor is closed')
        self._lock()
        try:
            # Discard any previous results
            self._fetcher = None

            # Prepare to retrieve new results.
            fetcher = self._fetcher = _FetchLazy(self._owner)
            cmd = fetcher._cmd
            cmd.ct_command(CS_LANG_CMD, sql)
            for name, value in params.items():
                buf = DataBuf(value)
                buf.name = name
                status = cmd.ct_param(buf)
                if status != CS_SUCCEED:
                    fetcher._raise_error(Error, 'ct_param')
            self.description = fetcher.start(self.arraysize)
            self.rowcount = fetcher.rowcount
        finally:
            self._unlock()

    def executemany(self, sql, params_seq = []):
        '''DB-API Cursor.executemany()
        '''
        _ctx.debug_msg('Cursor.executemany\n')
        if self._closed:
            raise ProgrammingError('cursor is closed')
        self._lock()
        try:
            rowcount = 0
            for params in params_seq:
                self.execute(sql, params)
                rowcount += self.rowcount
                if not self._fetcher._is_idle():
                    self._fetcher._raise_error(ProgrammingError, 'fetchable results on cursor')
            self.rowcount = rowcount
        finally:
            self._unlock()

    def fetchone(self):
        '''DB-API Cursor.fetchone()
        '''
        self._checkready()
        return self._fetcher.fetchone()

    def fetchmany(self, num = -1):
        '''DB-API Cursor.fetchmany()
        '''
        self._checkready()
        if num < 0:
            num = self.arraysize
        return self._fetcher.fetchmany(num)

    def fetchall(self):
        '''DB-API Cursor.fetchall()
        '''
        self._checkready()
        return self._fetcher.fetchall()

    def nextset(self):
        '''DB-API Cursor.nextset()
        '''
        self._checkready()
        desc = self._fetcher.nextset()
        if desc:
            self.description = desc
            self.rowcount = self._fetcher.rowcount
            return True
        return None

    def setinputsizes(self, *sizes):
        '''DB-API Cursor.setinputsizes()
        '''
        pass

    def setoutputsize(self, size, column = None):
        '''DB-API Cursor.setoutputsize()
        '''
        pass


class CtCursor:
    def __init__(self, owner):
        '''Implements DB-API Cursor object
        '''
        self.description = None         # DB-API
        self.rowcount = -1              # DB-API
        self.arraysize = 1              # DB-API
        self._owner = owner
        self._fetcher = None
        self._closed = 0
        self._cursor = 0

    def _lock(self):
        self._owner._lock()

    def _unlock(self):
        self._owner._unlock()

    def _checkready(self):
        if self._closed:
            raise ProgrammingError('cursor is closed')
        if not self._fetcher:
            raise ProgrammingError('query has not been executed')

    def _close_cursor(self):
        if not self._cursor:
            raise ProgrammingError('cursor undefined')

        _ctx.debug_msg("_close_cursor starts\n")

        cmd = self._fetcher._cmd

        _ctx.debug_msg('cancel9\n')
        status = cmd.ct_cancel(CS_CANCEL_ALL)
        if status != CS_SUCCEED:
            self._fetcher._raise_error(Error, 'ct_cancel')

        # while 1:
        #     status, result = cmd.ct_results()
        #     if status != CS_SUCCEED:
        #         break

        _ctx.debug_msg("cursor close\n")
        status = cmd.ct_cursor(CS_CURSOR_CLOSE)
        if status != CS_SUCCEED:
            self._fetcher._raise_error(Error, 'ct_cursor close')

        status = cmd.ct_send()
        if status != CS_SUCCEED:
            self._raise_error(Error, 'ct_send')

        while 1:
            status, result = cmd.ct_results()
            if status != CS_SUCCEED:
                break

        _ctx.debug_msg("cursor deallocate\n")
        status = cmd.ct_cursor(CS_CURSOR_DEALLOC)
        if status != CS_SUCCEED:
            self._fetcher._raise_error(Error, 'ct_cursor dealloc')

        status = cmd.ct_send()
        if status != CS_SUCCEED:
            self._raise_error(Error, 'ct_send')

        while 1:
            status, result = cmd.ct_results()
            if status != CS_SUCCEED:
                break

        self._fetcher._set_state(_LAZY_CLOSED)
        self._cursor = 0
        _ctx.debug_msg("fin cursor close\n")

    def callproc(self, name, params = ()):
        '''DB-API Cursor.callproc()
        '''
        _ctx.debug_msg('Cursor.callproc\n')
        if self._closed:
            raise ProgrammingError('cursor is closed')
        self._lock()
        try:
            # Discard any previous results
            self._fetcher = None
            self.return_status = None
            self.rowcount = -1

            # Prepare to retrieve new results.
            fetcher = self._fetcher = _FetchNowParams(self._owner)
            cmd = fetcher._cmd
            status = cmd.ct_command(CS_RPC_CMD, name)
            if status != CS_SUCCEED:
                fetcher._raise_error(Error, 'ct_command')
            # Send parameters.
            if type(params) is type({}):
                out_params = {}
                for name, value in params.items():
                    out_params[name] = value
                    if isinstance(value, DataBufType):
                        buf = value
                    else:
                        buf = DataBuf(value)
                    buf.name = name
                    status = cmd.ct_param(buf)
                    if status != CS_SUCCEED:
                        fetcher._raise_error(Error, 'ct_param')
            else:
                out_params = []
                for value in params:
                    out_params.append(value)
                    if isinstance(value, DataBufType):
                        buf = value
                    else:
                        buf = DataBuf(value)
                    status = cmd.ct_param(buf)
                    if status != CS_SUCCEED:
                        fetcher._raise_error(Error, 'ct_param')
            # Start retreiving results.
            self.description = fetcher.start(self.arraysize, out_params)
            self.return_status = fetcher.return_status
            self.rowcount = len(out_params)
            return out_params
        finally:
            self._unlock()

    def close(self):
        '''DB-API Cursor.close()
        '''
        if self._closed:
            raise ProgrammingError('cursor is closed')
        if self._cursor:
            self._close_cursor()
        self._fetcher = None
        self.rowcount = -1
        self._closed = 1

    def execute(self, sql, params = {}):
        '''DB-API Cursor.execute()
        '''

        _ctx.debug_msg('Cursor.execute\n')
        if self._closed:
            raise ProgrammingError('cursor is closed')
        self._lock()
        try:
            try:
                # Discard any previous results
                self._fetcher = None
    
                _ctx.debug_msg("using ct_cursor, %s\n" % sql)
                fetcher = self._fetcher = _FetchLazy(self._owner)
                cmd = fetcher._cmd
     
                _ctx.debug_msg("cursor declare\n")
                status = cmd.ct_cursor(CS_CURSOR_DECLARE, "ctmp%x" % id(self), sql, CS_UNUSED)
                if status != CS_SUCCEED:
                    fetcher._raise_error(Error, 'ct_cursor declare')
                self._cursor = 1
    
                _ctx.debug_msg("cursor define params type\n")
                for name, value in params.items():
                    _ctx.debug_msg("params: name '%s' value '%s'" % (name, value))
    
                    buf = DataBuf(value)
                    buf.name = name
    
                    fmt = CS_DATAFMT()
                    fmt.count = buf.count
                    fmt.datatype = buf.datatype
                    fmt.format = CS_FMT_UNUSED
                    fmt.maxlength = buf.maxlength
                    fmt.name = buf.name
                    fmt.precision = buf.precision
                    fmt.scale = buf.scale
                    fmt.status = CS_INPUTVALUE
                    fmt.strip = buf.strip
                    fmt.usertype = buf.usertype
    
                    status = cmd.ct_param(fmt)
                    if status != CS_SUCCEED:
                        fetcher._raise_error(Error, 'ct_param')
    
                nb_rows = 100
                cmd.ct_cursor(CS_CURSOR_ROWS, nb_rows)
                _ctx.debug_msg("using ct_cursor nb_rows %d\n" % nb_rows)
                if status != CS_SUCCEED:
                    fetcher._raise_error(Error, 'ct_cursor rows')
    
                _ctx.debug_msg("cursor open\n")
                status = cmd.ct_cursor(CS_CURSOR_OPEN)
                if status != CS_SUCCEED:
                    fetcher._raise_error(Error, 'ct_cursor open')
                    
                _ctx.debug_msg("cursor define params value\n")
                for name, value in params.items():
                    buf = DataBuf(value)
                    buf.name = name
                    status = cmd.ct_param(buf)
                    if status != CS_SUCCEED:
                        fetcher._raise_error(Error, 'ct_param')
    
                self.description = fetcher.start(self.arraysize)
            except:
                self._cursor = 0
                raise
        finally:
            self._unlock()

    def executemany(self, sql, params_seq = []):
        '''DB-API Cursor.executemany()
        '''
        _ctx.debug_msg('Cursor.executemany\n')
        if self._closed:
            raise ProgrammingError('cursor is closed')
        self._lock()
        try:
            rowcount = 0
            for params in params_seq:
                self.execute(sql, params)
                rowcount += self.rowcount
                if not self._fetcher._is_idle():
                    self._fetcher._raise_error(ProgrammingError, 'fetchable results on cursor')
            self.rowcount = rowcount
        finally:
            self._unlock()

    def fetchone(self):
        '''DB-API Cursor.fetchone()
        '''
        self._checkready()
        return self._fetcher.fetchone()

    def fetchmany(self, num = -1):
        '''DB-API Cursor.fetchmany()
        '''
        self._checkready()
        if num < 0:
            num = self.arraysize
        return self._fetcher.fetchmany(num)

    def fetchall(self):
        '''DB-API Cursor.fetchall()
        '''
        self._checkready()
        return self._fetcher.fetchall()

    def nextset(self):
        '''DB-API Cursor.nextset()
        '''
        self._checkready()
        desc = self._fetcher.nextset()
        if desc:
            self.description = desc
            self.rowcount = self._fetcher.rowcount
            return True
        return None

    def setinputsizes(self, *sizes):
        '''DB-API Cursor.setinputsizes()
        '''
        pass

    def setoutputsize(self, size, column = None):
        '''DB-API Cursor.setoutputsize()
        '''
        pass


class Cursor(object):
    def __init__(self, owner):
        '''Implements DB-API Cursor object
        '''
        self._owner = owner
        self._imp = None
        self.__arraysize = 1

    def callproc(self, name, params = ()):
        '''DB-API Cursor.callproc()
        '''
        if self._imp:
            self._imp.close()
        self._imp = CmdCursor(self._owner)
        return self._imp.callproc(name, params)

    def close(self):
        '''DB-API Cursor.close()
        '''
        if self._imp:
            self._imp.close()

    def execute(self, sql, params = {}):
        '''DB-API Cursor.execute()
        '''
        if self._imp:
            self._imp.close()
        if sql.lower().startswith("select"):
            self._imp = CtCursor(self._owner)
        else:
            self._imp = CmdCursor(self._owner)
        self._imp.arraysize = self.__arraysize
        return self._imp.execute(sql, params)

    def executemany(self, sql, params_seq = []):
        '''DB-API Cursor.executemany()
        '''
        if self._imp:
            self._imp.close()
        if sql.lower().startswith("select"):
            self._imp = CtCursor(self._owner)
        else:
            self._imp = CmdCursor(self._owner)
        self._imp.arraysize = self.__arraysize
        return self._imp.executemany(sql, params_seq)

    def fetchone(self):
        '''DB-API Cursor.fetchone()
        '''
        if not self._imp:
            raise ProgrammingError('cursor unimplemented')
        return self._imp.fetchone()

    def fetchmany(self, num = -1):
        '''DB-API Cursor.fetchmany()
        '''
        if not self._imp:
            raise ProgrammingError('cursor unimplemented')
        return self._imp.fetchmany(num)

    def fetchall(self):
        '''DB-API Cursor.fetchall()
        '''
        if not self._imp:
            raise ProgrammingError('cursor unimplemented')
        return self._imp.fetchall()

    def nextset(self):
        '''DB-API Cursor.nextset()
        '''
        if not self._imp:
            raise ProgrammingError('cursor unimplemented')
        return self._imp.nextset()

    def setinputsizes(self, *sizes):
        '''DB-API Cursor.setinputsizes()
        '''
        pass

    def setoutputsize(self, size, column = None):
        '''DB-API Cursor.setoutputsize()
        '''
        pass

    def get_arraysize(self):
        return self.__arraysize
    
    def set_arraysize(self, arraysize):
        self.__arraysize = arraysize
        if self._imp:
            self._imp.arraysize = arraysize

    def get_description(self):
        return self._imp.description
    
    def get_rowcount(self):
        return self._imp.rowcount

    arraysize = property(get_arraysize, set_arraysize)
    description = property(get_description)
    rowcount = property(get_rowcount)


class Connection:

    def __init__(self, dsn, user, passwd, database = None,
                 strip = 0, auto_commit = 0, delay_connect = 0, locking = 1,
                 datetime = None, bulkcopy = 0):
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
        if bulkcopy:
            status = conn.ct_con_props(CS_SET, CS_BULK_LOGIN, CS_TRUE)
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_con_props')
        if not delay_connect:
            self.connect()

        global use_datetime, _column_value, DATETIME, Date, Time, Timestamp

        if datetime == None:
            import warnings
            warnings.warn("native python datetime will be used by default in sybase-python 0.39 - specify datetime argument to choose another type", DeprecationWarning)
            datetime = "auto"
        if datetime == "auto":
            try:
                import DateTime as DT
                use_datetime = 1
            except ImportError:
                try:
                    import mx.DateTime as DT
                    use_datetime = 1
                except ImportError:
                    use_datetime = 0
        elif datetime == "sybase":
            use_datetime = 0
        elif datetime == "mx":
            try:
                import DateTime as DT
            except ImportError:
                import mx.DateTime as DT
            use_datetime = 1
        elif datetime == "python":
            import datetime
            use_datetime = 2
        else:
            raise ValueError, "Unknown datetime value: %s" % datetime

        if use_datetime == 0:
            DATETIME = DBAPITypeObject(CS_DATETIME4_TYPE, CS_DATETIME_TYPE)
            datetime = globals()["datetime"]
            def _column_value(val):
                return val
            def Date(year, month, day):
                return datetime('%s-%s-%s' % (year, month, day))
            def Time(hour, minute, second):
                return datetime('%d:%d:%d' % (hour, minute, second))
            def Timestamp(year, month, day, hour, minute, second):
                return datetime('%s-%s-%s %d:%d:%d' % (year, month, day,
                                           hour, minute, second))
        elif use_datetime == 1:
            DATETIME = DBAPITypeObject(CS_DATETIME4_TYPE, CS_DATETIME_TYPE, DT.DateTimeType)
            datetime = globals()["datetime"]
            def _column_value(val):
                if type(val) is DateTimeType:
                    return DT.DateTime(val.year, val.month + 1, val.day,
                                       val.hour, val.minute,
                                       val.second + val.msecond / 1000.0)
                else:
                    return val
            def Date(year, month, day):
                return datetime('%s-%s-%s' % (year, month, day))
            def Time(hour, minute, second):
                return datetime('%d:%d:%d' % (hour, minute, second))
            def Timestamp(year, month, day, hour, minute, second):
                return datetime('%s-%s-%s %d:%d:%d' % (year, month, day,
                                           hour, minute, second))
        elif use_datetime == 2:
            DATETIME = DBAPITypeObject(CS_DATETIME4_TYPE, CS_DATETIME_TYPE, datetime.datetime, datetime.time)
            def _column_value(val):
                if type(val) is DateTimeType:
                    return datetime.datetime(val.year, val.month + 1, val.day,
                                             val.hour, val.minute,
                                             val.second, val.msecond * 1000)
                else:
                    return val
            def Date(year, month, day):
                return datetime.datetime(year, month, day)
            def Time(hour, minute, second):
                return datetime.time(hour, minute, second)
            def Timestamp(year, month, day, hour, minute, second):
                return datetime.datetime(year, month, day, hour, minute, second)

    def __getattr__(self, name):
        # Expose exception classes via the Connection object so
        # programmers don't have to tie their code to this module with
        # "from Sybase import DatabaseError" all over the place.  See
        # PEP 249, "Optional DB API Extensions".
        names = ('Warning',
                 'Error',
                 'InterfaceError',
                 'DatabaseError',
                 'DataError',
                 'OperationalError',
                 'IntegrityError',
                 'InternalError',
                 'ProgrammingError',
                 'StoredProcedureError',
                 'NotSupportedError',
                )
        if name in names:
            return globals()[name]
        else:
            raise AttributeError(name)

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

    def set_output_hook(self, hook):
        if hook is None:
            if _output_hooks.has_key(self._conn):
                del _output_hooks[self._conn]
        else:
            _output_hooks[self._conn] = hook

    def get_output_hook(self):
        return _output_hooks.get(self._conn)

    def __del__(self):
        if self._conn:
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
                self._is_connected = 0
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

    def bulkcopy(self, tablename, *args, **kw):
        # Fake an alternate way to specify direction=CS_BLK_OUT
        if kw.get('out', 0):
            del kw['out']
            kw['direction'] = CS_BLK_OUT
        return Bulkcopy(self, tablename, *args, **kw)
    
    def execute(self, sql):
        '''Backwards compatibility
        '''
        self._lock()
        try:
            fetcher = _FetchNow(self)
            cmd = fetcher._cmd
            status = cmd.ct_command(CS_LANG_CMD, sql)
            if status != CS_SUCCEED:
                self._raise_error(Error, 'ct_command')
            fetcher.start(self.arraysize)
            return fetcher.result_list()
        finally:
            self._unlock()


class Bulkcopy(object):

    def __init__(self, conn, tablename, direction = CS_BLK_IN, arraysize = 20):
        '''Manage a BCP session for the named table'''

        if not conn.auto_commit:
            raise ProgrammingError('bulkcopy requires connection in auto_commit mode')
        if direction not in (CS_BLK_IN, CS_BLK_OUT):
            raise ProgrammingError("Bulkcopy direction must be CS_BLK_IN or CS_BLK_OUT")
        
        self._direction = direction
        self._arraysize = arraysize     # no of rows to transfer at once
        
        self._totalcount = 0            # Total number of rows transferred in/out so far
        # the next two for _flush() / _row()
        self._batchcount = 0            # Rows send in the current batch but not yet reported to user via self.batch()
        self._nextrow = 0               # Next row in the DataBuf to use
        
        self._alldone = 0

        conn._lock()
        try:
            #conn._conn.debug = 1
            status, blk = conn._conn.blk_alloc()
            if status != CS_SUCCEED:
                conn._raise_error(Error, 'blk_alloc')
            if blk.blk_init(direction, tablename) != CS_SUCCEED:
                conn._raise_error(Error, 'blk_init')
            #blk.debug = 1
        finally:
            conn._unlock()

        self._blk = blk
        
        # Now allocate buffers
        bufs = []
        while 1:
            try:
                status, fmt = blk.blk_describe(len(bufs) + 1)
                # This never happens, raises DatabaseError instead
                if status != CS_SUCCEED:
                    break
            except DatabaseError, e:
                break
            fmt.count = arraysize
            bufs.append(DataBuf(fmt))

        self.bufs = bufs

        # Now bind the buffers
        for i in range(len(bufs)):
            buf = bufs[i]
            if direction == CS_BLK_OUT:
                buf.format = 0
            else:
                buf.format = CS_BLK_ARRAY_MAXLEN
            if blk.blk_bind(i + 1, buf) != CS_SUCCEED:
                conn._raise_error(Error, 'blk_bind')

    def __del__(self):
        '''Make sure any incoming but unflushed data is sent!'''
        try:
            if not self._alldone:
                self.done()
        except:
            pass
            
    # Read-only property, as size of DataBufs is set in __init__
    arraysize = property(lambda x: x._arraysize)
    totalcount = property(lambda x: x._totalcount)
    
    def rowxfer(self, args = None):
        if self._direction == CS_BLK_OUT:
            if args is not None:
                raise ProgramError("Attempt to rowxfer() data in to a bcp out session")
            return self._row()
        
        if args is None:
            raise ProgramError("rowxfer() for bcp-IN needs a sequence arg")
            
        if len(args) != len(self.bufs):
            raise Error("BCP has %d columns, data has %d columns" % (len(self.bufs), len(args)))

        for i in range(len(args)):
            self.bufs[i][self._nextrow] = args[i]
        self._nextrow += 1

        if self._nextrow == self._arraysize:
            self._flush()

    def _flush(self):
        '''Flush any partially-filled DataBufs'''
        if self._nextrow > 0:
            status, rows  = self._blk.blk_rowxfer_mult(self._nextrow)
            if status != CS_SUCCEED:
                self.conn._raise_error(Error, 'blk_rowxfer_mult in')
            self._nextrow = 0
            self._totalcount += rows
            
    def batch(self):
        '''Flush a batch full to the server.  Return the number of rows in this batch'''
        self._flush()
        status, rows = self._blk.blk_done(CS_BLK_BATCH)
        if status != CS_SUCCEED:
            self.conn._raise_error(Error, 'blk_done batch in')
        # rows should be 0 here due to _flush()
        rows += self._batchcount
        self._batchcount = 0
        return rows
            
    def done(self):
        self._flush()
        status, rows = self._blk.blk_done(CS_BLK_ALL)
        if status != CS_SUCCEED:
            self.conn._raise_error(Error, 'blk_done in')
        self._alldone = 1
        return rows        

    def __iter__(self):
        '''An iterator for all the BCPd rows fetched from the database'''
        if self._direction == CS_BLK_IN:
            raise ProgramError("iter() is for bcp-OUT... use rowxfer() instead")
        while 1:
            r = self._row()
            if r is None:
                raise StopIteration
            yield r
    rows = __iter__

    def _row(self):
        if self._nextrow == self._batchcount:
            status, num = self._blk.blk_rowxfer_mult()
            if status == CS_END_DATA:
                status, rows = self._blk.blk_done(CS_BLK_ALL)
                if status != CS_SUCCEED:
                    self.conn._raise_error(Error, 'blk_done out')
                self._alldone = 1
                return None
            if status != CS_SUCCEED:
                self.conn._raise_error(Error, 'blk_rowxfer_mult out')
            self._nextrow = 0
            self._batchcount = num
            assert num > 0
            

        self._totalcount += 1
        rownum = self._nextrow
        self._nextrow += 1
        return _extract_row(self.bufs, rownum)


def connect(dsn, user, passwd, database = None,
            strip = 0, auto_commit = 0, delay_connect = 0, locking = 1, datetime = None, bulkcopy = 0):
    return Connection(dsn, user, passwd, database,
                      strip, auto_commit, delay_connect, locking, datetime, bulkcopy)
