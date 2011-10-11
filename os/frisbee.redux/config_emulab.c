/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Configuration "file" handling for Emulab.
 *
 * Uses info straight from the Emulab database.
 */ 

#ifdef USE_EMULAB_CONFIG
#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <mysql/mysql.h>
#include "log.h"
#include "configdefs.h"

/* Emulab includes */
#include "tbdb.h"	/* the DB library */
#include "config.h"	/* the defs-* defines */

extern int debug;

/*
 * Private configuration state that can be saved/restored.
 *
 * For Emulab, there are only a couple of DB values that don't change
 * that often.
 */
struct emulab_configstate {
	int image_maxsize;	/* sitevar:images/create/maxsize (in GB) */
	int image_maxwait;	/* sitevar:images/create/maxwait (in min) */
	int image_maxrate_std;	/* sitevar:images/frisbee/maxrate_std (in MB/s) */
	int image_maxrate_usr;	/* sitevar:images/frisbee/maxrate_usr (in MB/s) */
};

/* Extra info associated with a image information entry */
struct emulab_ii_extra_info {
	int DB_imageid;
};

/* Extra info associated with a host authentication entry */
struct emulab_ha_extra_info {
	char *pid;	/* project */
	int pidix;	/* project table index */
	char *gid;	/* group */
	int gidix;	/* group table index */
	char *eid;	/* experiment */
	int eidix;	/* experiment table index */
	char *sname;	/* swapper user name */
	int suidix;	/* swapper user table index */
	int suid;	/* swapper's unix uid */
	gid_t sgids[MAXGIDS];	/* swapper's unix gids */
	int ngids;	/* number of gids */
};

static char *MC_BASEADDR = FRISEBEEMCASTADDR;
static char *MC_BASEPORT = FRISEBEEMCASTPORT;
#ifdef FRISEBEENUMPORTS
static char *MC_NUMPORTS = FRISEBEENUMPORTS;
#else
static char *MC_NUMPORTS = "0";
#endif
static char *SHAREDIR	 = SHAREROOT_DIR;
static char *PROJDIR	 = PROJROOT_DIR;
static char *GROUPSDIR	 = GROUPSROOT_DIR;
static char *USERSDIR	 = USERSROOT_DIR;
static char *SCRATCHDIR	 = SCRATCHROOT_DIR;
#ifdef ELABINELAB
static int INELABINELAB  = ELABINELAB;
#else
static int INELABINELAB  = 0;
#endif

static uint64_t	put_maxsize = 10000000000ULL;	/* zero means no limit */
static uint32_t put_maxwait = 2000;		/* zero means no limit */
static uint32_t get_maxrate_std = 72000000;	/* zero means no limit */
static uint32_t get_maxrate_usr = 54000000;	/* zero means no limit */

/* Standard image directory: assumed to be "TBROOT/images" */
static char *STDIMAGEDIR;

/* Emit aliases when dumping the config info; makes it smaller */
static int dump_doaliases = 1;

/* Multicast address/port base info */
static int mc_a, mc_b, mc_c, mc_port_lo, mc_port_num;

/* Memory alloc functions that abort when no memory */
static void *mymalloc(size_t size);
static void *myrealloc(void *ptr, size_t size);
static char *mystrdup(const char *str);

/*
 * Return the value of a sitevar as a string.
 * Returns in order: the value if set, default_value if set, NULL
 */
static char *
emulab_getsitevar(char *name)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *value = NULL;

	res = mydb_query("SELECT value,defaultvalue FROM sitevariables "
			 " WHERE name='%s'", 2, name);
	if (res == NULL)
		return NULL;

	if (mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return NULL;
	}

	row = mysql_fetch_row(res);
	if (row[0])
		value = mystrdup(row[0]);
	else if (row[1])
		value = mystrdup(row[1]);
	mysql_free_result(res);

	return value;
}

static void
emulab_deinit(void)
{
	dbclose();
}

static int
emulab_read(void)
{
	char *val;
	int ival;

	/*
	 * Grab a couple of image creation related sitevars that don't
	 * change that often. We don't want to be reading these everytime
	 * set_put_values() gets called, so we do it explicitly here.
	 */
	val = emulab_getsitevar("images/create/maxsize");
	if (val) {
		ival = atoi(val);
		/* in GB, allow up to 10TB */
		if (ival >= 0 && ival < 10000)
			put_maxsize = (uint64_t)ival * 1024 * 1024 * 1024;
		free(val);
	}
	log("  image_put_maxsize = %d GB",
	    (int)(put_maxsize/(1024*1024*1024)));

	val = emulab_getsitevar("images/create/maxwait");
	if (val) {
		ival = atoi(val);
		/* in minutes, allow up to about 10TB @ 10MB/sec */
		if (ival >= 0 && ival < 20000)
			put_maxwait = (uint32_t)ival * 60;
		free(val);
	}
	log("  image_put_maxwait = %d min",
	    (int)(put_maxwait/60));

	val = emulab_getsitevar("images/frisbee/maxrate_std");
	if (val) {
		ival = atoi(val);
		/* in bytes/sec, allow up to 2Gb/sec */
		if (ival >= 0 && ival < 2000000000)
			get_maxrate_std = (uint32_t)ival;
		free(val);
	}
	log("  image_get_maxrate_std = %d MB/sec",
	    (int)(get_maxrate_std/1000000));

	val = emulab_getsitevar("images/frisbee/maxrate_usr");
	if (val) {
		ival = atoi(val);
		/* in bytes/sec, allow up to 2Gb/sec */
		if (ival >= 0 && ival < 2000000000)
			get_maxrate_usr = (uint32_t)ival;
		free(val);
	}
	log("  image_get_maxrate_usr = %d MB/sec",
	    (int)(get_maxrate_usr/1000000));

	return 0;
}

static void *
emulab_save(void)
{
	struct emulab_configstate *cs = mymalloc(sizeof(cs));

	cs->image_maxsize = put_maxsize;
	cs->image_maxwait = put_maxwait;
	cs->image_maxrate_std = get_maxrate_std;
	cs->image_maxrate_usr = get_maxrate_usr;

	return (void *)cs;
}

static int
emulab_restore(void *state)
{
	struct emulab_configstate *cs = state;

	put_maxsize = cs->image_maxsize;
	log("  image_put_maxsize = %d GB",
	    (int)(put_maxsize/(1024*1024*1024)));
	put_maxwait = cs->image_maxwait;
	log("  image_put_maxwait = %d min",
	    (int)(put_maxwait/60));
	get_maxrate_std = cs->image_maxrate_std;
	log("  image_get_maxrate_std = %d MB/sec",
	    (int)(get_maxrate_std/1000000));
	get_maxrate_usr = cs->image_maxrate_usr;
	log("  image_get_maxrate_usr = %d MB/sec",
	    (int)(get_maxrate_usr/1000000));

	return 0;
}

static void
emulab_free(void *state)
{
	free(state);
}

/*
 * Set the GET options for a particular node/image.
 *
 * Methods:
 *   XXX for now, just multicast and unicast
 * Options:
 *   loading a "standard" image: 72Mb/sec
 *   any other image:            54Mb/sec
 *   running in elabinelab:      add keepalive of 15 sec.
 */
