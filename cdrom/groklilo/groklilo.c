/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004-2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Code to grok a LILO bootblock and change the default command line string
 */

/*
 * XXX hack to recognize compilation in the boot loader as opposed to
 * as a standalone application.
 */
#ifdef BOOT_FORTH
#define INBOOTLOADER
#endif

#include <string.h>

#ifdef INBOOTLOADER
#include <stand.h>
#include "emuboot.h"

#ifdef DEBUG
#define debug 1
#else
#define debug 0
#endif
#else
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>

static int debug;
#endif

void *load_partition_bblock(int partition);
int load_sector(int sectno, void *buf);
int save_sector(int sectno, void *buf);

/*
 * These need to be consistant with LILO
 */
#define MAX_BOOT2_SECT	10
#define MAX_IMAGE_DESC	19
#define MAX_MAP_SECT	101

/*
 * Sector addresses.
 * These are the absolute values that must be relocated when moving
 * a bootable partition.
 */
typedef struct sectaddr {
	uint8_t sector;
	uint8_t track;
	uint8_t device;		/* + flags, see below */
	uint8_t head;
	uint8_t nsect;
} sectaddr_t;

/* flags encoded in device */
#define HARD_DISK	0x80	/* not a floppy */
#define LINEAR_ADDR	0x40	/* mark linear address */
#define LBA32_ADDR	0x20    /* mark lba 32-bit address */
#define LBA32_NOCOUNT   0x10    /* mark address with count absent */
#define DEVFLAGS	0xF0

struct bb1 {
	uint8_t jumpinst[6];
	char sig[4];		/* LILO */
	uint16_t stage;		/* 1 -- stage1 loader */
	uint16_t version;
	uint16_t timeout;
	uint16_t delay;
	uint8_t port, portparams;
	uint32_t timestamp;
	sectaddr_t idesc[2];	/* image descriptors */
	sectaddr_t cmdline;	/* command line (max 1 sector?) */
	uint8_t prompt;
	uint16_t msglen;
	sectaddr_t msg;		/* "initial greeting message" */
	sectaddr_t keytab;	/* "keyboard translation table" */
	sectaddr_t boot2[MAX_BOOT2_SECT+1]; /* 2nd stage boot sectors */
};

union bblock {
	struct bb1 bb1;
	char data[1*512];
};

struct cmdlineblock {
	u_int16_t magic;
	char cmdline[510];
};
#define CMDLINE_MAGIC	0xF4F2

static uint32_t
getsector(sectaddr_t *sect)
{
	int flags = (sect->device & DEVFLAGS) & ~HARD_DISK;
	uint32_t sector = 0;

	if (sect->sector == 0 && sect->track == 0 && sect->device == 0 &&
	    sect->head == 0 && sect->nsect == 0)
		return 0;

	if (flags == 0) {
		printf("groklilo: no can do CHS addresses!\n");
		return ~0;
	}

	if (flags & LINEAR_ADDR) {
		sector |= sect->head << 16;
		sector |= sect->track << 8;
		sector |= sect->sector;
	} else {
		if (flags & LBA32_NOCOUNT)
			sector |= sect->nsect << 24;
		sector |= sect->head << 16;
		sector |= sect->track << 8;
		sector |= sect->sector;
	}

	return sector;
}

int
set_lilo_cmdline(int partition, char *cmdline)
{
	struct bb1 *bb;
	struct cmdlineblock clblock;
	uint32_t cbsect;

	printf("Setting LILO cmdline in partition %d to `%s'\n",
	       partition, cmdline);

	if (partition < 1 || partition > 4) {
		if (debug)
			printf("%d: invalid partition number\n", partition);
		return 1;
	}

	bb = load_partition_bblock(partition);
	if (bb == NULL)
		return 1;

	if (strncmp(bb->sig, "LILO", 4) != 0) {
		if (debug)
			printf("%d: no LILO bootblock in partition\n",
			       partition);
		return 1;
	}

	if (bb->stage != 1) {
		if (debug)
			printf("%d: can only handle LILO stage1 bootblock\n",
			       partition);
		return 1;
	}

	cbsect = getsector(&bb->cmdline);
	if (cbsect == ~0) {
		if (debug)
			printf("%d: error parsing LILO cmdline sector\n",
			       partition);
		return 1;
	}

	if (load_sector(cbsect, &clblock)) {
		if (debug)
			printf("%d: could not read LILO cmdline sector %d\n",
			       partition, cbsect);
		return 1;
	}

	if (debug) {
		printf("%d: old command line was: %s\n",
		       partition, clblock.magic == CMDLINE_MAGIC ?
		       clblock.cmdline : "INVALID");
	}

	if (cmdline[0] == 0) {
		clblock.magic = 0x6d6b; /* XXX what lilo does */
	} else {
		strncpy(clblock.cmdline, cmdline, sizeof(clblock.cmdline));
		clblock.magic = CMDLINE_MAGIC;
	}

	if (save_sector(cbsect, &clblock)) {
		if (debug)
			printf("%d: could not write LILO cmdline sector %d\n",
			       partition, cbsect);
		return 1;
	}

	return 0;
}

#ifndef INBOOTLOADER

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static int devfd;
static int nogo;
static int comport = 0;
int set_lilo_comport(int, int);

