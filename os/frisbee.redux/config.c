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
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
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

	log("Reading new configuration.");
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
config_auth_by_IP(int isget, struct in_addr *reqip, struct in_addr *hostip,
		  char *imageid, struct config_host_authinfo **aip)
{
	struct config_host_authinfo *ai;

	if (config_get_host_authinfo(reqip, hostip, imageid,
				     isget ? &ai : 0, isget ? 0 : &ai))
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

/*
 * Utility functions for all configurations.
 */

/*
 * Return 'dir' if 'path' is in 'dir'; i.e., is 'dir' a prefix of 'path'?
 * Return NULL if 'path' is not in 'dir'.
 * Note: returns NULL if path == dir.
 * Note: assumes realpath has been run on both.
 */
char *
isindir(char *dir, char *path)
{
	int len = dir ? strlen(dir) : 0;
	char *rv = dir;

	if (dir == NULL || path == NULL || *dir == '\0' || *path == '\0' ||
	    strncmp(dir, path, len) || path[len] != '/')
		rv = NULL;
#if 0
	fprintf(stderr, "isindir(dir=%s, path=%s) => %s\n",
		dir ? dir : "", path ? path : "", rv ? rv : "NULL");
#endif
	return rv;
}

/*
 * Account for differences between BSD and Linux realpath.
 *
 * BSD realpath() apparently doesn't even test the final component.
 * It will return success even if the final component doesn't exist.
 * Settle on the Linux behavior, which does return an error if the
 * final component does not exist.
 *
 * BSD realpath() is documented to, on error, return the canonicalized
 * version of the path up to and including the component that caused the
 * error. While this behavior is not documented in the Linux version,
 * it is the observed behavior, so we assume it (making only a feeble
 * check to verify).
 *
 * Returns a pointer to the 'rpath' buffer if 'path' fully resolves,
 * and 'rpath' is filled with the complete canonicalized path.
 *
 * Returns NULL if there was an error resolving any component of 'path'
 * with errno set to the cause of the failure (e.g., ENOENT if a component
 * is missing) and 'rpath' is filled with the canonicalized path up to and
 * including the component that caused the error.
 */
char *
myrealpath(char *path, char rpath[PATH_MAX])
{
	char *rv;

	assert(path[0] != '\0');

	rpath[0] = '\0';
	rv = realpath(path, rpath);
	assert(rpath[0] != '\0');
#ifndef linux
	if (rv != NULL) {
		struct stat sb;
		/* also sets errno correctly */
		if (stat(path, &sb) < 0)
			rv = NULL;
	}
#endif
	return rv;
}

#define INDIR(d, dl, p) ((p)[dl] == '/' && strncmp(p, d, dl) == 0)

/*
 * "Resolve" a pathname.
 *
 * Makes sure that 'path' falls within the 'dir' after application of
 * realpath to resolve "..", symlinks, etc. If 'create' is set, it will
 * create missing intermediate directories. In detail:
 *
 * The path (and dir) must be absolute, or it is an error.
 * If 'create' is zero, then all components of the path must exist
 *   or it is an error.
 * If 'create' is non-zero, then missing components except the last
 *   will be created.
 * If 'dir' is non-null, the existing part of the path must resolve
 *   within this directory or it is an error.
 * If 'dir' is null, then the path is resolved as far as it exists
 *  ('create' is ignored and assumed to be zero).
 *   
 * Returns NULL on an error, a malloc'ed pointer to a canonicalized
 * path otherwise.
 */
char *
resolvepath(char *path, char *dir, int create)
{
	char rpath[PATH_MAX], *next, *ep, *pathcopy;
	struct stat sb;
	int pathlen = strlen(path);
	int dirlen = 0;
	int rpathlen;
	mode_t omask, cmask;
	char *npath;

	if (debug > 1)
		info("resolvepath '%s' in dir '%s'", path, dir);
	/* validate path */
	if (path == NULL) {
		if (debug > 1)
			info(" null path");
		return NULL;
	}
	if (path[0] != '/') {
		if (debug > 1)
			info(" path is not absolute");
		return NULL;
	}
	if (pathlen >= sizeof rpath) {
		if (debug > 1)
			info(" path is too long");
		return NULL;
	}

	/* validate dir */
	if (dir) {
		dirlen = strlen(dir);
		if (dir[0] != '/') {
			if (debug > 1)
				info(" path and dir (%s) must be absolute",
				     dir);
			return NULL;
		}
		/* XXX make dir=='/' work */
		if (dirlen == 1)
			dirlen = 0;
	} else {
		create = 0;
	}

	/*
	 * If the full path resolves, make sure it falls in dir (if given)
	 * and is a regular file.
	 */
	if (myrealpath(path, rpath) != NULL) {
		if (dir && !INDIR(dir, dirlen, rpath)) {
			if (debug > 1)
				info(" resolved path (%s) not in dir (%s)",
				     rpath, dir);
			return NULL;
		}
		if (stat(rpath, &sb) < 0 || !S_ISREG(sb.st_mode)) {
			if (debug > 1)
				info(" not a regular file");
			return NULL;
		}
		return strdup(rpath);
	}
	if (debug > 1) {
		int oerrno = errno;
		info(" partially resolved to '%s' (errno %d)", rpath, errno);
		errno = oerrno;
	}

	/*
	 * If create is not set or we failed for any reason other than
	 * a non-existent component, we return an error.
	 */
	if (!create || errno != ENOENT) {
		if (debug > 1)
			info(" realpath failed at %s with %d",
			     rpath, errno);
		return NULL;
	}

	/*
	 * Need to create intermediate directories.
	 *
	 * First, check that what does exist of the path falls within
	 * the directory given.
	 */
	assert(dir != NULL);
	if (!INDIR(dir, dirlen, rpath)) {
		if (debug > 1)
			info(" resolved path (%s) not in dir (%s)",
			     rpath, dir);
		return NULL;
	}

	/*
	 * Establish permission (mode) for new directories.
	 * Start with permission of 'dir'; this will get updated with
	 * every component that resolves, so that new directories will
	 * get created with the permission of the most recent ancestor.
	 */
	if (stat(dir, &sb) < 0) {
		if (debug > 1)
			info(" stat failed on dir (%s)?!", dir);
		return NULL;
	}
	cmask = (sb.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO)) | S_IRWXU;
	omask = umask(0);
	if (debug > 1)
		info(" umask=0 (was 0%o), initial cmask=0%o", omask, cmask);

	/*
	 * Find the first component of the original path that does not
	 * exist (i.e., what part of the original path maps to what realpath
	 * returned) and create everything from there on.
	 *
	 * Note that if what realpath returned is a prefix of the original
	 * path then nothing was translated and we are already at the first
	 * component that needs creation. So we find the appropriate location
	 * in the original string and go from there.
	 */
	npath = NULL;
	if ((pathcopy = strdup(path)) == NULL)
		goto done;
	rpathlen = strlen(rpath);
	assert(rpathlen <= pathlen);
	if (rpathlen > 1 && strncmp(pathcopy, rpath, rpathlen) == 0 &&
	    (pathcopy[rpathlen] == '\0' || pathcopy[rpathlen] == '/')) {
		/* same string, start at last slash in path */
		if (pathcopy[rpathlen] == '\0') {
			next = rindex(pathcopy, '/');
			assert(next != NULL);
		}
		/* rpath is a prefix, start at last slash in that prefix */
		else {
			next = rindex(rpath, '/');
			assert(next != NULL);
			next = &pathcopy[next-rpath];
		}
	}
	/* rpath is not a prefix, must scan entire path */
	else {
		next = pathcopy;
	}
	assert(next >= pathcopy && next < &pathcopy[pathlen]);
	assert(*next == '/');
	next++;
	if (debug > 1)
		info(" pathscan: path='%s', rpath='%s', start at '%s'",
		     pathcopy, rpath, next);

	while ((ep = index(next, '/')) != NULL) {
		*ep = '\0';
		if (debug > 1)
			info(" testing: %s", pathcopy);

		/*
		 * We use realpath here instead of just stat to make
		 * sure someone isn't actively tweaking the filesystem
		 * and messing with our head.
		 *
		 * If realpath fails, it should fail with ENOENT and
		 * the realpath-ified part should fall in our directory.
		 * Otherwise, someone really is playing games.
		 */
		if (myrealpath(pathcopy, rpath) == NULL) {
			if (errno != ENOENT ||
			    (dir && !INDIR(dir, dirlen, rpath))) {
				if (debug > 1)
					info("  resolves bad (%s)\n",
					     rpath);
				goto done;
			}
			/*
			 * We have hit a missing component.
			 * Create the component and carry on.
			 */
			if (mkdir(rpath, cmask) < 0) {
				if (debug > 1)
					info("  create failed (%s)\n",
					     rpath);
				goto done;
			}
			if (debug > 1)
				info("  created (%s)", rpath);
		} else {
			if (debug > 1)
				info("  exists (%s)", rpath);
			/*
			 * Update the creation permission; see comment above.
			 */
			if (stat(rpath, &sb) < 0) {
				if (debug > 1)
					info(" stat failed (%s)?!", rpath);
				goto done;
			}
			cmask = (sb.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO)) |
				S_IRWXU;
		}
		*ep = '/';
		next = ep+1;
	}

	/*
	 * We are down to the final component of the original path.
	 * It should either exist and be a regular file or not
	 * exist at all.
	 */
	if (myrealpath(pathcopy, rpath) == NULL &&
	    (errno != ENOENT || !INDIR(dir, dirlen, rpath))) {
		if (debug > 1)
			info(" final resolved path (%s) bad (%d)",
			     rpath, errno);
		goto done;
	}

	/*
	 * We are in our happy place. rpath is the canonicalized path.
	 */
	npath = strdup(rpath);
	if (debug > 1)
		info("resolvepath: '%s' resolved to '%s'", path, npath);

 done:
	umask(omask);
	if (pathcopy)
		free(pathcopy);
	return npath;
}
