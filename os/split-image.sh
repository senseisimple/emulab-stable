#!/bin/sh

#
# Split up an image and imagezip it. Beware, the sector offsets and sizes
# are hardcoded in here.
#

image=wd0
zip=imagezip

#
# Boot block: start 0, size 63
# 
dd if=$image of=${image}-mbr bs=1b count=63
$zip ${image}-mbr ${image}-mbr.ndz

#
# So, who picked these sizes? Da ya think maybe we could pick sizes
# that are nicely divisible by 32k or 64k, instead of an odd number?
# 

#
# FreeBSD: start 63, size 6281352
# 
dd if=$image of=${image}-fbsd ibs=1b skip=63 obs=1b count=6281352
$zip ${image}-fbsd ${image}-fbsd.ndz

#
# Linux: start 6281415, size 6281415
# 
dd if=$image of=${image}-rhat ibs=1b skip=6281415 obs=1b count=6281415
$zip ${image}-rhat ${image}-rhat.ndz

$zip $image $image-all.ndz
