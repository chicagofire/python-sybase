#!/usr/bin/env python
# To use:
#       python setup.py install
#

import distutils
import os
import sys
import string
import re
from distutils.core import setup, Extension

def api_exists(func, filename):
    try:
        text = open(filename).read()
    except:
        return 0
    if re.search(r'CS_PUBLIC %s' % func, text):
        return 1

if os.name == 'posix':                  # unix
    # Most people will define the location of their Sybase
    # installation in their environment.
    if os.environ.has_key('SYBASE'):
        sybase = os.environ['SYBASE']
    else:
        # Not in environment - assume /opt/sybase
        sybase = '/opt/sybase'
        if not os.access(sybase, os.F_OK):
            sys.stderr.write(
                'Please define the Sybase installation directory in'
                'the SYBASE environment variable.\n')
            sys.exit(1)
    # On Linux the Sybase tcl library is distributed as sybtcl
    syb_libs = []
    if os.uname()[0] == 'Linux':
        lib_names = ['blk', 'ct', 'cs', 'sybtcl', 'insck', 'comn', 'intl']
    else:
        lib_names = ['blk', 'ct', 'cs', 'tcl', 'comn', 'intl']
    for name in lib_names:
        lib_name = os.path.join(sybase, 'lib', 'lib%s.a' % name)
        if os.access(lib_name, os.R_OK):
            syb_libs.append(name)

elif os.name == 'nt':                   # win32
    # Not sure how the installation location is specified under NT
    if os.environ.has_key('SYBASE'):
        sybase = os.environ['SYBASE']
        if os.environ.has_key('SYBASE_OCS'):
            ocs = os.environ['SYBASE_OCS']
            sybase = os.path.join(sybase, ocs)
    else:
        sybase = r'i:\sybase\sql11.5'
        if not os.access(sybase, os.F_OK):
            sys.stderr.write(
                'Please define the Sybase installation directory in'
                'the SYBASE environment variable.\n')
            sys.exit(1)
    syb_libs = ['libblk', 'libct', 'libcs']

else:                                   # unknown
    import sys
    sys.stderr.write(
        'Sorry, I do not know how to build on this platform.\n'
        '\n'
        'Please edit setup.py and add platform specific settings.  If you\n'
        'figure out how to get it working for your platform, please send\n'
        'mail to djc@object-craft.com.au so you can help other people.\n')
    sys.exit(1)

syb_incdir = os.path.join(sybase, 'include')
syb_libdir = os.path.join(sybase, 'lib')
for dir in (syb_incdir, syb_libdir):
    if not os.access(dir, os.F_OK):
        sys.stderr.write('Directory %s does not exist - cannot build.\n' % dir)
        sys.exit(1)

extra_objects = None
runtime_library_dirs = None
try:
    if os.uname()[0] == 'SunOS':
        syb_libs.append('sybdb')
        syb_libs.remove('comn')
        extra_objects = [os.path.join(syb_libdir, 'libcomn.a')]
        runtime_library_dirs = [syb_libdir]
except:
    pass

syb_macros = [('WANT_BULKCOPY', None)]
for api in ('blk_alloc', 'blk_describe', 'blk_drop', 'blk_rowxfer_mult',
            'blk_textxfer',):
    if api_exists(api, os.path.join(syb_incdir, 'bkpublic.h')):
        syb_macros.append(('HAVE_' + string.upper(api), None))
for api in ('ct_cursor', 'ct_data_info', 'ct_dynamic', 'ct_send_data',
            'ct_setparam',):
    if api_exists(api, os.path.join(syb_incdir, 'ctpublic.h')):
        syb_macros.append(('HAVE_' + string.upper(api), None))
for api in ('cs_calc', 'cs_cmp',):
    if api_exists(api, os.path.join(syb_incdir, 'cspublic.h')):
        syb_macros.append(('HAVE_' + string.upper(api), None))

setup(name = "Sybase",
      version = "0.34",
      maintainer = "Dave Cole",
      maintainer_email = " djc@object-craft.com.au",
      description = "Sybase Extension to Python",
      url = "http://www.object-craft.com.au/projects/sybase/",
      py_modules = ['Sybase'],
      include_dirs = [syb_incdir],
      ext_modules = [
          Extension('sybasect',
                    ['blk.c', 'databuf.c', 'cmd.c', 'conn.c', 'ctx.c',
                     'datafmt.c', 'iodesc.c', 'locale.c', 'msgs.c',
                     'numeric.c', 'money.c', 'datetime.c',
                     'sybasect.c'],
                    define_macros = syb_macros,
                    libraries = syb_libs,
                    library_dirs = [syb_libdir],
                    runtime_library_dirs = runtime_library_dirs,
                    extra_objects = extra_objects
                    )
          ],
      )