static void
set_get_values(struct config_host_authinfo *ai, int ix)
{
	struct config_imageinfo *ii = &ai->imageinfo[ix];
	char str[256];

	/* get_methods */
	ii->get_methods = CONFIG_IMAGE_MCAST;
#if 1
	/*
	 * XXX the current frisbee server allows only a single client
	 * in unicast mode, which makes this option rather limited.
	 * So you may not want to allow it by default.
	 */
	ii->get_methods |= CONFIG_IMAGE_UCAST;
#endif

	/* get_timeout */
	/*
	 * In the short run, we leave this at pre-master-server levels
	 * for compatibility (we still support advance startup of servers
	 * via the localhost proxy).
	 *
	 * We also need this at Utah while we work out some MC problems on
	 * our control net (sometimes nodes can take minutes before they
	 * actually hook up with the server.
	 *
	 * In an inner elab, neither of these apply.
	 */
	if (!INELABINELAB)
		ii->get_timeout = 1800;
	/*
	 * We use a small server inactive timeout since we no longer have
	 * to start up a frisbeed well in advance of the client(s).
	 */
	else
		ii->get_timeout = 60;

	/* get_options */
	snprintf(str, sizeof str, " -W %u",
		 isindir(STDIMAGEDIR, ii->path) ?
		 get_maxrate_std : get_maxrate_usr);
	if (INELABINELAB)
		strcat(str, " -K 15");
	ii->get_options = mystrdup(str);

	/* and whack the put_* fields */
	ii->put_maxsize = 0;
	ii->put_timeout = 0;
	ii->put_options = NULL;
	ii->put_oldversion = NULL;
}

/*
 * Set the PUT maxsize/options for a particular node/image.
 * XXX right now these are completely pulled out of our posterior.
 */
static void
set_put_values(struct config_host_authinfo *ai, int ix)
{
	struct config_imageinfo *ii = &ai->imageinfo[ix];

	/* put_maxsize */
	ii->put_maxsize = put_maxsize;

	/* put_timeout */
	ii->put_timeout = put_maxwait;

	/* put_oldversion */
	/*
	 * For standard images, we keep ALL old versions as:
	 * <path>.<timestamp>, just because we are extra paranoid about those.
	 */
	if (isindir(STDIMAGEDIR, ii->path)) {
		time_t curtime;
		char *str;
		int len;

		curtime = time(NULL);
		len = strlen(ii->path) + 11;
		str = mymalloc(len);
		snprintf(str, len, "%s.%09u", ii->path, (unsigned)curtime);
		ii->put_oldversion = str;
	} else
		ii->put_oldversion = NULL;

	/* put_options */
	ii->put_options = NULL;

	/* and whack the get_* fields */
	ii->get_methods = 0;
	ii->get_timeout = 0;
	ii->get_options = NULL;
}

#define FREE(p) { if (p) free(p); }

/*
 * Free the dynamically allocated host_authinfo struct.
 */
static void
emulab_free_host_authinfo(struct config_host_authinfo *ai)
{
	int i;

	if (ai == NULL)
		return;

	FREE(ai->hostid);
	if (ai->imageinfo != NULL) {
		for (i = 0; i < ai->numimages; i++) {
			FREE(ai->imageinfo[i].imageid);
			FREE(ai->imageinfo[i].path);
			FREE(ai->imageinfo[i].sig);
			FREE(ai->imageinfo[i].get_options);
			FREE(ai->imageinfo[i].put_oldversion);
			FREE(ai->imageinfo[i].put_options);
			FREE(ai->imageinfo[i].extra);
		}
		free(ai->imageinfo);
	}
	if (ai->extra) {
		struct emulab_ha_extra_info *ei = ai->extra;

		FREE(ei->pid);
		FREE(ei->gid);
		FREE(ei->eid);
		FREE(ei->sname);
		free(ai->extra);
	}
	free(ai);
}

/*
 * Return the IP address/port to be used by the server/clients for
 * the image listed in ai->imageinfo[0].  Methods lists one or more transfer
 * methods that the client can handle, we return the method chosen.
 * If first is non-zero, then we need to return a "new" address and *addrp
 * and *portp are uninitialized.  If non-zero, then our last choice failed
 * (probably due to a port conflict) and we need to choose a new address
 * to try, possibly based on the existing info in *addrp and *portp.
 *
 * For Emulab, we use the frisbee_index from the DB along with the base
 * multicast address and port to compute a unique address/port.  Uses the
 * same logic that frisbeelauncher used to use.  For retries (first==0),
 * we choose a whole new addr/port for multicast.
 *
 * For unicast, we use the index as well, just to produce a unique port
 * number.
 *
 * Return zero on success, non-zero otherwise.
 */
static int
emulab_get_server_address(struct config_imageinfo *ii, int methods, int first,
			  in_addr_t *addrp, in_port_t *portp, int *methp)
{
	int	  idx;
	int	  a, b, c, d;

	if ((methods & (CONFIG_IMAGE_MCAST|CONFIG_IMAGE_UCAST)) == 0) {
		error("get_server_address: only support UCAST/MCAST");
		return 1;
	}

	if (!mydb_update("UPDATE emulab_indicies "
			" SET idx=LAST_INSERT_ID(idx+1) "
			" WHERE name='frisbee_index'")) {
		error("get_server_address: DB update error!");
		return 1;
	}
 again:
	idx = mydb_insertid();
	assert(idx > 0);

	a = mc_a;
	b = mc_b;
	c = mc_c;
	d = 1;

	d += idx;
	if (d > 254) {
		c += (d / 254);
		d = (d % 254) + 1;
	}
	if (c > 254) {
		b += (c / 254);
		c = (c % 254) + 1;
	}
	if (b > 254) {
		error("get_server_address: ran out of MC addresses!");
		return 1;
	}

	if (methods & CONFIG_IMAGE_MCAST) {
		/*
		 * XXX avoid addresses that "flood".
		 * 224.0.0.x and 224.128.0.x are defined to flood,
		 * but because of the way IP multicast addresses map
		 * onto ethernet addresses (only the low 23 bits are used)
		 * ANY MC address (224-239) with those bits will also flood.
		 * So avoid those, by skipping over the problematic range
		 * in the index.
		 *
		 * Note that because of the way the above increment process
		 * works, this should never happen except when the initial
		 * MC_BASEADDR is bad in this way (i.e., because of the
		 * "c = (c % 254) + 1" this function will never generate
		 * a zero value for c).
		 */
		if (c == 0 && (b == 0 || b == 128)) {
			if (!mydb_update("UPDATE emulab_indicies "
					 " SET idx=LAST_INSERT_ID(idx+254) "
					 " WHERE name='frisbee_index'")) {
				error("get_server_address: DB update error!");
				return 1;
			}
			goto again;
		}

		*methp = CONFIG_IMAGE_MCAST;
		*addrp = (a << 24) | (b << 16) | (c << 8) | d;
	} else if (methods & CONFIG_IMAGE_UCAST) {
		*methp = CONFIG_IMAGE_UCAST;
		/* XXX on retries, we don't mess with the address */
		if (first)
			*addrp = 0;
	}

	/*
	 * In the interest of uniform distribution, if we have a maximum
	 * number of ports to use we just use the index directly.
	 */
	if (mc_port_num) {
		*portp = mc_port_lo + (idx % mc_port_num);
	}
	/*
	 * In the interest of backward compat, if there is no maximum
	 * number of ports, we use the "classic" formula.
	 */
	else {
		*portp = mc_port_lo + (((c << 8) | d) & 0x7FFF);
	}

	return 0;
}

