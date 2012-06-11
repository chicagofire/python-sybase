#!/bin/sh
REL=$1
EXT=$2

SVNROOT=https://python-sybase.svn.sourceforge.net/svnroot/python-sybase

if [ "x$REL" = "x" ]; then
    echo "usage: dist-sybase release"
    echo "       where release is something like 0.04"
    exit 1
fi

if [ "x$EXT" != "x" ]; then
    SSH_USER="rboehne"
fi

echo "Packaging release $REL from $SVNROOT"

# Get tag from release number
TAG=`echo $REL | sed -e 's/\./_/;s/^/r/'`

cd /tmp

echo "Creating /tmp/sybase-dist"
rm -rf sybase-dist
mkdir sybase-dist
cd sybase-dist

svn copy $SVNROOT/trunk $SVNROOT/tags/$TAG  -m "Tagging the $REL release."

svn -q export $SVNROOT/tags/$TAG  ./sybase-$TAG
if [ ! -f sybase-$TAG/setup.py ]; then
    echo "Could not export code"
    exit 1
fi

# Use setup.py to create source distribution
cd sybase-$TAG
python setup.py sdist
if [ "$?" != "0" ]; then
    echo "Fix up release numbers!"
    exit 1
fi

cp dist/python-sybase-$REL.tar.gz ..

echo "Building documentation"
cd doc
DISPLAY=:0 PATH=$HOME/bin:$PATH make html pdf booklet

echo "Moving documentation to /tmp/sybase-dist"
mv sybase.pdf ../..
gzip -f sybase-booklet.ps
mv sybase-booklet.ps.gz ../..
tar czf ../../sybase-html.tar.gz sybase

echo "Cleaning up"
cd ../..
rm -rf sybase-$TAG

echo "New version of Sybase is available in /tmp/sybase-dist"

echo "Moving new release to web server"
webdir=/home/project-web/python-sybase/htdocs
fildir=/home/frs/project/p/py/python-sybase
www=$SSH_USER,python-sybase@shell.sourceforge.net
ssh -t $www create
# create a directory for the source download
ssh -t $www mkdir -p $fildir/python-sybase/python-sybase-$REL/
if [ -f python-sybase-$REL.zip ]; then
    scp python-sybase-$REL.zip  $www:$fildir/python-sybase/python-sybase-$REL/
fi
scp python-sybase-$REL.tar.gz  $www:$fildir/python-sybase/python-sybase-$REL/

scp sybase.pdf sybase-booklet.ps.gz sybase-html.tar.gz $www:$webdir

ssh $www \
"(cd $webdir
  tar xzf sybase-html.tar.gz
  mv pyrhon-sybase-$REL.tar.gz sybase.pdf sybase-booklet.ps.gz sybase-html.tar.gz ./download)"

