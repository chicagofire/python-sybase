from sybasect import *

__version__ = '0.24'

# _sybase is a thin wrapper on top of the Sybase CT library.  The
# objects in _sybase perform some CT functions automatically:
#
# _sybase.ctx_alloc()          returns CS_CONTEXT object
# _sybase.ctx_global()         returns CS_CONTEXT object
# CS_CONTEXT.ct_con_alloc()    returns CS_CONNECTION object
# CS_CONNECTION.__init__()     executes ct_con_alloc()
# CS_CONNECTION.__del__()      executes ct_con_drop()
# CS_CONNECTION.ct_cmd_alloc() returns CS_COMMAND object
# CS_COMMAND.__init__()        executes ct_cmd_alloc()
# CS_COMMAND.__del__()         executes ct_cmd_drop()

# DB-API values
apilevel = '2.0'                        # DB API level supported

threadsafety = 3                        # Threads may share the
                                        # module, connections and
                                        # cursors.

paramstyle = 'qmark'                    # Question mark style,
                                        # e.g. '...WHERE name=?'

import exceptions

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

class Warning(exceptions.StandardError):
    pass

class Error(exceptions.StandardError):
    pass

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

# Query diagnostic information from Sybase API

def _get_diag(func, type):
    status, num_msgs = func(CS_STATUS, type)
    if status != CS_SUCCEED:
        return []
    err = []
    for i in range(num_msgs):
        status, msg = func(CS_GET, type, i + 1)
        if status != CS_SUCCEED:
            continue
        dict = {}
        for attr in dir(msg):
            dict[attr] = getattr(msg, attr)
        err.append(dict)
    return err

def _build_ct_except(con, msg):
    err = [msg]
    err.extend(_get_diag(con.ct_diag, CS_SERVERMSG_TYPE))
    err.extend(_get_diag(con.ct_diag, CS_CLIENTMSG_TYPE))
    con.ct_diag(CS_CLEAR, CS_ALLMSG_TYPE)
    return err

def _build_cs_except(ctx, msg):
    err = [msg]
    err.extend(_get_diag(ctx.cs_diag, CS_CLIENTMSG_TYPE))
    ctx.cs_diag(CS_CLEAR, CS_CLIENTMSG_TYPE)
    return err

# Setup global library context
status, _ctx = cs_ctx_global()
if status != CS_SUCCEED:
    raise InternalError('cs_ctx_alloc failed')
if _ctx.cs_diag(CS_INIT) != CS_SUCCEED:
    raise InternalError('cs_diag failed')
if _ctx.ct_init() != CS_SUCCEED:
    raise InternalError(_build_cs_except(_ctx, 'ct_init'))
if _ctx.ct_config(CS_SET, CS_NETIO, CS_SYNC_IO) != CS_SUCCEED:
    raise InternalError(_build_cs_except(_ctx, 'ct_config'))

def _extract_row(bufs, n):
    '''Extract a row tuple from buffers.
    '''
    row = []
    for buf in bufs:
        row.append(buf[n])
    return tuple(row)
        
