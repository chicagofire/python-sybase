#!/usr/bin/python
#
# From orig/array_bind.c - sybase example program
#
import sys
import string
from sybasect import *
from example import *

def init_db():
    status, ctx = cs_ctx_alloc(EX_CTLIB_VERSION)
    if status != CS_SUCCEED:
        raise Error("cs_ctx_alloc failed")
    if ctx.cs_diag(CS_INIT) != CS_SUCCEED:
        raise Error('cs_diag failed')
    status = ctx.ct_init(EX_CTLIB_VERSION)
    if status != CS_SUCCEED:
        raise Error("ct_init failed")
    return ctx

def connect_db(ctx, user_name, password):
    # Allocate a connection pointer
    status, conn = ctx.ct_con_alloc()
    if status != CS_SUCCEED:
        raise Error("ct_con_alloc failed")
    # Set the username and password properties
    status = conn.ct_con_props(CS_SET, CS_USERNAME, user_name)
    if status != CS_SUCCEED:
        raise SybaseError(conn, "ct_con_props CS_USERNAME failed")
    status = conn.ct_con_props(CS_SET, CS_PASSWORD, password)
    if status != CS_SUCCEED:
        raise SybaseError(conn, "ct_con_props CS_PASSWORD failed")
    # enable bulk login property
    status = conn.ct_con_props(CS_SET, CS_BULK_LOGIN, CS_TRUE)
    if status != CS_SUCCEED:
        raise SybaseError(conn, "ct_con_props CS_BULK_LOGIN failed")
    # connect to the server
    status = conn.ct_connect()
    if status != CS_SUCCEED:
        raise SybaseError(conn, "ct_connect failed")
    return conn

def send_sql(cmd, sql):
    print "sql = %s" % sql
    # Build and send the command to the server
    status = cmd.ct_command(CS_LANG_CMD, sql)
    if status != CS_SUCCEED:
        raise SybaseError(cmd.conn, "ct_command failed")
    status = cmd.ct_send()
    if status != CS_SUCCEED:
        raise SybaseError(cmd.conn, "ct_send failed")

def bind_columns(cmd):
    status, num_cols = cmd.ct_res_info(CS_NUMDATA)
    if status != CS_SUCCEED:
        raise SybaseError(cmd.conn, "ct_res_info failed")

    bufs = [None] * num_cols
    for i in range(num_cols):
        fmt = CS_DATAFMT()
        fmt.datatype = CS_CHAR_TYPE
        fmt.maxlength = 255
        fmt.count = 1
        fmt.format = CS_FMT_NULLTERM
        # Bind returned data to host variables
        status, buf = cmd.ct_bind(i + 1, fmt)
        if status != CS_SUCCEED:
            raise SybaseError(cmd.conn, "ct_bind failed")
        bufs[i] = buf
    return bufs

def fetch_n_print(cmd, bufs):
    status, num_cols = cmd.ct_res_info(CS_NUMDATA)
    if status != CS_SUCCEED:
        raise SybaseError(cmd.conn, "ct_res_info failed")

    # Fetch the bound data into host variables
    while 1:
        status, rows_read = cmd.ct_fetch()
        if status not in (CS_SUCCEED, CS_ROW_FAIL):
            break
        if status == CS_ROW_FAIL:
            print "ct_fetch returned row fail"
            continue
        for i in range(num_cols):
            print " %s \t" % bufs[i][0],
        print
    if status != CS_END_DATA:
        raise SybaseError(cmd.conn, "ct_fetch failed")

def handle_returns(cmd):
    while 1:
        status, result = cmd.ct_results()
        if status != CS_SUCCEED:
            break
        if result == CS_ROW_RESULT:
            print "TYPE: ROW RESULT"
            bufs = bind_columns(cmd)
            fetch_n_print(cmd, bufs)
        elif result == CS_CMD_SUCCEED:    
            print "TYPE: CMD SUCCEEDED"
        elif result == CS_CMD_DONE:
            print "TYPE : CMD DONE"
        elif result == CS_CMD_FAIL:
            print "TYPE: CMD FAIL"
        elif result == CS_PARAM_RESULT:
            print "TYPE: PARAM RESULT"
            bufs = bind_columns(cmd)
            fetch_n_print(cmd)
        elif result == CS_STATUS_RESULT:
            print "TYPE: STATUS RESULTS"
            bufs = bind_columns(cmd)
            fetch_n_print(cmd)
        elif result == CS_COMPUTE_RESULT:
            print "TYPE: COMPUTE RESULTS"
        else:
            sys.stderr.write("unknown results\n")
            return
    if status != CS_END_RESULTS:
        raise SybaseError(cmd.conn, "ct_results failed")

