try:
    import DateTime
    use_datetime = 1
except:
    use_datetime = 0
from sybasect import *

__version__ = '0.30'

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
        col = buf[n]
        if use_datetime and type(col) is DateTimeType:
            row.append(DateTime.DateTime(col.year, col.month + 1, col.day,
                                         col.hour, col.minute,
                                         col.second + col.msecond / 1000.0))
        else:
            row.append(col)
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

    def ct_cmd_drop(self):
        cmd = self._cmd
        if cmd.ct_cmd_drop() != CS_SUCCEED:
            raise InternalError(_build_ct_except(cmd.conn, 'ct_cmd_drop'))

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
            raise OperationalError(_build_ct_except(cmd.conn, 'ct_cancel'))

    def row_bind(self, count = 1):
        '''Bind buffers for count rows of column data.
        '''
        num_cols = self.ct_res_info(CS_NUMDATA)
        bufs = []
        for i in range(num_cols):
            fmt = self.ct_describe(i + 1)
            #print '--- fmt ---'
            #for name in dir(fmt):
            #    print '%-12s: %s' % (name, getattr(fmt, name))
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

CUR_IDLE = 0                            # prepared command
CUR_FETCHING = 1                        # fetching rows
CUR_END_RESULT = 2                      # fetching rows
CUR_CLOSED = 3                          # cursor closed

# state      event      action                 next       
# -----------------------------------------------------------------------
# idle       execute    prepare,params,results fetching
#                                              end_result
# fetching   fetchone   fetch                  fetching 
#                                              end_result
# end_result nextset    results                fetching
#                                              idle
#            fetchone                          end_result
class Cursor:
    def __init__(self, owner):
        '''Implements DB-API Cursor object
        '''
        self.description = None         # DB-API
        self.rowcount = -1              # DB-API
        self.arraysize = 1              # DB-API
        self._owner = owner
        self._con = owner._con
        self._cmd = _Cmd(self._con)
        self._sql = None
        self._state = CUR_IDLE
        self._dyn_name = None

    def __del__(self):
        try:
            self._dealloc()
        except:
            pass

    def callproc(self, name, params = []):
        '''DB-API Cursor.callproc()
        '''
        cmd = self._cmd
        if self._state == CUR_CLOSED:
            raise ProgrammingError('cursor is closed')
        if self._state in (CUR_FETCHING, CUR_END_RESULT):
            while self._state != CUR_IDLE:
                self._cancel_all()
        if self._state == CUR_IDLE:
            self._sql = -1
            cmd.ct_command(CS_RPC_CMD, name)
            for param in params:
                buf = DataBuf(param)
                cmd.ct_param(buf)
            cmd.ct_send()
            self._start_results()
            self._state = CUR_FETCHING

    def close(self):
        '''DB-API Cursor.close()
        '''
        if self._state == CUR_CLOSED:
            raise ProgrammingError('cursor is closed')
        if self._state == CUR_IDLE:
            self._state = CUR_CLOSED
        elif self._state in (CUR_FETCHING, CUR_END_RESULT, CUR_END_SET):
            self._cancel_all()
            self._dealloc()
            self._cmd = None
        self._state = CUR_CLOSED

    def execute(self, sql, params = []):
        '''DB-API Cursor.execute()
        '''
        if self._state == CUR_CLOSED:
            raise ProgrammingError('cursor is closed')
        if self._state in (CUR_FETCHING, CUR_END_RESULT):
            while self._state != CUR_IDLE:
                self._cancel_all()
        if self._state == CUR_IDLE:
            if self._sql != sql:
                self._dealloc()
                self._prepare(sql)
            self._send_params(params)
            self._start_results()

    def executemany(self, sql, params_seq = []):
        '''DB-API Cursor.executemany()
        '''
        for params in params_seq:
            self.execute(sql, params)

    def fetchone(self):
        '''DB-API Cursor.fetchone()
        '''
        if self._state == CUR_CLOSED:
            raise ProgrammingError('cursor is closed')
        cmd = self._cmd
        if self._state == CUR_FETCHING:
            row = cmd.fetch_rows(self._bufs)
            if row:
                return row
            self._state = CUR_END_RESULT

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
        if self._state == CUR_CLOSED:
            raise ProgrammingError('cursor is closed')
        if self._state == CUR_IDLE:
            return
        if self._state == CUR_FETCHING:
            self._cancel_current()
        self._start_results()
        return self._state != CUR_IDLE or None

    def setinputsizes(self):
        '''DB-API Cursor.setinputsizes()
        '''
        pass

    def setoutputsize(self, size, column = None):
        '''DB-API Cursor.setoutputsize()
        '''
        pass

    def _new_cmd(self, sql):
        if self._dyn_name:
            self._teardown()
        self._dyn_name = 'dyn%s' % self._owner._next_dyn()
        self._sql = sql
        self._prepare()

    def _discard_results(self):
        '''Discard all pending results and prepare to execute again.
        '''
        while self._state != CMD_END_RESULTS:
            self.nextset()

    def _prepare(self, sql):
        '''Prepare the statement to be executed for this cursor.
        '''
        con = self._con
        cmd = self._cmd
        self._dyn_name = 'dyn%s' % self._owner._next_dyn()
        self._sql = sql
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

    def _send_params(self, params):
        '''Execute prepared statement, send parameters and prepare
        buffers for results.
        '''
        cmd = self._cmd
        dyn_name = self._dyn_name
        cmd.ct_dynamic(CS_EXECUTE, dyn_name)
        for fmt, param in map(None, self._fmts, params):
            buf = DataBuf(fmt)
            buf[0] = param
            cmd.ct_param(buf)
        cmd.ct_send()

    def _start_results(self):
        con = self._con
        cmd = self._cmd
        while 1:
            status, result = cmd.ct_results()
            if status == CS_END_RESULTS:
                self._state = CUR_IDLE
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
                self._state = CUR_FETCHING
                return
            elif result in (CS_CMD_DONE, CS_CMD_SUCCEED):
                self.rowcount = cmd.ct_res_info(CS_ROW_COUNT)
            else:
                raise InternalError(_build_ct_except(con, 'ct_results'))

    def _cancel_current(self):
        cmd = self._cmd
        cmd.ct_cancel(CS_CANCEL_CURRENT)

    def _cancel_all(self):
        cmd = self._cmd
        cmd.ct_cancel(CS_CANCEL_ALL)
        self._state = CUR_IDLE

    def _dealloc(self):
        if not self._dyn_name:
            return
        cmd = self._cmd
        cmd.ct_dynamic(CS_DEALLOC, self._dyn_name)
        cmd.ct_send()
        while 1:
            status, result = cmd.ct_results()
            if status != CS_SUCCEED:
                break
        if status != CS_END_RESULTS:
            raise InternalError(_build_ct_except(con, 'ct_results'))
        self._dyn_name = None

