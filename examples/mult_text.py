#!/usr/bin/python
#
# From orig/mult_text.c - sybase example program
#
import sys
import string
from sybasect import *
from example import *

def init_db():
    status, ctx = cs_ctx_alloc(CS_VERSION_100)
    if status != CS_SUCCEED:
        raise Error("cs_ctx_alloc failed")
    if ctx.cs_diag(CS_INIT) != CS_SUCCEED:
        raise Error('cs_diag failed')
    status = ctx.ct_init(CS_VERSION_100)
    if status != CS_SUCCEED:
        raise Error("ct_init failed")
    return ctx

def connect_db(ctx, user_name, password):
    status, conn = ctx.ct_con_alloc()
    if status != CS_SUCCEED:
        raise Error("ct_con_alloc failed")
    status = conn.ct_con_props(CS_SET, CS_USERNAME, user_name)
    if status != CS_SUCCEED:
        raise CTError(conn, "ct_con_props failed")
    status = conn.ct_con_props(CS_SET, CS_PASSWORD, password)
    if status != CS_SUCCEED:
        raise CTError(conn, "ct_con_props failed")
    status = conn.ct_connect()
    if status != CS_SUCCEED:
        raise CTError(conn, "ct_connect failed")
    return conn

def send_sql(cmd, sql):
    print "Command = %s" % sql
    status = cmd.ct_command(CS_LANG_CMD, sql)
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, "ct_command failed")
    status = cmd.ct_send()
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, "ct_send failed")

MAX_COLSIZE = 255

def ProcessTimestamp(cmd, textdata):
    status, fmt = cmd.ct_describe(1)
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, "ct_describe failed")
    # Check if the data is a timestamp. If so, save it to the
    # CS_IODESC structure for future text updates.
    if not (fmt.status & CS_TIMESTAMP):
        raise Error("unexpected parameter data received")
    # Bind the timestamp field of the io descriptor to assign the new
    # timestamp from the parameter results.
    fmt.maxlength = 8
    fmt.format = CS_FMT_UNUSED;
    status, buf = cmd.ct_bind(1, fmt)
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, "ct_bind failed")
    # Retrieve the parameter result containing the timestamp.
    status, rows_read = cmd.ct_fetch()
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, "ct_fetch failed")
    # The timestamp was retrieved, so cancel the rest of the result set.
    status = cmd.ct_cancel(CS_CANCEL_CURRENT)
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, "ct_cancel failed")

def updatetextdata(textdata, iodesc, newdata):
    # Allocate a command handle to send the text with
    status, cmd = conn2.ct_cmd_alloc()
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, "ct_cmd_alloc failed")
    # Inform Client-Library the next data sent will be used for a text
    # or image update.
    status = cmd.ct_command(CS_SEND_DATA_CMD)
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, "ct_command failed")
    # Fill in the description information for the update and send it
    # to Client-Library.
    iodesc.total_txtlen = len(newdata)
    iodesc.log_on_update = CS_TRUE
    status = cmd.ct_data_info(CS_SET, CS_UNUSED, iodesc)
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, "ct_data_info failed")
    # Send the text one byte at a time. This is not the best thing to
    # do for performance reasons, but does demonstrate the
    # ct_send_data() can handle arbitrary amounts of data.
    fmt = CS_DATAFMT()
    fmt.maxlength = 1
    buf = Buffer(fmt)
    for c in newdata:
        buf[0] = c
        status = cmd.ct_send_data(buf)
        if status != CS_SUCCEED:
            raise CTError(cmd.conn, "ct_send_data failed")
    # ct_send_data() does writes to internal network buffers. To
    # insure that all the data is flushed to the server, a ct_send()
    # is done.
    status = cmd.ct_send()
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, "ct_send failed")
    # Process the results of the command
    while 1:
        status, result = cmd.ct_results()
        if status != CS_SUCCEED:
            break
        if result == CS_PARAM_RESULT:
            # Retrieve a description of the parameter data.  Only
            # timestamp data is expected in this example.
            ProcessTimestamp(cmd, textdata)
        elif result == CS_STATUS_RESULT:
            # Not expecting CS_STATUS_RESULT in this example,
            # but if received results will be pending. Therefore,
            # cancel the current result set.
            status = cmd.ct_cancel(CS_CANCEL_CURRENT)
            if status != CS_SUCCEED:
                raise CTError(cmd.conn, "ct_cancel failed")
        elif result in (CS_CMD_SUCCEED, CS_CMD_DONE):
            # This means that the command succeeded or is finished.
            pass
        elif result == CS_CMD_FAIL:
            # The server encountered an error while processing our
            # command.
            print "UpdateTextData: ct_results: CS_CMD_FAIL"
        else:
            # We got something unexpected.
            print "UpdateTextData: ct_results: unexpected result"
            # Cancel all results.
            cmd.ct_cancel(CS_CANCEL_ALL)
    # We're done processing results. Let's check the return value of
    # ct_results() to see if everything went ok.
    if status == CS_END_RESULTS:
        # Everything went fine.
        status = CS_SUCCEED
    elif status == CS_FAIL:
        # ct_results() call failed.
        print "UpdateTextData: ct_results() failed"
    else:
        # We got an unexpected return value.
        print "UpdateTextData: ct_results: unexpected result"
    return status
 