/*
 * Check and see if the given image is in the set of standard directories
 * to which this node (actually pid/gid/eid/uid) has access.  The set is:
 *  - /share		      (GET only)
 *  - /proj/<pid>	      (GET or PUT)
 *  - in /groups/<pid>/<gid>  (GET or PUT)
 *  - in /users/<swapper-uid> (GET or PUT)
 * and, if it exists:
 *  - in /scratch/<pid>	      (GET or PUT)
 * For a GET, the entire path must exist and be accessible. For a PUT,
 * everything up to the final component must.
 *
 * If imageid is NULL, then we just return 

 * We assume the get and put structures have been initialized, in particular
 * that they contain the "extra" info which has the pid/gid/eid/uid.
 */
static void
allow_stddirs(char *imageid, 
	      struct config_host_authinfo *get,
	      struct config_host_authinfo *put)
{
	char tpath[PATH_MAX];
	char *fpath, *fdir;
	char *shdir, *pdir, *gdir, *scdir, *udir;
	struct emulab_ha_extra_info *ei;
	struct config_imageinfo *ci;
	struct stat sb;
	int exists;

	if (get == NULL && put == NULL)
		return;

	fpath = NULL;

	/* XXX extra info is the same for both, we just need a valid one */
	ei = get ? get->extra : put->extra;

	/*
	 * Construct the allowed directories for this pid/gid/eid/uid.
	 * Note that these paths are "realpath approved".
	 */
	shdir = SHAREDIR;
	snprintf(tpath, sizeof tpath, "%s/%s", PROJDIR, ei->pid);
	pdir = mystrdup(tpath);

	snprintf(tpath, sizeof tpath, "%s/%s/%s", GROUPSDIR, ei->pid, ei->gid);
	gdir = mystrdup(tpath);

	snprintf(tpath, sizeof tpath, "%s/%s", USERSDIR, ei->sname);
	udir = mystrdup(tpath);

	if (SCRATCHDIR) {
		snprintf(tpath, sizeof tpath, "%s/%s", SCRATCHDIR, ei->pid);
		scdir = mystrdup(tpath);
	} else
		scdir = NULL;

	/*
	 * No image specified, just return info about the directories
	 * that are accessible.
	 */
	if (imageid == NULL) {
		int ni, i, j;
		size_t ns;
		char *dirs[8];

		/*
		 * Put allows all but SHAREDIR
		 */
		if (put != NULL) {
			dirs[0] = pdir;
			dirs[1] = gdir;
			dirs[2] = udir;
			dirs[3] = scdir;
			ni = put->numimages + (scdir ? 4 : 3);
			ns = ni * sizeof(struct config_imageinfo);
			if (put->imageinfo)
				put->imageinfo = myrealloc(put->imageinfo, ns);
			else
				put->imageinfo = mymalloc(ns);
			for (i = put->numimages; i < ni; i++) {
				ci = &put->imageinfo[i];
				ci->imageid = NULL;
				ci->dir = NULL;
				ci->path = mystrdup(dirs[i - put->numimages]);
				ci->flags = CONFIG_PATH_ISDIR;
				if (stat(ci->path, &sb) == 0) {
					ci->flags |= CONFIG_PATH_EXISTS;
					ci->sig = mymalloc(sizeof(time_t));
					*(time_t *)ci->sig = sb.st_mtime;
					ci->flags |= CONFIG_SIG_ISMTIME;
				} else
					ci->sig = NULL;
				ci->uid = ei->suid;
				for (j = 0; j < ei->ngids; j++)
					ci->gids[j] = ei->sgids[j];
				ci->ngids = ei->ngids;
				set_put_values(put, i);
				ci->extra = NULL;
			}
			put->numimages = ni;
		}
		/*
		 * Get allows any of the above
		 */
		if (get != NULL) {
			dirs[0] = shdir;
			dirs[1] = pdir;
			dirs[2] = gdir;
			dirs[3] = udir;
			dirs[4] = scdir;
			ni = get->numimages + (scdir ? 5 : 4);
			ns = ni * sizeof(struct config_imageinfo);
			if (get->imageinfo)
				get->imageinfo = myrealloc(get->imageinfo, ns);
			else
				get->imageinfo = mymalloc(ns);
			for (i = get->numimages; i < ni; i++) {
				ci = &get->imageinfo[i];
				ci->imageid = NULL;
				ci->dir = NULL;
				ci->path = mystrdup(dirs[i - get->numimages]);
				ci->flags = CONFIG_PATH_ISDIR;
				if (stat(ci->path, &sb) == 0) {
					ci->flags |= CONFIG_PATH_EXISTS;
					ci->sig = mymalloc(sizeof(time_t));
					*(time_t *)ci->sig = sb.st_mtime;
					ci->flags |= CONFIG_SIG_ISMTIME;
				} else
					ci->sig = NULL;
				ci->uid = ei->suid;
				for (j = 0; j < ei->ngids; j++)
					ci->gids[j] = ei->sgids[j];
				ci->ngids = ei->ngids;
				set_get_values(get, i);
				ci->extra = NULL;
			}
			get->numimages = ni;
		}
		goto done;
	}

	/*
	 * Image was specified.
	 *
	 * At this point we cannot do a full path check since the full path
	 * need not exist and we are possibly running with enhanced privilege.
	 * So we only weed out obviously bogus paths here (possibly checking
	 * just the partial path returned by realpath) and mark the imageinfo
	 * as needed a full resolution later.
	 */
	if (myrealpath(imageid, tpath) == NULL) {
		if (errno != ENOENT)
			goto done;
		exists = 0;
	} else
		exists = 1;
	fpath = mystrdup(tpath);
	if (debug)
		info("%s: exists=%d, resolves to: '%s'",
		     imageid, exists, tpath);

