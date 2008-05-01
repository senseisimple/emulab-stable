/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <netinet/in.h>

/*
 * This structure is placed in the boot block. The loader reads the
 * structure to determine whether it should boot from CDROM or switch
 * to the disk.
 *
 * The basic idea is this:
 *
 * 1. The node always boots from the CD-ROM (first in boot chain).
 * 2. The loader looks for this magic structure:
 *     2a: If not present continues to load from the CD-ROM (step 4).
 *     2b: If present check the contents of the structure. If directed to
 *         boot from the disk goto step 3. If not, continues to load from
 *         the CDROM (step 4).
 * 3. Boot from the disk. Switch the boot device to the disk so that the
 *    kernel is loaded from the disk. Also reset the flag (written to disk)
 *    to indicate that next reboot should happen from the CDROM. Also set
 *    another flag, cleared by the kernel, that indicates the kernel booted
 *    okay. This avoids reboot loops in the case of a scrogged disk.
 * 4. Boot from the CD-ROM. The user level code on the CD-ROM will take care
 *    of the rest, writing the proper flags to the structure on the disk.
 *    Normally, this means checking in with Emulab, and then setting the flag
 *    to cause it to boot from the disk.
 *
 * This entire structure has to be less than a sector.
 */
#define SECSIZE			512
#define TBBOOT_MAXIFACELEN	12
#define TBBOOT_MAXHOSTLEN	32
#define TBBOOT_MAXDOMAINLEN	64
#define TBBOOT_MAXKEYLEN	64
#define TBBOOT_MAXDISKDEVLEN	32

typedef struct tbboot_header
{
	unsigned long		magic1;
	short			version;

	/*
	 * BIOS device number to boot from, with two magic values:
	 * - 0xfe means the boot device (CD or USB dongle)
	 * - 0xff means the disk on which this sector is found
	 */
	unsigned char			bootdisk;

	/*
	 * Set bootfromdisk to 1 to force loader to boot from disk.
	 */
	char			bootfromdisk;

	/*
	 * Set bootfromcdrom to 0 when booting from the disk. The kernel
	 * will set this to 1. If the cdrom boots with bootfromdisk 0
	 * and bootfromcdrom 0, something went wrong and the kernel did not
	 * boot properly. Avoids a loop.
	 */
	char			bootfromcdrom;

	/*
	 * Flag to indicate the image is valid. Clear this when writing
	 * a new image, and set it when done.
	 */
	char			validimage;

	/*
	 * Flag to indicate the system config block is valid (has info).
	 */
	char			validconfig;

	/*
	 * crc32 from the zlib library.
	 */ 
	unsigned long		checksum;

	/* Paranoia */
	unsigned long		magic2;

	/*
	 * The emulab key. 
	 */
	char			emulabkey[TBBOOT_MAXKEYLEN];

	/*
	 * System configuration.
	 */
	struct {
		char		interface[TBBOOT_MAXIFACELEN];
		char		hostname[TBBOOT_MAXHOSTLEN];
		char		domain[TBBOOT_MAXDOMAINLEN];
		struct in_addr	IP;
		struct in_addr	netmask;
		struct in_addr	nameserver;
		struct in_addr	gateway;
	} sysconfig;
} tbboot_t;

/* Magic value identifying the header.  */
#define TBBOOT_MAGIC1		0x9badbeef
#define TBBOOT_MAGIC2		0x69ceafd8

/* Current Version */
#define TBBOOT_VERSION		101

/*
 * Offset from start of the disk. Hardwired to sector 60 which should be
 * clear on our images.
 */
#define TBBOOT_SECTOR		60
#define TBBOOT_OFFSET		(TBBOOT_SECTOR * SECSIZE)


