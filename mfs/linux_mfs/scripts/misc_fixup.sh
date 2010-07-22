#! /bin/sh

target_dir="$1"

if [ -z "$target_dir" ] || ! [ -d "$target_dir" ]; then
	echo "${0##*/}: invalid target directory \"$target_dir\""
	exit 1
fi

if ! [ -f "$target_dir/bin/tcsh" ]; then
	ln -sf /bin/tcsh.fake "$target_dir/bin/tcsh"
	ln -sf /bin/tcsh.fake "$target_dir/bin/csh"
fi
