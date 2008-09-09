/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * "Grow" a testbed disk
 *
 * Used to expand the final (DOS) partition in a testbed image to
 * fill the remainder of the disk.  A typical testbed disk image
 * is sized to fit in the least-common-denominator disk we have,
 * currently 13GB.  This current image is laid out as:
 *
 *	       0 to       62: bootarea
 *	      63 to  6281414: FreeBSD (3GB)
 *	 6281415 to 12562829: Linux (3GB)
 *	12562830 to 12819869: Linux swap (128MB)
 *	12819870 to 26700029: unused
 *
 * for multi-OS disks, or:
 *
 *	       0 to       62: bootarea
 *	      63 to      N-1: some OS
 *	       N to 26700029: unused
 *
 * The goal of this program is to locate the final, unused partition and
 * resize it to match the actual disk size.  This program does *not* know
 * how to extend a filesystem, it only works on unused partitions and only
 * adjusts the size of the partition in the DOS partition table.
 *
 * The tricky part is determining how large the disk is.  Currently we do
 * this by reading the value out of a FreeBSD partition table using
 * DIOCGDINFO.  Even if there is no FreeBSD partition table, it should
 * attempt to fake one with a single partition whose size is the entire
 * disk.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <err.h>
#ifdef __FreeBSD__
#define DEFDISK "/dev/ad0"
#if __FreeBSD__ >= 5
#include <sys/disk.h>
#else
#include <sys/disklabel.h>
#endif
#else
#ifdef __linux__
#define DEFDISK "/dev/hda"
#include <linux/fs.h>
#endif
#endif
#include "sliceinfo.h"

struct diskinfo {
	char bootblock[512];
	int cpu, tpc, spt;
	unsigned long disksize;
	struct dospart *parts;
} diskinfo;

char optionstr[] =
"[-fhvW] [disk]\n"
"Create or extend a DOS partition to contain all trailing unallocated space\n"
"	-h print usage message\n"
"	-v verbose output\n"
"	-f output fdisk style partition entry\n"
"	   (sets slice type=FreeBSD if not already set)\n"
"	-N create a new partition to include the extra space (the default)\n"
"	-W actually change the partition table\n"
"	   (default is to just show what would be done)\n"
"	-X extend the final partition to include the extra space\n"
"	   (alternative to -N)\n"
"	[disk] is the disk special file to operate on";

#define usage()	errx(1, "Usage: %s %s\nDefault disk is %s", \
		     progname, optionstr, DEFDISK);

void getdiskinfo(char *disk);
int setdiskinfo(char *disk);
int tweakdiskinfo(char *disk);
int showdiskinfo(char *disk);

char *progname;
int list = 1, verbose, fdisk, usenewpart = 1;

