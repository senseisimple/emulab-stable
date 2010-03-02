#! /bin/sh

compression=gzip

usage() {
	echo "USAGE: ${0##*/} [-c gzip|bzip2|lzma|none] root_dir image"
}

if [ "$1" = "-c" ]; then
	case $2 in
		gzip|bzip2|lzma|none)
			compression=$2
			;;
		*) echo \
			"${0##*/}: invalid compresion method \"$2\"" 1>&2
		   usage 1>&2
		   exit 1
		   ;;
	esac

	shift
	shift
fi

if [ $# -lt 2 ]; then
	usage 1>&2
	exit 1
fi

TARGET="$1"
INITRAMFS="$2"

(
cd "$TARGET"
if [ $compression = none ]; then
	find . -print | cpio --quiet -H newc -o
else
	find . -print | cpio --quiet -H newc -o | $compression -c -9
fi
) > "$INITRAMFS"
