/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2009-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef STANDALONE
#include "log.h"
#include "config.h"
#else
#define error		printf
#endif /* STANDALONE */

#include <sys/time.h>
#include <unistd.h>
#include <strings.h>

static unsigned int nonce_counter = 0;

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

#ifdef TPM
static ENGINE *tpme;
/*
 * hash is the SHA1 hash of our TPM storage root passsword (not the owner
 * password!)
 */
static char *tpmhash = "\x71\x10\xed\xa4\xd0\x9e\x06\x2a\xa5\xe4\xa3\x90\xb0"
			"\xa5\x72\xac\x0d\x2c\x02\x20";
#endif

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

int
tmcd_tpm_generate_nonce(unsigned char *nonce)
{
    struct timeval time;
    pid_t pid;

    int byte_count = 0;
    bzero(nonce,TPM_NONCE_BYTES);
    
    /* Nonce must be 160 bits long, and we must be quite sure that we will
     * never use the same one twice.  We put three things into the nonce to
     * make it unique:
     * 1) Timestamp to the best accuracy we can get
     * 2) The PID of the current process, to avoid someone asking two 
     *    different tmcds for nonces at thte same time
     * 3) A local counter, in case someone can ask for nonces faster than our
     *    clock resolution
     */

    // timestamp
    if (gettimeofday(&time,NULL)) {
        return -1;
    }

    if (sizeof(time) + byte_count > TPM_NONCE_BYTES) {
        return -1;
    }
    bcopy(&time, nonce, sizeof(time));
    byte_count += sizeof(time);

    // pid
    pid = getpid();
    if (sizeof(pid) + byte_count > TPM_NONCE_BYTES) {
        return -1;
    }
    bcopy(&pid, nonce + byte_count, sizeof(pid));
    byte_count += sizeof(pid);

    // counter
    if (sizeof(nonce_counter) + byte_count > TPM_NONCE_BYTES) {
        return -1;
    }
    bcopy(&nonce_counter, nonce + byte_count, sizeof(nonce_counter));
    byte_count += sizeof(nonce_counter);

    // TODO: Maybe hash to avoid giving away info on state on boss?

    return 0;

}

/*
 * Verify that a quote is what we expect it to be.  We need SHA1 hashing to do
 * this so we require SSL.
 *
 * Returns 1 if the quote is valid, 0 otherwise.
 */
int tmcd_tpm_verify_quote(char *quote, ssize_t quotelen, char *pcomp,
    ssize_t pcomplen, TPM_NONCE nonce, unsigned short wantpcrs,
    TPM_PCR *pcrs
    )
{
#ifdef	WITHSSL
	struct signed_pcomp sp;

	unsigned short pcrmlen;
	/* XXX: The pcr mask is supposedly variable length but really 2 bytes
	 * in practice */
	unsigned short pcrm;
	uint32_t pcrlen;
	int i, c, pcrtot = 0;
	unsigned char *idkey;
	unsigned char hash[20];

	if (!quote) {
		error("NULL quote to %s\n", __FUNCTION__);
		return 0;
	}
	if (!pcomp) {
		error("NULL pcomp to %s\n", __FUNCTION__);
		return 0;
	}
	if (!nonce) {
		error("NULL nonce to %s\n", __FUNCTION__);
		return 0;
	}

	pcrmlen = ntohs(pcomp[PCOMP_PCRMASK_LEN]);
	pcrlen = ntohl(pcomp[PCOMP_PCRBLOB_LEN]);
	/* Some sanity - 28 bytes is the smallest quote size possible on our
	 * TPMs.
	 * XXX: We do not deal with variable length pcr masks yet (it is
	 * probably useless unless you want to quote the dynamic pcrs). */
	i = pcrmlen + pcrlen + sizeof(short) + sizeof(uint32_t);
	if (pcrmlen != 2 || i != pcomplen || pcomplen < 28) {
		error("Corrupt quote blob; unexpected quote size\n");
		error("pcr mask len: %d, pcomplen: %d, calculated len: %d\n",
		    pcrmlen, pcomplen, i);
		return 0;
	}
	pcrm = pcomp[PCOMP_PCRMASK];
	for (i = 0; i < PCOMP_PCRMASK_BITS; i++)
		if (pcrm & (1 << i))
			pcrtot++;

	if (pcrlen != pcrtot * PCOMP_PCR_LEN) {
		error("Corrupt quote blob; pcrlen %d, should be: %d\n", pcrlen,
		    pcrtot * PCOMP_PCR_LEN);
		return 0;
	}

	/* Check that it includes the PCRs we want */
	// TODO: Get wantpcr from some state
	//wantpcrs = ?
	if ((pcrm & wantpcrs) != wantpcrs) {
		error("Missing required PCRs; wantpcr: %x pcrmask: %x\n",
		    wantpcrs, pcrm);
		return 0;
	}

	/* Make sure that the PCRs are what we expect them to be.  Dig up
	 * required PCRs */
	for (i = 0, c = 0; i < PCOMP_PCRMASK_BITS; i++) {
		if (pcrm & (1 << i)) {
			// TODO: Get required PCR values from state
			//pcr = ?
			if (memcmp(&pcomp[PCOMP_PCRBLOB + PCOMP_PCR_LEN * c],
			    pcrs[c], PCOMP_PCR_LEN)) {
				error("PCR %d doesn't match\n", i);
				return 0;
                        }
                        c++;
		}
	}

	/* SHA1 pcomp and stuff it into struct _signed_comp */
	sp.fixed[0] = 1; sp.fixed[1] = 1;
	sp.fixed[2] = 0; sp.fixed[3] = 0;
	sp.fixed[4] = 'Q'; sp.fixed[5] = 'U';
	sp.fixed[6] = 'O'; sp.fixed[7] = 'T';

	if (tmcd_quote_hash(&pcomp, pcomplen, &sp.comphash)) {
		error("Error hashing pcr composite\n");
		return 0;
	}

	memcpy(&sp.nonce, nonce, TPM_NONCE_BYTES);

	/* SHA1 _signed_comp */
	if (tmcd_quote_hash(&sp, sizeof(sp), hash)) {
		error("Error hashing signed pcomp\n");
		return 0;
	}

	/* Verify that quote is indeed a signature of the SHA1 of
	 * _signed_comp */
	// TODO: We also need it's identity key to verify the signature
	// idkey = ?
	if (!tmcd_quote_verifysig(hash, quote, quotelen, idkey)) {
		error("Signature check failed\n");
		return 0;
	}

	/* They survived the gauntlet! */
	return 1;

#else
	error("Can't verify quotes without SSL\n");
	return 0;
#endif
}
