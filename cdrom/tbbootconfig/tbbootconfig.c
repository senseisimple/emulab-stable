/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/param.h>
#include <fcntl.h>
#include <zlib.h>
#include <err.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "testbed_boot.h"

/*
 * Operate on the testbed boot header.
 */
static char *usagestr = 
 "usage: %s <options> devicefile\n"
 " -f              Force operation\n"
 " -i              Info mode only; do not write anything\n"
 " -d              Turn on debugging.\n"
 " -v              Verify and exit with status; Do not print anything\n"
 " -p              Purge the header block (write zeros)\n"
 " -b biosdev      Specify the BIOS device to boot from\n"
 " -w filename     Write system config to header block from file\n"
 " -r filename     Read system config from header block and write to file\n"
 " -k 0,#          Set the bootfromdisk flag to either 0 or a DOS part#\n"
 " -c 0,1          Set the bootfromcdrom flag to either 0 or 1\n"
 " -m 0,1          Set the validimage flag to either 0 or 1\n"
 " -e string       Get/Set the emulab key. Use - to get the key\n"
 " device          The device special file (ie: /dev/rad0)\n";

/* Locals */
static int	info, debug, force, verify, purge;
static char    *progname;

/* Forward decls */
static void	usage(void);
static int	readhdr(int devfd, tbboot_t *tbhdr);
static int	writehdr(int devfd, tbboot_t *tbhdr);
static int	readconfigfromfile(int devfd, tbboot_t *tbhdr, char *path);
static int	writeconfigtofile(int devfd, tbboot_t *tbhdr, char *path);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int		rval, ch, devfd;
	tbboot_t	tbboot_hdr;
	int		bootfromcdrom = -1, bootfromdisk = -1, validimage = -1;
	int		bootdisk = 0xff;
	char	       *configfile = NULL, *emulabkey = NULL;
	int		readconfig = 0, writeconfig = 0;

	progname = argv[0];

	if (geteuid()) {
		fprintf(stderr, "You must be root!\n");
		exit(-1);
	}

	if (sizeof(tbboot_hdr) > SECSIZE) {
		warnx("boot header size is too big!\n");
		exit(-1);
	}
	
	while ((ch = getopt(argc, argv, "fidk:c:vm:r:w:pe:b:")) != -1)
		switch(ch) {
		case 'i':
			info++;
			break;
		case 'd':
			debug++;
			break;
		case 'f':
			force++;
			break;
		case 'v':
			verify++;
			break;
		case 'p':
			purge++;
			break;
		case 'b':
			bootdisk = strtoul(optarg, NULL, 16);
			if (bootdisk < 0 || bootdisk > 255)
				usage();
			break;
		case 'k':
			bootfromdisk = atoi(optarg);
			if (bootfromdisk < 0 || bootfromdisk > 255)
				usage();
			break;
		case 'c':
			bootfromcdrom = atoi(optarg);
			if (bootfromcdrom < 0 || bootfromcdrom > 1)
				usage();
			break;
		case 'm':
			validimage = atoi(optarg);
			if (validimage < 0 || validimage > 1)
				usage();
			break;
		case 'r':
			configfile  = optarg;
			readconfig  = 1;
			break;
		case 'w':
			configfile  = optarg;
			writeconfig = 1;
			break;
		case 'e':
			emulabkey  = optarg;
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	if ((devfd = open(argv[0], (info ? O_RDONLY : O_RDWR))) < 0) {
		if (! verify) /* Be silent */
			perror("opening device");
		exit(1);
	}
	rval = readhdr(devfd, &tbboot_hdr);
	if (verify)
		exit(rval);

	if (configfile && readconfig)
		exit(writeconfigtofile(devfd, &tbboot_hdr, configfile));

	if (emulabkey && emulabkey[0] == '-') {
		if (! strlen(tbboot_hdr.emulabkey))
			exit(-1);
		printf("%s\n", tbboot_hdr.emulabkey);
		exit(0);
	}

	if (purge && !info) {
		char	buf[SECSIZE];
		
		bzero(buf, sizeof(buf));

		if (pwrite(devfd, buf, sizeof(buf), (off_t)TBBOOT_OFFSET) < 0){
			perror("purging header");
			exit(1);
		}
		exit(0);
	}

	if (bootfromcdrom != -1)
		tbboot_hdr.bootfromcdrom = bootfromcdrom;
	if (bootfromdisk != -1)
		tbboot_hdr.bootfromdisk  = bootfromdisk;
	if (validimage != -1)
		tbboot_hdr.validimage    = validimage;

	tbboot_hdr.bootdisk = bootdisk;

	if (emulabkey) {
		if (strlen(emulabkey) > TBBOOT_MAXKEYLEN - 1) {
			warnx("Key is longer that %d chars!",
			      TBBOOT_MAXKEYLEN - 1);
			exit(-1);
		}
		strcpy(tbboot_hdr.emulabkey, emulabkey);
	}

	if (tbboot_hdr.bootfromdisk == tbboot_hdr.bootfromcdrom && !force) {
		warnx("Attempt to set bootfromcdrom=bootfromdisk! "
		      "Use -f option.\n");
		exit(-1);
	}

	if (configfile && writeconfig &&
	    readconfigfromfile(devfd, &tbboot_hdr, configfile))
		exit(-1);
	
	writehdr(devfd, &tbboot_hdr);

	close(devfd);
	exit(0);
}