	/*
	 * Make the appropriate access checks for get/put
	 */
	if (put != NULL &&
	    ((fdir = isindir(pdir, fpath)) ||
	     (fdir = isindir(gdir, fpath)) ||
	     (fdir = isindir(udir, fpath)) ||
	     (fdir = isindir(scdir, fpath)))) {
		int j;
		assert(put->imageinfo == NULL);

		put->imageinfo = mymalloc(sizeof(struct config_imageinfo));
		put->numimages = 1;
		ci = &put->imageinfo[0];
		ci->imageid = mystrdup(imageid);
		ci->dir = mystrdup(fdir);
		ci->path = mystrdup(imageid);
		ci->flags = CONFIG_PATH_ISFILE|CONFIG_PATH_RESOLVE;
		if (exists && stat(ci->path, &sb) == 0) {
			ci->flags |= CONFIG_PATH_EXISTS;
			ci->sig = mymalloc(sizeof(time_t));
			*(time_t *)ci->sig = sb.st_mtime;
			ci->flags |= CONFIG_SIG_ISMTIME;
		} else
			ci->sig = NULL;
		ci->uid = ei->suid;
		for (j = 0; j < ei->ngids; j++)
			ci->gids[j] = ei->sgids[j];
		ci->ngids = ei->ngids;
		set_put_values(put, 0);
		ci->extra = NULL;
	}
	if (get != NULL &&
	    ((fdir = isindir(shdir, fpath)) ||
	     (fdir = isindir(pdir, fpath)) ||
	     (fdir = isindir(gdir, fpath)) ||
	     (fdir = isindir(scdir, fpath)) ||
	     (fdir = isindir(udir, fpath)))) {
		int j;
		assert(get->imageinfo == NULL);

		get->imageinfo = mymalloc(sizeof(struct config_imageinfo));
		get->numimages = 1;
		ci = &get->imageinfo[0];
		ci->imageid = mystrdup(imageid);
		ci->dir = mystrdup(fdir);
		ci->path = mystrdup(imageid);
		ci->flags = CONFIG_PATH_ISFILE|CONFIG_PATH_RESOLVE;
		if (exists && stat(ci->path, &sb) == 0) {
			ci->flags |= CONFIG_PATH_EXISTS;
			ci->sig = mymalloc(sizeof(time_t));
			*(time_t *)ci->sig = sb.st_mtime;
			ci->flags |= CONFIG_SIG_ISMTIME;
		} else
			ci->sig = NULL;
		ci->uid = ei->suid;
		for (j = 0; j < ei->ngids; j++)
			ci->gids[j] = ei->sgids[j];
		ci->ngids = ei->ngids;
		set_get_values(get, 0);
		ci->extra = NULL;
	}

 done:
	free(pdir);
	free(gdir);
	free(udir);
	if (scdir)
		free(scdir);
	if (fpath)
		free(fpath);
}

/*
 * Get the Emulab nodeid for the node with the indicated control net IP.
 * Return a malloc'ed string on success, NULL otherwise.
 */
static char *
emulab_nodeid(struct in_addr *cnetip)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	char *node;

	res = mydb_query("SELECT node_id FROM interfaces"
			 " WHERE IP='%s' AND role='ctrl'",
			 1, inet_ntoa(*cnetip));
	if (res == NULL)
		return NULL;

	/* No such node */
	if (mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return NULL;
	}

	/* NULL node_id!? */
	row = mysql_fetch_row(res);
	if (!row[0]) {
		error("config_host_authinfo: null node_id!?");
		mysql_free_result(res);
		return NULL;
	}
	node = mystrdup(row[0]);
	mysql_free_result(res);

	return node;
}

/*
 * Get the Emulab internal imageid for the image identified by ipid/iname.
 * Return a non-zero index on success, zero otherwise.
 */
static int
emulab_imageid(char *ipid, char *iname)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	int id;

	res = mydb_query("SELECT imageid FROM images"
			 " WHERE pid='%s' AND imagename='%s'",
			 1, ipid, iname);
	if (res == NULL)
		return 0;

	/* No such image */
	if (mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	/* NULL imageid!? */
	row = mysql_fetch_row(res);
	if (!row[0]) {
		error("config_host_authinfo: null imageid!?");
		mysql_free_result(res);
		return 0;
	}
	id = atoi(row[0]);
	mysql_free_result(res);

	return id;
}

/*
 * Get the list of unix gids needed to access an image.
 * Returns the count of IDs put in igids, or zero on error.
 */
static int
emulab_imagegids(int imageidx, int *igids)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	int nrows;

	igids[0] = igids[1] = -1;

	res = mydb_query("SELECT unix_gid FROM groups AS g, images AS i"
			 " WHERE g.pid_idx=i.pid_idx"
			 " AND (g.gid_idx=g.pid_idx OR g.gid_idx=i.gid_idx)"
			 " AND imageid=%d", 1, imageidx);
	if (res == NULL) {
		error("config_host_authinfo: image gid query failed!?");
		return 0;
	}

	nrows = mysql_num_rows(res);
	if (nrows > 2) {
		error("config_host_authinfo: image gid query returned too many rows!?");
		mysql_free_result(res);
		return 0;
	}

	if (nrows > 0) {
		row = mysql_fetch_row(res);
		if (row[0] && row[0][0])
			igids[0] = atoi(row[0]);
		if (nrows > 1) {
			row = mysql_fetch_row(res);
			if (row[0] && row[0][0])
				igids[1] = atoi(row[0]);
		}
	}
	mysql_free_result(res);

	return nrows;
}

/*
 * Based on the image_permissions table, see if the user or group of the
 * experiment identified by ei can access the image with the given Emulab
 * imagid for either upload or download.
 *
 * Returns 1 if explicitly yes, 0 otherwise.
 */
static int
can_access(int imageidx, struct emulab_ha_extra_info *ei, int upload)
{
	MYSQL_RES *res;
	char *clause = "";

	if (upload)
		clause = "AND ip.allow_write=1";

	/*
	 * Check user permissions first.
	 */
	res = mydb_query("SELECT allow_write FROM image_permissions AS ip"
			 " WHERE ip.imageid=%d"
			 " AND ip.permission_type='user'"
			 " AND ip.permission_idx=%d %s",
			 1, imageidx, ei->suidix, clause);
	if (res) {
		if (mysql_num_rows(res) > 0) {
			mysql_free_result(res);
			return 1;
		}
		mysql_free_result(res);
	}

	/*
	 * No go? Try group permissions instead.
	 */
        res = mydb_query("SELECT allow_write FROM group_membership AS g"
			 " LEFT JOIN image_permissions AS ip ON"
			 "   ip.permission_type='group'"
			 "   AND ip.permission_idx=g.gid_idx"
			 " WHERE ip.imageid=%d"
			 " AND g.uid_idx=%d AND g.trust!='none' %s",
			 1, imageidx, ei->suidix, clause);
	if (res) {
		if (mysql_num_rows(res) > 0) {
			mysql_free_result(res);
			return 1;
		}
		mysql_free_result(res);
	}

	return 0;
}

/*
 * Find all images (imageid==NULL) or a specific image (imageid!=NULL)
 * that a particular node can access for GET/PUT.  At any time, a node is
 * associated with a specific project and group.  These determine the
 * ability to GET/PUT an image.  For the all image case, the list of
 * accessible images will include:
 *
 * - all "global" images (GET only)
 * - all "shared" images in the project and any group (GET only)
 * - all images in the project and group of the node (GET and PUT)
 *
 * We later added a more fine-grained mechanism for accessing "real"
 * (imageid[0]!='/') images. If the standard checks fail, we look for
 * specific images in the image_permissions table which allows per-group
 * and per-user access at individual image granularity. Right now we are
 * only checking this table if called for a specific image (imageid!=NULL).
 *
 * For a single image this will either return a single image or no image.
 * The desired image must be one of the images that would be returned in
 * the all images case.
 *
 * Imageids are constructed from "pid/imageid" which is the minimal unique
 * identifier (i.e., image name are a per-project namespace and not a
 * per-project, per-group namespace).
 *
 * Return zero on success, non-zero otherwise.
 *
 * Notes on validating paths:
 *
 *    For a GET of a DB-registered image (imageid in "pid/imageid" format),
 *    we just return the path recorded in the DB without any validation.
 *    That path can point anywhere and the file does not have to exist.
 *
 *    For a GET of a file (imageid starts with '/'), we canonicalize
 *    (via realpath) the user-provided path and ensure that the portion
 *    of the path that exists falls within one of the allowed directories.
 *    Note that this is still susceptible to time-of-check-to-time-of-use
 *    attacks by using symlinks, but the actual frisbeed that feeds the
 *    image will run as the user.
 */
