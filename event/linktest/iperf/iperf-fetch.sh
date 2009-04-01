#!/bin/sh

if [ -x /usr/bin/fetch ]; then
    fetch=/usr/bin/fetch
elif [ -x /usr/bin/wget ]; then
    fetch=/usr/bin/wget
else
    echo "ERROR: iperf-fetch.sh: need either 'fetch' or 'wget' installed"
    exit 1
fi

if [ ! -d iperf-2.0.2/src ]; then
    echo "Downloading iperf source from www.emulab.net to $1 ..."
    cd $1
    if [ ! -f iperf-2.0.2.tar.gz ]; then
      $fetch http://www.emulab.net/downloads/iperf-2.0.2.tar.gz
      if [ $? -ne 0 ]; then
           echo "Failed..."
	   exit 1
      fi
    fi
    echo "Unpacking/patching iperf-2.0.2 source ..."
    tar xzof iperf-2.0.2.tar.gz || {
        echo "ERROR: iperf-fetch.sh: tar failed"
	exit 1
    }
    cd iperf-2.0.2 && patch -p0 < ../iperf-patch || {
        echo "ERROR: iperf-fetch.sh: patch failed"
	exit 1
    }
    rm -f ../iperf-2.0.2.tar.gz */*.orig
fi
exit 0
