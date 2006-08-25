/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "popenf.h"

#define TMCC "/usr/local/etc/emulab/tmcc"

/*
 * Silly little helper function to run tmcc. 
 */
int
tmcc(char *fmt, char *ret, int retlen, ...)
{
	char		buf[2*BUFSIZ], *bp = ret;
	va_list		ap;
	FILE		*fp;
	int		retval = 0;
	int		count  = 0;

	/* Need to tack on the path to tmcc */
	if (strlen(TMCC) + strlen(fmt) + 1 > sizeof(buf)) {
		errno = ENOMEM;
		return -1;
	}
	strcpy(buf, TMCC);
	strcat(buf, " ");
	strcat(buf, fmt);

	va_start(ap, retlen);
	fp = vpopenf(buf, "r", ap);
	va_end(ap);

	if (fp == NULL) {
		/* errno set by vpopenf */
		return -1;
	}

	while (1) {
		int rc = fread(buf, 1, sizeof(buf), fp);
		if (rc <= 0) {
			if (rc < 0 || ferror(fp)) {
				retval = -1;
				break;
			}
			break;
		}
		if (retlen < rc) {
			/* Not enough room in buffer */
			errno  = ENOMEM;
			retval = -1;
			break;
		}
		memcpy(bp, buf, rc);
		bp     += rc;
		retlen -= rc;
		count  += rc;
	}
	if (!retval)
		retval = count;
	
	if (pclose(fp)) {
		errno  = EIO;
		retval = -1;
	}
	return retval;
}

/*
 * Port lookup and registration
 */
int
PortLookup(char *service, char *hostname, int namelen, int *port)
{
	char	buf[BUFSIZ], fmt[32];
	int	rc;
	
	if (tmcc("-i portregister %s", buf, sizeof(buf), service) <= 0)
		return -1;

	sprintf(fmt, "PORT=%%d NODEID=%%%ds", namelen);
	rc = sscanf(buf, fmt, port, hostname);
	if (rc != 2)
		return -1;

	return 0;
}

int
PortRegister(char *service, int port)
{
	char	buf[BUFSIZ];
	
	return tmcc("-i portregister %s %d", buf, sizeof(buf), service, port);
}
