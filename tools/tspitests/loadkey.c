/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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
		printf("\t\tTSS_E_INVALID_HANDLE\n");
	else if(in == TSS_E_INTERNAL_ERROR)
		printf("\t\tTSS_E_INTERNAL_ERROR\n");
	else if(in == TSS_E_BAD_PARAMETER)
		printf("\t\tTSS_E_BAD_PARAMETER\n");
	else if(in == TSS_E_HASH_INVALID_LENGTH)
		printf("\t\tTSS_E_HASH_INVALID_LENGTH\n");
	else if(in == TSS_E_HASH_NO_DATA)
		printf("\t\tTSS_E_HASH_NO_DATA\n");
	else if(in == TSS_E_INVALID_SIGSCHEME)
		printf("\t\tTSS_E_INVALID_SIGSCHEME\n");
	else if(in == TSS_E_HASH_NO_IDENTIFIER)
		printf("\t\tTSS_E_HASH_NO_IDENTIFIER\n");
	else if(in == TSS_E_PS_KEY_NOTFOUND)
		printf("\t\tTSS_E_PS_KEY_NOTFOUND\n");
	else if(in == TSS_E_BAD_PARAMETER)
		printf("\t\tTSS_E_BAD_PARAMETER\n");
	else
		printf("\t\tNot here: 0x%x\n", in);

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
	printf("valdata len: %d\n ", valdata->ulValidationDataLength);
	for (i = 0; i < valdata->ulValidationDataLength; i++)
		printf("%02x ", valdata->rgbValidationData[i]);
	printf("\n");
}

void pquote2(TSS_VALIDATION *valdata)
{
	int i;

	if (!valdata)
		return;

	TPM_QUOTE_INFO2 *qinfo2 = (TPM_QUOTE_INFO2*)valdata->rgbData;
	
	if (valdata->ulDataLength < sizeof(TPM_QUOTE_INFO2)) {
		printf("**too small to fit in TPM_QUOTE_INFO2 struct\n");
		printf("\tlen: %d TPM_QUOTE_INFO2: %d\n", valdata->ulDataLength,
		    sizeof(TPM_QUOTE_INFO2));
	}
	printf("tag: %d\n", qinfo2->tag);
	printf("fixed: ");
	for (i = 0; i < 4; i++)
		printf("%c", qinfo2->fixed[i]);
	printf("\n");
	printf("nonce: ");
	for (i = 0; i < 20; i++)
		printf("%02x ", qinfo2->externalData.nonce[i]);
	printf("\n");
	printf("size of selection: %d\n",
	    ntohs(qinfo2->infoShort.pcrSelection.sizeOfSelect));
	printf("pcr select: %p\n", qinfo2->infoShort.pcrSelection.pcrSelect);
	printf("locality: 0x%x\n", qinfo2->infoShort.localityAtRelease);
	printf("digest at release: ");
	for (i = 0; i < 20; i++)
		printf("%02x ", qinfo2->infoShort.digestAtRelease.digest[i]);
	printf("\n");

	printf("the whole blob: \n");
	for (i = 0; i < valdata->ulDataLength; i++) {
		if (i % 20 == 0)
			printf("\n");
		printf("\\x%02x", valdata->rgbData[i]);
	}
	printf("\n");
	printf("quote2 size: %d returned size: %d\n", sizeof(TPM_QUOTE_INFO2),
	    valdata->ulDataLength);
	printf("info short %d\n", sizeof(TPM_PCR_INFO_SHORT));
}

