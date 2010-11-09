/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
	return;
}

int
main(void)
{
	TSS_HCONTEXT hContext;
	TSS_HKEY hKey, hSRK;
	TSS_HPOLICY hPolicy;
	TSS_UUID srkUUID = TSS_UUID_SRK;
	TSS_UUID myuuid = {1,1,1,1,1,{1,1,1,1,1,1}};
	TSS_HPOLICY srkpol;

	int ret,i, blobos;

	BYTE *blobo;

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

	ret = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY, 
		  //TSS_KEY_TYPE_STORAGE
		  TSS_KEY_TYPE_IDENTITY
		  //TSS_KEY_TYPE_SIGNING
		| TSS_KEY_SIZE_2048 | TSS_KEY_NO_AUTHORIZATION
		| TSS_KEY_NOT_MIGRATABLE
		, &hKey);
	check("create object - key", ret);
	ret = Tspi_Key_CreateKey(hKey, hSRK, 0);
	check("create key", ret);

	blobo = NULL;
	ret = Tspi_GetAttribData(hKey, TSS_TSPATTRIB_KEY_BLOB,
	    TSS_TSPATTRIB_KEYBLOB_PUBLIC_KEY, &blobos, &blobo);
	check("get blob", ret);

	if (!blobo)
		FATAL("mexican");

	printf("size: %d\n", blobos);
	for (i = 0;i < blobos; i++) {
		printf("\\x%x", blobo[i]);
	}
	printf("\n");
/*
	ret = Tspi_Context_RegisterKey(hContext, hKey, TSS_PS_TYPE_SYSTEM,
		myuuid, TSS_PS_TYPE_SYSTEM, srkUUID);
	check("register key", ret);
*/

	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_Close(hContext);

	return 0;
}

