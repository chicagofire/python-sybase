#!/usr/bin/env python
# To use:
#       python setup.py install
#

import distutils, os, sys, stat
from distutils.core import setup, Extension

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
    if os.access(os.path.join(sybase, 'lib', 'libsybtcl.a'), os.R_OK):
        syb_libs = ['blk', 'ct', 'cs', 'sybtcl', 'comn', 'intl']
    else:
        syb_libs = ['blk', 'ct', 'cs', 'tcl', 'comn', 'intl']

elif os.name == 'nt':                   # win32
    # Not sure how the installation location is specified under NT
    if os.environ.has_key('SYBASE'):
        sybase = os.environ['SYBASE']
    else:
        sybase = r'i:\sybase\sql11.5'
        if not os.access(sybase, os.F_OK):
            sys.stderr.write(
                'Please define the Sybase installation directory in'
                'the SYBASE environment variable.\n')
            sys.exit(1)
    syb_libs = [ 'libblk', 'libct', 'libcs' ]

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

# The version is set in Lib/numeric_version.py

setup (name = "Sybase",
       version = "0.30",
       maintainer = "Dave Cole",
       maintainer_email = " djc@object-craft.com.au",
       description = "Sybase Extension to Python",
       url = "http://www.object-craft.com.au/projects/sybase/",
       py_modules = ['Sybase'],
       include_dirs = [syb_incdir],
       ext_modules = [
           Extension('sybasect',
                     ['blk.c', 'databuf.c', 'cmd.c', 'conn.c', 'ctx.c',
                      'datafmt.c', 'iodesc.c', 'msgs.c',
                      'numeric.c', 'money.c', 'datetime.c', 'sybasect.c'],
                     libraries = syb_libs,
                     library_dirs = [syb_libdir]
                     )
           ],
       )