void usage(char *name)
{
	printf("\n");
	printf("%s -t <tpmpass> -s <srkpass> [-f <keyfile>]\n", name);
	printf("\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	int ret, i, fd, j;
	TSS_HCONTEXT hContext;
	TSS_HKEY hSRK, hKey;
	TSS_UUID srkUUID = TSS_UUID_SRK;
	TSS_HPOLICY srkpol, hTPMPolicy;
	TSS_HPCRS	pcrs;
	TSS_HTPM	hTPM;
	UINT32		bloblen;
	BYTE		*srkpub, blob[1024];
	TSS_HPCRS	hpcomp;
	TSS_VALIDATION	valdata;
	TSS_HHASH	hHash;
	TPM_QUOTE_INFO *qinfo;
	TPM_QUOTE_INFO2 *qinfo2;
	UINT32		rub1, ch;
	BYTE		*rub2;
	BYTE		*srkpass, *tpmpass, *keyfile;

	RSA	*rsa;

	srkpass = tpmpass = keyfile = NULL;
	while ((ch = getopt(argc, argv, "f:hs:t:")) != -1) {
		switch (ch) {
			case 's':
				srkpass = optarg;
				break;
			case 't':
				tpmpass = optarg;
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

	if (!srkpass || !tpmpass)
		usage(argv[0]);

	if (!keyfile)
		keyfile = "key.blob";
	fd = open(keyfile, O_RDONLY, 0);
	if (fd < 0)
		errx(1, "Couldn't open %s\n", keyfile);

	printf("opened; fd %d\n", fd);
	bloblen = read(fd, blob, 1024);
	if (bloblen == -1) {
		perror(NULL);
		errx(1, "error reading %s\n", keyfile);
	}

	printf("read %d bytes from %s\n", bloblen, keyfile);

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
	ret = Tspi_Policy_SetSecret(srkpol, TSS_SECRET_MODE_PLAIN,
	    strlen(srkpass), srkpass);
	check("policy set secret", ret);

	/* owner TPM password */
	ret = Tspi_Context_GetTpmObject(hContext, &hTPM);
	check("get policy object", ret);

	/* Insert the owner auth into the TPM's policy */
	/*ret = Tspi_GetPolicyObject(hTPM, TSS_POLICY_USAGE, &hTPMPolicy);
	check("get tpm policy", ret);

	ret = Tspi_Policy_SetSecret(hTPMPolicy, TSS_SECRET_MODE_PLAIN,
		3, "123");
	check("set owner secret", ret);*/

	ret = Tspi_Context_LoadKeyByBlob(hContext, hSRK, bloblen, blob, &hKey);
	check("load key by blob", ret);

	/* For Quote */
	ret = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_PCRS, 0,
	    &hpcomp);
	check("create pcrs object", ret);
	ret = Tspi_PcrComposite_SelectPcrIndex(hpcomp, 0);
	check("select pcr index", ret);
	/* For Quote2 */
	/*ret = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_PCRS,
	    TSS_PCRS_STRUCT_INFO_SHORT, &hpcomp);
	check("create pcrs object", ret);
	ret = Tspi_PcrComposite_SelectPcrIndexEx(hpcomp, 9,
	    TSS_PCRS_DIRECTION_RELEASE);
	check("select pcr index ex", ret);*/

	/* nonce */
	memset(&valdata, 0, sizeof(valdata));
	valdata.ulExternalDataLength = 20;
	valdata.rgbExternalData = "\x80\x0\x0\x0\x0\x0\x0\x0"
	    "\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x0\x1";

	ret = Tspi_TPM_Quote(hTPM, hKey, hpcomp, &valdata);
	check("quote", ret);
	/*ret = Tspi_TPM_Quote2(hTPM, hKey, TRUE, hpcomp, &valdata, &rub1, &rub2);
	check("quote2", ret);*/

	/* Verify hash */
	/*ret = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_HASH,
	    TSS_HASH_SHA1, &hHash);
	check("create hash", ret);
	ret = Tspi_Hash_UpdateHashValue(hHash, valdata.ulDataLength,
	    valdata.rgbData);
	check("update hash", ret);
	ret = Tspi_Hash_VerifySignature(hHash, hKey,
	    valdata.ulValidationDataLength, valdata.rgbValidationData);
	check("verify hash", ret);*/

	/* Print out Quote info */
	pquote(&valdata);

	/* Print out Quote2 info */
	//pquote2(&valdata);

	/* This nonsense doesn't work */
	/*printf("*****\n");
	rub1 = 0;
	ret = Tspi_PcrComposite_GetPcrValue(hpcomp, 1, &rub1, &rub2);
	check("get pcr value", ret);
	printf("len: %d: ", rub1);
	for (j = 0; j < rub1; j++)
		printf("%02x ", rub2[j]);
	printf("\n");*/

	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_Close(hContext);

	return 0;
}