static char *usagestr = 
 "usage: %s <options> partition devicefile\n"
 " -d              Turn on debugging.\n"
 " -c 'str'        Set the LILO command line\n"
 " -p {1 or 2}     Switch to com1 or com2 if already set up\n"
 " -n              Do not actually perform the operation\n"
 " partition       The DOS partition number where LILO resides (1-4)\n"
 " device          The *whole* disk device special file (ie: /dev/ad0)\n";

static void
usage(char *progname)
{
	fprintf(stderr, usagestr, progname);
	exit(1);
}

int
main(int argc, char **argv)
{
	int ch, part, rv = 0;
	char *cmdline = 0, *progname;

	progname = argv[0];
	while ((ch = getopt(argc, argv, "p:c:dn")) != -1)
		switch(ch) {
		case 'd':
			debug++;
			break;
		case 'c':
			cmdline = optarg;
			break;
		case 'p':
			comport = atoi(optarg);
			break;
		case 'n':
			nogo++;
			break;
		case '?':
		default:
			usage(progname);
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage(progname);

	part = atoi(argv[0]);
	if (part < 1 || part > 4)
		usage(progname);

	if ((devfd = open(argv[1], nogo ? O_RDONLY : O_RDWR)) < 0) {
		perror("opening device");
		exit(1);
	}

	if (cmdline != 0) {
		rv = set_lilo_cmdline(part, cmdline);
		if (rv) {
			fprintf(stderr, "%s: command failed on part %d, "
				"no change made\n",
				argv[1], part);
		}
	}

	if (comport != 0) {
		rv = set_lilo_comport(part, comport);
		if (rv) {
			fprintf(stderr, "%s: command failed on part %d, "
				"no change made\n",
				argv[1], part);
		}
	}

	exit(rv);
}

struct bios_mbr {
	char bootcode[446];
	char partinfo[64];
	unsigned short magic;
};

/* magic */
#define BIOS_MAGIC	0xaa55

struct bios_partition_info {
        unsigned char bootid;
        unsigned char beghead;
        unsigned char begsect;
        unsigned char begcyl; 
        unsigned char sysid;
        unsigned char endhead;
        unsigned char endsect;
        unsigned char endcyl;
        unsigned long offset;
        unsigned long numsectors;
};

static struct bios_mbr diskmbr;
static char partbr[512];
static int partbr_secno;

static int
check_mbr(void)
{
	struct bios_mbr *mbr;
	struct bios_partition_info *entry;
	int i;

	mbr = &diskmbr;
	if (mbr->magic != BIOS_MAGIC)
		return -1;

	entry = (struct bios_partition_info *) mbr->partinfo;
	for (i = 0; i < 4; i++, entry++) {
		if (entry->numsectors == 0)
			continue;

		/* XXX sanity check fields */
	}

	return 0;
}

int
load_sector(int sectno, void *buf)
{
	if (lseek(devfd, (off_t)sectno * 512, SEEK_SET) < 0 ||
	    read(devfd, buf, 512) < 512)
		return errno;
	return 0;
}

int
save_sector(int sectno, void *buf)
{
	if (nogo) {
		fprintf(stderr, "Would write back sector %d\n", sectno);
		return 0;
	}
	if (lseek(devfd, (off_t)sectno * 512, SEEK_SET) < 0 ||
	    write(devfd, buf, 512) < 512)
		return errno;
	return 0;
}

static void
load_mbr(void)
{
	if (load_sector(0, &diskmbr) == 0 && check_mbr() == 0)
		return;

	fprintf(stderr, "Could not load/verify MBR\n");
	exit(-1);
}

void *
load_partition_bblock(int pnum)
{
	struct bios_partition_info *entry;

	load_mbr();

	pnum--;
	entry = ((struct bios_partition_info *)diskmbr.partinfo) + pnum;
	if (load_sector((partbr_secno = (int)entry->offset), partbr) != 0 ||
	    ((struct bios_mbr *)partbr)->magic != BIOS_MAGIC) {
		printf("Could not load/verify partition %d boot record\n",
		       pnum);
		return 0;
	}

	return partbr;
}


int
set_lilo_comport(int partition, int comport)
{
	struct bb1 *bb;

	printf("Asked to set LILO comport in partition %d to %d\n",
	       partition, comport);

	if (comport < 1 || comport > 2) {
		if (debug)
			printf("%d: invalid comport number\n", comport);
		return 1;
	}

	bb = load_partition_bblock(partition);
	if (bb == NULL)
		return 1;

	if (strncmp(bb->sig, "LILO", 4) != 0) {
		if (debug)
			printf("%d: no LILO bootblock in partition\n",
			       partition);
		return 1;
	}

	if (bb->stage != 1) {
		if (debug)
			printf("%d: can only handle LILO stage1 bootblock\n",
			       partition);
		return 1;
	}

	if (bb->port == 0) {
		if (debug)
			printf("%d: can only switch between COM1 and COM2\n",
			       partition);
		return 1;
	}

	if (debug) {
		printf("%d: old comport was: %d (%s)\n",
		       partition, bb->port, bb->port == 1 ?  "COM1" : "COM2");
	}

	bb->port = comport;

	if (save_sector(partbr_secno, partbr)) {
		if (debug)
			printf("%d: could not write LILO boot sector %d\n",
			       partition, partbr_secno);
		return 1;
	}

	return 0;
}
#endif
