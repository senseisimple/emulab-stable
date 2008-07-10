// A wrapper for common checksum code between imageunzip and imagedump

#ifndef CHECKSUM_H_IMAGEZIP_0
#define CHECKSUM_H_IMAGEZIP_0

void
init_checksum(void);

void
cleanup_checksum(void);

// Returns 1 if checksums matched, 0 if they failed to match.
// If the checksums fail to match, an error message is also printed.
int
verify_checksum(blockhdr_t *blockhdr, const char *bodybufp);

void
mem_to_hex(unsigned char * dest, const unsigned char * source,
           int memsize);

#endif