MAX_PUBID   = 5
MAX_PUBNAME = 41
MAX_PUBCITY = 21
MAX_PUBST   = 3
MAX_BIO     = 255
MAXLEN      = 255
DATA_END    = (0)
MAX_COLUMN  = 10
MAX_COLSIZE = 255
 
def write_to_table(conn, data):
    # We are now ready to start the bulk copy operation.  Start by
    # getting the bulk descriptor and initialize
    status, blk = conn.blk_alloc(EX_BLK_VERSION)
    if status != CS_SUCCEED:
        raise SybaseError(conn, "blk_alloc failed")
    status = blk.blk_init(CS_BLK_IN, "test_pubs")
    if status != CS_SUCCEED:
        raise SybaseError(conn, "blk_init failed")

    # Now bind the variables to the columns and transfer the data
    fmt = CS_DATAFMT()
    fmt.count = 1;
    for row in data:
        fmt.datatype = CS_INT_TYPE
        fmt.maxlength = sizeof_type(CS_INT_TYPE)
        buf1 = Buffer(fmt)
        buf1[0] = row[0]
        if blk.blk_bind(1, buf1) != CS_SUCCEED:
            raise SybaseError(conn, "blk_bind failed")
        fmt.datatype = CS_CHAR_TYPE
        fmt.maxlength = MAX_PUBNAME - 1
        buf2 = Buffer(fmt)
        buf2[0] = row[1]
        if blk.blk_bind(2, buf2) != CS_SUCCEED:
            raise SybaseError(conn, "blk_bind failed")
        fmt.datatype = CS_CHAR_TYPE
        fmt.maxlength = MAX_PUBCITY - 1
        buf3 = Buffer(fmt)
        buf3[0] = row[2]
        if blk.blk_bind(3, buf3) != CS_SUCCEED:
            raise SybaseError(conn, "blk_bind failed")
        fmt.maxlength = MAX_PUBST - 1
        buf4 = Buffer(fmt)
        buf4[0] = row[3]
        if blk.blk_bind(4, buf4) != CS_SUCCEED:
            raise SybaseError(conn, "blk_bind failed")
        fmt.maxlength = MAX_BIO - 1
        buf5 = Buffer(fmt)
        buf5[0] = row[4]
        if blk.blk_bind(5, buf5) != CS_SUCCEED:
            raise SybaseError(conn, "blk_bind failed")
        if blk.blk_rowxfer() == CS_FAIL:
            raise SybaseError(conn, "blk_rowxfer failed")
    status, num_rows = blk.blk_done(CS_BLK_ALL)
    if status == CS_FAIL:
        raise SybaseError(conn, "blk_done failed")
    print "Number of data rows transferred = %d" % num_rows
    if blk.blk_drop() == CS_FAIL:
        raise SybaseError(conn, "blk_drop failed")

def cleanup_db(ctx, status):
    if status != CS_SUCCEED:
        exit_type = CS_FORCE_EXIT
    else:
        exit_type = CS_UNUSED
    # close and cleanup connection to the server
    if ctx.ct_exit(exit_type) != CS_SUCCEED:
        raise Error("ct_exit failed")
    # drop the context
    if ctx.cs_ctx_drop() != CS_SUCCEED:
        raise Error("cs_ctx_drop failed")

data = ((1, "Taylor & Ng",  "San Francisco", "CA", ""),
        (2, "Scarey Books", "Sleepy Hollow", "MA", ""),
        (3, "Witch Craft & Spells", "Salem", "MA", ""))

# Initialize a context
ctx = init_db()
# Routine to make a connection to the server
conn = connect_db(ctx, EX_USERNAME, EX_PASSWORD)
# Allocate a command structure
status, cmd = conn.ct_cmd_alloc()
if status != CS_SUCCEED:
    raise SybaseError(conn, "ct_cmd_alloc failed")
# Send a command to set the database context
send_sql(cmd, "use tempdb")
# handle results from the current command
handle_returns(cmd)
# Call the bulk copy routine
write_to_table(conn, data)
# Drop the command structure
status = cmd.ct_cmd_drop()
# Close the connection to the server
status = conn.ct_close()
# Clean up the context
cleanup_db(ctx, status)
print "\n End of program run!"