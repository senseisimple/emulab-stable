#! /bin/sh

devices=$1
target_path=$2

if [ -z "$devices" ] || [ -z "$target_path" ]; then
	echo "Usage ${0##*/} devices target_path"
	exit 1
fi

if ! [ -f "$devices" ]; then
	echo "${0##*/}: $devices: no such file or directory"
	exit 2
fi

#if ! [ -d "$target_path" ]; then
#	echo "${0##*/}: $target_path: no such file or directory"
#	exit 3
#fi
mkdir -p "$target_path" || exit 1

mkdir -p "$target_path/dev"

#sed 's/# .*$//;/^[ 	]*$/d' $devices | \
sed 's/#.*$//;/^[ \t]*$/d' $devices | \
while read name type major minor uid gid mode target; do
	device="$target_path/dev/$name"
	if [ "$type" = d ]; then
		mkdir -p "$device"
	elif [ -n "${device%/*}" ]; then
		mkdir -p "${device%/*}"
	fi

	case $type in
		b|c) mknod "$device" $type $major $minor ;;
		p) mkfifo "$device" ;;
		l) ln -s "$target" "$device" ;;
		s) ;; # not supported yet
	esac
	chown $uid:$gid "$device"
	chmod $mode "$device"
done
