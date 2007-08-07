#!/bin/ksh

if [[ ! -d target ]]
then
   rm -rf ../target
fi

ARCH=$(./config.guess)
export ARCH
SUPPORT_HOME=/livraison/test/${ARCH}/support/support-2.6.5
export SUPPORT_HOME
PATH=${SUPPORT_HOME}/bin:${PATH}

case "${ARCH}" in
    hppa2.0w-hp-hpux11.23)
        ;;
    powerpc-ibm-aix5.3.0.0)
        export MAKE=gmake
        export ABI=32
        export PATH=/usr/vac/bin:/usr/bin:${PATH}
        export CC=xlc_r
        export LIBPATH=${SUPPORT_HOME}/lib:/usr/lib:${LIBPATH}
        export SYBASE=/sybase/product/15.0
        if [[ -f ${SYBASE}/SYBASE.sh ]]
        then
            . ${SYBASE}/SYBASE.sh
        fi
        ;;
    sparc-sun-solaris2.10)
        export PATH=/opt/SUNWspro/bin:/usr/local/bin:/usr/sfw/bin:/usr/ccs/bin:/usr/xpg4/bin:$PATH
        export CC=cc
        export CFLAGS="-g -O"
        export MAKE=gmake
        export ABI=32

        export LD_LIBRARY_PATH=${SUPPORT_HOME}/lib:${LD_LIBRARY_PATH}
        export SYBASE=/sybase/product/15.0
        if [[ -f ${SYBASE}/SYBASE.sh ]]
        then
            . ${SYBASE}/SYBASE.sh
        fi
        ;;
    *)
        export LD_LIBRARY_PATH=${SUPPORT_HOME}/lib:${LD_LIBRARY_PATH}
        if [[ -d /sybase/product/12.5 ]]
        then
           export SYBASE=/sybase/product/12.5
        fi
        if [[ -d /sybase/product/15.0 ]]
        then
           export SYBASE=/sybase/product/15.0
        fi
        if [[ -f ${SYBASE}/SYBASE.sh ]]
        then
            . ${SYBASE}/SYBASE.sh
        fi
        ;;
esac
        unset LANG



cd ..
python setup.py install --prefix=./target
PYTHONPATH=`pwd`/target/lib/python2.4/site-packages:${PYTHONPATH}
export PYTHONPATH

nosetests --source-folder=. --xml-report-folder=./target/nosexunit-xml --with-nosexunit -v
