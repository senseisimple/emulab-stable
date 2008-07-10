// This is shared code between imagedump and imageunzip. It is used to
// verify that the checksum is correct.

#include <openssl/evp.h>
#include <openssl/sha.h>

#include "imagehdr.h"
#include "checksum.h"

static RSA * signature_key = NULL;

static void
input_public_key(RSA * key)
{
  int MAX_LEN = 1024;
  FILE * file = fopen("public.sig", "r");
  char str[MAX_LEN];
  fscanf(file, "%1024s", str);
  BN_hex2bn(& key->n, str);
  fscanf(file, "%1024s", str);
  BN_hex2bn(& key->e, str);
  fscanf(file, "%1024s", str);
  BN_hex2bn(& key->dmp1, str);
  fscanf(file, "%1024s", str);
  BN_hex2bn(& key->dmq1, str);
  fscanf(file, "%1024s", str);
  BN_hex2bn(& key->iqmp, str);
  fclose(file);
}

void
init_checksum(void)
{
  signature_key = RSA_new();
  input_public_key(signature_key);
}

void
cleanup_checksum(void)
{
  RSA_free(signature_key);
}

static void
decrypt_checksum(unsigned char * alleged_sum)
{
  unsigned char raw_sum[CHECKSUM_LEN_MAX];
  memcpy(raw_sum, alleged_sum, CHECKSUM_LEN_MAX);
  memset(alleged_sum, '\0', CHECKSUM_LEN_MAX);
  RSA_public_decrypt(CHECKSUM_LEN_MAX, raw_sum, alleged_sum, signature_key,
                     RSA_PKCS1_PADDING);
}

int
verify_checksum(blockhdr_t *blockhdr, const char *bodybufp)
{
    SHA_CTX       sum_context;
    unsigned char alleged_sum[CHECKSUM_LEN_MAX];
    unsigned char calculated_sum[CHECKSUM_LEN_MAX];

    /* initialize checksum state */
    memcpy(alleged_sum, blockhdr->checksum, CHECKSUM_LEN_MAX);
    memset(blockhdr->checksum, '\0', CHECKSUM_LEN_MAX);
    memset(calculated_sum, '\0', CHECKSUM_LEN_MAX);
    SHA1_Init(&sum_context);

    /* calculate the checksum */
    SHA1_Update(&sum_context, bodybufp, blockhdr->size + blockhdr->regionsize);

    /* save the checksum */
    SHA1_Final(calculated_sum, &sum_context);
    memcpy(blockhdr->checksum, alleged_sum, CHECKSUM_LEN_MAX);

#ifdef SIGN_CHECKSUM
    decrypt_checksum(alleged_sum);
#endif

    if (memcmp(alleged_sum, calculated_sum, CHECKSUM_LEN_SHA1) != 0)
    {
        char sumstr[CHECKSUM_LEN_MAX*2 + 1];
        fprintf(stderr, "Checksums do not match:\n");
        mem_to_hex(sumstr, alleged_sum, CHECKSUM_LEN_MAX);
        fprintf(stderr, "  Alleged:    0x%s\n", sumstr);
        mem_to_hex(sumstr, calculated_sum, CHECKSUM_LEN_MAX);
        fprintf(stderr, "  Calculated: 0x%s\n", sumstr);
        return 0;
    }
    else
    {
      return 1;
    }
}

/*
 * Read memsize bytes from dest and write the hexadecimal equivalent
 * into source. source must have 2*memsize + 1 bytes available.
 */
void mem_to_hex(unsigned char * dest, const unsigned char * source,
                int memsize)
{
    int i = 0;
    for (i = 0; i < memsize; i++)
    {
        sprintf(&dest[2*i], "%02x", source[i]);
    }
}
