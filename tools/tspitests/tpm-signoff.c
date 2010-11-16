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

#define TSS_ERROR_CODE(x)       (x & 0xFFF)

#define TPM_PASS_HASH	"\x71\x10\xed\xa4\xd0\x9e\x06\x2a\xa5\xe4" \
			"\xa3\x90\xb0\xa5\x72\xac\x0d\x2c\x02\x20"

TSS_HCONTEXT hContext;

void check(char *msg, int cin)
{
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

	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_Close(hContext);

	exit(1);
}

void usage(char *n)
{
	printf("%s - extend rubbish into a PCR\n", n);
	printf("\n");
	printf("%s pcr\n", n);
	printf("\n");
	printf("pcr - the PCR into which we will extend rubbish\n");
	printf("\n");
	printf("\n");

	exit(1);
}

int
main(int argc, char **argv)
{
	TSS_HKEY hSRK, hKey;
	TSS_UUID srkUUID = TSS_UUID_SRK;
	TSS_HPOLICY srkpol, hTPMPolicy;
	TSS_HTPM	hTPM;
	TSS_PCR_EVENT event;
	BYTE		*pcr;

	int ret, i, pcri;
	uint32_t size;
	char rub[20] = "BLAHBLAHBLAHBLAHBLAH";

	if (argc != 2)
		usage(argv[0]);

	pcri = atoi(argv[1]);
	if (pcri < 0 || pcri > 23)
		errx(1, "PCR out of range (0-16) (no extending to dynamic PCRS this way)\n");

	memset(&event, 0, sizeof(event));
	event.ulPcrIndex = pcri;

	/* create context and connect */
	ret = Tspi_Context_Create(&hContext);
	check("context create", ret);
	ret = Tspi_Context_Connect(hContext, NULL);
	check("context connect", ret);

	ret = Tspi_Context_GetTpmObject(hContext, &hTPM);
	check("get policy object", ret);

	ret = Tspi_TPM_PcrExtend(hTPM, pcri, 20, rub, &event, &size, &pcr);
	check("pcr extend", ret);

	printf("pcr %d is now: \n", pcri);
	for (i = 0; i < size; i++)
		printf("%x ", pcr[i]);
	printf("\n");

	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_Close(hContext);

	return 0;
}