class _Cmd:
    def __init__(self, con):
        self._cmd = None
        status, cmd = con.ct_cmd_alloc()
        if status != CS_SUCCEED:
            raise InternalError(_build_ct_except(con, 'ct_cmd_alloc'))
        self._cmd = cmd

    def ct_command(self, *args):
        cmd = self._cmd
        if apply(cmd.ct_command, args) != CS_SUCCEED:
            raise InternalError(_build_ct_except(cmd.conn, 'ct_command'))

    def ct_dynamic(self, *args):
        cmd = self._cmd
        if apply(cmd.ct_dynamic, args) != CS_SUCCEED:
            raise InternalError(_build_ct_except(cmd.conn, 'ct_dynamic'))

    def ct_send(self):
        cmd = self._cmd
        if cmd.ct_send() != CS_SUCCEED:
            raise InternalError(_build_ct_except(cmd.conn, 'ct_send'))

    def ct_res_info(self, option):
        cmd = self._cmd
        status, result = cmd.ct_res_info(option)
        if status != CS_SUCCEED:
            raise InternalError(_build_ct_except(cmd.conn, 'ct_res_info'))
        return result

    def ct_describe(self, col_num):
        cmd = self._cmd
        status, fmt = cmd.ct_describe(col_num)
        if status != CS_SUCCEED:
            raise InternalError(_build_ct_except(cmd.conn, 'ct_describe'))
        return fmt

    def ct_bind(self, col_num, fmt):
        cmd = self._cmd
        status, buf = cmd.ct_bind(col_num, fmt)
        if status != CS_SUCCEED:
            raise InternalError(_build_ct_except(cmd.conn, 'ct_bind'))
        return buf

    def ct_param(self, buf):
        cmd = self._cmd
        if cmd.ct_param(buf) != CS_SUCCEED:
            raise ProgrammingError(_build_ct_except(cmd.conn, 'ct_param'))

    def ct_results(self):
        return self._cmd.ct_results()

    def ct_cancel(self, arg):
        cmd = self._cmd
        if cmd.ct_cancel(arg) != CS_SUCCEED:
            raise ProgrammingError(_build_ct_except(cmd.conn, 'ct_cancel'))

    def row_bind(self, count = 1):
        '''Bind buffers for count rows of column data.
        '''
        num_cols = self.ct_res_info(CS_NUMDATA)
        bufs = []
        for i in range(num_cols):
            fmt = self.ct_describe(i + 1)
            fmt.count = count
            buf = self.ct_bind(i + 1, fmt)
            bufs.append(buf)
        return bufs

    def fetch_rows(self, bufs):
        '''Fetch rows into bufs.

        When bound to buffers for a single row, return a row tuple.
        When bound to multiple row buffers, return a list of row
        tuples.
        '''
        cmd = self._cmd
        status, rows_read = cmd.ct_fetch()
        if status == CS_SUCCEED:
            pass
        elif status == CS_END_DATA:
            return None
        elif status in (CS_ROW_FAIL, CS_FAIL, CS_CANCELED,
                        CS_PENDING, CS_BUSY):
            err = _build_ct_except(cmd.conn, 'ct_fetch')
	    self.abort_quietly()
            raise InternalError(err)
        if bufs[0].count > 1:
            rows = []
            for i in xrange(rows_read):
                rows.append(_extract_row(bufs, i))
            return rows
        else:
            return _extract_row(bufs, 0)

    def abort_quietly(self):
        cmd = self._cmd
        cmd.ct_cancel(CS_CANCEL_ALL)

CMD_PREPARED = 0                        # prepared command
CMD_END_RESULTS = 1                     # all result sets fetched
CMD_FETCHING = 2                        # fetching rows
CMD_END_SET = 3                         # current set finished

