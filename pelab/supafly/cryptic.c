/**
 * This is a simple little program that measure time required to encrypt 
 * n m-KB blocks with triple des.
 * 
 * It does try to work around the L2 cache, but VERY half-heartedly -- we'd
 * need a different avoidance strategy for each of the major types.  We just
 * offer the option to reserve n KB (i.e., the size of the cache), and do
 * crypto ops in another block of memory.  Then, before we do any ops, we skim
 * over each block in the n KB pool to waste the cache (or at least try :-)).
 * Obviously, this can be easily dodged by some caches.
 *
 */

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <openssl/rand.h>
#include <openssl/des.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int block_size = 4096;
int block_count = 100;
int cache_size = 0;

char *optarg;
int optind,opterr,optopt;


void usage(char *bin) {
    fprintf(stdout,
	    "USAGE: %s -slc  (option defaults in parens)\n"
	    "\t-s blocksize  Blocksize (bytes) for single crypto op (4096)\n"
	    "\t-l num blocks Number of blocks to encrypt (100)\n"
	    "\t-c cache size L2 cache size in KB (0)\n",
	    bin
	    );
}

void parse_args(int argc,char **argv) {
    int c;
    char *ep = NULL;

    while ((c = getopt(argc,argv,"s:l:c:h")) != -1) {
	switch(c) {
	case 's':
	    block_size = (int)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    break;
	case 'l':
	    block_count = (int)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    break;
	case 'c':
	    cache_size = (int)strtol(optarg,&ep,10);
	    if (ep == optarg) {
		usage(argv[0]);
		exit(-1);
	    }
	    cache_size *= 1024;
	    break;
	case 'h':
	    usage(argv[0]);
	    exit(0);
	default:
	    break;
	}
    }
}

DES_cblock *genkey() {
    DES_cblock *retval;

    retval = (DES_cblock *)malloc(sizeof(DES_cblock));
    if (retval != NULL) {
	DES_random_key(retval);
    }

    return retval;
}

DES_cblock *geniv() {
    DES_cblock *retval;

    retval = (DES_cblock *)malloc(sizeof(DES_cblock));
    if (retval != NULL) {
	DES_random_key(retval);
    }

    return retval;
}

/* note: if IV is null, don't do cbc; otherwise, do it. */
int bencrypt(unsigned char *input,unsigned char *output,
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

int bdecrypt(unsigned char *input,unsigned char *output,
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

int main(int argc,char **argv) {
/*      uint8_t *cache; */
    DES_cblock *k1,*k2,*iv;
    struct timeval *etimesStart;
    struct timeval *etimesStop;
    char *sblock;
    char *eblock;
    int len;
    int i;
    int sd,ud;
    int bytesFilled;
    long sum;

    parse_args(argc,argv);

    if (block_size % 8 != 0) {
	fprintf(stderr,"Block size must be multiple of 8!\n");
	exit(-3);
    }

    //fprintf(stderr,"bs=%d,n=%d\n",block_size,block_count);
    //fflush(stdout);

    /* set up the cache avoidance... */
/*      cache = (uint8_t *)malloc(sizeof(uint8_t)*cache_size); */
/*      if (!cache) { */
/*  	fprintf(stderr, */
/*  		"Out of memory; exiting!"); */
/*  	exit(-2); */
/*      } */

    /* initialize ourself... */
    srand(time(NULL));

    etimesStart = (struct timeval *)malloc(sizeof(struct timeval)*block_count);
    etimesStop = (struct timeval *)malloc(sizeof(struct timeval)*block_count);
    if (etimesStart == NULL || etimesStop == NULL) {
	fprintf(stderr,
		"Out of memory; exiting!");
	exit(-2);
    }
    
    sblock = (char *)malloc(sizeof(char *)*block_size);
    eblock = (char *)malloc(sizeof(char *)*block_size);
    if (sblock == NULL || eblock == NULL) {
	fprintf(stderr,
		"Out of memory; exiting!");
	exit(-2);
    }
    
    k1 = genkey();
    k2 = genkey();

    /** 
     * fill up the block with rand crap; change a bit of it each time.
     */
    fprintf(stdout,"generating rand block... ");
    fflush(stdout);
    bytesFilled = 0;
    while (bytesFilled < block_size) {
	iv = geniv();
	len = block_size - bytesFilled;
	//printf("len = %d %d\n",len,sizeof(DES_cblock));
	len = (sizeof(DES_cblock) > len) ? len : sizeof(DES_cblock);
	memcpy(&sblock[bytesFilled],
	       iv,
	       len);
	free(iv);
	//fprintf(stdout,".");

	bytesFilled += len;
    }
    fprintf(stdout,"done\n");
    fflush(stdout);

    /* grab the real one... */
    iv = geniv();

    /**
     * run through a bunch of blocks, keeping note of the times
     * at the start of the encryption and at the end.
     */
    for (i = 0; i < block_count; ++i) {
	gettimeofday(&etimesStart[i],NULL);

	bencrypt(sblock,eblock,block_size,k1,k2,iv);

	gettimeofday(&etimesStop[i],NULL);
    }

    /**
     * dump times...
     */

    for (i = 0; i < block_count; ++i) {
	etimesStop[i].tv_sec = etimesStop[i].tv_sec - etimesStart[i].tv_sec;
	etimesStop[i].tv_usec = etimesStop[i].tv_usec - etimesStart[i].tv_usec;
	if (etimesStop[i].tv_usec < 0) {
	    etimesStop[i].tv_sec--;
	    etimesStop[i].tv_usec += 1000000;
	}
	fprintf(stdout,
		"%d %d\n",i,
		etimesStop[i].tv_sec * 1000 + etimesStop[i].tv_usec);
	fflush(stdout);
    }

    free(k1);
    free(k2);
    free(iv);
    free(sblock);
    free(eblock);
    
    exit(0);
}