static int
emulab_get_host_authinfo(struct in_addr *req, struct in_addr *host,
			 char *imageid,
			 struct config_host_authinfo **getp,
			 struct config_host_authinfo **putp)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		*node, *proxy, *role = NULL;
	int		i, j, nrows;
	char		*wantpid = NULL, *wantname = NULL;
	struct config_host_authinfo *get = NULL, *put = NULL;
	struct emulab_ha_extra_info *ei = NULL;
	int		imageidx = 0, ngids, igids[2];

	if (getp == NULL && putp == NULL)
		return 0;

	if (getp) {
		get = mymalloc(sizeof *get);
		memset(get, 0, sizeof *get);
	}
	if (putp) {
		put = mymalloc(sizeof *put);
		memset(put, 0, sizeof(*put));
	}

	/*
	 * If the requester is not the same as the host, then it is a proxy
	 * request.  In Emulab, the only proxy scenarios we support are
	 * elabinelab inner bosses and subbosses.  So we first ensure that
	 * the requester is listed as one of those.
	 */
	if (req->s_addr != host->s_addr) {
#ifdef USE_LOCALHOST_PROXY
		/*
		 * XXX if node_id is localhost (i.e., boss), then allow
		 * proxying for any node.
		 */
		if (req->s_addr == htonl(INADDR_LOOPBACK)) {
			role = "boss";
			proxy = mystrdup(role);
			goto isboss;
		}
#endif
		proxy = emulab_nodeid(req);
		if (proxy == NULL) {
			emulab_free_host_authinfo(get);
			emulab_free_host_authinfo(put);
			return 1;
		}

		/* Make sure the node really is a inner-boss/subboss */
		res = mydb_query("SELECT erole,inner_elab_role FROM reserved"
				 " WHERE node_id='%s'", 2, proxy);
		assert(res != NULL);

		/* Node is free */
		if (mysql_num_rows(res) == 0) {
			mysql_free_result(res);
			free(proxy);
			emulab_free_host_authinfo(get);
			emulab_free_host_authinfo(put);
			return 1;
		}

		/*
		 * Is node an inner boss?
		 * Note that string could be "boss" or "boss+router"
		 * so we just check the prefix.
		 */
		row = mysql_fetch_row(res);
		if (row[1] && strncmp(row[1], "boss", 4) == 0)
			role = "innerboss";
		/* or a subboss? */
		else if (row[0] && strcmp(row[0], "subboss") == 0)
			role = "subboss";
		/* neither, return an error */
		else {
			mysql_free_result(res);
			free(proxy);
			emulab_free_host_authinfo(get);
			emulab_free_host_authinfo(put);
			return 1;
		}
		mysql_free_result(res);
	} else
		proxy = NULL;

#ifdef USE_LOCALHOST_PROXY
 isboss:
