/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2009-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * This is shared code between imagedump and imageunzip. It is used to
 * verify that the checksum is correct.
 */
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "imagehdr.h"
#include "checksum.h"

#ifdef WITH_CRYPTO
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>

#ifdef SIGN_CHECKSUM
static RSA *signature_key = NULL;

char *
checksum_keyfile(char *imagename)
{
	char *fname;

	if (strcmp(imagename, "-")) {
		fname = malloc(strlen(imagename) + 8);
		if (fname == NULL) {
			fprintf(stderr, "No memory\n");
			return NULL;
		}
		strcpy(fname, imagename);
		strcat(fname, ".ndk");
	} else {
		fname = strdup("stdin.ndk");
	}

	return fname;
}

/*
 * Initialize everything necessary to check the signature of the given image.
 * Returns non-zero if successful, 0 otherwise.
 */
int
init_checksum(char *keyfile)
{
	char str[1024];
	FILE *file;

	if (keyfile == NULL || (file = fopen(keyfile, "r")) == NULL) {
		fprintf(stderr, "%s: cannot open keyfile\n", keyfile);
		return 0;
	}
	if ((signature_key = RSA_new()) == NULL) {
		fprintf(stderr, "%s: cannot allocate RSA struct\n", keyfile);
		return 0;
	}

	fscanf(file, "%1024s", str);
	BN_hex2bn(&signature_key->n, str);
	fscanf(file, "%1024s", str);
	BN_hex2bn(&signature_key->e, str);
	fscanf(file, "%1024s", str);
	BN_hex2bn(&signature_key->dmp1, str);
	fscanf(file, "%1024s", str);
	BN_hex2bn(&signature_key->dmq1, str);
	fscanf(file, "%1024s", str);
	BN_hex2bn(&signature_key->iqmp, str);
	fclose(file);

	return 1; 
}

void
cleanup_checksum(void)
{
	RSA_free(signature_key);
	signature_key = NULL;
}

static void
decrypt_checksum(unsigned char *alleged_sum)
{
	unsigned char raw_sum[CSUM_MAX_LEN];

	memcpy(raw_sum, alleged_sum, CSUM_MAX_LEN);
	memset(alleged_sum, '\0', CSUM_MAX_LEN);
	RSA_public_decrypt(sizeof(raw_sum), raw_sum, alleged_sum,
			   signature_key, RSA_PKCS1_PADDING);
}
#endif

int
verify_checksum(blockhdr_t *blockhdr, const unsigned char *bodybufp, int type)
{
	SHA_CTX sum_context;
	int sum_length;
	unsigned char alleged_sum[CSUM_MAX_LEN];
	unsigned char calculated_sum[CSUM_MAX_LEN];

	if (type == CSUM_NONE) {
		fprintf(stderr, "Chunk has no checksum\n");
		return 0;
	}
#ifdef SIGN_CHECKSUM
	if ((type & CSUM_SIGNED) == 0) {
		fprintf(stderr, "Chunk checksum must be signed\n");
		return 0;
	}
#else
	if ((type & CSUM_SIGNED) != 0) {
		fprintf(stderr, "Chunk checksum must not be signed\n");
		return 0;
	}
#endif
	type &= CSUM_TYPE;
	if (type != CSUM_SHA1) {
		fprintf(stderr, "Chunk checksum type %d not supported\n",
			type);
		return 0;
	}

	/* initialize checksum state */
	memcpy(alleged_sum, blockhdr->checksum, CSUM_MAX_LEN);
	memset(blockhdr->checksum, '\0', CSUM_MAX_LEN);
	memset(calculated_sum, '\0', CSUM_MAX_LEN);
	SHA1_Init(&sum_context);

	/* calculate the checksum */
	SHA1_Update(&sum_context, bodybufp,
		    blockhdr->size + blockhdr->regionsize);

	/* save the checksum */
	SHA1_Final(calculated_sum, &sum_context);
	memcpy(blockhdr->checksum, alleged_sum, CSUM_MAX_LEN);

#ifdef SIGN_CHECKSUM
	decrypt_checksum(alleged_sum);
#endif

	/* XXX only SHA1 right now */
	sum_length = CSUM_SHA1_LEN;

	if (memcmp(alleged_sum, calculated_sum, sum_length) != 0) {
		char sumstr[CSUM_MAX_LEN*2 + 1];
		fprintf(stderr, "Checksums do not match:\n");
		mem_to_hexstr(sumstr, alleged_sum, sum_length);
		fprintf(stderr, "  Alleged:    0x%s\n", sumstr);
		mem_to_hexstr(sumstr, calculated_sum, sum_length);
		fprintf(stderr, "  Calculated: 0x%s\n", sumstr);
		return 0;
	}

	return 1;
}

/*
 * Common encryption stuff.
 * XXX doesn't really belong here.
 */

/*
 * Read the encryption key from a file.
 * Should be a single newline-terminated line of ascii hex chars.
 * Returns 1 if successful, 0 otherwise.
 */
int
encrypt_readkey(char *keyfile, unsigned char *keybuf, int buflen)
{
	char akeybuf[ENC_MAX_KEYLEN*2+1], *cp;
	FILE *fp;

	/* XXX */
	if (buflen > ENC_MAX_KEYLEN)
		buflen = ENC_MAX_KEYLEN;

	fp = fopen(keyfile, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s: cannot open encryption keyfile\n",
			keyfile);
		return 0;
	}
	fgets(akeybuf, sizeof(akeybuf), fp);
	fclose(fp);
	if ((cp = strrchr(akeybuf, '\n')))
		*cp = '\0';
	if (!hexstr_to_mem(keybuf, akeybuf, buflen)) {
		fprintf(stderr, "%s: cannot parse encryption key\n",
			keyfile);
		return 0;
	}

	return 1;
}
#endif

/*
 * Conversion functions
 */

/*
 * Read memsize bytes from dest and write the hexadecimal equivalent
 * into source. source must have 2*memsize + 1 bytes available.
 */
void
mem_to_hexstr(char *dest, const unsigned char *source, int memsize)
{
	int i;

	for (i = 0; i < memsize; i++)
		sprintf(&dest[2*i], "%02x", source[i]);
}

/*
 * Convert an ASCII hex char into a 0..15 value.
 */
static char
hex_to_char(char in)
{
	if (isdigit(in)) {
		return in - '0';
	} else if (islower(in)) {
		return in - 'a' + 10;
	} else {
		return in - 'A' + 10;
	}
}

/*
 * Read a string of hexadecimal digits and write out bytes to a
 * string. Returns 0 if there are any non-hex characters, or there are
 * less than 2*memsize characters in the source null-terminated string.
 */
int
hexstr_to_mem(unsigned char * dest, const char * source, int memsize)
{
	int result = 1;
	int i = 0;
	for (i = 0; i < memsize && result; i++) {
		if (isxdigit(source[2*i]) && isxdigit(source[2*i + 1])) {
			dest[i] = (hex_to_char(source[2*i]) << 4) |
				hex_to_char(source[2*i + 1]);
		} else {
			result = 0;
		}
	}
	return result;
}
