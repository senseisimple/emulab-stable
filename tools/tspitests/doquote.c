/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <err.h>

#include <tss/tspi.h>
#include <tss/platform.h>
#include <tss/tss_typedef.h>
#include <tss/tss_structs.h>

#define TSS_ERROR_CODE(x)       (x & 0xFFF)

#define NONCE_LEN 		20

#define PCR_SIZE 		20

#define PCOMP_LEN(x) 		(2 + 2 + 4 + (PCR_SIZE * x))
#define	PCOMP_PMASK_LEN		0
#define	PCOMP_PMASK		2
#define	PCOMP_PMASK_BITS	16
#define	PCOMP_PCR_LEN		4
#define	PCOMP_PCR_BLOB		8

#define ishex(x)        ((x >= '0' && x <= '9') || (x >= 'A' && x <= 'F') || \
                        (x >= 'a' && x <= 'f'))

#define TPM_PASS_HASH	"\x71\x10\xed\xa4\xd0\x9e\x06\x2a\xa5\xe4" \
			"\xa3\x90\xb0\xa5\x72\xac\x0d\x2c\x02\x20"

/*
 * Helpers like madness!
 */
void check(char *msg, int cin)
{
	int in = TSS_ERROR_CODE(cin);
	fprintf(stderr, "%s: ", msg);
	if(in == TSS_SUCCESS) {
		fprintf(stderr, "TSS_SUCCESS\n");
		return;
	} else if(in == TSS_E_INVALID_HANDLE)
		fprintf(stderr, "\t\tTSS_E_INVALID_HANDLE\n");
	else if(in == TSS_E_INTERNAL_ERROR)
		fprintf(stderr, "\t\tTSS_E_INTERNAL_ERROR\n");
	else if(in == TSS_E_BAD_PARAMETER)
		fprintf(stderr, "\t\tTSS_E_BAD_PARAMETER\n");
	else if(in == TSS_E_HASH_INVALID_LENGTH)
		fprintf(stderr, "\t\tTSS_E_HASH_INVALID_LENGTH\n");
	else if(in == TSS_E_HASH_NO_DATA)
		fprintf(stderr, "\t\tTSS_E_HASH_NO_DATA\n");
	else if(in == TSS_E_INVALID_SIGSCHEME)
		fprintf(stderr, "\t\tTSS_E_INVALID_SIGSCHEME\n");
	else if(in == TSS_E_HASH_NO_IDENTIFIER)
		fprintf(stderr, "\t\tTSS_E_HASH_NO_IDENTIFIER\n");
	else if(in == TSS_E_PS_KEY_NOTFOUND)
		fprintf(stderr, "\t\tTSS_E_PS_KEY_NOTFOUND\n");
	else if(in == TSS_E_BAD_PARAMETER)
		fprintf(stderr, "\t\tTSS_E_BAD_PARAMETER\n");
	else
		fprintf(stderr, "\t\tNot here: 0x%x\n", in);

	exit(1);
}

static unsigned char hextoi(char in)
{
	if (in >= '0' && in <= '9')
		return (in - '0');
	else if (in >= 'A' && in <= 'F')
		return (10 + in - 'A');
	else if (in >= 'a' && in <= 'f')
		return (10 + in - 'a');
}

static void printhex(const unsigned char in)
{
        unsigned char rh, lh, t;
        t = in >> 4;
        if (t < 10)
                rh = '0' + t;
        else
                rh = 'A' + (t - 10);
        t = in & 0x0f;
        if (t < 10)
                lh = '0' + t;
        else
                lh = 'A' + (t - 10);

        printf("%c%c", rh, lh);
}


static void bintoa(unsigned char in, char *dst)
{
	unsigned char rh, lh, t;

	t = in >> 4;
	if (t < 10)
		lh = '0' + t;
	else
		lh = 'A' + (t - 10);
	t = in & 0x0f;
	if (t < 10)
		rh = '0' + t;
	else
		rh = 'A' + (t - 10);

	dst[0] = lh; dst[1] = rh;
}

/*
 * Writes the binary buffer in as hexadecimal to out and returns the string
 * length of the buffer
 */
static int blobtoa(char *in, size_t inlen, char *out, size_t outlen)
{
	unsigned int i;
	int rc = 0;

	/* Size of binary input + NULL termination */
	if (inlen * 2 + 1 > outlen) {
		fprintf(stderr, "not enough room in out buffer\n");
		return -1;
	}

	for (i = 0; i < inlen; i++) {
		bintoa(in[i], out);
		out += 2;
		rc += 2;
	}

	*out = '\0';

	return rc;
}

/*
 * qstr is NULL terminated and contains the PCRS=.. part of the quoteprep info
 */
