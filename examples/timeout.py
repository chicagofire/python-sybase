#!/usr/bin/python
#
# From orig/timeout.c - sybase example program
#
import sys
from sybasect import *
from example import *

MAX_COLSIZE = 255

def init_db():
    # allocate a context
    status, ctx = cs_ctx_alloc(CS_VERSION_100)
    if status != CS_SUCCEED:
        raise Error('cs_ctx_alloc failed')
    if ctx.cs_diag(CS_INIT) != CS_SUCCEED:
        raise CSError(ctx, 'cs_diag failed')
    # initialize the library
    if ctx.ct_init(CS_VERSION_100) != CS_SUCCEED:
        raise CSError(ctx, 'ct_init failed')
    return ctx

def connect_db(ctx, user_name, password):
    # Allocate a connection pointer
    status, conn = ctx.ct_con_alloc()
    if status != CS_SUCCEED:
        raise CSError(ctx, 'ct_con_alloc failed')
    if conn.ct_diag(CS_INIT) != CS_SUCCEED:
        raise CTError(conn, 'ct_diag failed')
    # Set the username and password properties
    if conn.ct_con_props(CS_SET, CS_USERNAME, user_name) != CS_SUCCEED:
        raise CTError(conn, 'ct_con_props CS_USERNAME failed')
    if conn.ct_con_props(CS_SET, CS_PASSWORD, password) != CS_SUCCEED:
        raise CTError(conn, 'ct_con_props CS_PASSWORD failed')
    # connect to the server
    if conn.ct_connect() != CS_SUCCEED:
        raise CTError(conn, 'ct_connect failed')
    return conn
							
def send_sql(cmd, sql):
    # Build and send the command to the server 
    if cmd.ct_command(CS_LANG_CMD, sql) != CS_SUCCEED:
        raise CTError(cmd.conn, 'ct_connect failed')
    if cmd.ct_send() != CS_SUCCEED:
        raise CTError(cmd.conn, 'ct_send failed')

def bind_columns(cmd):
    status, num_cols = cmd.ct_res_info(CS_NUMDATA)
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, 'ct_res_info failed')

    bufs = [None] * num_cols
    for i in range(num_cols):
        fmt = CS_DATAFMT()
        fmt.datatype = CS_CHAR_TYPE
        fmt.maxlength = MAX_COLSIZE
        fmt.count = 1
        fmt.format = CS_FMT_NULLTERM
        # Bind returned data to host variables
        status, buf = cmd.ct_bind(i + 1, fmt)
        if status != CS_SUCCEED:
            raise CTError(cmd.conn, 'ct_bind failed')
        bufs[i] = buf
    return bufs
 
def fetch_n_print(cmd, bufs):
    status, num_cols = cmd.ct_res_info(CS_NUMDATA)
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, 'ct_res_info failed')

    # Fetch the bound data into host variables
    while 1:
        status, rows_read = cmd.ct_fetch()
        if status not in (CS_SUCCEED, CS_ROW_FAIL):
            break
        if status == CS_ROW_FAIL:
            print 'ct_fetch returned row fail'
            continue
        for i in range(num_cols):
            print ' %s \t ' % bufs[i][0],
        print
    if status != CS_END_DATA:
        raise CTError(cmd.conn, 'ct_fetch failed')

def handle_returns(cmd):
    # Process all returned result types
    while 1:
        status, result = cmd.ct_results()
        if status != CS_SUCCEED:
            break
        if result == CS_ROW_RESULT:
            bufs = bind_columns(cmd)
            fetch_n_print(cmd, bufs)
        elif result == CS_CMD_SUCCEED:    
            pass
        elif result == CS_CMD_DONE:
            pass
        elif result == CS_CMD_FAIL:
            raise CTError(cmd.conn, 'ct_results: CS_CMD_FAIL')
        elif result == CS_STATUS_RESULT:
            print 'TYPE: STATUS RESULTS'
        else:
            sys.stderr.write('unknown results\n')
            return
    if status != CS_END_RESULTS:
        raise CTError(cmd.conn, 'ct_results failed')

def cleanup_db(ctx):
    # close and cleanup connection to the server
    if ctx.ct_exit() != CS_SUCCEED:
        raise CSError(ctx, 'ct_exit failed')
    # drop the context
    if ctx.cs_ctx_drop() != CS_SUCCEED:
        raise CSError(ctx, 'cs_ctx_drop failed')

# Allocate context and initialize client-library 
ctx = init_db()
# Establish connections to the server 
conn1 = connect_db(ctx, EX_USERNAME, EX_PASSWORD)
conn2 = connect_db(ctx, EX_USERNAME, EX_PASSWORD)
# Allocate command structures on connection 1 
status, cmd1a = conn1.ct_cmd_alloc()
if status != CS_SUCCEED:
    raise CTError(conn1, 'ct_cmd_alloc failed')
status, cmd1b = conn1.ct_cmd_alloc()
if status != CS_SUCCEED:
    raise CTError(conn1, 'ct_cmd_alloc failed')
# Allocate a command structure on connection 2 
status, cmd2 = conn2.ct_cmd_alloc()
if status != CS_SUCCEED:
    raise CTError(conn2, 'ct_cmd_alloc failed')
# Set the timeout interval to 5 seconds.
timeout = 5
if ctx.ct_config(CS_SET, CS_TIMEOUT, timeout) != CS_SUCCEED:
    raise CSError(ctx, 'ct_config failed')
print 'Timeout value = %d seconds' % timeout
# send sql text to server 	
print 'Executing Command 1 on connection 1..'
send_sql(cmd1a, 'select * from pubs2.dbo.publishers')
# process results 
handle_returns(cmd1a)
print 'Executing Command 2 on connection 1..'
send_sql(cmd1b, "select name from pubs2.dbo.sysobjects where type = 'U'")
handle_returns(cmd1b)
print 'Command 3 on connection 2..'
send_sql(cmd2, "select name from tempdb..sysobjects where type = 'U'")
handle_returns(cmd2)
# Drop all the command structures 
if cmd1a.ct_cmd_drop() != CS_SUCCEED:
    raise CTError(conn1, 'ct_cmd_drop failed')
if cmd1b.ct_cmd_drop() != CS_SUCCEED:
    raise CTError(conn1, 'ct_cmd_drop failed')
if cmd2.ct_cmd_drop() != CS_SUCCEED:
    raise CTError(conn2, 'ct_cmd_drop failed')
# Close the connections to the server 
if conn1.ct_close() != CS_SUCCEED:
    raise CTError(conn1, 'ct_close failed')
if conn2.ct_close() != CS_SUCCEED:
    raise CTError(conn2, 'ct_close failed')
# Drop context and do general cleanup 
cleanup_db(ctx)
print 'End of program run!'