#endif
	/*
	 * Find the node name from its control net IP.
	 * If the node doesn't exist, we return an empty list.
	 */
	node = emulab_nodeid(host);
	if (node == NULL) {
		if (proxy) free(proxy);
		if (getp) *getp = get;
		if (putp) *putp = put;
		return 0;
	}
	if (get != NULL)
		get->hostid = mystrdup(node);
	if (put != NULL)
		put->hostid = mystrdup(node);

	/*
	 * We have a proxy node.  It should be one of:
	 * - boss (for any node)
	 * - a subboss (that is a frisbee subboss for the node)
	 * - an inner-boss (in the same experiment as the node)
	 * Note that we could not do this check until we had the node name.
	 * Note also that we no longer care about proxy or not after this.
	 */
	if (proxy) {
#ifdef USE_LOCALHOST_PROXY
		if (strcmp(role, "boss") == 0) {
			res = NULL;
		} else
#endif
		if (strcmp(role, "subboss") == 0) {
			res = mydb_query("SELECT node_id"
					 " FROM subbosses"
					 " WHERE subboss_id='%s'"
					 "  AND node_id='%s'"
					 "  AND service='frisbee'",
					 1, proxy, node);
			assert(res != NULL);
		} else {
			res = mydb_query("SELECT r1.node_id"
					 " FROM reserved as r1,"
					 "  reserved as r2"
					 " WHERE r1.node_id='%s'"
					 "  AND r2.node_id='%s'"
					 "  AND r1.pid=r2.pid"
					 "  AND r1.eid=r2.eid",
					 1, proxy, node);
			assert(res != NULL);
		}
		if (res) {
			if (mysql_num_rows(res) == 0) {
				mysql_free_result(res);
				free(proxy);
				free(node);
				emulab_free_host_authinfo(get);
				emulab_free_host_authinfo(put);
				return 1;
			}
			mysql_free_result(res);
		}
		free(proxy);
	}

	/*
	 * Find the pid/gid to which the node is currently assigned
	 * and also the unix uid of the swapper of the experiment and
	 * their gids in case we need to run a server process.
	 */
	res = mydb_query("SELECT e.pid,e.gid,r.eid,u.uid,u.unix_uid,"
			 "       e.pid_idx,e.gid_idx,e.idx,u.uid_idx"
			 " FROM reserved AS r,experiments AS e,users AS u"
			 " WHERE r.pid=e.pid AND r.eid=e.eid"
			 "  AND e.swapper_idx=u.uid_idx"
			 "  AND r.node_id='%s'", 9, node);
	assert(res != NULL);

	/* Node is free */
	if (mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		free(node);
		if (getp) *getp = get;
		if (putp) *putp = put;
		return 0;
	}

	row = mysql_fetch_row(res);
	if (!row[0] || !row[1] || !row[2] || !row[3] || !row[4] ||
	    !row[5] || !row[6] || !row[7] || !row[8]) {
		error("config_host_authinfo: null pid/gid/eid/uname/uid!?");
		mysql_free_result(res);
		emulab_free_host_authinfo(get);
		emulab_free_host_authinfo(put);
		free(node);
		return 1;
	}

	if (get != NULL) {
		ei = mymalloc(sizeof *ei);
		ei->pid = mystrdup(row[0]);
		ei->gid = mystrdup(row[1]);
		ei->eid = mystrdup(row[2]);
		ei->sname = mystrdup(row[3]);
		ei->suid = atoi(row[4]);
		ei->pidix = atoi(row[5]);
		ei->gidix = atoi(row[6]);
		ei->eidix = atoi(row[7]);
		ei->suidix = atoi(row[8]);
		ei->ngids = 0;
		get->extra = ei;
	}
	if (put != NULL) {
		ei = mymalloc(sizeof *ei);
		ei->pid = mystrdup(row[0]);
		ei->gid = mystrdup(row[1]);
		ei->eid = mystrdup(row[2]);
		ei->sname = mystrdup(row[3]);
		ei->suid = atoi(row[4]);
		ei->pidix = atoi(row[5]);
		ei->gidix = atoi(row[6]);
		ei->eidix = atoi(row[7]);
		ei->suidix = atoi(row[8]);
		ei->ngids = 0;
		put->extra = ei;
	}
	mysql_free_result(res);

	/*
	 * Get all unix groups the swapper is a member of.
	 * XXX this does not include the extra non-Emulab managed groups
	 *     from unixgroup_membership.
	 */
	res = mydb_query("SELECT g.unix_gid"
			 " FROM groups AS g,group_membership AS gm"
			 " WHERE g.gid_idx=gm.gid_idx AND gm.uid='%s'",
			 1, ei->sname);
	assert(res != NULL);

	nrows = mysql_num_rows(res);
	if (nrows > MAXGIDS)
		warning("User '%s' in more than %d groups, truncating list",
			ei->sname, MAXGIDS);
	for (i = 0; i < nrows; i++) {
		row = mysql_fetch_row(res);
		if (get != NULL) {
			ei = (struct emulab_ha_extra_info *)get->extra;
			ei->sgids[i] = atoi(row[0]);
			ei->ngids++;
		}
		if (put != NULL) {
			ei = (struct emulab_ha_extra_info *)put->extra;
			ei->sgids[i] = atoi(row[0]);
			ei->ngids++;
		}
	}
	mysql_free_result(res);

	/*
	 * If imageid is specified and starts with a '/', it is a path.
	 * In this case we compare that path to those accessible by
	 * the experiment swapper and pid/gid/eid.
	 */
	if (imageid && imageid[0] == '/') {
		allow_stddirs(imageid, get, put);
		goto done;
	}

	/*
	 * If we are interested in a specific image, extract the
	 * pid and imagename from the ID.
	 */
	if (imageid != NULL) {
		wantpid = mystrdup(imageid);
		wantname = index(wantpid, '/');
		if (wantname == NULL) {
			wantname = wantpid;
			wantpid = mystrdup("emulab-ops");
		} else {
			*wantname = '\0';
			wantname = mystrdup(wantname+1);
		}
		imageidx = emulab_imageid(wantpid, wantname);
		assert(imageidx > 0);
	}

	if (put != NULL) {
		struct emulab_ha_extra_info *ei = put->extra;

		if (imageid != NULL) {
			/* Interested in a specific image */
			if (can_access(imageidx, ei, 1)) {
				res = mydb_query("SELECT pid,gid,imagename,"
						 " path,imageid"
						 " FROM images WHERE"
						 " pid='%s'"
						 " AND imagename='%s'",
						 5, wantpid, wantname);
			} else {
				/*
				 * Pid of expt must be same as pid of image
				 * and gid the same or image "shared".
				 */
				res = mydb_query("SELECT pid,gid,imagename,"
						 "path,imageid"
						 " FROM images WHERE"
						 " pid='%s' AND imagename='%s'"
						 " AND pid='%s'"
						 " AND (gid='%s' OR"
						 "    (gid=pid AND shared=1))",
						 5, wantpid, wantname,
						 ei->pid, ei->gid);
			}
		} else {
			/* Find all images that this pid/gid can PUT */
			res = mydb_query("SELECT pid,gid,imagename,"
					 "path,imageid"
					 " FROM images"
					 " WHERE pid='%s'"
					 " AND (gid='%s' OR"
					 "     (gid=pid AND shared=1))"
					 " ORDER BY pid,gid,imagename",
					 5, ei->pid, ei->gid);
		}
		assert(res != NULL);

		/* Construct the list of "pid/imagename" imageids */
		nrows = mysql_num_rows(res);
		if (nrows)
			put->imageinfo =
				mymalloc(nrows *
					 sizeof(struct config_imageinfo));
		put->numimages = 0;
		for (i = 0; i < nrows; i++) {
			struct emulab_ii_extra_info *ii;
			struct config_imageinfo *ci;
			struct stat sb;
			char *iid;
			int iidx;

			row = mysql_fetch_row(res);
			/* XXX ignore rows with null or empty info */
			if (!row[0] || !row[0][0] ||
			    !row[1] || !row[1][0] ||
			    !row[2] || !row[2][0] ||
			    !row[3] || !row[3][0] ||
			    !row[4] || !row[4][0])
				continue;

			/*
			 * XXX if image is in the standard image directory,
			 * disallow PUTs.
			 *
			 * We would like to allow PUTs to the standard image
			 * directory, but I am not sure how to authenticate
			 * them. Checking that the swapper of the experiment
			 * (that the node is in) is an admin or otherwise
			 * "special" won't work because nodes get put into
			 * hwdown and other special experiments possibly
			 * without revoking the access of the previous user.
			 */
			if (isindir(STDIMAGEDIR, row[3])) {
				error("%s: cannot update standard images "
				      "right now", row[3]);
				continue;
			}

			iid = mymalloc(strlen(row[0]) + strlen(row[2]) + 2);
			strcpy(iid, row[0]);
			strcat(iid, "/");
			strcat(iid, row[2]);
			ci = &put->imageinfo[put->numimages];
			ci->imageid = iid;
			ci->dir = NULL;
			ci->path = mystrdup(row[3]);
			ci->flags = CONFIG_PATH_ISFILE;
			if (stat(ci->path, &sb) == 0) {
				ci->flags |= CONFIG_PATH_EXISTS;
				ci->sig = mymalloc(sizeof(time_t));
				*(time_t *)ci->sig = sb.st_mtime;
				ci->flags |= CONFIG_SIG_ISMTIME;
			} else
				ci->sig = NULL;
			ci->uid = ei->suid;
			iidx = atoi(row[4]);

			/*
			 * Find the unix gids to use for any server process.
			 * This includes the gid of the image's project and
			 * any subgroup gid if the image is associated with
			 * one. If for any reason we don't come up with
			 * these gids, we use the gids for the swapper of
			 * the experiment.
			 */
			ngids = emulab_imagegids(iidx, igids);
			if (ngids > 0) {
				for (j = 0; j < ngids; j++)
					ci->gids[j] = igids[j];
				ci->ngids = ngids;
			} else {
				for (j = 0; j < ei->ngids; j++)
					ci->gids[j] = ei->sgids[j];
				ci->ngids = ei->ngids;
			}

			set_put_values(put, put->numimages);
			ii = mymalloc(sizeof *ii);
			ii->DB_imageid = iidx;
			ci->extra = ii;
			put->numimages++;
		}
		mysql_free_result(res);
	}

	if (get != NULL) {
		struct emulab_ha_extra_info *ei = get->extra;

		if (imageid != NULL) {
			/* Interested in a specific image */
			if (can_access(imageidx, ei, 0)) {
				res = mydb_query("SELECT pid,gid,imagename,"
						 "path,imageid"
						 " FROM images WHERE"
						 " pid='%s'"
						 " AND imagename='%s'",
						 5, wantpid, wantname);
			} else {
				res = mydb_query("SELECT pid,gid,imagename,"
						 "path,imageid"
						 " FROM images WHERE"
						 " pid='%s' AND imagename='%s'"
						 " AND (global=1"
						 "     OR (pid='%s'"
						 "        AND (gid='%s'"
						 "            OR shared=1)))"
						 " ORDER BY pid,gid,imagename",
						 5, wantpid, wantname,
						 ei->pid, ei->gid);
			}
		} else {
			/* Find all images that this pid/gid can GET */
			res = mydb_query("SELECT pid,gid,imagename,"
					 "path,imageid"
					 " FROM images WHERE"
					 "  (global=1"
					 "  OR (pid='%s'"
					 "     AND (gid='%s' OR shared=1)))"
					 " ORDER BY pid,gid,imagename",
					 5, ei->pid, ei->gid);
		}
		assert(res != NULL);

		/* Construct the list of "pid/imagename" imageids */
		nrows = mysql_num_rows(res);
		if (nrows)
			get->imageinfo =
				mymalloc(nrows *
					 sizeof(struct config_imageinfo));
		get->numimages = 0;
		for (i = 0; i < nrows; i++) {
			struct emulab_ii_extra_info *ii;
			struct config_imageinfo *ci;
			struct stat sb;
			char *iid;
			int iidx;

			row = mysql_fetch_row(res);
			/* XXX ignore rows with null or empty info */
			if (!row[0] || !row[0][0] ||
			    !row[1] || !row[1][0] ||
			    !row[2] || !row[2][0] ||
			    !row[3] || !row[3][0] ||
			    !row[4] || !row[4][0])
				continue;
			iid = mymalloc(strlen(row[0]) + strlen(row[2]) + 2);
			strcpy(iid, row[0]);
			strcat(iid, "/");
			strcat(iid, row[2]);
			ci = &get->imageinfo[get->numimages];
			ci->imageid = iid;
			ci->dir = NULL;
			ci->path = mystrdup(row[3]);
			ci->flags = CONFIG_PATH_ISFILE;
			if (stat(ci->path, &sb) == 0) {
				ci->flags |= CONFIG_PATH_EXISTS;
				ci->sig = mymalloc(sizeof(time_t));
				*(time_t *)ci->sig = sb.st_mtime;
				ci->flags |= CONFIG_SIG_ISMTIME;
			} else
				ci->sig = NULL;
			ci->uid = ei->suid;
			iidx = atoi(row[4]);

			/*
			 * Find the unix gids to use for any server process.
			 * This includes the gid of the image's project and
			 * any subgroup gid if the image is associated with
			 * one. If for any reason we don't come up with
			 * these gids, we use the gids for the swapper of
			 * the experiment.
			 */
			ngids = emulab_imagegids(iidx, igids);
			if (ngids > 0) {
				for (j = 0; j < ngids; j++)
					ci->gids[j] = igids[j];
				ci->ngids = ngids;
			} else {
				for (j = 0; j < ei->ngids; j++)
					ci->gids[j] = ei->sgids[j];
				ci->ngids = ei->ngids;
			}

			set_get_values(get, get->numimages);
			ii = mymalloc(sizeof *ii);
			ii->DB_imageid = iidx;
			ci->extra = ii;
			get->numimages++;
		}
		mysql_free_result(res);
	}

	/*
	 * Finally add on the standard directories that a node can access.
	 */
	if (imageid == NULL)
		allow_stddirs(imageid, get, put);

 done:
	free(node);
	if (wantpid)
		free(wantpid);
	if (wantname)
		free(wantname);
	if (getp) *getp = get;
	if (putp) *putp = put;
	return 0;
}

