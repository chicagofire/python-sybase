#!/bin/sh
REL=$1

if [ "x$REL" = "x" ]; then
    echo "usage: dist-sybase release"
    echo "       where release is something like 0.04"
    exit 1
fi

echo "Packaging release $REL"

# Get tag from release number
TAG=`echo $REL | sed -e 's/\./_/;s/^/r/'`

check_rev() {
    echo -n Checking $1
    ver=`grep "$2" $1 | sed -e "$3"`
    if [ ! $? -o ! "$REL" = "$ver" ]; then
	echo " - version in $1 is $ver"
	exit 1
    fi
    echo " - found $ver"
}

# Check that the release number exists in all necessary places
check_rev setup.py 'version' 's/.* "//;s/".*//'
check_rev doc/sybase.tex '^.release' 's/.*{//;s/}.*//'
check_rev Sybase.py 'version' "s/.* = '//;s/'.*//"

echo "Ready to build!"

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
# do not distribute the distribution parts
mv sybase-$REL/dist .
tar czf sybase-$REL.tar.gz sybase-$REL

echo "Building documentation"
cd sybase-$REL/doc
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
www=www.object-craft.com.au
ssh $www mkdir $tmpdir
scp sybase-$REL.tar.gz sybase.pdf sybase-booklet.ps.gz sybase-html.tar.gz dist/make_webpage.py dist/index.ahtml $www:$tmpdir
ssh $www \
"(cd $tmpdir
  tar xzf sybase-html.tar.gz
  mv sybase-$REL.tar.gz sybase.pdf sybase-booklet.ps.gz sybase-html.tar.gz ..
  mv ../sybase ../sybase-
  mv sybase ..
  rm -rf ../sybase-
  python make_webpage.py .. > index.html
  mv index.html ..
  cd ..
  rm -rf $tmpdir)"

exit 0
