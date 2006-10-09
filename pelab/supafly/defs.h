#ifndef __DEFS_H__
#define __DEFS_H__

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

/** sane systems honor this as 32 bits regardless */
/**
 * each sent packet group (allowing for app/ip frag) must prefix a new
 * unit with the length of crap being sent.  This might be useful...
 */
typedef uint32_t slen_t;

#define MIDDLEMAN_SEND_CLIENT_PORT 6878
#define MIDDLEMAN_RECV_CLIENT_PORT 6888

#define MIDDLEMAN_MAX_CLIENTS 32

#define MAX_NAME_LEN 255

#endif /* __DEFS_H__ */