main(int argc, char *argv[])
{
	int ch;
	char *disk = DEFDISK;

	progname = argv[0];
	while ((ch = getopt(argc, argv, "fvhNWX")) != -1)
		switch(ch) {
		case 'v':
			verbose++;
			break;
		case 'f':
			fdisk++;
			break;
		case 'N':
			usenewpart = 1;
			break;
		case 'W':
			list = 0;
			break;
		case 'X':
			usenewpart = 0;
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if (argc == 1)
		disk = argv[0];

	getdiskinfo(disk);
	exit((list || fdisk) ? showdiskinfo(disk) : setdiskinfo(disk));
}

void
getdiskinfo(char *disk)
{
	int fd;
	unsigned long chs = 0;
	struct sector0 {
		char stuff[DOSPARTOFF];
		char parts[512-2-DOSPARTOFF];
		unsigned short magic;
	} *s0;

	memset(&diskinfo, 0, sizeof(diskinfo));
	fd = open(disk, O_RDONLY);
	if (fd < 0)
		err(1, "%s: opening for read", disk);
#ifdef __linux__
	if (verbose)
		warnx("using BLKGETSIZE to determine size");
	if (ioctl(fd, BLKGETSIZE, &diskinfo.disksize) < 0)
		err(1, "%s: BLKGETSIZE", disk);
	diskinfo.cpu = diskinfo.tpc = diskinfo.spt = 0;
	chs = diskinfo.disksize;
#else
#ifdef DIOCGMEDIASIZE
	/*
	 * FreeBSD 5.
	 *
	 * Note we still use the contrived geometry here rather than the
	 * simple "gimme the disk size" DIOCGMEDIASIZE.  Why?  I'm glad you
	 * asked...
	 *
	 * FreeBSD fdisk is adamant about DOS partitions starting/ending
	 * on "cylinder" boundaries.  If we try to resize the final partition
	 * here so that it is not a multiple of the cylinder size and later
	 * run fdisk, fdisk will automatically resize the rogue partition.
	 * However, the new size will only appear on disk and not "in core"
	 * until after a reboot.  So after running fdisk and before rebooting,
	 * there is an inconsisency that can create chaos.  For example,
	 * the mkextrafs script will use fdisk info to create a BSD disklabel.
	 * If it uses the older, larger size it will work, but will suddenly
	 * be too large for the DOS partition after rebooting.  If we try to
	 * use the newer, smaller size disklabel will complain about trying
	 * to shrink the 'c' partition and will fail.  If we only create a
	 * new 'e' partition that is the smaller size and don't try to resize
	 * 'c', then after reboot the 'c' partition will be too big and the
	 * kernel will reject the whole disklabel.
	 */
	{
		unsigned nsect, nhead, ssize;
		off_t dsize;

		if (verbose)
			warnx("using DIOCG* to determine size/geometry");
		if (ioctl(fd, DIOCGSECTORSIZE, &ssize) < 0)
			err(1, "%s: DIOCGSECTORSIZE", disk);
		if (ioctl(fd, DIOCGMEDIASIZE, &dsize) < 0)
			err(1, "%s: DIOCGMEDIASIZE", disk);
		diskinfo.disksize = (unsigned long)(dsize / ssize);
		if (ioctl(fd, DIOCGFWSECTORS, &nsect) < 0)
			err(1, "%s: DIOCGFWSECTORS", disk);
		diskinfo.spt = nsect;
		if (ioctl(fd, DIOCGFWHEADS, &nhead) < 0)
			err(1, "%s: DIOCGFWHEADS", disk);
		diskinfo.tpc = nhead;
		diskinfo.cpu = diskinfo.disksize / (nsect * nhead);
		chs = diskinfo.cpu * diskinfo.tpc * diskinfo.spt;
	}
#else
#ifdef DIOCGDINFO
	{
		struct disklabel label;

		if (verbose)
			warnx("using DIOCGDINFO to determine size/geometry");
		if (ioctl(fd, DIOCGDINFO, &label) < 0)
			err(1, "%s: DIOCGDINFO", disk);
		diskinfo.cpu = label.d_ncylinders;
		diskinfo.tpc = label.d_ntracks;
		diskinfo.spt = label.d_nsectors;
		diskinfo.disksize = label.d_secperunit;
		chs = diskinfo.cpu * diskinfo.tpc * diskinfo.spt;
	}
#endif
#endif
#endif
	if (chs == 0)
		errx(1, "%s: could not get disk size/geometry!?", disk);
	if (diskinfo.disksize < chs)
		errx(1, "%s: secperunit (%lu) < CxHxS (%lu)",
		     disk, diskinfo.disksize, chs);
	else if (diskinfo.disksize > chs) {
		if (verbose)
			warnx("%s: only using %lu of %lu reported sectors",
			      disk, chs, diskinfo.disksize);
		diskinfo.disksize = chs;
	}

	if (read(fd, diskinfo.bootblock, sizeof(diskinfo.bootblock)) < 0)
		err(1, "%s: error reading bootblock", disk);
	s0 = (struct sector0 *)diskinfo.bootblock;
	if (s0->magic != 0xAA55)
		errx(1, "%s: invalid bootblock", disk);
	diskinfo.parts = (struct dospart *)s0->parts;
	close(fd);
}

/*
 * Return non-zero if a partition was modified, zero otherwise.
 */
int
tweakdiskinfo(char *disk)
{
	int i, lastspace = NDOSPART, lastunused = NDOSPART;
	struct dospart *dp;
	long firstfree = -1;

	for (i = 0; i < NDOSPART; i++) {
		dp = &diskinfo.parts[i];
		if (dp->dp_typ != 0) {
			if (firstfree < 0 ||
			    dp->dp_start + dp->dp_size > firstfree) {
				lastspace = i;
				firstfree = dp->dp_start + dp->dp_size;
			}
		}
	}

	/*
	 * If wanting to extend the final used partition but there is
	 * no such partition, just create a new partition instead.
	 */
	if (!usenewpart && lastspace == NDOSPART)
		usenewpart = 1;

	/*
	 * No trailing free space, nothing to do
	 */
	if (firstfree >= diskinfo.disksize) {
		/*
		 * Warn about an allocated partition that exceeds the
		 * physical disk size.  This can happen if someone
		 * creates a disk image on a large disk and attempts
		 * to load it on a smaller one.  Not much we can do
		 * at this point except set off alarms...
		 */
		if (firstfree > diskinfo.disksize)
			warnx("WARNING! WARNING! "
			      "Allocated partitions too large for disk");
		return 0;
	}

	if (usenewpart) {
		int found = 0;

		/*
		 * Look for unused partitions already correctly defined.
		 * If we don't find one of those, we pick the first unused
		 * partition after the last used partition if possible.
		 * This prevents us from unintuitive behavior like defining
		 * partition 4 when no other partition is defined.
		 */
		for (i = NDOSPART-1; i >= 0; i--) {
			dp = &diskinfo.parts[i];
			if (dp->dp_typ != 0) {
				if (!found && lastunused != NDOSPART)
					found = 1;
			} else {
				if (dp->dp_start == firstfree &&
				    dp->dp_size == diskinfo.disksize-firstfree)
					return 0;
				/*
				 * Paranoia: avoid partially defined but
				 * unused partitions unless the start
				 * corresponds to the beginning of the
				 * unused space.
				 */
				if (!found &&
				    ((dp->dp_start == 0 && dp->dp_size == 0) ||
				     dp->dp_start == firstfree))
					lastunused = i;
			}
		}
	} else {
		/*
		 * Only change if:
		 *	- um...nothing else to check
		 *
		 * But tweak variables for the rest of this function.
		 */
		firstfree = diskinfo.parts[lastspace].dp_start;
		lastunused = lastspace;
	}

	if (lastunused == NDOSPART) {
		warnx("WARNING! No usable partition for free space");
		return 0;
	}
	dp = &diskinfo.parts[lastunused];

	if (fdisk) {
		printf("p %d %d %d %d\n",
		       lastunused+1, dp->dp_typ ? dp->dp_typ : DOSPTYP_386BSD,
		       dp->dp_start ? dp->dp_start : firstfree,
		       diskinfo.disksize-firstfree);
		return 1;
	}
	if (verbose || list) {
		if (dp->dp_start)
			printf("%s: %s size of partition %d "
			       "from %lu to %lu\n", disk,
			       list ? "would change" : "changing",
			       lastunused+1, dp->dp_size,
			       diskinfo.disksize-firstfree);
		else
			printf("%s: %s partition %d "
			       "as start=%lu, size=%lu\n", disk,
			       list ? "would define" : "defining",
			       lastunused+1, firstfree,
			       diskinfo.disksize-firstfree);
	}
	dp->dp_start = firstfree;
	dp->dp_size = diskinfo.disksize - firstfree;
	return 1;
}

int
showdiskinfo(char *disk)
{
	int i;
	struct dospart *dp;

	if (!fdisk) {
		printf("%s: %lu sectors", disk, diskinfo.disksize);
		if (diskinfo.cpu)
			printf(" (%dx%dx%d CHS)",
			       diskinfo.cpu, diskinfo.tpc, diskinfo.spt);
		printf("\n");
		for (i = 0; i < NDOSPART; i++) {
			dp = &diskinfo.parts[i];
			printf("  %d: start=%9lu, size=%9lu, type=0x%02x\n",
			       i+1, dp->dp_start, dp->dp_size, dp->dp_typ);
		}
	}
	if (!tweakdiskinfo(disk))
		return 1;
	return 0;
}

int
setdiskinfo(char *disk)
{
	int fd, cc, i, error;

	if (!tweakdiskinfo(disk)) {
		if (verbose)
			printf("%s: no change made\n", disk);
		return 0;
	}

	fd = open(disk, O_RDWR);
	if (fd < 0) {
		warn("%s: opening for write", disk);
		return 1;
	}
	cc = write(fd, diskinfo.bootblock, sizeof(diskinfo.bootblock));
	if (cc < 0) {
		warn("%s: bootblock write", disk);
		return 1;
	}
	if (cc != sizeof(diskinfo.bootblock)) {
		warnx("%s: partial write (%d != %d)\n", disk,
		      cc, sizeof(diskinfo.bootblock));
	}
#ifdef __linux__
	printf("Calling ioctl() to re-read partition table.\n");
	sync();
	sleep(2);
	if ((i = ioctl(fd, BLKRRPART)) != 0) {
                error = errno;
        } else {
                /* some kernel versions (1.2.x) seem to have trouble
                   rereading the partition table, but if asked to do it
		   twice, the second time works. - biro@yggdrasil.com */
                sync();
                sleep(2);
                if ((i = ioctl(fd, BLKRRPART)) != 0)
                        error = errno;
        }

	if (i) {
		printf("\nWARNING: Re-reading the partition table "
			 "failed with error %d: %s.\n"
			 "The kernel still uses the old table.\n"
			 "The new table will be used "
			 "at the next reboot.\n"),
			error, strerror(error);
	}
#endif

	close(fd);
	if (verbose)
		printf("%s: partition table modified\n", disk);
	return 0;
}
