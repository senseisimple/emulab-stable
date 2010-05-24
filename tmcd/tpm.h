/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2009-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef		_ETPM_
#define		_ETPM_

#include <sys/types.h>
#include <openssl/engine.h>

extern EVP_PKEY		*tpmk;

int tmcd_tpm_loadengine();
int tmcd_tpm_getkey(char *);
int tmcd_tpm_free(void);

/*
 * Nonce-related functions
 */
#define TPM_NONCE_BYTES 0x14 // 160 bits
#define TPM_PCR_BYTES 0x14 // 160 bits
typedef unsigned char TPM_NONCE[TPM_NONCE_BYTES];
typedef unsigned char TPM_PCR[TPM_PCR_BYTES];
int tmcd_tpm_generate_nonce(unsigned char*);

/*
 * Quote verifying stuff
 */
struct signed_pcomp {
	unsigned char fixed[8];
	/* Hash of PCR composite */
	unsigned char comphash[20];
	TPM_NONCE nonce;
};

struct pubkeydata {
	uint32_t algorithm;
	uint16_t encscheme;
	uint16_t sigscheme;
	uint32_t keybitlen;
	uint32_t numprimes;
	uint32_t expsize;
	unsigned char exponent[3];
	uint32_t keylength;
	unsigned char modulus[256];
	uint32_t pcrinfolen;
	unsigned char pcrinfo[256];
};

struct keydata {
	unsigned char version[4];
	uint16_t keyusage;
	uint32_t keyflags;
	unsigned char authdatausage;
	struct pubkeydata pub;
	uint32_t privkeylen;
	unsigned char encprivkey[1024];
};


/* Help for navigating around a PCR composite - they are variable length
 * depending on how many PCRs you request in the quote */
#define	PCOMP_PCRMASK_LEN	0
#define	PCOMP_PCRMASK		2
#define	PCOMP_PCRBLOB_LEN	4
#define	PCOMP_PCRBLOB		8

#define	PCOMP_PCR_LEN		20
#define	PCOMP_PCRMASK_BITS	16

int tmcd_tpm_verify_quote(char *, ssize_t, char *, ssize_t, TPM_NONCE,
        unsigned short, TPM_PCR*, unsigned char*);
int tpm_extract_key(unsigned char *, struct keydata *);

#endif		
