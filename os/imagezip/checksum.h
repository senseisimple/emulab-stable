// A wrapper for common checksum code between imageunzip and imagedump

#ifndef CHECKSUM_H_IMAGEZIP_0
#define CHECKSUM_H_IMAGEZIP_0

char *
checksum_keyfile(char *imagename);

int
init_checksum(char *keyfile);

void
cleanup_checksum(void);

// Returns 1 if checksums matched, 0 if they failed to match.
// If the checksums fail to match, an error message is also printed.
int
verify_checksum(blockhdr_t *blockhdr, const unsigned char *bodybufp, int type);

int
encrypt_readkey(char *keyfile, unsigned char *keybuf, int buflen);

void
mem_to_hexstr(char *dest, const unsigned char *source, int memsize);

int
hexstr_to_mem(unsigned char *dest, const char *source, int memsize);

#endif
