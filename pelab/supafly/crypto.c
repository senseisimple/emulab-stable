#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "crypto.h"

DES_cblock *sgenkey() {
    DES_cblock *retval;

    retval = (DES_cblock *)malloc(sizeof(DES_cblock));
    if (retval != NULL) {
	DES_random_key(retval);
    }

    return retval;
}

DES_cblock *sgeniv() {
    DES_cblock *retval;

    retval = (DES_cblock *)malloc(sizeof(DES_cblock));
    if (retval != NULL) {
	DES_random_key(retval);
    }

    return retval;
}

/* note: if IV is null, don't do cbc; otherwise, do it. */
int sencrypt(unsigned char *input,unsigned char *output,
	     unsigned int len,DES_cblock *k1,DES_cblock *k2,
	     DES_cblock *iv
	     ) {
    DES_key_schedule s1,s2;
    int i;
    
    if (len%8 != 0) {
	return -1;
    }

    /* the idea is to process the keys each time to keep them out of memory
     * if possible (besides, it's easier)
     */

    DES_set_key_checked(k1,&s1);
    DES_set_key_checked(k2,&s2);
    
    if (iv == NULL) {
	/* actually encrypt, now */
	//printf("%d\n",len/8);
	for (i = 0; i < len; i+=8) {
	    //printf("encrypt block %d\n",i);
	    DES_ecb2_encrypt(input+i,output+i,&s1,&s2,DES_ENCRYPT);
	}
    }
    else {
	DES_ede2_cbc_encrypt(input,output,len,&s1,&s2,iv,DES_ENCRYPT);
    }

    return 0;

}

int sdecrypt(unsigned char *input,unsigned char *output,
	     unsigned int len,DES_cblock *k1,DES_cblock *k2,
	     DES_cblock *iv
	     ) {
    DES_key_schedule s1,s2;
    int i;

    if (len%8 != 0) {
	return -1;
    }

    /* the idea is to process the keys each time to keep them out of memory
     * if possible (besides, it's easier)
     */

    DES_set_key_checked(k1,&s1);
    DES_set_key_checked(k2,&s2);
    //printf("blah2\n");
    if (iv == NULL) {
	//printf("blah3\n");
	/* actually encrypt, now */
	for (i = 0; i < len; i+=8) {
	    //printf("decrypt block %d\n",i);
	    DES_ecb2_encrypt(input+i,output+i,&s1,&s2,DES_DECRYPT);
	}
    }
    else {
	//printf("blah4\n");
	DES_ede2_cbc_encrypt(input,output,len,&s1,&s2,iv,DES_DECRYPT);
    }

    return 0;
}
