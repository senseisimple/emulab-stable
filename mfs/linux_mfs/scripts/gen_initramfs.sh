#! /bin/sh

TARGET="$1"
INITRAMFS="$2"

cd "$TARGET"
find . -print | cpio --quiet -H newc -o | gzip -c -9 > "$INITRAMFS"
