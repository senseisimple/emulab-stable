/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002, 2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

#define SERVERPORT	855
#define LOGGERPORT	858
#define DEVPATH		"/dev"
#define TIPPATH		"/dev/tip"
#ifdef HPBSD
#define LOGPATH		"/usr/adm/tiplogs"
#else
#define LOGPATH		"/var/log/tiplogs"
#endif
/* Socket based tip/capture uses an ACL file to hold key below. */
#define ACLPATH		LOGPATH

/*
 * The key is transferred from capture to capserver in ascii text.
 */
typedef struct {
	int		keylen;		/* of the key string */
	char		key[256];	/* and the string itself. */
} secretkey_t;
#define DEFAULTKEYLEN	32

/*
 * This is for the cap logger handshake, which passes additional stuff.
 */
typedef struct {
    secretkey_t		secretkey;
    char		node_id[128];
    int			offset;
} logger_t;

/*
 * The capserver then returns this structure as part of the handshake.
 */
typedef struct {
	uid_t		uid;
	gid_t		gid;
} tipowner_t;

/*
 * The remote capture sends this back when it starts up
 */
typedef struct {
	char		name[64];	/* "tipname" in tiplines table */
	int		portnum;
	secretkey_t	key;
} whoami_t;

/*
 * Return Status. Define a constant size return to ensure that the
 * status is read as an independent block, distinct from any output
 * that might be sent. An int is a reasonable thing to use.
 *
 * XXX: If you change this, be sure to change the PERL code!
 */
#define CAPOK		0
#define CAPBUSY		1
#define CAPNOPERM	2
#define CAPERROR        3
typedef int		capret_t;