/*
 * Read and validate the header. Returns -1 if the header is invalid.
 */
static int
readhdr(int devfd, tbboot_t *tbhdr)
{
	char		buf[SECSIZE];
	unsigned	oldcrc, newcrc;
	
	if (lseek(devfd, (off_t) TBBOOT_OFFSET, SEEK_SET) < 0) {
		perror("seeking to read header");
		exit(1);
	}

	if (read(devfd, buf, sizeof(buf)) < 0) {
		perror("reading header");
		exit(1);
	}
	memcpy(tbhdr, buf, sizeof(*tbhdr));

	if (tbhdr->magic1 != TBBOOT_MAGIC1 || tbhdr->magic2 != TBBOOT_MAGIC2) {
		if (verify)
			return -1;
		
		warnx("Bad magic number in header!");
		if (! force)
			exit(1);
		bzero(tbhdr, sizeof(*tbhdr));
		tbhdr->magic1  = TBBOOT_MAGIC1;
		tbhdr->magic2  = TBBOOT_MAGIC2;
		tbhdr->version = TBBOOT_VERSION;
		
		/* No info to print */
		return -1;
	}

	if (debug) {
		fprintf(stderr, "Reading TB Boot Header:\n");
		fprintf(stderr, "  bootfromdisk:   %d\n",
			tbhdr->bootfromdisk);
		fprintf(stderr, "  bootfromcdrom:  %d\n",
			tbhdr->bootfromcdrom);
		fprintf(stderr, "  checksum:       %lx\n",
			tbhdr->checksum);
		fprintf(stderr, "  emulabkey:      %s\n",
			tbhdr->emulabkey);
		fprintf(stderr, "  validimage:     %d\n",
			tbhdr->validimage);
		fprintf(stderr, "  validconfig:    %d\n",
			tbhdr->validconfig);

		if (tbhdr->validconfig) {
			fprintf(stderr, "    interface:  %s\n",
			       tbhdr->sysconfig.interface);
			fprintf(stderr, "    hostname:   %s\n",
			       tbhdr->sysconfig.hostname);
			fprintf(stderr, "    domain:     %s\n",
			       tbhdr->sysconfig.domain);
			fprintf(stderr, "    IP:         %s\n",
			       inet_ntoa(tbhdr->sysconfig.IP));
			fprintf(stderr, "    netmask:    %s\n",
			       inet_ntoa(tbhdr->sysconfig.netmask));
			fprintf(stderr, "    nameserver: %s\n",
			       inet_ntoa(tbhdr->sysconfig.nameserver));
			fprintf(stderr, "    gateway:    %s\n",
			       inet_ntoa(tbhdr->sysconfig.gateway));
		}
	}

	/*
	 * Check the checksum.
	 */
	oldcrc = tbhdr->checksum;
	tbhdr->checksum = 0;
	newcrc = crc32(0L, Z_NULL, 0);
	newcrc = crc32(newcrc, (const char *) tbhdr, sizeof(*tbhdr));

	if (newcrc != oldcrc) {
		if (verify)
			return -1;

		warnx("Bad crc header! %x != %x\n", newcrc, oldcrc);
		if (! force)
			exit(1);
		return -1;
	}
	return 0;
}