static int parse_pcrs(char *qstr, short *dst, int *pcrtot)
{
	char *rest;
	char *tag = "PCR=";
	char *sep = ",";
	int ptot = 0, pcr;

	if (!qstr || !dst) {
		fprintf(stderr, "NULL arg\n");
		return -1;
	}

	if (strncmp(qstr, tag, strlen(tag))) {
		fprintf(stderr, "Doesn't look like the PCR chunk of quoteprep to me!\n");
		return -1;
	}

	/* Point to PCRs */
	qstr += strlen(tag);
	rest = qstr;
	*dst = 0;

	while (qstr && *qstr != '\0') {
		int len;
		qstr = strsep(&rest, sep);
		len = strlen(qstr);

		if (len > 2) {
			fprintf(stderr, "Malformed PCR string\n");
			return -1;

		} else if (len == 2) {

			if (!isdigit(qstr[0]) || !isdigit(qstr[1])) {
				fprintf(stderr, "bad PCRs= string\n");
				return -1;
			}
			pcr = (int)hextoi(qstr[0]) * 10;
			pcr += (int)hextoi(qstr[1]);

		} else {
			if (!isdigit(*qstr)) {
				fprintf(stderr, "bad PCRs= string\n");
				return -1;
			}
			pcr = (int)hextoi(*qstr);
		}

		*dst |= (1 << pcr);
		ptot++;
		qstr = rest;
	}

	if (!ptot) {
		fprintf(stderr, "No PCRs requested or malformed request\n");
		return -1;
	}

	*pcrtot = ptot;

	return 0;
}

/*
 * Fills dst with the key and returns a pointer to the buffer.
 */
static int parse_key(char *kstr, char **dst, size_t *dstlen)
{
	char *tag = "IDENTITY=";
	char *p;

	if (!kstr || !dst || !dstlen) {
		fprintf(stderr, "NULL arg\n");
		return -1;
	}

	if (strncmp(kstr, tag, strlen(tag))) {
		fprintf(stderr, "Doesn't look like the IDENTITY chunk of"
		    " quoteprep to me!\n");
		return -1;
	}

	kstr += strlen(tag);
	if ((strlen(kstr) % 2) != 0) {
		fprintf(stderr, "Hex string for idkey isn't even;"
		    " missing digits?\n");
		return -1;
	}

	*dst = malloc(strlen(kstr) / 2);
	if (*dst == NULL) {
		fprintf(stderr, "Couldn't allocate memory for idkey of "
		    "size %d bytes\n",
		    strlen(kstr) / 2);
		return -1;
	}
	p = *dst;

	*dstlen = 0;
	while (*kstr != '\0') {
		if (!ishex(kstr[0]) || !ishex(kstr[1])) {
			fprintf(stderr, "Non-hex digits in idkey\n");
			free(*dst);
			return -1;
		}
		*p++ = (hextoi(kstr[0]) << 4) | (hextoi(kstr[1]));
		kstr += 2;
		*dstlen += 1;
	}

	return 0;
}

static int parse_nonce(char *nstr, char *dst)
{
	char *tag = "NONCE=";

	if (!nstr || !dst) {
		fprintf(stderr, "NULL arg\n");
		return -1;
	}

	if (strncmp(nstr, tag, strlen(tag))) {
		fprintf(stderr, "Doesn't look like the NONCE chunk of "
		    "quoteprep to me!\n");
		return -1;
	}

	nstr += strlen(tag);

	if (strlen(nstr) != NONCE_LEN * 2) {
		fprintf(stderr, "Invalid nonce len: %d\n", strlen(nstr));
		return -1;
	}

	while (*nstr != '\0') {
		if (!ishex(nstr[0]) || !ishex(nstr[1])) {
			fprintf(stderr, "Non-hex digits in nonce\n");
			return -1;
		}
		*dst++ = (hextoi(nstr[0]) << 4) | (hextoi(nstr[1]));
		nstr += 2;
	}

	return 0;
}

