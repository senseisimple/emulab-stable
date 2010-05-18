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
typedef unsigned char TPM_NONCE[TPM_NONCE_BYTES];
int tmcd_tpm_generate_nonce(unsigned char*);

#endif		
