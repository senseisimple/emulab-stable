#!/bin/sh

version=0.70
srcurl="http://sourceforge.net/projects/rude/files/rude/rude-$version"
tarball="rude-$version.tar.gz"

if [ -x /usr/bin/fetch ]; then
    fetch=/usr/bin/fetch
elif [ -x /usr/bin/wget ]; then
    fetch=/usr/bin/wget
else
    echo "ERROR: rude-fetch.sh: need either 'fetch' or 'wget' installed"
    exit 1
fi

if [ -n "$1" ]; then srcdir=$1; else srcdir=$PWD ; fi
if [ -n "$2" ]; then tarball=$2; fi
if [ -n "$3" ]; then host=$3; else host=www.emulab.net ; fi
dir=$PWD

if [ ! -d $dir/rude-$version/src ]; then
    if [ ! -f "$tarball" ]; then
      cd $dir
      echo "Downloading rude source from $host to $dir ..."
      $fetch http://$host/$tarball
      if [ $? -ne 0 ]; then
           echo "Failed..."
           echo "Downloading rude source from $srcurl to $dir ..."
           $fetch $srcurl/$tarball || {
	       echo "ERROR: rude-fetch: $fetch failed"
	       exit 1
	   }
      fi
    fi
    echo "Unpacking/patching rude-$version source ..."
    tar xzof $tarball || {
        echo "ERROR: rude-fetch.sh: tar failed"
	exit 1
    }
    if [ -d rude -a ! -d rude-$version ]; then
        mv rude rude-$version
    fi
    cd rude-$version && patch -p0 < ../$srcdir/rude-patch || {
        echo "ERROR: rude-fetch.sh: patch failed"
	exit 1
    }
    rm -f */*.orig
fi
exit 0
