/* 
 * File:	decls.h
 * Description: 
 * Author:	Leigh Stoller
 * 		Computer Science Dept.
 * 		University of Utah
 * Date:	2-Aug-2001
 *
 * (c) Copyright 1992, 2000, University of Utah, all rights reserved.
 */

#define SERVERPORT	855
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
 * The key is transferred in ascii text.
 */
typedef struct {
	int		keylen;		/* of the key string */
	char		key[256];	/* and the string itself. */
} secretkey_t;
#define DEFAULTKEYLEN	32

/*
 * The remote capture sends this back when it starts up
 */
typedef struct {
	char		nodeid[64];
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
typedef int		capret_t;
