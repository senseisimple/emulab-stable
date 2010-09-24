/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <openssl/sha.h>

/*
 *
 * This code only shows how to predict composite hashes.  For quote verifying,
 * see ltpmloadkey.c
 *
 */

int main(int argc, char **argv)
{
	struct {
		unsigned short ssize;
		union {
			unsigned short slen;
			unsigned char slenc[2];
		} uni;
		unsigned int pcrsize;
		unsigned char pcr[20];
	} thingy;

	SHA_CTX	sc;
	unsigned char hash[20];
	char *p;
	int i;
	char *pcr0 = "\x16\xCD\xD4\xEA\xA9\x47\x12\x30\xEB\x05"
		     "\x99\x36\xE8\xD9\x4D\x4F\xB0\x30\x73\x16";
	char *pcr1 = "\x5B\x93\xBB\xA0\xA6\x64\xA7\x10\x52\x59"
		     "\x4A\x70\x95\xB2\x07\x75\x77\x03\x45\x0B";
	char *pcr4 = "\x80\x34\x8A\xB9\xE6\x53\x27\x9F\x5E\x62"
		     "\xB2\x1C\x2D\x6C\x0E\x9E\x88\xBE\xE1\x63";
	char *pcr9 = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		     "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

	/* size of slen */
	thingy.ssize = htons(2);
	/* shift by the pcr number (pcr 1 = 1 << 1, pcr 0 = 1 << 0, etc.) */
	thingy.uni.slen = 1 << 0;
	/* concatenated pcr length */
	thingy.pcrsize = htonl(20);
	memcpy(thingy.pcr, pcr0, 20);

	SHA1((char*)&thingy, sizeof(thingy), hash);

	for (i = 0; i < 20; i++)
		printf("%02x ", hash[i]);
	printf("\n");

	return 0;
}

