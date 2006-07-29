#!/bin/sh

version=0.70

if [ -x /usr/bin/fetch ]; then
    fetch=/usr/bin/fetch
elif [ -x /usr/bin/wget ]; then
    fetch=/usr/bin/wget
else
    echo "ERROR: rude-fetch.sh: need either 'fetch' or 'wget' installed"
    exit 1
fi

if [ ! -d rude-$version/src ]; then
    echo "Downloading rude source from www.emulab.net to $1 ..."
    cd $1
    $fetch http://www.emulab.net/downloads/rude-$version.tar.gz
    if [ $? -ne 0 ]; then
         echo "Failed..."
	 exit 1
    fi
    echo "Unpacking/patching $rude-version source ..."
    tar xzof rude-$version.tar.gz || {
        echo "ERROR: rude-fetch.sh: tar failed"
	exit 1
    }
    if [ -d rude -a ! -d rude-$version ]; then
        mv rude rude-$version
    fi
    cd rude-$version && patch -p0 < ../rude-patch || {
        echo "ERROR: rude-fetch.sh: patch failed"
	exit 1
    }
    rm -f ../rude-$version.tar.gz */*.orig
fi
exit 0
