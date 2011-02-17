/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Configuration file functions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include "decls.h"
#include "configdefs.h"
#include "log.h"

static struct config *myconfig;

/*
 * Got a SIGHUP.
 * Reread the config file.
 */
static void
config_signal(int sig)
{
	void *savedconfig = NULL;

	if (myconfig == NULL)
		return;

	savedconfig = myconfig->config_save();
	if (myconfig->config_read() != 0) {
		warning("WARNING: could not load new configuration, "
			"restoring old config.");
		if (savedconfig == NULL ||
		    myconfig->config_restore(savedconfig)) {
			error("*** Could not restore old configuration; "
			      "aborting!");
			abort();
		}
	} else
		log("New configuration loaded.");
	if (savedconfig)
		myconfig->config_free(savedconfig);
}

int
config_init(char *style, int readit)
{
	int rv;

	if (strcmp(style, "emulab") == 0) {
#ifdef USE_EMULAB_CONFIG
		extern struct config *emulab_init();
		if (myconfig == NULL) {
			if ((myconfig = emulab_init()) != NULL)
				log("Using Emulab configuration");
		} else
			log("Emulab config init failed");
#else
		log("Not built with Emulab configuration");
#endif
	}
	if (strcmp(style, "null") == 0) {
#ifdef USE_NULL_CONFIG
		extern struct config *null_init();
		if (myconfig == NULL) {
			if ((myconfig = null_init()) != NULL)
				log("Using null configuration");
		} else
			log("Null config init failed");
#else
		log("Not built with Null configuration");
#endif
	}
	if (myconfig == NULL) {
		error("*** No configuration found");
		return -1;
	}

	if (readit) {
		rv = myconfig->config_read();
		if (rv) {
			myconfig->config_deinit();
			return rv;
		}
	}
	signal(SIGHUP, config_signal);
	return 0;
}

int
config_read(void)
{
	assert(myconfig != NULL);
	return myconfig->config_read();
}

/*
 * Return the set of images that IP address 'auth' can access for GET or PUT.
 *
 * If 'imageid' is not NULL, then we are requesting information for a specific
 * image, and the 'get' and 'put' lists will return at most one entry depending
 * on whether the node can GET/PUT that image.
 *
 * If 'reqip' is not equal to 'authip', then the former is requesting info
 * on behalf of the latter.  The requester must have "proxy" capability for
 * this to work.
 */
int
config_get_host_authinfo(struct in_addr *reqip,
			 struct in_addr *authip, char *imageid,
			 struct config_host_authinfo **get,
			 struct config_host_authinfo **put)
{
	assert(myconfig != NULL);
	return myconfig->config_get_host_authinfo(reqip, authip, imageid,
						  get, put);
}

void
config_dump_host_authinfo(struct config_host_authinfo *ai)
{
	char *none = "<NONE>";
	int i;

	if (ai == NULL)
		return;

	fprintf(stderr, "HOST authinfo %p:\n", ai);
	fprintf(stderr, "  hostid: %s\n", ai->hostid ? ai->hostid : none);
	if (ai->numimages > 0) {
		fprintf(stderr, "  %d image(s):\n", ai->numimages);
		for (i = 0; i < ai->numimages; i++)
			fprintf(stderr, "    [%d]: imageid='%s', path='%s'\n",
				i, ai->imageinfo[i].imageid,
				ai->imageinfo[i].path);
	}
	fprintf(stderr, "  extra: %p\n", ai->extra);
}

void
config_free_host_authinfo(struct config_host_authinfo *ai)
{
	assert(myconfig != NULL);
	return myconfig->config_free_host_authinfo(ai);
}

/*
 * Authenticate access to an image based on the IP address of a host.
 * Return zero and (optionally) a pointer to authentication information
 * on success, an error code otherwise.
 */
int
config_auth_by_IP(struct in_addr *reqip, struct in_addr *hostip, char *imageid,
		  struct config_host_authinfo **aip)
{
	struct config_host_authinfo *ai;

	if (config_get_host_authinfo(reqip, hostip, imageid, &ai, 0))
		return MS_ERROR_FAILED;
	if (ai->hostid == NULL) {
		config_free_host_authinfo(ai);
		return MS_ERROR_NOHOST;
	}
	if (ai->numimages == 0) {
		config_free_host_authinfo(ai);
		return MS_ERROR_NOACCESS;
	}
	if (aip)
		*aip = ai;
	return 0;
}

/*
 * If first is non-zero, then we need to return a "new" address and *addr
 * and *port are uninitialized.  If non-zero, then our last choice failed
 * (probably due to a port conflict) and we need to choose a new address
 * to try, possibly based on the existing info in *addr and *port.
 */
int
config_get_server_address(struct config_imageinfo *ii, int methods, int first,
			  in_addr_t *addr, in_port_t *port, int *method)
{
	assert(myconfig != NULL);
	return myconfig->config_get_server_address(ii, methods, first,
						   addr, port, method);
}

void
config_dump(FILE *fd)
{
	signal(SIGHUP, SIG_IGN);
	if (myconfig == NULL)
		warning("config_dump: config file not yet read.");
	else
		myconfig->config_dump(fd);
	signal(SIGHUP, config_signal);
}
