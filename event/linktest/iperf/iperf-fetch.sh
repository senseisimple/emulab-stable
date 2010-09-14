#!/bin/sh

version=2.0.2
srcurl="http://sourceforge.net/projects/iperf/files/iperf/iperf 2.02 source"
tarball="iperf-$version.tar.gz"

if [ -x /usr/bin/fetch ]; then
    fetch=/usr/bin/fetch
elif [ -x /usr/bin/wget ]; then
    fetch=/usr/bin/wget
else
    echo "ERROR: iperf-fetch.sh: need either 'fetch' or 'wget' installed"
    exit 1
fi

if [ -n "$1" ]; then srcdir=$1; else srcdir=$PWD ; fi
if [ -n "$2" ]; then tarball=$2; fi
if [ -n "$3" ]; then host=$3; else host=www.emulab.net ; fi
dir=$PWD

if [ ! -d $dir/iperf-$version/src ]; then
    if [ ! -f "$tarball" ]; then
      cd $dir
      echo "Downloading iperf source from $host to $dir ..."
      $fetch http://$host/$tarball
      if [ $? -ne 0 ]; then
           echo "Failed..."
           echo "Downloading rude source from \"$srcurl\" to $dir ..."
           $fetch "$srcurl/$tarball" || {
	       echo "ERROR: iperf-fetch: $fetch failed"
	       exit 1
	   }
      fi
    fi
    echo "Unpacking/patching iperf-$version source ..."
    tar xzof $tarball || {
        echo "ERROR: iperf-fetch.sh: tar failed"
	exit 1
    }
    cd iperf-$version && patch -p0 < ../$srcdir/iperf-patch || {
        echo "ERROR: iperf-fetch.sh: patch failed"
	exit 1
    }
    rm -f */*.orig
fi
exit 0
