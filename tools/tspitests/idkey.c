/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <iconv.h>
#include <langinfo.h>
#include <sys/stat.h>
#include <tss/tspi.h>
#include <tss/platform.h>
#include <tss/tss_typedef.h>
#include <tss/tss_structs.h>

#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

#define TSS_ERROR_CODE(x)       (x & 0xFFF)

#define FATAL(x)	do{printf("**\t");printf(x);printf("\n"); return 1;}while(0);
#define print_error(x,y)	do{printf(x);}while(0);

/*
 * This code is hideous because it has never been loved properly.
 */

void check(char *msg, int cin)
{
	int in = TSS_ERROR_CODE(cin);
	printf("%s: ", msg);
	if(in == TSS_SUCCESS)
		printf("TSS_SUCCESS\n");
	else if(in == TSS_E_INVALID_HANDLE)
		printf("TSS_E_INVALID_HANDLE\n");
	else if(in == TSS_E_INTERNAL_ERROR)
		printf("TSS_E_INTERNAL_ERROR\n");
	else if(in == TSS_E_BAD_PARAMETER)
		printf("TSS_E_BAD_PARAMETER\n");
	else if(in == TSS_E_HASH_INVALID_LENGTH)
		printf("TSS_E_HASH_INVALID_LENGTH\n");
	else if(in == TSS_E_HASH_NO_DATA)
		printf("TSS_E_HASH_NO_DATA\n");
	else if(in == TSS_E_INVALID_SIGSCHEME)
		printf("TSS_E_INVALID_SIGSCHEME\n");
	else if(in == TSS_E_HASH_NO_IDENTIFIER)
		printf("TSS_E_HASH_NO_IDENTIFIER\n");
	else if(in == TSS_E_PS_KEY_NOTFOUND)
		printf("TSS_E_PS_KEY_NOTFOUND\n");
	else if(in == TSS_E_BAD_PARAMETER)
		printf("TSS_E_BAD_PARAMETER\n");
	else if(in == TSS_E_PS_KEY_NOTFOUND)
		printf("TSS_E_P_KEY_NOTFOUND\n");
	else
		printf("Not here: 0x%x\n", in);
}

void check_fail(char *msg, int cin)
{
	int in = TSS_ERROR_CODE(cin);
	printf("%s: ", msg);
	if(in == TSS_SUCCESS) {
		printf("TSS_SUCCESS\n");
		return;
	} else if(in == TSS_E_INVALID_HANDLE)
		printf("TSS_E_INVALID_HANDLE\n");
	else if(in == TSS_E_INTERNAL_ERROR)
		printf("TSS_E_INTERNAL_ERROR\n");
	else if(in == TSS_E_BAD_PARAMETER)
		printf("TSS_E_BAD_PARAMETER\n");
	else if(in == TSS_E_HASH_INVALID_LENGTH)
		printf("TSS_E_HASH_INVALID_LENGTH\n");
	else if(in == TSS_E_HASH_NO_DATA)
		printf("TSS_E_HASH_NO_DATA\n");
	else if(in == TSS_E_INVALID_SIGSCHEME)
		printf("TSS_E_INVALID_SIGSCHEME\n");
	else if(in == TSS_E_HASH_NO_IDENTIFIER)
		printf("TSS_E_HASH_NO_IDENTIFIER\n");
	else if(in == TSS_E_PS_KEY_NOTFOUND)
		printf("TSS_E_PS_KEY_NOTFOUND\n");
	else if(in == TSS_E_BAD_PARAMETER)
		printf("TSS_E_BAD_PARAMETER\n");
	else if(in == TSS_E_PS_KEY_NOTFOUND)
		printf("TSS_E_P_KEY_NOTFOUND\n");
	else
		printf("Not here: 0x%x\n", in);
	exit(EXIT_FAILURE);
}

int blob_pubkey(char *out, unsigned int olen, char *in, unsigned int ilen,
    char *p, unsigned int psize)
{
	unsigned int offset, size;

	if (olen < ilen) {
		printf("\t\tblob_pubkey failed: olen < ilen\n");
		return;
	}
	if (psize == 0) {
		printf("\t\tblob_pubkey failed: psize == 0\n");
		return;
	}
	memcpy(out, in, ilen);

	/* Keep all the keyparms the same, overwrite the public key at end.
	 * Check sizes to make sure we get the right offset.  They are
	 * big-endian. */

	offset = 4 + 2 + 2;
	/* Key parms length */
	size = Decode_UINT32(in + offset);
	offset += 4;
	offset += size;

	/* Key length */
	UINT32ToArray(psize, out + offset);
	offset += 4;
	/* out + offset should now be pointing to the public key */
	memcpy(out + offset, p, psize);
	offset += psize;

	return offset;
}

