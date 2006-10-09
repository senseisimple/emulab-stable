#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#include <openssl/rand.h>
#include <openssl/des.h>

DES_cblock *sgenkey();
DES_cblock *sgeniv();
/* note: if IV is null, don't do cbc; otherwise, do it. */
int sencrypt(unsigned char *input,unsigned char *output,
	     unsigned int len,DES_cblock *k1,DES_cblock *k2,
	     DES_cblock *iv);
int sdecrypt(unsigned char *input,unsigned char *output,
	     unsigned int len,DES_cblock *k1,DES_cblock *k2,
	     DES_cblock *iv);


#endif /* __CRYPTO_H__ */
