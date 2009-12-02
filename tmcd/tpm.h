#ifndef		_ETPM_
#define		_ETPM_

#include<openssl/engine.h>

extern EVP_PKEY		*tpmk;

int tmcd_tpm_loadengine();
int tmcd_tpm_getkey(char *);
int tmcd_tpm_free(void);

#endif		