TSS_RESULT
make_fake_key(TSS_HCONTEXT hContext, TSS_HKEY *hCAKey, RSA **rsa, int padding)
{
	TSS_RESULT result;
	UINT32 encScheme, size_n, pub_size;
	BYTE n[2048];
	TCPA_PUBKEY pubkey;
	UINT32 blob_size, size;
	BYTE *blob, pub_blob[1024];

	switch (padding) {
		case RSA_PKCS1_PADDING:
			encScheme = TSS_ES_RSAESPKCSV15;
			break;
		case RSA_PKCS1_OAEP_PADDING:
			encScheme = TSS_ES_RSAESOAEP_SHA1_MGF1;
			break;
		case RSA_NO_PADDING:
			encScheme = TSS_ES_NONE;
			break;
		default:
			return TSS_E_INTERNAL_ERROR;
			break;
	}

		//Create CA Key Object
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
	    TSS_KEY_TYPE_LEGACY|TSS_KEY_SIZE_2048, hCAKey);
	if (result != TSS_SUCCESS) {
		check("Tspi_Context_CreateObject", result);
		return result;
	}

		// generate a software key to represent the CA's key
	if ((*rsa = RSA_generate_key(2048, 65537, NULL, NULL)) == NULL) {
		ERR_print_errors_fp(stdout);
		return 254; // ?
	}

		// get the pub CA key
	if ((size_n = BN_bn2bin((*rsa)->n, n)) <= 0) {
		fprintf(stderr, "BN_bn2bin failed\n");
		ERR_print_errors_fp(stdout);
		RSA_free(*rsa);
                return 254; // ?
        }

	result = Tspi_GetAttribData(*hCAKey, TSS_TSPATTRIB_KEY_BLOB,
	    TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY, &blob_size, &blob);
	if (result != TSS_SUCCESS) {
		check("Tspi_GetAttribData", result);
		return result;
	}

	pub_size = blob_pubkey(pub_blob, 1024, blob, blob_size, n, size_n);

	result = Tspi_SetAttribData(*hCAKey, TSS_TSPATTRIB_KEY_BLOB,
	    TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY, pub_size, pub_blob);
	if (result != TSS_SUCCESS) {
		check("Tspi_SetAttribData", result);
		return result;
	}

		// set the CA key's algorithm
	result = Tspi_SetAttribUint32(*hCAKey, TSS_TSPATTRIB_KEY_INFO,
				      TSS_TSPATTRIB_KEYINFO_ALGORITHM,
				      TSS_ALG_RSA);
	if (result != TSS_SUCCESS) {
		check("Tspi_SetAttribUint32", result);
		RSA_free(*rsa);
		return result;
	}

		// set the CA key's number of primes
	result = Tspi_SetAttribUint32(*hCAKey, TSS_TSPATTRIB_RSAKEY_INFO,
				      TSS_TSPATTRIB_KEYINFO_RSA_PRIMES,
				      2);
	if (result != TSS_SUCCESS) {
		check("Tspi_SetAttribUint32", result);
		RSA_free(*rsa);
		return result;
	}

		// set the CA key's encryption scheme
	result = Tspi_SetAttribUint32(*hCAKey, TSS_TSPATTRIB_KEY_INFO,
				      TSS_TSPATTRIB_KEYINFO_ENCSCHEME,
				      encScheme);
	if (result != TSS_SUCCESS) {
		check("Tspi_SetAttribUint32", result);
		RSA_free(*rsa);
		return result;
	}

	return TSS_SUCCESS;
}

