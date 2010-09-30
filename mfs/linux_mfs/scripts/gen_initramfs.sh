#! /bin/sh

compression=gzip

usage() {
	echo "USAGE: ${0##*/} root_dir [image.gz|image.bz2|image.lzma|image]"
}

if [ $# -lt 2 ]; then
	usage 1>&2
	exit 1
fi

TARGET="$1"
INITRAMFS="$2"

extension=${INITRAMFS##*.}
case "$extension" in
	[lL][zZ][mM][aA]) compression=lzma ;;
	[gG][zZ]) compression=gzip ;;
	[bB][zZ]2) compression=bzip2 ;;
	*) compression=none ;;
esac

(
cd "$TARGET"
if [ $compression = none ]; then
	find . -print | cpio --quiet -H newc -o
else
	find . -print | cpio --quiet -H newc -o | $compression -c -9
fi
) > "$INITRAMFS"