/*
 * Write the header,
 */
static int
writehdr(int devfd, tbboot_t *tbhdr)
{
	char		buf[SECSIZE];
	unsigned long	newcrc;

	if (tbhdr->magic1 != TBBOOT_MAGIC1 || tbhdr->magic2 != TBBOOT_MAGIC2) {
		warnx("Bad magic number in header!");
		if (! force)
			exit(1);
	}
	tbhdr->checksum = 0;
	newcrc = crc32(0L, Z_NULL, 0);
	newcrc = crc32(newcrc, (const char *) tbhdr, sizeof(*tbhdr));
	tbhdr->checksum = newcrc;
	
	if (debug) {
		if (info) 
			fprintf(stderr, "Would write TB Boot Header:\n");
		else
			fprintf(stderr, "Writing TB Boot Header:\n");
		
		fprintf(stderr, "  bootfromdisk:   %d\n",
			tbhdr->bootfromdisk);
		fprintf(stderr, "  bootfromcdrom:  %d\n",
			tbhdr->bootfromcdrom);
		fprintf(stderr, "  checksum:       %lx\n",
			tbhdr->checksum);
		fprintf(stderr, "  emulabkey:      %s\n",
			tbhdr->emulabkey);
		fprintf(stderr, "  validimage:     %d\n",
			tbhdr->validimage);
		fprintf(stderr, "  validconfig:    %d\n",
			tbhdr->validconfig);

		if (tbhdr->validconfig) {
			fprintf(stderr, "    interface:  %s\n",
			       tbhdr->sysconfig.interface);
			fprintf(stderr, "    hostname:   %s\n",
			       tbhdr->sysconfig.hostname);
			fprintf(stderr, "    domain:     %s\n",
			       tbhdr->sysconfig.domain);
			fprintf(stderr, "    IP:         %s\n",
			       inet_ntoa(tbhdr->sysconfig.IP));
			fprintf(stderr, "    netmask:    %s\n",
			       inet_ntoa(tbhdr->sysconfig.netmask));
			fprintf(stderr, "    nameserver: %s\n",
			       inet_ntoa(tbhdr->sysconfig.nameserver));
			fprintf(stderr, "    gateway:    %s\n",
			       inet_ntoa(tbhdr->sysconfig.gateway));
		}
	}
	if (info)
		return 0;

	bzero(buf, sizeof(buf));
	memcpy(buf, tbhdr, sizeof(*tbhdr));

	if (pwrite(devfd, buf, sizeof(buf), (off_t) TBBOOT_OFFSET) < 0) {
		perror("writing header");
		exit(1);
	}
	
	return 0;
}

#define CONFIG_STRING	1
#define CONFIG_INADDR	2
#define CONFIG_IGNORE	0x10000
tbboot_t		foohdr;

struct configval {
	char	*name;
	int	type;
	int	size;
	int	offset;
	union {
		char		*string;
		struct in_addr  in;
	} value;
} configvals[] = {
	{"IP",		CONFIG_INADDR, sizeof(foohdr.sysconfig.IP),
	 (char *) &foohdr.sysconfig.IP - (char *) &foohdr },
	{"netmask",	CONFIG_INADDR, sizeof(foohdr.sysconfig.netmask),
	 (char *) &foohdr.sysconfig.netmask - (char *) &foohdr },
	{"nameserver",  CONFIG_INADDR, sizeof(foohdr.sysconfig.nameserver),
	 (char *) &foohdr.sysconfig.nameserver - (char *) &foohdr },
	{"gateway",     CONFIG_INADDR, sizeof(foohdr.sysconfig.gateway),
	 (char *) &foohdr.sysconfig.gateway - (char *) &foohdr },
	{"interface",   CONFIG_STRING, sizeof(foohdr.sysconfig.interface),
	 (char *) &foohdr.sysconfig.interface - (char *) &foohdr },
	{"hostname",    CONFIG_STRING, sizeof(foohdr.sysconfig.hostname),
	 (char *) &foohdr.sysconfig.hostname - (char *) &foohdr },
	{"domain",      CONFIG_STRING, sizeof(foohdr.sysconfig.domain),
	 (char *) &foohdr.sysconfig.domain - (char *) &foohdr },
	{"bootdisk",    CONFIG_IGNORE, 0, 0 },
};
int maxconfigs = sizeof(configvals)/sizeof(struct configval);

