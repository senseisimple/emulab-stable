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

#define BOSSNODE	"boss.emulab.net"
#define SERVERPORT	855
#define DEVPATH		"/dev"
#define TIPPATH		"/dev/tip"

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