def fetch_n_print(cmd):
    status, num_cols = cmd.ct_res_info(CS_NUMDATA)
    if status != CS_SUCCEED:
        raise CTError(cmd.conn, "ct_res_info failed")
    fmt = CS_DATAFMT()
    fmt.datatype = CS_CHAR_TYPE
    fmt.maxlength = MAX_COLSIZE
    fmt.count = 1
    fmt.format = CS_FMT_NULLTERM
    status, target_buf = cmd.ct_bind(1, fmt)
    while 1:
        status, rows_read = cmd.ct_fetch()
        if status not in (CS_SUCCEED, CS_ROW_FAIL):
            break
        if status == CS_ROW_FAIL:
            print "ct_fetch returned row fail"
            continue
        fmt = CS_DATAFMT()
        fmt.maxlength = 5
        buf = Buffer(fmt)
        parts = []
        while 1:
            status, count = cmd.ct_get_data(2, buf)
            if count:
                parts.append(buf[0])
            if status != CS_SUCCEED:
                break
        text = string.join(parts, '')
        print "Length of first text column = %d" % len(text)
        if status != CS_END_ITEM:
            raise CTError(cmd.conn, "ct_get_data failed")
        status, iodesc = cmd.ct_data_info(CS_GET, 2)
        if status != CS_SUCCEED:
            raise CTError(cmd.conn, "ct_data_info failed")
        print "\n\n The text data retrieved is: %s" % text

        print "Updating first text column .."
        updatetextdata(text, iodesc, "Row updated!")
        print "Successfully updated first text column!"

        parts = []
        while 1:
            status, count = cmd.ct_get_data(3, buf)
            if count:
                parts.append(buf[0])
            if status != CS_SUCCEED:
                break
        text2 = string.join(parts, '')
        print "Length of 2nd text column = %d" % len(text2)
        if status != CS_END_DATA:
            raise CTError(cmd.conn, "ct_get_data failed")
        status, iodesc = cmd.ct_data_info(CS_GET, 3)
        if status != CS_SUCCEED:
            raise CTError(cmd.conn, "ct_data_info failed")
        print "\n\n The text data retrieved is: %s" % text2

        print "Updating second text column .."
        status = updatetextdata(text, iodesc, "Second column updated too!")
        print "Successfully updated second text column!"
    print

def handle_returns(cmd):
    while 1:
        status, result = cmd.ct_results()
        if status != CS_SUCCEED:
            break
        if result == CS_ROW_RESULT:
            print "TYPE: ROW RESULT"
            fetch_n_print(cmd)
        elif result == CS_CMD_SUCCEED:    
            print "TYPE: CMD SUCCEEDED"
        elif result == CS_CMD_DONE:
            print "TYPE : CMD DONE"
        elif result == CS_CMD_FAIL:
            print "TYPE: CMD FAIL"
        else:
            sys.stderr.write("unknown results\n")
            return
    if status == CS_END_RESULTS:
        return CS_SUCCEED
    raise CTError(cmd.conn, "ct_results failed")

ctx = init_db()
conn = connect_db(ctx, EX_USERNAME, EX_PASSWORD)
conn2 = connect_db(ctx, EX_USERNAME, EX_PASSWORD)
status, cmd = conn.ct_cmd_alloc()
if status != CS_SUCCEED:
    raise CTError(conn, "ct_cmd_alloc failed")
send_sql(cmd, "insert tempdb..test values(1,'hello world','hello worl')")
handle_returns(cmd)
send_sql(cmd, "select * from tempdb..test")
handle_returns(cmd)
print "\n End of program run!"
