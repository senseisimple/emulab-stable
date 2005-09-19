/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Simple hack to set the type of a DOS partition in the MBR partition table.
 * Both Linux and BSD versions of fdisk have enough idiosyncratic behaviours
 * that it got to be a pain to work around them, just to set the type field
 * for mkextrafs.pl.
 *
 * In Linux, fdisk is interactive only and sfdisk had a habit of trashing BSD
 * boot blocks in partition 1 when asked to change the type of partition 4.
 *
 * In FreeBSD, fdisk wants to validate and correct the size and alignment of
 * all the partitions when asked to add the 4th partition.  Since FBSD4 and
 * FBSD5 have differing ideas about BIOS geometry, the size of the 4th part
 * as created by growdisk under the frisbee MFS depended on whether it was
 * a FBSD4 or 5 MFS.  If the system was then booted under the other flavor
 * of FBSD and fdisk were run (from mkextrafs), fdisk would want to tweak
 * the size of the final partition.  It would do this to the on disk copy
 * of the MBR, but not the "in core" version, creating an inconsistancy
 * (e.g. with disklabel) that I could not resolve.
 */

#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#ifndef __CYGWIN__
#include <sys/types.h>
#include <inttypes.h>
#if __FreeBSD__ >= 5
#include <sys/diskmbr.h>
#endif
#endif

#define BOOT_MAGIC	0xAA55

#define DOSBBSECTOR	0
#define DOSPARTOFF	446
#define NDOSPART	4

struct dospart {
	uint8_t		dp_flag;	/* bootstrap flags */
	uint8_t		dp_shd;		/* starting head */
	uint8_t		dp_ssect;	/* starting sector */
	uint8_t		dp_scyl;	/* starting cylinder */
	uint8_t		dp_typ;		/* partition type */
	uint8_t		dp_ehd;		/* end head */
	uint8_t		dp_esect;	/* end sector */
	uint8_t		dp_ecyl;	/* end cylinder */
	uint32_t	dp_start;	/* absolute starting sector number */
	uint32_t	dp_size;	/* partition size in sectors */
};

struct doslabel {
	int8_t		align[sizeof(short)];	/* Force alignment */
	int8_t		pad2[DOSPARTOFF];
	struct dospart	parts[NDOSPART];
	uint16_t	magic;
};
#define DOSLABELSIZE \
	(DOSPARTOFF + NDOSPART*sizeof(struct dospart) + sizeof(uint16_t))

static int force = 0;
static int verbose = 0;
static int fdw = -1;

int fixmbr(int outfd, int slice, int dtype);

static void
usage(void)
{
	fprintf(stderr, "usage: "
		"dostype [-fv] <diskdev> <DOS_part> <DOS_type>\n"
		" -f          force setting of type even if already non-zero\n"
		" -v          verbose output\n"
		" <diskdev>   disk special file to operate on\n"
		" <DOS_part>  DOS partition number to change (1-4)\n"
		" <DOS_type>  DOS partition type to set.\n");
	exit(1);
}	

int
main(int argc, char **argv)
{
	char *disk;
	int pnum, ptype;
	int ch, fd;

	while ((ch = getopt(argc, argv, "fv")) != -1)
		switch(ch) {
		case 'f':
			force++;
			break;
		case 'v':
			verbose++;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if (argc < 3)
		usage();

	disk = argv[0];
	pnum = atoi(argv[1]);
	if (pnum < 1 || pnum > 4) {
		fprintf(stderr, "Invalid partition number '%s'\n", argv[1]);
		exit(1);
	}
	ptype = (int)strtol(argv[2], 0, 0);
	if (ptype > 255) {
		fprintf(stderr, "Invalid partition type '%s'\n", argv[2]);
		exit(1);
	}

	fd = open(disk, O_RDWR);
#ifdef DIOCSMBR
	/*
	 * Deal with FreeBSD5 funkyness for writing the MBR.  You have to use
	 * an ioctl on the disk to do it.  But, you apparently cannot perform
	 * the ioctl on the "whole disk" device, you have to do it on a slice
	 * device.  So we try opening slice devices until we get one.
	 *
	 * This code was derived from fdisk.
	 */
	if (fd < 0 && errno == EPERM) {
		fd = open(disk, O_RDONLY);
		if (fd >= 0) {
			char sstr[64];
			int p;

			for (p = 1; p <= 4; p++) {
				snprintf(sstr, sizeof sstr, "%ss%d", disk, p);
				fdw = open(sstr, O_RDWR);
				if (fdw >= 0)
					break;
			}
			if (fdw < 0)
				fd = -1;
		}
	}
#endif
	if (fd < 0) {
		perror(disk);
		exit(1);
	}

	return fixmbr(fd, pnum, ptype);
}

int
fixmbr(int outfd, int slice, int dtype)
{
#ifdef __CYGWIN__
	fprintf(stderr, "Does't work under Windows yet\n");
	return 1;
#else
	struct doslabel doslabel;
	int		cc;

	if (lseek(outfd, (off_t)0, SEEK_SET) < 0) {
		perror("Could not seek to DOS label");
		return 1;
	}
	if ((cc = read(outfd, doslabel.pad2, DOSLABELSIZE)) < 0) {
		perror("Could not read DOS label");
		return 1;
	}
	if (cc != DOSLABELSIZE) {
		fprintf(stderr, "Could not get the entire DOS label\n");
 		return 1;
	}
	if (doslabel.magic != BOOT_MAGIC) {
		fprintf(stderr, "Wrong magic number in DOS partition table\n");
 		return 1;
	}

	if (doslabel.parts[slice-1].dp_typ != dtype) {
		if (doslabel.parts[slice-1].dp_typ != 0 && !force) {
			fprintf(stderr, "part%d: type already set, "
				"use '-f' to override\n", slice);
			return 1;
		}
		doslabel.parts[slice-1].dp_typ = dtype;
		if (fdw < 0) {
			if (lseek(outfd, (off_t)0, SEEK_SET) < 0) {
				perror("Could not seek to DOS label");
				return 1;
			}
			cc = write(outfd, doslabel.pad2, DOSLABELSIZE);
			if (cc != DOSLABELSIZE) {
				perror("Could not write DOS label");
				return 1;
			}
		}
#ifdef DIOCSMBR
		else {
			if (ioctl(fdw, DIOCSMBR, doslabel.pad2) < 0) {
				perror("Could not write DOS label");
				return 1;
			}
		}
#endif
		if (verbose)
			printf("Set type of DOS partition %d to %d\n",
			       slice, dtype);
	}
	return 0;
#endif
}
