/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#define HASH_MAGIC	".ndzsig"
#define HASH_VERSION	0x20031107
#define HASHBLK_SIZE	(64*1024)
#define HASH_MAXSIZE	20

struct hashregion {
	struct region region;
	uint32_t chunkno;
	unsigned char hash[HASH_MAXSIZE];
};

struct hashinfo {
	uint8_t	 magic[8];
	uint32_t version;
	uint32_t hashtype;
	uint32_t nregions;
	uint8_t	 pad[12];
	struct hashregion regions[0];
};

#define HASH_TYPE_MD5	1
#define HASH_TYPE_SHA1	2
#define HASH_TYPE_RAW	3