static void
dump_host_authinfo(FILE *fd, char *node, char *cmd,
		   struct config_host_authinfo *ai)
{
	struct emulab_ha_extra_info *ei = ai->extra;
	int i;

	if (strcmp(cmd, "GET") == 0)
		fprintf(fd, "# %s in %s/%s\n", node, ei->pid, ei->eid);
	fprintf(fd, "%s %s ", node, cmd);

	/*
	 * If we have defined image aliases, just dump the appropriate
	 * aliases here for each node.
	 */
	if (dump_doaliases) {
		if (strcmp(cmd, "GET") == 0)
			fprintf(fd, "global-images %s-images %s-%s-images ",
				ei->pid, ei->pid, ei->gid);
		else
			fprintf(fd, "%s-%s-images ", ei->pid, ei->gid);
	}
	/*
	 * Otherwise, dump the whole list of images for each node
	 */
	else {
		for (i = 0; i < ai->numimages; i++)
			if (ai->imageinfo[i].flags == CONFIG_PATH_ISFILE)
				fprintf(fd, "%s ", ai->imageinfo[i].imageid);
	}

	/*
	 * And dump any directories that can be accessed
	 */
	for (i = 0; i < ai->numimages; i++)
		if (ai->imageinfo[i].flags == CONFIG_PATH_ISDIR)
			fprintf(fd, "%s/* ", ai->imageinfo[i].path);

	fprintf(fd, "\n");
}

/*
 * Dump image alias lines for:
 * - global images (global=1)                     => "global-images"
 * - project-wide images (pid=<pid> and shared=1) => "<pid>-images"
 * - per-group images (pid=<pid> and gid=<gid>)   => "<pid>-<gid>-images"
 */
