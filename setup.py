#!/usr/bin/env python
# To use:
#       python setup.py install
#

import distutils, os
from distutils.core import setup, Extension

if os.name == 'posix':
    sybase = '/opt/sybase'
    syb_libs = [ 'blk', 'ct', 'cs', 'sybtcl', 'comn', 'intl' ]
elif os.name == 'nt':
    sybase = 'i:\\sybase\\sql11.5'
    syb_libs = [ 'libblk', 'libct', 'libcs' ]
else:
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

