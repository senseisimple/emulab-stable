/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Another little utility that groks DOS partitions and neuters boot blocks
 * and/or superblocks.
 *
 * XXX should be combined with dostype.c.
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

/*
 * For superblocks we wipe the first 8192 bytes,
 * For boot blocks just the first 512. 
 */
#define SB_ZAPSIZE	8192
#define BB_ZAPSIZE	512
#define MAX_ZAPSIZE	8192

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

static int verbose = 0;
static int pnum = 0;
static int bootblocks = 0;
static int superblocks = 0;
static int doit = 0;

static char zapdata[MAX_ZAPSIZE];
static int zapsize;

int readmbr(char *dev);
int zappart(char *dev, int pnum);
int zapmbr(char *disk);

static void
usage(void)
{
	fprintf(stderr, "usage: "
		"zapdisk [-BS] <diskdev>\n"
		" -p <pnum>   operate only on the given partition\n"
		" -B          zap MBR and partition boot programs\n"
		" -S          zap possible superblocks in all partitions\n"
		" -Z          really do the zap and don't just talk about it\n"
		" <diskdev>   disk special file to operate on\n");
	exit(1);
}	

int
main(int argc, char **argv)
{
	char *disk;
	int ch, i;
	int errors = 0;

	while ((ch = getopt(argc, argv, "p:vBSZ")) != -1)
		switch(ch) {
		case 'Z':
			doit++;
			break;
		case 'p':
			pnum = atoi(optarg);
			break;
		case 'v':
			verbose++;
			break;
		case 'B':
			bootblocks++;
			break;
		case 'S':
			superblocks++;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if (argc < 1)
		usage();

	if (!bootblocks && !superblocks) {
		fprintf(stderr, "Must specify either -B or -S\n");
		usage();
	}
	disk = argv[0];
	if (pnum < 0 || pnum > 4) {
		fprintf(stderr, "Invalid partition number %d\n", pnum);
		exit(1);
	}

#ifdef __CYGWIN__
	fprintf(stderr, "Does't work under Windows yet\n");
	exit(1);
#else
	if ((i = readmbr(disk)) != 0) {
		/* lack of a valid MBR is ok */
		if (i < 0) {
			fprintf(stderr, "%s: no valid MBR, skipped\n", disk);
			exit(0);
		}

		fprintf(stderr, "%s: error reading DOS MBR\n", disk);
		exit(1);
	}

	/*
	 * We are assuming that writing zeros provides proper zap-age
	 */
	zapsize = superblocks ? SB_ZAPSIZE : BB_ZAPSIZE;
	memset(zapdata, 0, zapsize);

	for (i = 1; i <= 4; i++)
		if (pnum == 0 || i == pnum)
			if (zappart(disk, i))
				errors++;
	if (pnum == 0 && bootblocks)
		if (zapmbr(disk))
			errors++;

	exit(errors);
#endif
}

static struct doslabel doslabel;

int
readmbr(char *dev)
{
	int fd, cc;

	fd = open(dev, O_RDONLY);
	if (fd < 0) {
		perror(dev);
		return 1;
	}

	if (lseek(fd, (off_t)0, SEEK_SET) < 0) {
		perror("Could not seek to DOS label");
		close(fd);
		return 1;
	}
	if ((cc = read(fd, doslabel.pad2, DOSLABELSIZE)) < 0) {
		perror("Could not read DOS label");
		close(fd);
		return 1;
	}
	if (cc != DOSLABELSIZE) {
		fprintf(stderr, "Could not get the entire DOS label\n");
		close(fd);
 		return 1;
	}

	/*
	 * This is not an error for our purposes.
	 * We assume that if the MBR is not valid, nothing would boot anyway.
	 */
	if (doslabel.magic != BOOT_MAGIC) {
		fprintf(stderr, "Wrong magic number in DOS partition table\n");
		close(fd);
 		return -1;
	}

	return 0;
}

/*
 * Zap the bootblock/superblock in a partition.
 */
int
zappart(char *dev, int pnum)
{
	int fd, cc;
	struct dospart *pinfo = &doslabel.parts[pnum-1];

	if (verbose)
		printf("part%d: start=%d, size=%d\n",
		       pnum, pinfo->dp_start, pinfo->dp_size);
	if (pinfo->dp_start == 0 || pinfo->dp_size == 0) {
		if (verbose || !doit)
			printf("part%d: empty, skipped\n", pnum);
		return 0;
	}

	fd = open(dev, O_RDWR);
	if (fd < 0) {
		perror(dev);
		return 1;
	}
	if (lseek(fd, (off_t)pinfo->dp_start * 512, SEEK_SET) < 0) {
		perror("Could not seek to partition start");
		close(fd);
		return 1;
	}

	if (!doit) {
		printf("part%d: would zero %d bytes at sector %d\n",
		       pnum, zapsize, pinfo->dp_start);
		cc = zapsize;
	} else {
		if (verbose)
			printf("part%d: zeroing %d bytes at sector %d\n",
			       pnum, zapsize, pinfo->dp_start);
		cc = write(fd, zapdata, zapsize);
	}
	if (cc != zapsize) {
		perror("Could not zap partition block");
		close(fd);
		return 1;
	}
	close(fd);
	return 0;
}

/*
 * All manner of dark magic is required to write the MBR on various OSes.
 */
int
zapmbr(char *disk)
{
	int fd, fdw = -1;
	int cc;

	/*
	 * For the MBR we just zero out the 400+ bytes before the
	 * partition table.
	 */
	memset(doslabel.pad2, 0, sizeof(doslabel.pad2));

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
			close(fd);
			if (fdw < 0)
				fd = -1;
		}
	}
#endif
	if (fd < 0) {
		perror(disk);
		exit(1);
	}

	if (!doit) {
		printf("mbr: would zero %d bytes at sector 0\n",
		       sizeof(doslabel.pad2));
		close(fd);
		close(fdw);
		return 0;
	}

	if (verbose)
		printf("mbr: zeroing %d bytes at sector 0\n",
		       sizeof(doslabel.pad2));
	if (fdw < 0) {
		if (lseek(fd, (off_t)0, SEEK_SET) < 0) {
			perror("Could not seek to DOS label");
			close(fd);
			return 1;
		}
		cc = write(fd, doslabel.pad2, DOSLABELSIZE);
		if (cc != DOSLABELSIZE) {
			perror("Could not write DOS label");
			close(fd);
			return 1;
		}
	}
#ifdef DIOCSMBR
	else {
		if (ioctl(fdw, DIOCSMBR, doslabel.pad2) < 0) {
			perror("Could not write DOS label");
			close(fdw);
			return 1;
		}
		close(fdw);
	}
#endif
	return 0;
}