static int
readconfigfromfile(int devfd, tbboot_t *tbhdr, char *path)
{
	FILE	*fp;
	int	i, badconfig = 0;
	char	buf[BUFSIZ];

	if ((fp = fopen(path, "r")) == NULL) {
		perror("opening configfile for read");
		exit(-1);
	}

	while (fgets(buf, sizeof(buf), fp)) {
		char	*name, *value, *bp;

		if (buf[0] == '\n' || buf[0] == '#')
			continue;
		
		if ((bp = rindex(buf, '\n')))
			*bp = 0;

		if (! (bp = index(buf, '='))) {
			warnx("Misformed input line: '%s'\n", buf);
			return -1;
		}
		*bp   = 0;
		name  = buf;
		value = bp + 1;
		
		if (!strlen(name) || !strlen(value)) {
			warnx("Misformed input line: '%s:%s'\n", name, value);
			return -1;
		}
		
		for (i = 0; i < maxconfigs; i++) {
			if (! strcmp(configvals[i].name, name))
				break;
		}
		if (i == maxconfigs) {
			warnx("Invalid input line: '%s'\n", buf);
			return -1;
		}

		switch (configvals[i].type) {
		case CONFIG_STRING:
			if (strlen(value) > configvals[i].size - 1) {
				warnx("Value too long!: %s\n", value);
				return -1;
			}
			configvals[i].value.string = strdup(value);
			break;
			
		case CONFIG_INADDR:
			inet_aton(value, &configvals[i].value.in);
			break;

		case CONFIG_IGNORE:
			break;

		default:
			warnx("Bad config type: %d\n", configvals[i].type);
			return -1;
		}
	}

	/*
	 * Check to make sure all the fields were specified. If so, we can
	 * set the sysconfig to valid and overwrite the current info.
	 */
	for (i = 0; i < maxconfigs; i++) {
		if (configvals[i].type == CONFIG_IGNORE)
			continue;
		if (configvals[i].type == CONFIG_STRING &&
		    ! configvals[i].value.string) {
			warnx("Config is missing: '%s'\n", configvals[i].name);
			badconfig++;
		}
	}
	if (badconfig) {
		warnx("Not writing configuration because its incomplete.\n");
		return -1;
	}

	/*
	 * Valid config. Change the boot header.
	 */
	for (i = 0; i < maxconfigs; i++) {
		char	*foo = ((char *) tbhdr) + configvals[i].offset;

		switch (configvals[i].type) {
		case CONFIG_STRING:
			strcpy(foo, configvals[i].value.string);
			break;
			
		case CONFIG_INADDR:
			memcpy(foo, &configvals[i].value,
			       sizeof(struct in_addr));
			break;

		case CONFIG_IGNORE:
			break;
		}
	}
	tbhdr->validconfig = 1;
	
	return 0;
}

static int
writeconfigtofile(int devfd, tbboot_t *tbhdr, char *path)
{
	FILE	*fp;

	if (!tbhdr->validconfig) {
		warnx("There is no system configuration in the header block!");
		return -1;
	}

	if ((fp = fopen(path, "w")) == NULL) {
		perror("opening configfile for write");
		exit(-1);
	}

	fprintf(fp, "interface=%s\n", tbhdr->sysconfig.interface);
	fprintf(fp, "hostname=%s\n",  tbhdr->sysconfig.hostname);
	fprintf(fp, "domain=%s\n",    tbhdr->sysconfig.domain);
	fprintf(fp, "IP=%s\n",        inet_ntoa(tbhdr->sysconfig.IP));
	fprintf(fp, "netmask=%s\n",   inet_ntoa(tbhdr->sysconfig.netmask));
	fprintf(fp, "nameserver=%s\n",inet_ntoa(tbhdr->sysconfig.nameserver));
	fprintf(fp, "gateway=%s\n",   inet_ntoa(tbhdr->sysconfig.gateway));
	
	fclose(fp);
	return 0;
}

static void
usage()
{
	fprintf(stderr, usagestr, progname);
	exit(1);
}


