#!/bin/sh
REL=$1
EXT=$2

if [ "x$REL" = "x" ]; then
    echo "usage: dist-sybase release"
    echo "       where release is something like 0.04"
    exit 1
fi

if [ "x$EXT" != "x" ]; then
    SSH_PORT=-p22002
    SCP_PORT=-P22002
fi

echo "Packaging release $REL from $CVSROOT"

# Get tag from release number
TAG=`echo $REL | sed -e 's/\./_/;s/^/r/'`

cd /tmp

echo "Creating /tmp/sybase-dist"
rm -rf sybase-dist
mkdir sybase-dist
cd sybase-dist
cvs -q export -r$TAG -d sybase-$REL object-craft/sybase
if [ ! -f sybase-$REL/setup.py ]; then
    echo "Could not export code"
    exit 1
fi

# Use setup.py to create source distribution
cd sybase-$REL
python setup.py sdist
if [ "$?" != "0" ]; then
    echo "Fix up release numbers!"
    exit 1
fi

cp dist/sybase-$REL.tar.gz ..

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
rm -rf sybase-$REL

echo "New version of Sybase is available in /tmp/sybase-dist"

echo "Moving new release to web server"
tmpdir=/var/www/projects/sybase/.new
www=numbat
ssh $SSH_PORT $www mkdir $tmpdir
scp $SCP_PORT sybase-$REL.tar.gz sybase.pdf sybase-booklet.ps.gz sybase-html.tar.gz $www:$tmpdir
ssh $SSH_PORT $www \
"(cd $tmpdir
  tar xzf sybase-html.tar.gz
  mv sybase-$REL.tar.gz sybase.pdf sybase-booklet.ps.gz sybase-html.tar.gz ../download
  mv ../sybase ../sybase-
  mv sybase ..
  rm -rf ../sybase-
  cd ..
  rm -rf $tmpdir)"

exit 0
