/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _OSKIT_BOOT_BOOTWHAT_H_
#define _OSKIT_BOOT_BOOTWHAT_H_

#define BOOTWHAT_DSTPORT		6969
#define BOOTWHAT_SRCPORT		9696

/*
 * This is the structure we pass back and forth between a oskit kernel
 * and a server running on some other machine, that tells what to do.
 */
#define  MAX_BOOT_DATA		512
#define  MAX_BOOT_PATH		256
#define  MAX_BOOT_CMDLINE	((MAX_BOOT_DATA - MAX_BOOT_PATH) - 32)

typedef struct {
	int	opcode;
	int	status;
	char	data[MAX_BOOT_DATA];
} boot_info_t;

/* Opcode */
#define BIOPCODE_BOOTWHAT_REQUEST	1	/* What to boot request */
#define BIOPCODE_BOOTWHAT_REPLY		2	/* What to boot reply */
#define BIOPCODE_BOOTWHAT_ACK		3	/* Ack to Reply */

/* BOOTWHAT Reply */
typedef struct {
	int	type;
	union {
		/*
		 * Type is BIBOOTWHAT_TYPE_PART
		 *
		 * Specifies the partition number.
		 */
		int			partition;
		
		/*
		 * Type is BIBOOTWHAT_TYPE_SYSID
		 *
		 * Specifies the PC BIOS filesystem type.
		 */
		int			sysid;
		
		/*
		 * Type is BIBOOTWHAT_TYPE_MB
		 *
		 * Specifies a multiboot kernel pathway suitable for TFTP.
		 */
		struct {
			struct in_addr	tftp_ip;
			char		filename[MAX_BOOT_PATH];
		} mb;
	} what;
	char	cmdline[1];
} boot_what_t;

/* What type of thing to boot */
#define BIBOOTWHAT_TYPE_PART	1	/* Boot a partition number */
#define BIBOOTWHAT_TYPE_SYSID	2	/* Boot a system ID */
#define BIBOOTWHAT_TYPE_MB	3	/* Boot a multiboot image */

/* Status */
#define BISTAT_SUCCESS		0
#define BISTAT_FAIL		1

#endif /* _OSKIT_BOOT_BOOTWHAT_H_ */