class Bulkcopy:
    def __init__(self, owner, table, direction):
        self._owner = owner
        self._con = con = owner._con
        self._table = table
        self._direction = direction
        status, blk = con.blk_alloc()
        if status != CS_SUCCEED:
            raise InternalError(_build_ct_except(con, 'blk_alloc'))
        self._blk = blk
        status = blk.blk_init(direction, table)
        if status != CS_SUCCEED:
            raise InternalError(_build_ct_except(con, 'blk_init'))

    def rowxfer(self, data):
        if type(data) not in (type([]), type(())):
            raise ProgrammingError('list or tuple expected')
        blk = self._blk
        bufs = []
        for col in range(len(data)):
            buf = DataBuf(data[col])
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
    def __init__(self, dsn, user, passwd, database = None,
                 strip = 0, auto_commit = 0, bulkcopy = 0, delay_connect = 0):
        '''DB-API Sybase.Connect()
        '''
        self._con = self._cmd = None
        self.dsn = dsn
        self.user = user
        self.passwd = passwd
        self.database = database
        self.auto_commit = auto_commit
        status, con = _ctx.ct_con_alloc()
        if status != CS_SUCCEED:
            raise InternalError(_build_cs_except(_ctx, 'ct_con_alloc'))
        self._con = con
        con.strip = strip
        if con.ct_diag(CS_INIT) != CS_SUCCEED:
            raise OperationalError(_build_ct_except(con, 'ct_diag'))
        if con.ct_con_props(CS_SET, CS_USERNAME, user) != CS_SUCCEED:
            raise DatabaseError(_build_ct_except(con, 'ct_con_props CS_USERNAME'))
        if con.ct_con_props(CS_SET, CS_PASSWORD, passwd) != CS_SUCCEED:
            raise DatabaseError(_build_ct_except(con, 'ct_con_props CS_PASSWORD'))
        if bulkcopy and con.ct_con_props(CS_SET, CS_BULK_LOGIN, 1) != CS_SUCCEED:
            raise DatabaseError(_build_ct_except(con, 'ct_con_props CS_BULK_LOGIN'))
        if not delay_connect:
            self.connect()

    def connect(self):
        con = self._con
        if con.ct_connect(self.dsn) != CS_SUCCEED:
            raise InternalError(_build_ct_except(con, 'ct_connect'))
        if con.ct_options(CS_SET, CS_OPT_CHAINXACTS,
                          not self.auto_commit) != CS_SUCCEED:
            raise DatabaseError(_build_ct_except(con, 'ct_options'))
        con.ct_diag(CS_CLEAR, CS_ALLMSG_TYPE)
        if self.database:
            self.execute('use %s' % self.database)
        self._dyn_num = 0

    def get_property(self, prop):
        con = self._con
        status, value = con.ct_con_props(CS_GET, prop)
        if status != CS_SUCCEED:
            raise InternalError(_build_ct_except(con, 'ct_con_props'))
        return value

    def set_property(self, prop, value):
        con = self._con
        if con.ct_con_props(CS_SET, prop, value) != CS_SUCCEED:
            raise InternalError(_build_ct_except(con, 'ct_con_props'))

    def __del__(self):
        try:
            self.close()
        except:
            pass

    def close(self):
        '''DBI-API Connection.close()
        '''
        con = self._con
        status, result = con.ct_con_props(CS_GET, CS_CON_STATUS)
        if status == CS_SUCCEED and not result & CS_CONSTAT_CONNECTED:
            raise ProgrammingError('Connection is already closed')
        if self._cmd:
            self._cmd = None
        con.ct_close(CS_FORCE_CLOSE)

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
        cmd = self._cmd = _Cmd(self._con)
        cmd.ct_command(CS_LANG_CMD, sql)
        cmd.ct_send()
        result_list = self._fetch_results()
        self._con.ct_diag(CS_CLEAR, CS_ALLMSG_TYPE)
        self._cmd = None
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

def connect(dsn, user, passwd, database = None,
            strip = 0, auto_commit = 0, bulkcopy = 0, delay_connect = 0):
    return Connection(dsn, user, passwd, database,
                      strip, auto_commit, bulkcopy, delay_connect)