void usage(char *name)
{
	printf("\n");
	printf("%s -t <tpmpass> -s <srkpass>\n", name);
	printf("\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	RSA			*rsa = NULL;
	TSS_HCONTEXT 		hContext;
	TSS_HKEY 		hKey, hSRK, hCAKey;
	TSS_HPOLICY 		hTPMPolicy, hidpol;
	TSS_UUID 		srkUUID = TSS_UUID_SRK;
	TSS_HPOLICY		srkpol;
	TSS_HTPM 		hTPM;
	UINT32			idbloblen, ch;
	int			ret,i, blobos, fd;
	BYTE			*srkpass, *tpmpass;
	BYTE			*blobo, *idblob;

	srkpass = tpmpass = NULL;
	while ((ch = getopt(argc, argv, "hs:t:")) != -1) {
		switch (ch) {
			case 's':
				srkpass = optarg;
				break;
			case 't':
				tpmpass = optarg;
				break;
			case 'h':
			default:
				usage(argv[0]);
				break;
		}
	}

	if (!srkpass || !tpmpass)
		usage(argv[0]);

	/* create context and connect */
	ret = Tspi_Context_Create(&hContext);
	check_fail("context create", ret);
	ret = Tspi_Context_Connect(hContext, NULL);
	check_fail("context connect", ret);

	ret = Tspi_Context_LoadKeyByUUID(hContext, TSS_PS_TYPE_SYSTEM, srkUUID,
	    &hSRK);
	check_fail("loadkeybyuuid", ret);

	ret = Tspi_GetPolicyObject(hSRK, TSS_POLICY_USAGE, &srkpol);
	check_fail("get policy object", ret);
	//ret = Tspi_Policy_SetSecret(srkpol, TSS_SECRET_MODE_PLAIN, 4, "1234");
	ret = Tspi_Policy_SetSecret(srkpol, TSS_SECRET_MODE_PLAIN,
	    strlen(srkpass), srkpass);
	check_fail("policy set secret", ret);

	ret = Tspi_Context_GetTpmObject(hContext, &hTPM);
	check_fail("get policy object", ret);

	//Insert the owner auth into the TPM's policy
	ret = Tspi_GetPolicyObject(hTPM, TSS_POLICY_USAGE, &hTPMPolicy);
	check_fail("get tpm policy", ret);

	ret = Tspi_Policy_SetSecret(hTPMPolicy, TSS_SECRET_MODE_PLAIN,
		strlen(tpmpass), tpmpass);
	check_fail("set owner secret", ret);

	ret = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY, 
		  //TSS_KEY_TYPE_STORAGE
		  TSS_KEY_TYPE_IDENTITY
		  //TSS_KEY_TYPE_SIGNING
		| TSS_KEY_SIZE_2048 | TSS_KEY_NO_AUTHORIZATION
		| TSS_KEY_NOT_MIGRATABLE | TSS_KEY_VOLATILE
		, &hKey);
	check_fail("create object - key", ret);

	ret = Tspi_GetPolicyObject(hKey, TSS_POLICY_USAGE, &hidpol);
	check_fail("get id key policy", ret);

	ret = Tspi_Policy_SetSecret(hidpol, TSS_SECRET_MODE_PLAIN,
	    strlen(srkpass), srkpass);
	check_fail("set idkey secret", ret);

	/* We must create this fake privacy CA key in software so that
	 * Tspi_TPM_CollateIdentityRequest will happily work.  It needs it to
	 * create the cert request which is required in a normal remote
	 * attestion procedure.  It is not needed in our setup though.
	 */
	ret = make_fake_key(hContext, &hCAKey, &rsa, RSA_PKCS1_OAEP_PADDING);
	check_fail("ca nonsense", ret);

	/* We do not care about idblob - that is the certificate request that
	 * we are supposed to send to our CA in normal remote attestation.  The
	 * fifth argument is our identity label (it is supposed to be unicode).
	 */
	ret = Tspi_TPM_CollateIdentityRequest(hTPM, hSRK, hCAKey, 8, "id label",
	    hKey, TSS_ALG_3DES, &idbloblen, &idblob);
	check_fail("collate id", ret);

	blobo = NULL;
	/*ret = Tspi_GetAttribData(hKey, TSS_TSPATTRIB_KEY_BLOB,
	    TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY, &blobos, &blobo);*/
	ret = Tspi_GetAttribData(hKey, TSS_TSPATTRIB_KEY_BLOB,
	    TSS_TSPATTRIB_KEYBLOB_BLOB, &blobos, &blobo);
	check_fail("get blob", ret);

	if (!blobo) {
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		FATAL("no blobo");
	}

	printf("size: %d\n", blobos);
	for (i = 0;i < blobos; i++) {
		printf("\\x%x", blobo[i]);
	}
	printf("\n");

	fd = open("key.blob", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		FATAL("Open\n");
	}
	ret = write(fd, blobo, blobos);
	if (ret != blobos)
		printf("Warning: couldn't write the whole key\n");
	close(fd);

	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_Close(hContext);

	return 0;
}
