/*
 * Configuration file functions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
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
config_init(int readit)
{
	int rv;

#ifdef USE_EMULAB_CONFIG
	extern struct config emulab_config;
	myconfig = &emulab_config;
#else
	extern struct config file_config;
	myconfig = &file_config;
#endif
	rv = myconfig->config_init();
	if (rv)
		return rv;

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

int
config_get_host_authinfo(struct in_addr *in, char *imageid,
		     struct config_host_authinfo **get,
		     struct config_host_authinfo **put)
{
	assert(myconfig != NULL);
	return myconfig->config_get_host_authinfo(in, imageid, get, put);
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
config_auth_by_IP(struct in_addr *host, char *imageid,
		  struct config_host_authinfo **aip)
{
	struct config_host_authinfo *ai;

	if (config_get_host_authinfo(host, imageid, &ai, 0))
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

int
config_get_server_address(struct config_host_authinfo *ai, int methods,
			  in_addr_t *addr, in_port_t *port, int *method)
{
	assert(myconfig != NULL);
	return myconfig->config_get_server_address(ai, methods,
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
