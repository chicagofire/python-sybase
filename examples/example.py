class Error(Exception):
    pass

def get_diag(func, type):
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

def build_ct_except(conn, msg):
    err = [msg]
    err.extend(get_diag(conn.ct_diag, CS_SERVERMSG_TYPE))
    err.extend(get_diag(conn.ct_diag, CS_CLIENTMSG_TYPE))
    conn.ct_diag(CS_CLEAR, CS_ALLMSG_TYPE)
    return err

class SybaseError:
    def __init__(self, conn, msg):
        self.err = build_ct_except(conn, msg)

    def __str__(self):
        return string.join(self.err, '\n')