static void
dump_image_aliases(FILE *fd)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		i, nrows;
	char		*lastpid;

	/* First the global image alias */
	res = mydb_query("SELECT pid,imagename"
			 " FROM images"
			 " WHERE global=1"
			 " ORDER BY pid,imagename", 2);
	assert(res != NULL);

	nrows = mysql_num_rows(res);
	if (nrows) {
		fprintf(fd, "imagealias global-images ");
		for (i = 0; i < nrows; i++) {
			row = mysql_fetch_row(res);
			/* XXX ignore rows with null or empty info */
			if (!row[0] || !row[0][0] ||
			    !row[1] || !row[1][0])
				continue;
			fprintf(fd, "%s/%s ", row[0], row[1]);
		}
		fprintf(fd, "\n");
	}
	mysql_free_result(res);

	/* Only create aliases for pid/gids that have imageable nodes now */
	res = mydb_query("SELECT e.pid,e.gid"
			 " FROM nodes AS n,node_types AS t,"
			 "  node_type_attributes AS a,"
			 "  reserved AS r,experiments AS e"
			 " WHERE n.type=t.type AND n.type=a.type"
			 "  AND n.node_id=r.node_id"
			 "  AND r.pid=e.pid AND r.eid=e.eid"
			 "  AND n.role='testnode' AND t.class='pc'"
			 "  AND a.attrkey='imageable' AND a.attrvalue!='0'"
			 " GROUP by e.pid,e.gid", 2);
	assert(res != NULL);

	lastpid = mystrdup("NONE");
	nrows = mysql_num_rows(res);
	for (i = 0; i < nrows; i++) {
		MYSQL_RES	*res2;
		MYSQL_ROW	row2;
		int		i2, nrows2;

		row = mysql_fetch_row(res);
		/* XXX ignore rows with null or empty info */
		if (!row[0] || !row[0][0] ||
		    !row[1] || !row[1][0])
			continue;

		/* New pid, put out shared project image alias */
		if (strcmp(lastpid, row[0])) {
			free(lastpid);
			lastpid = mystrdup(row[0]);

			res2 = mydb_query("SELECT imagename"
					  " FROM images"
					  " WHERE pid='%s' AND shared=1"
					  " ORDER BY imagename",
					  1, lastpid);
			assert(res2 != NULL);

			nrows2 = mysql_num_rows(res2);
			if (nrows2) {
				fprintf(fd, "imagealias %s-images ", lastpid);
				for (i2 = 0; i2 < nrows2; i2++) {
					row2 = mysql_fetch_row(res2);
					if (!row2[0] || !row2[0][0])
						continue;
					fprintf(fd, "%s/%s ",
						lastpid, row2[0]);
				}
				fprintf(fd, "\n");
			}
			mysql_free_result(res2);
		}

		/* Put out pid/eid image alias */
		res2 = mydb_query("SELECT imagename"
				  " FROM images"
				  " WHERE pid='%s' AND gid='%s'"
				  " ORDER BY imagename",
				  1, row[0], row[1]);
		assert(res2 != NULL);

		nrows2 = mysql_num_rows(res2);
		if (nrows2) {
			fprintf(fd, "imagealias %s-%s-images ",
				row[0], row[1]);
			for (i2 = 0; i2 < nrows2; i2++) {
				row2 = mysql_fetch_row(res2);
				if (!row2[0] || !row2[0][0])
					continue;
				fprintf(fd, "%s/%s ", lastpid, row2[0]);
			}
			fprintf(fd, "\n");
		}
		mysql_free_result(res2);
	}
	mysql_free_result(res);
	free(lastpid);
}

static void
emulab_dump(FILE *fd)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		i, nrows;

	fprintf(fd, "Emulab master frisbee config:\n");

	if (dump_doaliases)
		dump_image_aliases(fd);

	res = mydb_query("SELECT n.node_id,i.IP"
			 " FROM nodes AS n,node_types AS t,"
			 "  node_type_attributes AS a,interfaces AS i"
			 " WHERE n.type=t.type AND n.type=a.type"
			 "  AND n.node_id=i.node_id AND i.role='ctrl'"
			 "  AND n.role='testnode' AND t.class='pc'"
			 "  AND a.attrkey='imageable' AND a.attrvalue!='0'"
			 " ORDER BY n.priority", 2);
	assert(res != NULL);

	nrows = mysql_num_rows(res);
	for (i = 0; i < nrows; i++) {
		row = mysql_fetch_row(res);
		if (row[0] && row[1]) {
			struct config_host_authinfo *get, *put;
			struct in_addr in;

			inet_aton(row[1], &in);
			if (emulab_get_host_authinfo(&in, &in, NULL,
						     &get, &put)) {
				warning("Could not get node authinfo for %s",
					row[0]);
			}
			if (strcmp(get->hostid, row[0])) {
				warning("node_id mismatch (%s != %s)",
					get->hostid, row[0]);
			}
			if (get->numimages)
				dump_host_authinfo(fd, row[0], "GET", get);
			emulab_free_host_authinfo(get);
			if (put->numimages)
				dump_host_authinfo(fd, row[0], "PUT", put);
			emulab_free_host_authinfo(put);
		}
	}
	mysql_free_result(res);
}

struct config emulab_config = {
	emulab_deinit,
	emulab_read,
	emulab_get_host_authinfo,
	emulab_free_host_authinfo,
	emulab_get_server_address,
	emulab_save,
	emulab_restore,
	emulab_free,
	emulab_dump
};

struct config *
emulab_init(void)
{
	static int called;
	char pathbuf[PATH_MAX], *path;
	int len;

	if (!dbinit())
		return NULL;

	if (called)
		return &emulab_config;
	called++;

	/* One time parsing of MC address info */
	if (sscanf(MC_BASEADDR, "%d.%d.%d", &mc_a, &mc_b, &mc_c) != 3) {
		error("emulab_init: MC_BASEADDR '%s' not in A.B.C format!",
		      MC_BASEADDR);
		return NULL;
	}
	mc_port_lo = atoi(MC_BASEPORT);
	mc_port_num = atoi(MC_NUMPORTS);
	if (mc_port_num < 0 || mc_port_num >= 65536) {
		error("emulab_init: MC_NUMPORTS '%s' not in valid range!",
		      MC_NUMPORTS);
		return NULL;
	}

	if ((path = realpath(SHAREROOT_DIR, pathbuf)) == NULL) {
		error("emulab_init: could not resolve '%s'", SHAREROOT_DIR);
		return NULL;
	}
	SHAREDIR = mystrdup(path);

	if ((path = realpath(PROJROOT_DIR, pathbuf)) == NULL) {
		error("emulab_init: could not resolve '%s'", PROJROOT_DIR);
		return NULL;
	}
	PROJDIR = mystrdup(path);

	if ((path = realpath(GROUPSROOT_DIR, pathbuf)) == NULL) {
		error("emulab_init: could not resolve '%s'", GROUPSROOT_DIR);
		return NULL;
	}
	GROUPSDIR = mystrdup(path);

	if ((path = realpath(USERSROOT_DIR, pathbuf)) == NULL) {
		error("emulab_init: could not resolve '%s'", USERSROOT_DIR);
		return NULL;
	}
	USERSDIR = mystrdup(path);

	if (strlen(SCRATCHROOT_DIR) > 0) {
		if ((path = realpath(SCRATCHROOT_DIR, pathbuf)) == NULL) {
			error("emulab_init: could not resolve '%s'",
			      SCRATCHROOT_DIR);
			return NULL;
		}
		SCRATCHDIR = mystrdup(path);
	} else
		SCRATCHDIR = NULL;

	/*
	 * Construct the standard image directory path.
	 * XXX Should we run realpath on this? I don't think so.
	 */
	len = strlen(TBROOT) + strlen("/images") + 1;
	STDIMAGEDIR = mymalloc(len);
	snprintf(STDIMAGEDIR, len, "%s/images", TBROOT);

	return &emulab_config;
}

/*
 * XXX memory allocation functions that either return memory or abort.
 * We shouldn't run out of memory and don't want to check every return values.
 */
static void *
mymalloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr == NULL) {
		error("config_emulab: out of memory!");
		abort();
	}
	return ptr;
}

static void *
myrealloc(void *ptr, size_t size)
{
	void *nptr = realloc(ptr, size);
	if (nptr == NULL) {
		error("config_emulab: out of memory!");
		abort();
	}
	return nptr;
}

static char *
mystrdup(const char *str)
{
	char *nstr = strdup(str);
	if (nstr == NULL) {
		error("config_emulab: out of memory!");
		abort();
	}
	return nstr;
}

#else
struct config *
emulab_init(void)
{
	return NULL;
}
#endif
