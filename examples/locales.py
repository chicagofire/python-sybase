#!/usr/bin/python
#
# From orig/locales.c - sybase example program
#
import sys
from sybasect import *
from example import *

MAX_COLUMN = 100 
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

def set_locale(ctx):
    # Get a locale handle.  
    status, locale = ctx.cs_loc_alloc()
    if status != CS_SUCCEED:
        raise CSError(ctx, 'cs_loc_alloc failed')
    # Set the new value for date the required date format. In this
    # example, we set the format to be dd/mm/yy. Please refer to
    # cstypes.h for a complete list of date formats and their
    # corresponding defined values.
    if locale.cs_dt_info(CS_SET, CS_DT_CONVFMT, CS_DATES_DMY1) != CS_SUCCEED:
        raise CSError(ctx, 'cs_dt_info failed')
    # set context locale to the one we just defined 
    if ctx.cs_config(CS_SET, CS_LOC_PROP, locale) != CS_SUCCEED:
        raise CSError(ctx, 'cs_config failed')
    # Now retrieve the date format simply to ensure that we will be
    # retrieving data in the format that we specified above.  In this
    # case, we expect CS_DATES_DMY1, which is defined in cstypes.h as
    # being = 3.
    status, value = locale.cs_dt_info(CS_GET, CS_DT_CONVFMT)
    if status != CS_SUCCEED:
        raise CSError(ctx, 'cs_dt_info failed')
    print "Date format has been set to be = %d, (dd/mm/yy)" % value

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

def send_sql(cmd):
    # Build and send the command to the server
    if cmd.ct_command(CS_LANG_CMD, 'select getdate()') != CS_SUCCEED:
        raise CTError(cmd.conn, 'ct_command failed')
    if cmd.ct_send()  != CS_SUCCEED:
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
            print ' %s \t' % bufs[i][0],
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
            print 'TYPE: ROW RESULT'
            bufs = bind_columns(cmd)
            fetch_n_print(cmd, bufs)
        elif result == CS_CMD_SUCCEED:    
            print 'TYPE: CMD SUCCEEDED'
        elif result == CS_CMD_DONE:
            print 'TYPE : CMD DONE'
        elif result == CS_CMD_FAIL:
            raise CTError(cmd.conn, 'ct_results: CS_CMD_FAIL')
        elif result == CS_PARAM_RESULT:
            print "TYPE: PARAM RESULT"
            bufs = bind_columns(cmd)
            fetch_n_print(cmd, bufs)
        elif result == CS_STATUS_RESULT:
            print "TYPE: STATUS RESULTS"
            bufs = bind_columns(cmd)
            fetch_n_print(cmd, bufs)
        elif result == CS_COMPUTE_RESULT:
            print "TYPE: COMPUTE RESULTS"
        else:
            sys.stderr.write('unknown results\n')
            return
    if status != CS_END_RESULTS:
        raise CTError(cmd.conn, 'ct_results failed')

def cleanup_db(ctx, status):
    if status != CS_SUCCEED:
        exit_type = CS_FORCE_EXIT
    else:
        exit_type = CS_UNUSED
    # close and cleanup connection to the server
    if ctx.ct_exit(exit_type) != CS_SUCCEED:
        raise CSError(ctx, 'ct_exit failed')
    # drop the context
    if ctx.cs_ctx_drop() != CS_SUCCEED:
        raise CSError(ctx, 'cs_ctx_drop failed')

# Initialize the context 
ctx = init_db()
# Declare and customize the CS_LOCALE structure 
set_locale(ctx)
# Connect to the server 
conn = connect_db(ctx, EX_USERNAME, EX_PASSWORD)
# Allocate a command structure 
status, cmd = conn.ct_cmd_alloc()
if status != CS_SUCCEED:
    raise CTError(conn, 'ct_cmd_alloc failed')
# Get a command and send it to the server 
send_sql(cmd)
# Process results from the server 
handle_returns(cmd)
# Drop the command structure 
if cmd.ct_cmd_drop() != CS_SUCCEED:
    raise CTError(conn, 'ct_cmd_drop failed')
# Close the connection to the server 
if conn.ct_close() != CS_SUCCEED:
    raise CTError(conn, 'ct_close failed')
# Deallocate context and do general cleanup 
cleanup_db(ctx, status)
print '\n End of program run!'
