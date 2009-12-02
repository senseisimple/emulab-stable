#ifndef STANDALONE
#include "log.h"
#include "config.h"
#else
#define error		printf
#endif /* STANDALONE */

/*
 * XXX: Teach me how to do this properly . . .  This is so boss can build ssl.o
 * 	properly without installing trousers headers
 */
#ifdef TPMOVERRIDE
#undef TPM
#endif /* TPMOVERRIDE */

#ifdef TPM
#include <tss/tss_defines.h>
#endif	/* TPM */

#include "tpm.h"

EVP_PKEY *tpmk;

static ENGINE *tpme;
/*
 * hash is the SHA1 hash of our TPM storage root passsword (not the owner
 * password!)
 */
static char *tpmhash = "\x71\x10\xed\xa4\xd0\x9e\x06\x2a\xa5\xe4\xa3\x90\xb0"
			"\xa5\x72\xac\x0d\x2c\x02\x20";

int
tmcd_tpm_loadengine(void)
{
#ifdef	TPM
	ENGINE_load_builtin_engines();
	tpme = ENGINE_by_id("tpm");
	if (!tpme){
		error("ENGINE_by_id\n");
		return 1;
	}

	if (!ENGINE_init(tpme)){
		error("ENGINE_init\n");
		//ENGINE_free(tpme);	/* this segfaults?? */
		return 1;
	}
	if (!ENGINE_set_default_RSA(tpme) || !ENGINE_set_default_RAND(tpme)){
		error("ENGINE_set_default\n");
		ENGINE_free(tpme);
		ENGINE_finish(tpme);
		return 1;
	}

	/* Don't need this anymore */
	ENGINE_free(tpme);

	/* Set auth */
        if (!ENGINE_ctrl_cmd(tpme, "SECRET_MODE",
	    (long)TSS_SECRET_MODE_SHA1, NULL, NULL, 0)){
		error("set SECRET_MODE hash\n");
		ENGINE_finish(tpme);
		return 1;
	}
        if (!ENGINE_ctrl_cmd(tpme, "PIN", 0, tpmhash, NULL, 0)){
		error("set SECRET_MODE hash\n");
		ENGINE_finish(tpme);
		return 1;
	}

	return 0;
#else
	error("Oops!  Want TPM but we're compiled without TPM support\n");
	return 1;
#endif /* TPM */
}

int
tmcd_tpm_getkey(char *kf)
{
#ifdef	TPM
	if (!tpme){
		error("invalid tpm engine reference\n");
		return 1;
	}
	if (!kf){
		error("invalid keyfile pointer\n");
		return 1;
	}
	/*
	 * XXX: This call to ENGINE_load_private_key apparently can segfault if
	 * you pass in a keyfile that it doesn't like. . .  need to do
	 * something about that . . .
	 */
	if ((tpmk = ENGINE_load_private_key(tpme, kf, NULL, NULL)) == NULL){
		error("error loading keyfile: %s\n", kf);
		return 1;
	}

	return 0;
#else
	error("Oops!  Want TPM but we're compiled without TPM support\n");
	return 1;
#endif /* TPM */
}

int
tmcd_tpm_free(void)
{
#ifdef TPM
	ENGINE_finish(tpme);
	tpme = NULL;
	return 0;
#else
	error("Oops!  Want TPM but we're compiled without TPM support\n");
	return 1;
#endif /* TPM */
}
