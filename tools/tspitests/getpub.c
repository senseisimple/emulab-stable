/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
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

	exit(1);
}

int
main(void)
{
	TSS_HCONTEXT hContext;
	TSS_HHASH hHash;
	TSS_HKEY hSigningKey, hSRK;
	TSS_HPOLICY hPolicy;
	TSS_UUID srkUUID = TSS_UUID_SRK;
	TSS_UUID myuuid = {1,1,1,1,1,{1,1,1,1,1,1}};
	TSS_HPOLICY srkpol;

	int ret,i;
	int plen = 0;
	BYTE *pub;

	int explen=0;
	BYTE *exp;


	/* create context and connect */
	ret = Tspi_Context_Create(&hContext);
	check("context create", ret);
	ret = Tspi_Context_Connect(hContext, NULL);
	check("context connect", ret);

	ret = Tspi_Context_LoadKeyByUUID(hContext, TSS_PS_TYPE_SYSTEM, srkUUID, &hSRK);
	check("loadkeybyuuid", ret);

	ret = Tspi_GetPolicyObject(hSRK, TSS_POLICY_USAGE, &srkpol);
	check("get policy object", ret);
	ret = Tspi_Policy_SetSecret(srkpol, TSS_SECRET_MODE_PLAIN, 4, "1234");
	check("policy set secret", ret);

	ret = Tspi_Context_LoadKeyByUUID(hContext, TSS_PS_TYPE_SYSTEM, myuuid,
		&hSigningKey);

	/*
	ret = Tspi_Key_GetPubKey(hSigningKey, &plen, &pub);
	check("getpubkey", ret);
	*/
	ret = Tspi_GetAttribData(hSigningKey, TSS_TSPATTRIB_RSAKEY_INFO,
		TSS_TSPATTRIB_KEYINFO_RSA_MODULUS, &plen, &pub);

	if(pub){
		printf("modulus len: %d\n", plen);
		for(i=0;i < plen;++i)
			printf("\\x%.02x", pub[i]);
		printf("\n");
	}
	ret = Tspi_GetAttribData(hSigningKey, TSS_TSPATTRIB_RSAKEY_INFO,
		TSS_TSPATTRIB_KEYINFO_RSA_EXPONENT, &explen, &exp);
	if(exp){
		printf("exp len: %d\n", explen);
		for(i=0;i < explen;++i)
			printf("\\x%.02x", exp[i]);
		printf("\n");
	}

	/* close context */
	ret = Tspi_Key_UnloadKey(hSigningKey);
	check("unload key", ret);

	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_Close(hContext);

	return 0;
}

