#!/usr/bin/python
import os
import sys
import re
import stat
import albatross

class File:
    def __init__(self, name):
        global dist_dir
        self.name = name
        stats = os.stat(os.path.join(dist_dir, name))
        self.size = '%dk' % (int(stats[stat.ST_SIZE]) / 1024)
        self.mtime = stats[stat.ST_MTIME]

    def __cmp__(self, other):
        return cmp(self.name, other.name)

dist_dir = sys.argv[1]

ctx = albatross.SimpleContext('.')
files = []
for name in os.listdir(dist_dir):
    if re.match(r'sybase-\d.\d\d\.tar\.gz', name):
        files.append(File(name))
files.sort()
files.reverse()
ctx.locals.files = files
ctx.locals.pdf = File('sybase.pdf')
ctx.locals.booklet = File('sybase-booklet.ps.gz')
ctx.locals.html = File('sybase-html.tar.gz')

templ = ctx.load_template('index.ahtml')
templ.to_html(ctx)
ctx.flush_content()