class Cursor:
    def __init__(self, owner):
        '''Implements DB-API Cursor object
        '''
        self.description = None         # DB-API
        self.rowcount = -1              # DB-API
        self.arraysize = 1              # DB-API
        self._owner = owner
        self._con = owner._con
        self._cmd = None
        self._sql = None

    def __del__(self):
        self._cancel()

    def callproc(self, name, params = []):
        '''DB-API Cursor.callproc()
        '''
        self._cancel()
        cmd = self._cmd = _Cmd(self._con)
        self._sql = -1
        cmd.ct_command(CS_RPC_CMD, name)
        for param in params:
            buf = Buffer(param)
            cmd.ct_param(buf)
        cmd.ct_send()
        self._state = CMD_PREPARED
        self._start_result_set()

    def close(self):
        '''DB-API Cursor.close()
        '''
        self._cancel()

    def execute(self, sql, params = []):
        '''DB-API Cursor.execute()
        '''
        if self._sql:
            if self._sql != sql:
                self._cancel()
                self._sql = None
            else:
                self._discard_results()
        if not self._sql:
            self._new_cmd(sql)
        self._send_params(params)

    def executemany(self, sql, params_seq = []):
        '''DB-API Cursor.executemany()
        '''
        for params in params_seq:
            self.execute(sql, params)

    def fetchone(self):
        '''DB-API Cursor.fetchone()
        '''
        cmd = self._cmd
        if self._state == CMD_FETCHING:
            row = cmd.fetch_rows(self._bufs)
            if row:
                return row
            self._state = CMD_END_SET

    def fetchmany(self, num = -1):
        '''DB-API Cursor.fetchmany()
        '''
        if num == -1:
            num = self.arraysize
        rows = []
        for i in xrange(num):
            row = self.fetchone()
            if not row:
                break
            rows.append(row)
        return rows

    def fetchall(self):
        '''DB-API Cursor.fetchall()
        '''
        rows = []
        while 1:
            row = self.fetchone()
            if not row:
                break
            rows.append(row)
        return rows

    def nextset(self):
        '''DB-API Cursor.nextset()
        '''
        if self._state == CMD_END_RESULTS:
            return
        if self._state != CMD_END_SET:
            self._cmd.ct_cancel(CS_CANCEL_CURRENT)
        self._start_result_set()
        return self._state != CMD_END_RESULTS or None

    def setinputsizes(self):
        '''DB-API Cursor.setinputsizes()
        '''
        pass

    def setoutputsize(self, size, column = None):
        '''DB-API Cursor.setoutputsize()
        '''
        pass

    def _new_cmd(self, sql):
        self._dyn_name = 'dyn%s' % self._owner._next_dyn()
        self._sql = sql
        self._prepare()

    def _cancel(self):
        if self._cmd:
            self._cmd.ct_cancel(CS_CANCEL_ALL)

    def _discard_results(self):
        '''Discard all pending results and prepare to execute again.
        '''
        while self._state != CMD_END_RESULTS:
            self.nextset()

    def _prepare(self):
        '''Prepare the statement to be executed for this cursor.
        '''
        con = self._con
        cmd = self._cmd = _Cmd(con)
        dyn_name = self._dyn_name
        # send command to server
        cmd.ct_dynamic(CS_PREPARE, dyn_name, self._sql)
        cmd.ct_send()
        while 1:
            status, result = cmd.ct_results()
            if status != CS_SUCCEED:
                break
        if status != CS_END_RESULTS:
            raise InternalError(_build_ct_except(con, 'ct_results'))
        # get input parameter description
        cmd.ct_dynamic(CS_DESCRIBE_INPUT, dyn_name)
        cmd.ct_send()
        while 1:
            status, result = cmd.ct_results()
            if status != CS_SUCCEED:
                break
            elif result == CS_DESCRIBE_RESULT:
                num = cmd.ct_res_info(CS_NUMDATA)
                fmts = []
                for i in range(num):
                    fmt = cmd.ct_describe(i + 1)
                    fmt.status = CS_INPUTVALUE
                    fmt.count = 1
                    fmts.append(fmt)
                self._fmts = fmts
        if status != CS_END_RESULTS:
            raise InternalError(_build_ct_except(con, 'ct_results'))
        self._state = CMD_PREPARED

    def _send_params(self, params):
        '''Execute prepared statement, send parameters and prepare
        buffers for results.
        '''
        cmd = self._cmd
        dyn_name = self._dyn_name
        cmd.ct_dynamic(CS_EXECUTE, dyn_name)
        for fmt, param in map(None, self._fmts, params):
            buf = Buffer(fmt)
            buf[0] = param
            cmd.ct_param(buf)
        cmd.ct_send()
        self._start_result_set()

    def _start_result_set(self):
        con = self._con
        cmd = self._cmd
        while 1:
            status, result = cmd.ct_results()
            if status == CS_END_RESULTS:
                self._state = CMD_END_RESULTS
                return
            elif status != CS_SUCCEED:
                raise InternalError(_build_ct_except(con, 'ct_results'))
            if result in (CS_COMPUTE_RESULT, CS_CURSOR_RESULT,
                          CS_PARAM_RESULT, CS_ROW_RESULT, CS_STATUS_RESULT):
                bufs = self._bufs = cmd.row_bind()
                desc = []
                for buf in bufs:
                    desc.append((buf.name, buf.datatype, 0,
                                 buf.maxlength, buf.precision, buf.scale,
                                 buf.status & CS_CANBENULL))
                self.description = desc
                self._state = CMD_FETCHING
                return
            elif result in (CS_CMD_DONE, CS_CMD_SUCCEED):
                self.rowcount = cmd.ct_res_info(CS_ROW_COUNT)
            else:
                raise InternalError(_build_ct_except(con, 'ct_results'))

