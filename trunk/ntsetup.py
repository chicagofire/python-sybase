#!/usr/bin/env python
# To use:
#       python ntsetup.py install
#

import distutils, os
from distutils.core import setup, Extension

sybase = 'i:\\sybase\\sql11.5'
syb_incdir = os.path.join( sybase, 'include' )
syb_libdir = os.path.join( sybase, 'lib' )
syb_libs = [ 'libblk', 'libct', 'libcs' ]

mxdatetime_incdir = 'i:\\DateTime\\mxDateTime'

# The version is set in Lib/numeric_version.py

setup (name = "Sybase",
       version = "0.24",
       maintainer = "Dave Cole",
       maintainer_email = " djc@object-craft.com.au",
       description = "Sybase Extension to Python",
       url = "http://www.object-craft.com.au/projects/sybase/",
       py_modules = ['Sybase'],
       include_dirs = [syb_incdir, mxdatetime_incdir],
       ext_modules = [
           Extension('sybasect',
                     ['blk.c', 'buffer.c', 'cmd.c', 'conn.c', 'ctx.c',
                      'datafmt.c', 'msgs.c', 'numeric.c', 'sybasect.c'],
                     libraries = syb_libs,
                     library_dirs = [syb_libdir]
                     )
           ],
       )

