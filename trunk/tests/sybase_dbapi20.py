#!/usr/bin/env python
import dbapi20
import Sybase
import popen2


class TestSybase(dbapi20.DatabaseAPI20Test):

    driver = Sybase    
    driver._ctx.debug = 1
    driver.set_debug(open('sybase_debug.log', 'w'))

    connect_args = ()
    lower_func = 'lower' # For stored procedure test

    force_commit = 0

    def commit(self,con):
        if not self.connect_kw_args.get("auto_commit", 0) or self.force_commit:
            con.commit()        

    def executeDDL1(self,con,cursor):
        # Sybase needs NULL to be specified in order to allow NULL values

        self.ddl1 = 'create table %sbooze (name varchar(20) NULL)' % self.table_prefix
        cursor.execute(self.ddl1)
        self.commit(con)

    def executeDDL2(self,con,cursor):
        cursor.execute(self.ddl2)
        self.commit(con)

    def setUp(self):
        # Call superclass setUp In case this does something in the
        # future
        dbapi20.DatabaseAPI20Test.setUp(self) 

        try:
            con = self._connect()
            con.close()
        except:
            # cmd = "psql -c 'create database dbapi20_test'"
            # cout,cin = popen2.popen2(cmd)
            # cin.close()
            # cout.read()
            raise

    def tearDown(self):
        dbapi20.DatabaseAPI20Test.tearDown(self)

    def _paraminsert(self,con,cur):
        # Overridden: Sybase does not handle named parameters like
        # dbapi20 expects

        # dbapi20:  c.execute("insert into foo values (:beer)", {"beer": "whatever"})
        # Sybase:   c.execute("insert into foo values (@beer)", {"@beer": "whatever"})

        self.executeDDL1(con,cur)
        cur.execute("insert into %sbooze values ('Victoria Bitter')" % (
            self.table_prefix
            ))
        self.failUnless(cur.rowcount in (-1,1))

        cur.execute(
            'insert into %sbooze values (@beer)' % self.table_prefix, 
            {'@beer':"Cooper's"}
            )
        self.failUnless(cur.rowcount in (-1,1))

        cur.execute('select name from %sbooze' % self.table_prefix)
        res = cur.fetchall()
        self.assertEqual(len(res),2,'cursor.fetchall returned too few rows')
        beers = [res[0][0],res[1][0]]
        beers.sort()
        self.assertEqual(beers[0],"Cooper's",
            'cursor.fetchall retrieved incorrect data, or data inserted '
            'incorrectly'
            )
        self.assertEqual(beers[1],"Victoria Bitter",
            'cursor.fetchall retrieved incorrect data, or data inserted '
            'incorrectly'
            )

    def test_executemany(self):
        # Overridden: same reason as _paraminsert

        con = self._connect()
        try:
            cur = con.cursor()
            self.executeDDL1(con,cur)
            largs = [ ("Cooper's",) , ("Boag's",) ]
            margs = [ {'@beer': "Cooper's"}, {'@beer': "Boag's"} ]
            cur.executemany(
                'insert into %sbooze values (@beer)' % self.table_prefix,
                margs
                )
            self.failUnless(cur.rowcount in (-1,2),
                'insert using cursor.executemany set cursor.rowcount to '
                'incorrect value %r' % cur.rowcount
                )
            cur.execute('select name from %sbooze' % self.table_prefix)
            res = cur.fetchall()
            self.assertEqual(len(res),2,
                'cursor.fetchall retrieved incorrect number of rows'
                )
            beers = [res[0][0],res[1][0]]
            beers.sort()
            self.assertEqual(beers[0],"Boag's",'incorrect data retrieved')
            self.assertEqual(beers[1],"Cooper's",'incorrect data retrieved')
        finally:
            con.close()

    def test_drop_non_existing(self):
        con = self._connect()
        cur = con.cursor()
        try:
            self.assertRaises(self.driver.DatabaseError, cur.execute, 'drop table %sdummy' % self.table_prefix)
        finally:
            con.close()

    def test_select_non_existing(self):
        con = self._connect()
        cur = con.cursor()
        try:
            self.assertRaises(self.driver.DatabaseError, cur.execute, 'select * from %sdummy' % self.table_prefix)
        finally:
            con.close()

    def test_bug_result_pending(self):
        # unitary test for a bug reported by Ralph Heinkel
        # http://www.object-craft.com.au/pipermail/python-sybase/2002-May/000034.html

        con = self._connect()
        try:
            cur = con.cursor()
            try:
                cur.execute('select * from %sdummy' % self.table_prefix)
            except:
                pass
            cur.execute('create table %sbooze (name varchar(20))' % self.table_prefix)
            con.commit()
        finally:
            con.close()


    def test_bug_result_pending2(self):
        con = self._connect()
        try:
            cur = con.cursor()
            try:
                cur.execute('drop table %sdummy' % self.table_prefix)
            except:
                pass
            cur.execute('create table %sbooze (name varchar(20))' % self.table_prefix)
            con.commit()
        finally:
            con.close()

    def test_bug_result_pending3_fetchall(self):
        # When not in auto_commit mode, it was not possible to commit after a fetchall because of results pending
        con = self._connect()
        try:
            cur = con.cursor()
            cur.execute('create table %sbooze (name varchar(20))' % self.table_prefix)
            con.commit()
            cur.execute('select name from %sbooze' % self.table_prefix)
            cur.fetchall()
            self.commit(con)
        finally:
            con.close()

    def test_duplicate_error(self):
        con = self._connect()
        try:
            cur = con.cursor()
            try:
                cur.execute('drop table %sbooze' % self.table_prefix)
            except self.driver.DatabaseError:
                pass
            cur.execute('create table %sbooze (name varchar(20) PRIMARY KEY)' % self.table_prefix)
            cur.execute("insert into %sbooze values ('Victoria Bitter')" % self.table_prefix)
            self.assertRaises(self.driver.IntegrityError, cur.execute, "insert into %sbooze values ('Victoria Bitter')" % self.table_prefix)
        finally:
            con.close()

    def test_callproc(self):
        con = self._connect()
        try:
            # create procedure lower
            cur = con.cursor()
            try:
                cur.execute("drop procedure lower")
                self.commit(con)
            except con.DatabaseError:
                pass
            cur.execute("create procedure lower(@name varchar(256)) as select lower(@name) commit transaction")
            self.commit(con)
        finally:
            con.close()
        dbapi20.DatabaseAPI20Test.test_callproc(self)

    def test_non_existing_proc(self):
        con = self._connect()
        try:
            # create procedure lower
            cur = con.cursor()
            try:
                cur.execute("drop procedure lower")
                self.commit(con)
            except con.DatabaseError:
                pass
            self.assertRaises(self.driver.StoredProcedureError,cur.callproc,self.lower_func,('FOO',))
        finally:
            con.close()

    def test_python_datetime(self):
        kw_args = self.connect_kw_args
        kw_args.update({'datetime': "python"})
        con = self.driver.connect(
            *self.connect_args,**kw_args
            )
        try:
            cur = con.cursor()
            cur.execute('create table %sbooze (day date)' % self.table_prefix)
            self.commit(con)
            cur.execute("insert into %sbooze values ('20061124')" % self.table_prefix)
            self.commit(con)
            cur.execute("select * from %sbooze" % self.table_prefix)
            res = cur.fetchall()
            date = res[0][0]
            self.assertEqual(self.driver.use_datetime, 2)
            import datetime
            self.assert_(isinstance(date, datetime.datetime))
            self.assertEquals(date.year, 2006)
            self.assertEquals(date.month, 11)
            self.assertEquals(date.day, 24)
            self.assert_(type(date) >= self.driver.DATETIME)
        finally:
            con.close()

    def test_mx_datetime(self):
        kw_args = self.connect_kw_args
        kw_args.update({'datetime': "mx"})
        con = self.driver.connect(
            *self.connect_args,**kw_args
            )
        try:
            cur = con.cursor()
            cur.execute('create table %sbooze (day date)' % self.table_prefix)
            self.commit(con)
            cur.execute("insert into %sbooze values ('20061124')" % self.table_prefix)
            self.commit(con)
            cur.execute("select * from %sbooze" % self.table_prefix)
            res = cur.fetchall()
            date = res[0][0]
            self.assertEqual(self.driver.use_datetime, 1)
            import mx.DateTime
            self.assert_(isinstance(date, mx.DateTime.DateTimeType))
            self.assertEquals(date.year, 2006)
            self.assertEquals(date.month, 11)
            self.assertEquals(date.day, 24)
            self.assert_(type(date) >= self.driver.DATETIME)
        finally:
            con.close()

    def test_sybase_datetime(self):
        kw_args = self.connect_kw_args
        kw_args.update({'datetime': "sybase"})
        con = self.driver.connect(
            *self.connect_args,**kw_args
            )
        try:
            cur = con.cursor()
            cur.execute('create table %sbooze (day date)' % self.table_prefix)
            self.commit(con)
            cur.execute("insert into %sbooze values ('20061124')" % self.table_prefix)
            self.commit(con)
            cur.execute("select * from %sbooze" % self.table_prefix)
            res = cur.fetchall()
            date = res[0][0]
            self.assertEqual(self.driver.use_datetime, 0)
            # print type(date)
            self.assert_(isinstance(date, self.driver.DateTimeType))
            self.assertEquals(date.year, 2006)
            self.assertEquals(date.month, 10)
            self.assertEquals(date.day, 24)
            # self.assertEquals(type(date), self.driver.DATETIME)
            self.assert_(type(date) >= self.driver.DATETIME)
        finally:
            con.close()

# TODO: the following tests must be overridden

    def test_nextset(self):
        pass

    def test_setoutputsize(self):
        pass

