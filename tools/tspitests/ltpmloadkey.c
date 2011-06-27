/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <err.h>
#include <fcntl.h>
#include <sys/types.h>

#include <openssl/objects.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

#include "libtpm/tpmkeys.h"

/* pcomp is the buffer that we must fill in with our PCR hash/info.
 * The hash of this buffer (pcomp) is called the composite hash.  After
 * we have the composite hash, we stick the composite hash in the next
 * buffer (signedhash), fill in the nonce field, and then hash
 * signedhash.  This hash is what the TPM gives to us but it is signed
 * with the requested identity key.
 *
 * So if we decrypt the blob that the TPM gives us with the identity
 * key's public key and it the resulting hash matches our hash with the
 * expected PCR(s) and nonce, then the PCRs are indeed what we think
 * they are.
 */
struct {
	/* big endian */
	unsigned short slen;
	/* the length of this field is slen - it is 2 on our tpms.
	 * This field is a bitmask of which PCRS have been included.
	 * It is little endian. */
	unsigned short s;
	/* big endian */
	uint32_t plen;
	/* p will also be plen bytes long.  I only request one PCR so
	 * it will be 20 bytes. */
	unsigned char p[20];
} pcomp;

struct {
	unsigned char fixed[8];
	/* Hash of pcomp */
	unsigned char comphash[20];
	unsigned char nonce[20];
} signedhash;

void usage(char *name)
{
	printf("\n");
	printf("%s -s <srkpass> [-f <keyfile>]\n", name);
	printf("\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int fd, size, i, ret;
	uint32_t kh, pcrs;

	unsigned char buf[1024], hash[20], pass[20];
	char *srkpass, *keyfile, ch;

	/* SHA1 hash of TPM's SRK password */
	char *tpmhash = "\x71\x10\xed\xa4\xd0\x9e\x06\x2a\xa5\xe4\xa3"
			"\x90\xb0\xa5\x72\xac\x0d\x2c\x02\x20";
	char *nonce =	"\x80\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0"
			"\x0\x0\x0\x0\x1";
	keydata k;
	RSA *rpub;

	srkpass = keyfile = NULL;
	while ((ch = getopt(argc, argv, "hs:f:")) != -1) {
		switch (ch) {
			case 's':
				srkpass = optarg;
				break;
			case 'f':
				keyfile = optarg;
				break;
			case 'h':
			default:
				usage(argv[0]);
				break;
		}
	}

	if (!srkpass)
		usage(argv[0]);
	if (!keyfile)
		keyfile = "key.blob";

	SHA1(srkpass, strlen(srkpass), pass);

	fd = open(keyfile, O_RDONLY);
	if (fd == -1)
		errx(1, "couldn't open %s\n", keyfile);

	size = read(fd, buf, 1024);
	if (size == -1)
		errx(1, "couldn't read\n");

	size = TSS_KeyExtract(buf, &k);
	printf("keybuf size: %d\n", size);

	close(fd);

	printf("loading . . .\n");
	/* 0x40000000 is the UID for the SRK */
	if (ret = TPM_LoadKey(0x40000000, pass, &k, &kh)) {
		printf("%s\n", TPM_GetErrMsg(ret));
		errx(1, "TPM_LoadKey\n");
	}

	/* Quote PCR 0 */
	printf("quoting . . .\n");
	if (ret = TPM_Quote(kh, (0x00000001 << 0), pass, nonce, &pcomp, buf,
	    &size)) {
		printf("%s\n", TPM_GetErrMsg(ret));
		errx(1, "TPM_Quote\n");
	}

	/* TPM will run out of memory if you forget to evict keys.  This can be
	 * fixed with a good ol' reboot.*/
	printf("evicting. . .\n");
	if (ret = TPM_EvictKey(kh)) {
		printf("%s\n", TPM_GetErrMsg(ret));
		errx(1, "TPM_EvictKey\n");
	}

	/* Compute composite hash */
	SHA1((char*)&pcomp, sizeof(pcomp), hash);

	printf("slen: %d\n", ntohs(pcomp.slen));
	printf("select: 0x%x\n", pcomp.s);
	printf("plen %d\n", ntohl(pcomp.plen));
	printf("pcr hash: ");
	for (i = 0; i < 20; i++)
		printf("%02x ", pcomp.p[i]);
	printf("\n");
	printf("composite hash: ");
	for (i = 0; i < 20; i++)
		printf("%02x ", hash[i]);
	printf("\n");
	printf("signed blob len: %d\n", size);
	printf("signed blob: ");
	for (i = 0; i < size; i++)
		printf("%02x ", buf[i]);
	printf("\n");

	/* See if the signed object matches the composite hash concatenated
	 * with the nonce */
	signedhash.fixed[0] = 1; signedhash.fixed[1] = 1;
	signedhash.fixed[2] = 0; signedhash.fixed[3] = 0;
	signedhash.fixed[4] = 'Q'; signedhash.fixed[5] = 'U';
	signedhash.fixed[6] = 'O'; signedhash.fixed[7] = 'T';
	memcpy(&signedhash.comphash, hash, 20);
	memcpy(&signedhash.nonce, nonce, 20);

	SHA1((char*)&signedhash, sizeof(signedhash), hash);
	/* Gives us an RSA public key from the TPM key */
	rpub = TSS_convpubkey(&k.pub);
	if (!rpub)
		errx(1, "TSS_convpubkey\n");

	if (!RSA_verify(NID_sha1, hash, 20, buf, size, rpub))
		printf("SIGNATURE FAILED\n");
	else
		printf("Signature is correct\n");

	return 0;
}