static void usage(char *name)
{
	fprintf(stderr, "%s - do a quote for Emulab\n", name);
	fprintf(stderr, "\n");
	fprintf(stderr, "%s takes the quoteprep info either as\n", name);
	fprintf(stderr, "arguments or it reads them from stdin and does a\n");
	fprintf(stderr, "quote with the given identity key including the\n");
	fprintf(stderr, "requested PCRs and nonce.  It then prints out all\n");
	fprintf(stderr, "the required info in ASCII (suitable for tmcc) in\n");
	fprintf(stderr, "order to do the securestate tmcd command.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "The newstate must be the first (or only) argument.\n");
	fprintf(stderr, "Example:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "%s RELOADSETUP PCR=... IDENTITY=... NONCE=...\n",
	    name);
	fprintf(stderr, "\n");
	fprintf(stderr, "or:\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "./tmcc -s boss quoteprep RELOADSETUP | %s "
	    "RELOADSETUP\n", name);

	exit(1);
}

int
main(int argc, char **argv)
{
	TSS_HCONTEXT hContext;
	TSS_HKEY hSRK, hKey;
	TSS_UUID srkUUID = TSS_UUID_SRK;
	TSS_HPOLICY srkpol;
	TSS_HPCRS	hpcomp;
	TSS_VALIDATION	valdata;
	TSS_HTPM	hTPM;
	BYTE *gpcr;

	int ret, i, pcrtot, pcrlen, fail = 0;
	size_t keylen;
	unsigned short pcrs;
	char buf[2048], newstate[128], nonce[20], a, b, c;
	char *ptag = "PCR=", *idtag = "IDENTITY=", *ntag = "NONCE=";
	char *key, *pcomp, *p;

	//if (argc != 5)
	if (argc < 2)
		usage(argv[0]);

	/* Get newstate */
	newstate[0] = '\0';
	strncat(newstate, argv[1], sizeof(newstate) - 1);

	a = b = c = 0;
	for (i = 2; i < argc; i++) {
		if (!strncmp(ptag, argv[i], strlen(ptag))) {
			parse_pcrs(argv[i], &pcrs, &pcrtot);
			a = 1;
		} else if (!strncmp(idtag, argv[i], strlen(idtag))) {
			parse_key(argv[i], &key, &keylen);
			b = 1;
		} else if (!strncmp(ntag, argv[i], strlen(ntag))) {
			parse_nonce(argv[i], nonce);
			c = 1;
		} else
			usage(argv[0]);
	}

	/* Read from standard input if they're not args */
	while (!(a && b && c) && (fscanf(stdin, "%2047s", buf) != EOF)) {
		if (!strncmp(ptag, buf, strlen(ptag))) {
			parse_pcrs(buf, &pcrs, &pcrtot);
			a = 1;
		} else if (!strncmp(idtag, buf, strlen(idtag))) {
			parse_key(buf, &key, &keylen);
			b = 1;
		} else if (!strncmp(ntag, buf, strlen(ntag))) {
			parse_nonce(buf, nonce);
			c = 1;
		} else
			usage(argv[0]);
	}
	if (!(a && b && c))
		usage(argv[0]);

	/* Start doing all the quote rubbish */

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

	ret = Tspi_Policy_SetSecret(srkpol, TSS_SECRET_MODE_SHA1, 20,
	    TPM_PASS_HASH);
	check("policy set secret", ret);

	ret = Tspi_Context_GetTpmObject(hContext, &hTPM);
	check("tpm get policy object", ret);

	ret = Tspi_Context_LoadKeyByBlob(hContext, hSRK, keylen, key, &hKey);
	check("load key by blob", ret);

	/* For Quote */
	ret = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_PCRS, 0,
	    &hpcomp);
	check("create pcrs object", ret);

	for (i = 0; i < PCOMP_PMASK_BITS; i++) {
		if (pcrs & (1 << i)) {
			fprintf(stderr, "Selecting PCR %d\n", i);
			ret = Tspi_PcrComposite_SelectPcrIndex(hpcomp, i);
			check("select pcr index", ret);
		}
	}

	/* Nonce */
	memset(&valdata, 0, sizeof(valdata));
	valdata.ulExternalDataLength = 20;
	valdata.rgbExternalData = nonce;

	ret = Tspi_TPM_Quote(hTPM, hKey, hpcomp, &valdata);
	check("quote", ret);

	/* Create the pcomp since rubbish Trousers doesn't give it to us in raw
	 * form like good ol' libtpm */
	pcomp = malloc(PCOMP_LEN(pcrtot));
	if (!pcomp) {
		fprintf(stderr, "Wow, oom.  We must be in gPXE again!\n");
		fail = 1;
		goto key;
	}

	*((short *)&pcomp[PCOMP_PMASK_LEN]) = htons(2);
	*((short *)&pcomp[PCOMP_PMASK]) = pcrs;
	*((uint32_t *)&pcomp[PCOMP_PCR_LEN]) = htonl(PCR_SIZE * pcrtot);
	p = &pcomp[PCOMP_PCR_BLOB];

	for (i = 0; i < PCOMP_PMASK_BITS; i++) {
		if (pcrs & (1 << i)) {
			ret = Tspi_PcrComposite_GetPcrValue(hpcomp, i, &pcrlen,
			    &gpcr);
			check("get pcr index", ret);
			if (pcrlen != 20) {
				fprintf(stderr,
				    "something fishy with getpcrvalue!\n");
				fail = 1;
				goto all;
			}
			memcpy(p, gpcr, 20);
			p += 20;
		}
	}

	/* Print out everything in ASCII */
	printf("%s", newstate);
	printf(" ");

	for (i = 0; i < valdata.ulValidationDataLength; i++)
		printhex(valdata.rgbValidationData[i]);
	printf(" ");
	for (i = 0; i < PCOMP_LEN(pcrtot); i++)
		printhex(pcomp[i]);
	printf("\n");

all:
	free(pcomp);
key:
	free(key);

trous:
	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_Close(hContext);

	if (fail)
		return 1;

	return 0;
}
