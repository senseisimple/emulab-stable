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
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/disklabel.h>

struct diskinfo {
	char bootblock[512];
	int cpu, tpc, spt;
	unsigned long disksize;
	struct dos_partition *parts;
} diskinfo;

char optionstr[] =
"[-hvW] [disk]\n"
"	-h print usage message\n"
"	-v verbose output\n"
"	-W actually change the partition table\n"
"	   (default is to just show what would be done)\n"
"	[disk] is the disk special file to operate on\n"
"	   (default is /dev/ad0)";

#define usage()	errx(1, "Usage: %s %s\n", progname, optionstr);

void getdiskinfo(char *disk);
int setdiskinfo(char *disk);
int tweakdiskinfo(char *disk);
int showdiskinfo(char *disk);

char *progname;
int list = 1, verbose;

main(int argc, char *argv[])
{
	int ch;
	char *disk = "/dev/ad0";

	progname = argv[0];
	while ((ch = getopt(argc, argv, "vhW")) != -1)
		switch(ch) {
		case 'v':
			verbose++;
			break;
		case 'W':
			list = 0;
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
	exit(list ? showdiskinfo(disk) : setdiskinfo(disk));
}

void
getdiskinfo(char *disk)
{
	int fd;
	struct disklabel label;
	unsigned long chs = 1;
	struct sector0 {
		char stuff[DOSPARTOFF];
		char parts[512-2-DOSPARTOFF];
		unsigned short magic;
	} *s0;


	memset(&diskinfo, 0, sizeof(diskinfo));
	fd = open(disk, O_RDONLY);
	if (fd < 0)
		err(1, "%s: opening for read", disk);
	if (ioctl(fd, DIOCGDINFO, &label) < 0)
		err(1, "%s: DIOCGDINFO", disk);
	diskinfo.cpu = label.d_ncylinders;
	chs *= diskinfo.cpu;
	diskinfo.tpc = label.d_ntracks;
	chs *= diskinfo.tpc;
	diskinfo.spt = label.d_nsectors;
	chs *= diskinfo.spt;
	diskinfo.disksize = label.d_secperunit;
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
	diskinfo.parts = (struct dos_partition *)s0->parts;
	close(fd);
}

/*
 * Return non-zero if a partition was modified, zero otherwise.
 */
int
tweakdiskinfo(char *disk)
{
	int i, last = NDOSPART;
	struct dos_partition *dp;
	long firstfree = -1;

	for (i = 0; i < NDOSPART; i++) {
		dp = &diskinfo.parts[i];
		if (dp->dp_typ != 0) {
			last = i;
			if (firstfree < 0 ||
			    dp->dp_start + dp->dp_size > firstfree)
				firstfree = dp->dp_start + dp->dp_size;
		}
	}

	/*
	 * Extremely paranoid.  Only change if:
	 *	- there is an unused (type==0) partition available
	 *	- there is unused (not covered by another partition) space
	 *	- unused partition start/size is 0 or start corresponds
	 *		to the beginning of the unused space
	 */
	if (last >= NDOSPART-1 || firstfree >= diskinfo.disksize ||
	    (dp->dp_start != 0 && dp->dp_start != firstfree) ||
	    (dp->dp_start == 0 && dp->dp_size != 0)) {
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

	/*
	 * And of course make sure the partition isn't already correctly
	 * defined.
	 */
	if (dp->dp_start == firstfree &&
	    dp->dp_size == diskinfo.disksize - firstfree)
		return 0;

	dp = &diskinfo.parts[last+1];
	if (verbose || list) {
		if (dp->dp_start)
			printf("%s: %s size of partition %d "
			       "from %lu to %lu\n", disk,
			       list ? "would change" : "changing",
			       last+2, dp->dp_size,
			       diskinfo.disksize-firstfree);
		else
			printf("%s: %s partition %d "
			       "as start=%lu, size=%lu\n", disk,
			       list ? "would define" : "defining",
			       last+2, firstfree,
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
	struct dos_partition *dp;

	printf("%s: %lu sectors (%dx%dx%d CHS)\n", disk,
	       diskinfo.disksize, diskinfo.cpu, diskinfo.tpc, diskinfo.spt);
	for (i = 0; i < NDOSPART; i++) {
		dp = &diskinfo.parts[i];
		printf("  %d: start=%9lu, size=%9lu, type=0x%02x\n", i+1,
		       dp->dp_start, dp->dp_size, dp->dp_typ);
	}
	tweakdiskinfo(disk);
	return 0;
}

int
setdiskinfo(char *disk)
{
	int fd, cc;

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
	close(fd);
	if (verbose)
		printf("%s: partition table modified\n", disk);
	return 0;
}