class Bulkcopy:
    def __init__(self, owner, table, direction):
        self._owner = owner
        self._con = owner._con
        self._table = table
        self._direction = direction
        status, blk = con.blk_alloc()
        if status != CS_SUCCEED:
            raise InternalError(_build_ct_except(con, 'blk_alloc'))
        self._blk = blk
        status = blk.blk_init(direction, table)
        if status != CS_SUCCEED:
            raise InternalError(_build_ct_except(con, 'blk_init'))

    def format(self):
        pass

    def split(self):
        pass

    def rowxfer(self, data):
        if type(data) not in (type([]), type(())):
            raise ProgrammingError('list or tuple expected')
        blk = self._blk
        bufs = []
        for col in range(len(data)):
            buf = Buffer(data[col])
            bufs.append(buf)
            if blk.blk_bind(col + 1, buf) != CS_SUCCEED:
                raise InternalError(_build_cs_except(self._con, 'blk_bind'))
        if blk.blk_rowxfer() != CS_SUCCEED:
            raise InternalError(_build_cs_except(self._con, 'blk_rowxfer'))

    def batch(self):
        blk = self._blk
        status, num_rows = blk.blk_done(CS_BLK_BATCH)
        if status != CS_SUCCEED:
            raise InternalError(_build_cs_except(self._con, 'blk_done'))
        return num_rows

    def done(self):
        blk = self._blk
        status, num_rows = blk.blk_done(CS_BLK_ALL)
        if status != CS_SUCCEED:
            raise InternalError(_build_cs_except(self._con, 'blk_done'))
        return num_rows

class Connection:
    def __init__(self, dsn, user, passwd, database = None, strip = 0):
        '''DB-API Sybase.Connect()
        '''
        self._con = self._cmd = None
        self.dsn = dsn
        self.user = user
        self.passwd = passwd
        status, con = _ctx.ct_con_alloc()
        if status != CS_SUCCEED:
            raise InternalError(_build_cs_except(_ctx, 'ct_con_alloc'))
        self._con = con
        con.strip = strip
        if con.ct_diag(CS_INIT) != CS_SUCCEED:
            raise InternalError(_build_ct_except(con, 'ct_diag'))
        if con.ct_con_props(CS_SET, CS_USERNAME, user) != CS_SUCCEED:
            raise DatabaseError(_build_ct_except(con, 'ct_con_props CS_USERNAME'))
        if con.ct_con_props(CS_SET, CS_PASSWORD, passwd) != CS_SUCCEED:
            raise DatabaseError(_build_ct_except(con, 'ct_con_props CS_PASSWORD'))
        if con.ct_connect(dsn) != CS_SUCCEED:
            raise DatabaseError(_build_ct_except(con, 'ct_connect'))
        if con.ct_options(CS_SET, CS_OPT_CHAINXACTS, 1) != CS_SUCCEED:
            raise DatabaseError(_build_ct_except(con, 'ct_options'))
        con.ct_diag(CS_CLEAR, CS_ALLMSG_TYPE)
        self._cmd = _Cmd(con)
        if database:
            self.execute('use %s' % database)
        self._dyn_num = 0

    def __del__(self):
        self.close()

    def close(self):
        '''DBI-API Connection.close()
        '''
        if not self._con:
            return
        if self._cmd:
            self._cmd = None
        self._con.ct_close()
        self._con = None

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

    def _next_dyn(self):
        self._dyn_num = self._dyn_num + 1
        return self._dyn_num

    def bulkcopy(self, table, copy_out = 0):
        '''Create a new bulkcopy context
        '''
        if copy_out:
            direction = CS_BLK_OUT
        else:
            direction = CS_BLK_IN
        return Bulkcopy(self, table, direction)

    def execute(self, sql):
        '''Backwards compatibility
        '''
        cmd = self._cmd
        cmd.ct_command(CS_LANG_CMD, sql)
        cmd.ct_send()
        result_list = self._fetch_results()
        self._con.ct_diag(CS_CLEAR, CS_ALLMSG_TYPE)
        return result_list

    def _fetch_results(self):
        result_list = []
        con = self._con
        cmd = self._cmd
        while 1:
            status, result = cmd.ct_results()
            if status == CS_END_RESULTS:
                return result_list
            elif status != CS_SUCCEED:
                raise InternalError(_build_ct_except(con, 'ct_results'))
            if result in (CS_COMPUTE_RESULT, CS_CURSOR_RESULT,
                          CS_PARAM_RESULT, CS_ROW_RESULT, CS_STATUS_RESULT):
                bufs = cmd.row_bind(16)
		logical_result = self._fetch_logical_result(bufs)
                result_list.append(logical_result)
            elif result not in (CS_CMD_DONE, CS_CMD_SUCCEED):
                raise InternalError(_build_ct_except(con, 'ct_results'))

    def _fetch_logical_result(self, bufs):
        cmd = self._cmd
        logical_result = []
        while 1:
            rows = cmd.fetch_rows(bufs)
            if not rows:
                return logical_result
            logical_result.extend(rows)

def connect(dsn, user, passwd, database = None, strip = 0):
    return Connection(dsn, user, passwd, database, strip)
