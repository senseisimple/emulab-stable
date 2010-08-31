#!/bin/sh

version=2.34
tclversion=1.19
srcurl="http://heanet.dl.sourceforge.net/project/nsnam/allinone/ns-allinone-$version"
tarball="ns-allinone-$version.tar.gz"

if [ -x /usr/bin/fetch ]; then
    fetch=/usr/bin/fetch
elif [ -x /usr/bin/wget ]; then
    fetch=/usr/bin/wget
else
    echo "ERROR: ns-fetch.sh: need either 'fetch' or 'wget' installed"
    exit 1
fi

if [ -n "$1" ]; then srcdir=$1; else srcdir=$PWD ; fi
if [ -n "$2" ]; then tarball=$2; fi
if [ -n "$3" ]; then host=$3; else host=www.emulab.net ; fi
dir=$PWD

if [ ! -d $dir/ns-allinone-$version ]; then
    if [ ! -f "$tarball" ]; then
        cd $dir
        # since this is such a big bubba, grab from /share rather than http
        if [ -f /share/tarballs/$tarball ]; then
            echo "Using NFS cached version ..."
	    tarball=/share/tarballs/$tarball
        else
            echo "Downloading ns source from $host to $dir ..."
            $fetch http://$host/$tarball
            if [ $? -ne 0 ]; then
                echo "Failed..."
                 echo "Downloading ns source from $srcurl to $dir ..."
                 $fetch $srcurl/$tarball || {
	             echo "ERROR: ns-fetch: $fetch failed"
	             exit 1
	         }
            fi
        fi
    fi
    echo "Unpacking/patching ns-$version source ..."
    tar xzof $tarball || {
        echo "ERROR: ns-fetch.sh: tar failed"
	exit 1
    }
    patch -d ns-allinone-$version/ns-$version < $srcdir/ns-$version.patch || {
        echo "ERROR: ns-fetch.sh: patch failed"
	exit 1
    }
    if [ -e $srcdir/tclcl-$tclversion.patch ]; then \
        echo "Patching tclcl-$tclversion ..."
	patch -d ns-allinone-$version/tclcl-$tclversion \
	    < $srcdir/tclcl-$tclversion.patch; \
    fi
    find . -name '*.orig' -exec rm -f '{}' \;
fi
exit 0
