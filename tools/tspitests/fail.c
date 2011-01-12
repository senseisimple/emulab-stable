/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <sys/stat.h>
#include <tss/tspi.h>
#include <tss/platform.h>
#include <tss/tss_typedef.h>
#include <tss/tss_structs.h>

#define TSS_ERROR_CODE(x)       (x & 0xFFF)

#define FATAL(x)	do{printf("**\t");printf(x);printf("\n");}while(0);
//#define FATAL(x)	do{printf(x);printf("\n");exit(1);}while(0);

void check(char *msg, int cin){
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
	else
		printf("Not here: 0x%x\n", in);

	//exit(1);
}

void pquote(TSS_VALIDATION *valdata)
{
	int i;
	TPM_QUOTE_INFO *qinfo = (TPM_QUOTE_INFO*)valdata->rgbData;
	if (valdata->ulDataLength < sizeof(TPM_QUOTE_INFO))
		printf("**too small to fit in TPM_QUOTE_INFO struct\n");
	printf("version: maj: %d min: %d\n", qinfo->version.major,
	    qinfo->version.minor);
	printf("fixed: ");
	for (i = 0; i < 4; i++)
		printf("%c", qinfo->fixed[i]);
	printf("\n");
	printf("SHA1 hash: ");
	for (i = 0; i < 20; i++)
		printf("%02x ", qinfo->compositeHash.digest[i]);
	printf("\n");
	printf("nonce: ");
	for (i = 0; i < 20; i++)
		printf("%02x ", qinfo->externalData.nonce[i]);
	printf("\n");
}

int
main(void)
{
	int ret, i, fd;
	TSS_HCONTEXT hContext;
	TSS_HKEY hSRK, hKey;
	TSS_HPOLICY hPolicy;
	TSS_UUID srkUUID = TSS_UUID_SRK;
	TSS_HPOLICY srkpol, hTPMPolicy;
	TSS_HENCDATA	hencdata;
	TSS_HPCRS	pcrs;
	TSS_HTPM	hTPM;
	UINT32		srklen, bloblen;
	BYTE		*srkpub, blob[1024];
	BYTE		wellknown[20] = TSS_WELL_KNOWN_SECRET;
	TSS_HPCRS	hpcomp;
	TSS_VALIDATION	valdata;
	TSS_HHASH hHash;
	UINT32 siglen;
	BYTE *sig;

	RSA	*rsa;

	fd = open("key.blob", O_RDONLY, S_IRUSR | S_IWUSR);
	if (fd < 0)
		errx(1, "Couldn't open key.blob\n");

	printf("opened; fd %d\n", fd);
	bloblen = read(fd, blob, 1024);
	if (bloblen == -1) {
		perror(NULL);
		errx(1, "error reading key.blob\n");
	}

	printf("read %d bytes from key.blob\n", bloblen);

	/* create context and connect */
	ret = Tspi_Context_Create(&hContext);
	check("context create", ret);
	ret = Tspi_Context_Connect(hContext, NULL);
	check("context connect", ret);

	ret = Tspi_Context_LoadKeyByUUID(hContext, TSS_PS_TYPE_SYSTEM,
	    srkUUID, &hSRK);
	check("loadkeybyuuid", ret);

	/* srk password */
	ret = Tspi_GetPolicyObject(hSRK, TSS_POLICY_USAGE, &srkpol);
	check("get policy object", ret);
	ret = Tspi_Policy_SetSecret(srkpol, TSS_SECRET_MODE_SHA1, 20, wellknown);
	check("policy set secret", ret);

	/* owner TPM password */
	ret = Tspi_Context_GetTpmObject(hContext, &hTPM);
	check("get policy object", ret);

	//Insert the owner auth into the TPM's policy
	ret = Tspi_GetPolicyObject(hTPM, TSS_POLICY_USAGE, &hTPMPolicy);
	check("get tpm policy", ret);

	ret = Tspi_Policy_SetSecret(hTPMPolicy, TSS_SECRET_MODE_PLAIN,
		3, "123");
	check("set owner secret", ret);

	ret = Tspi_Context_LoadKeyByBlob(hContext, hSRK, bloblen, blob, &hKey);
	check("load key by blob", ret);

	ret = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_PCRS, 0,
	    &hpcomp);
	check("create pcrs object", ret);
	ret = Tspi_PcrComposite_SelectPcrIndex(hpcomp, 1);
	check("select pcr index", ret);

	/* nonce */
	valdata.ulExternalDataLength = 20;
	valdata.rgbExternalData = "12345678901234567890";

	ret = Tspi_TPM_Quote(hTPM, hKey, hpcomp, &valdata);
	check("quote", ret);
	pquote(&valdata);

	/* try to sign arbitrary data instead */
	ret = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_HASH,
					   TSS_HASH_SHA1, &hHash);
	check("create hash object", ret);

	ret = Tspi_Hash_SetHashValue(hHash, 20, "Je pense, danc je sa");
	check("set has value", ret);

	/* Try to sign some hash with our identity key instead of a signing key
	 * (we shouldn't be able to do this)
	 */
	ret = Tspi_Hash_Sign(hHash, hKey, &siglen, &sig);
	check("hash sign", ret);

	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_Close(hContext);

	return 0;
}

