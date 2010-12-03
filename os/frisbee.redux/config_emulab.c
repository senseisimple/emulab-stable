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
#include <assert.h>
#include <mysql/mysql.h>
#include "log.h"
#include "configdefs.h"

/* Emulab includes */
#include "tbdb.h"	/* the DB library */
#include "config.h"	/* the defs-* defines */

/* Extra info associated with a image information entry */
struct emulab_ii_extra_info {
	int DB_imageid;
};

/* Extra info associated with a host authentication entry */
struct emulab_ha_extra_info {
	char *pid;	/* project */
	char *gid;	/* group */
	char *eid;	/* experiment */
	char *suid;	/* swapper user name */
};

static char *MC_BASEADDR = FRISEBEEMCASTADDR;
static char *MC_BASEPORT = FRISEBEEMCASTPORT;
static char *SHAREDIR	 = SHAREROOT_DIR;
static char *PROJDIR	 = PROJROOT_DIR;
static char *GROUPSDIR	 = GROUPSROOT_DIR;
static char *USERSDIR	 = USERSROOT_DIR;
static char *SCRATCHDIR	 = SCRATCHROOT_DIR;

#ifndef ELABINELAB
/* XXX should be autoconfiged as part of Emulab build */
static char *IMAGEDIR    = "/usr/testbed/images";
#endif

/* Emit aliases when dumping the config info; makes it smaller */
static int dump_doaliases = 1;

/* Multicast address/port base info */
static int mc_a, mc_b, mc_c, mc_port;

/* Memory alloc functions that abort when no memory */
static void *mymalloc(size_t size);
static void *myrealloc(void *ptr, size_t size);
static char *mystrdup(const char *str);

static int
emulab_init(void)
{
	static int called;
	char pathbuf[PATH_MAX], *path;

	if (!dbinit())
		return -1;

	if (called)
		return 0;
	called++;

	/* One time parsing of MC address info */
	if (sscanf(MC_BASEADDR, "%d.%d.%d", &mc_a, &mc_b, &mc_c) != 3) {
		error("emulab_init: MC_BASEADDR '%s' not in A.B.C format!",
		      MC_BASEADDR);
		return -1;
	}
	mc_port = atoi(MC_BASEPORT);

	if ((path = realpath(SHAREROOT_DIR, pathbuf)) == NULL) {
		error("emulab_init: could not resolve '%s'", SHAREROOT_DIR);
		return -1;
	}
	SHAREDIR = mystrdup(path);

	if ((path = realpath(PROJROOT_DIR, pathbuf)) == NULL) {
		error("emulab_init: could not resolve '%s'", PROJROOT_DIR);
		return -1;
	}
	PROJDIR = mystrdup(path);

	if ((path = realpath(GROUPSROOT_DIR, pathbuf)) == NULL) {
		error("emulab_init: could not resolve '%s'", GROUPSROOT_DIR);
		return -1;
	}
	GROUPSDIR = mystrdup(path);

	if ((path = realpath(USERSROOT_DIR, pathbuf)) == NULL) {
		error("emulab_init: could not resolve '%s'", USERSROOT_DIR);
		return -1;
	}
	USERSDIR = mystrdup(path);

	if (strlen(SCRATCHROOT_DIR) > 0) {
		if ((path = realpath(SCRATCHROOT_DIR, pathbuf)) == NULL) {
			error("emulab_init: could not resolve '%s'",
			      SCRATCHROOT_DIR);
			return -1;
		}
		SCRATCHDIR = mystrdup(path);
	} else
		SCRATCHDIR = NULL;

	return 0;
}

static void
emulab_deinit(void)
{
	dbclose();
}

static int
emulab_read(void)
{
	/* "Reading" the config file is a no-op. */
	return 0;
}

static void *
emulab_save(void)
{
	static int dummy;

	/* Just return non-zero value */
	return (void *)&dummy;
}

static int
emulab_restore(void *state)
{
	return 0;
}

static void
emulab_free(void *state)
{
}

/*
 * Set the allowed GET methods.
 * XXX for now, just multicast.
 */
void
set_get_methods(struct config_host_authinfo *ai, int ix)
{
	ai->imageinfo[ix].get_methods = CONFIG_IMAGE_MCAST;
}

/*
 * Set the GET options for a particular node/image.
 * XXX right now these are implied by various bits of state in the DB:
 *
 * running in elabinelab:      54Mb/sec, keepalive of 15 sec.
 * loading a "standard" image: 72Mb/sec
 * any other image:            54Mb/sec
 */
void
set_get_options(struct config_host_authinfo *ai, int ix)
{
	char str[256];

	strcpy(str, "");
#ifdef ELABINELAB
	strcat(str, " -W 54000000 -K 15");
#else
	if (!strncmp(ai->imageinfo[ix].path, IMAGEDIR, strlen(IMAGEDIR)))
		strcat(str, " -W 72000000");
	else
		strcat(str, " -W 54000000");
#endif

	/*
	 * We use a small server inactive timeout since we no longer have
	 * to start up a frisbeed well in advance of the client(s).
	 */
	strcat(str, " -T 60");

	ai->imageinfo[ix].get_options = mystrdup(str);
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
		FREE(ei->suid);
		free(ai->extra);
	}
	free(ai);
}

/*
 * Return the IP address/port to be used by the server/clients for
 * the image listed in ai->imageinfo[0].  Methods lists one or more transfer
 * methods that the client can handle, we return the method chosen.
 *
 * For Emulab, we use the frisbee_index from the DB along with the base
 * multicast address and port to compute a unique address/port.  Uses the
 * same logic that frisbeelauncher used to use.
 * XXX right now we only do multicast.
 *
 * Return zero on success, non-zero otherwise.
 */
static int
emulab_get_server_address(struct config_host_authinfo *ai, int methods,
			  in_addr_t *addrp, in_port_t *portp, int *methp)
{
	int	  idx;
	int	  a, b, c, d;

	assert(ai->numimages == 1 && ai->imageinfo != NULL);

	if ((methods & CONFIG_IMAGE_MCAST) == 0) {
		error("get_server_address: only support MCAST right now!");
		return 1;
	}

	if (!mydb_update("UPDATE emulab_indicies "
			" SET idx=LAST_INSERT_ID(idx+1) "
			" WHERE name='frisbee_index'")) {
		error("get_server_address: DB update error!");
		return 1;
	}
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

	*methp = CONFIG_IMAGE_MCAST;
	*addrp = (a << 24) | (b << 16) | (c << 8) | d;
	*portp = mc_port + (((c << 8) | d) & 0x7FFFF);

	return 0;
}

/*
 * Return one if 'path' is in 'dir'; i.e., is 'dir' a prefix of 'path'?
 * Note: returns zero if path == dir.
 * Note: assumes realpath has been run on both.
 */
static int
isindir(char *dir, char *path)
{
	int len = dir ? strlen(dir) : 0;
	int rv = 1;

	if (dir == NULL || path == NULL || *dir == '\0' || *path == '\0' ||
	    strncmp(dir, path, len) || path[len] != '/')
		rv = 0;
	if (0)
		fprintf(stderr, "isindir(dir=%s, path=%s) => %d\n",
			dir ? dir : "", path ? path : "", rv);
	return rv;
}

/*
 * Check and see if the given image is in the set of standard directories
 * to which this node (actually pid/gid/eid/uid) has access.  The set is:
 *  - /share		      (GET only)
 *  - /proj/<pid>	      (GET only right now)
 *  - in /groups/<pid>/<gid>  (GET only right now)
 *  - in /users/<swapper-uid> (GET only right now)
 * and, if it exists:
 *  - in /scratch/<pid>	      (GET or PUT)
 *
 * We assume the get and put structures have been initialized, in particular
 * that they contain the "extra" info which has the pid/gid/eid/uid.
 */
static void
allow_stddirs(char *imageid, 
	      struct config_host_authinfo *get,
	      struct config_host_authinfo *put)
{
	char tpath[PATH_MAX];
	char *fpath, *shdir, *pdir, *gdir, *scdir, *udir;
	int doput = 0, doget = 0;
	struct emulab_ha_extra_info *ei;
	struct config_imageinfo *ci;
	struct stat sb;

	if (get == NULL && put == NULL)
		return;

	ei = get ? get->extra : put->extra;
	fpath = NULL;

	/*
	 * Construct the allowed directories for this pid/gid/eid/uid.
	 */
	shdir = SHAREDIR;
	snprintf(tpath, sizeof tpath, "%s/%s", PROJDIR, ei->pid);
	pdir = mystrdup(tpath);

	snprintf(tpath, sizeof tpath, "%s/%s/%s", GROUPSDIR, ei->pid, ei->gid);
	gdir = mystrdup(tpath);

	snprintf(tpath, sizeof tpath, "%s/%s", USERSDIR, ei->suid);
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
		int ni, i;
		size_t ns;
		char *dirs[8];

		/*
		 * Right now, only allow PUT to scratchdir if it exists.
		 */
		if (put != NULL && scdir) {
			dirs[0] = scdir;
			ni = put->numimages + 1;
			ns = ni * sizeof(struct config_imageinfo);
			if (put->imageinfo)
				put->imageinfo = myrealloc(put->imageinfo, ns);
			else
				put->imageinfo = mymalloc(ns);
			for (i = put->numimages; i < ni; i++) {
				ci = &put->imageinfo[i];
				ci->imageid = NULL;
				ci->path = mystrdup(dirs[i - put->numimages]);
				ci->flags = CONFIG_PATH_ISDIR;
				if (stat(ci->path, &sb) == 0) {
					ci->sig = mymalloc(sizeof(time_t));
					*(time_t *)ci->sig = sb.st_mtime;
					ci->flags |= CONFIG_SIG_ISMTIME;
				} else
					ci->sig = NULL;
				ci->get_options = NULL;
				ci->get_methods = 0;
				ci->put_options = NULL;
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
				ci->path = mystrdup(dirs[i - get->numimages]);
				ci->flags = CONFIG_PATH_ISDIR;
				if (stat(ci->path, &sb) == 0) {
					ci->sig = mymalloc(sizeof(time_t));
					*(time_t *)ci->sig = sb.st_mtime;
					ci->flags |= CONFIG_SIG_ISMTIME;
				} else
					ci->sig = NULL;
				set_get_options(get, i);
				set_get_methods(get, i);
				ci->put_options = NULL;
				ci->extra = NULL;
			}
			get->numimages = ni;
		}
		goto done;
	}

	/*
	 * Image was specified; find the real path for the targetted file.
	 * Don't want users symlinking to files outside their allowed space.
	 */
	if (realpath(imageid, tpath) == NULL)
		goto done;
	fpath = mystrdup(tpath);

	/*
	 * Make the appropriate access checks for get/put
	 */
	if (put != NULL &&
	    (isindir(scdir, fpath)))
		doput = 1;
	if (get != NULL &&
	    (isindir(shdir, fpath) ||
	     isindir(pdir, fpath) ||
	     isindir(gdir, fpath) ||
	     isindir(scdir, fpath) ||
	     isindir(udir, fpath)))
		doget = 1;

	if (doput) {
		assert(put->imageinfo == NULL);

		put->imageinfo = mymalloc(sizeof(struct config_imageinfo));
		put->numimages = 1;
		ci = &put->imageinfo[0];
		ci->imageid = mystrdup(imageid);
		ci->path = mystrdup(fpath);
		ci->flags = CONFIG_PATH_ISFILE;
		if (stat(ci->path, &sb) == 0) {
			ci->sig = mymalloc(sizeof(time_t));
			*(time_t *)ci->sig = sb.st_mtime;
			ci->flags |= CONFIG_SIG_ISMTIME;
		} else
			ci->sig = NULL;
		ci->get_options = NULL;
		ci->get_methods = 0;
		ci->put_options = NULL;
		ci->extra = NULL;
	}
	if (doget) {
		assert(get->imageinfo == NULL);

		get->imageinfo = mymalloc(sizeof(struct config_imageinfo));
		get->numimages = 1;
		ci = &get->imageinfo[0];
		ci->imageid = mystrdup(imageid);
		ci->path = mystrdup(fpath);
		ci->flags = CONFIG_PATH_ISFILE;
		if (stat(ci->path, &sb) == 0) {
			ci->sig = mymalloc(sizeof(time_t));
			*(time_t *)ci->sig = sb.st_mtime;
			ci->flags |= CONFIG_SIG_ISMTIME;
		} else
			ci->sig = NULL;
		set_get_options(get, 0);
		set_get_methods(get, 0);
		ci->put_options = NULL;
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
 * Find all images (imageid==NULL) or a specific image (imageid!=NULL)
 * that a particular node can access for GET/PUT.  At any time, a node is
 * associated with a specific project and group.  These determine the
 * ability to GET/PUT an image.  For the all image case, the list of
 * accessible images will include:
 *
 * - all "global" images (GET only)
 * - all "shared" images in the project and any group (GET only)
 * - all images in the project and group of the node
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
 */
static int
emulab_get_host_authinfo(struct in_addr *in, char *imageid,
			    struct config_host_authinfo **getp,
			    struct config_host_authinfo **putp)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		*node;
	int		i, nrows;
	char		*wantpid = NULL, *wantname = NULL;
	struct config_host_authinfo *get = NULL, *put = NULL;

	if (getp) {
		get = mymalloc(sizeof *get);
		memset(get, 0, sizeof *get);
	}
	if (putp) {
		put = mymalloc(sizeof *put);
		memset(put, 0, sizeof(*put));
	}
	/* Find the node name from its control net IP */
	res = mydb_query("SELECT node_id"
			 " FROM interfaces"
			 " WHERE IP='%s' AND role='ctrl'",
			 1, inet_ntoa(*in));
	assert(res != NULL);

	/* No such node */
	if (mysql_num_rows(res) == 0) {
		if (getp) *getp = get;
		if (putp) *putp = put;
		return 0;
	}

	row = mysql_fetch_row(res);
	if (!row[0]) {
		error("config_host_authinfo: null node_id!?");
		mysql_free_result(res);
		emulab_free_host_authinfo(get);
		emulab_free_host_authinfo(put);
		return 1;
	}
	node = mystrdup(row[0]);
	if (get != NULL)
		get->hostid = mystrdup(node);
	if (put != NULL)
		put->hostid = mystrdup(node);
	mysql_free_result(res);

	/* Find the pid/gid to which the node is currently assigned */
	res = mydb_query("SELECT e.pid,e.gid,r.eid,u.uid"
			 " FROM reserved AS r,experiments AS e,users AS u"
			 " WHERE r.pid=e.pid AND r.eid=e.eid"
			 "  AND e.swapper_idx=u.uid_idx"
			 "  AND r.node_id='%s'", 4, node);
	assert(res != NULL);

	/* Node is free */
	if (mysql_num_rows(res) == 0) {
		free(node);
		if (getp) *getp = get;
		if (putp) *putp = put;
		return 0;
	}

	row = mysql_fetch_row(res);
	if (!row[0] || !row[1] || !row[2] || !row[3]) {
		error("config_host_authinfo: null pid/gid/eid/uid!?");
		mysql_free_result(res);
		emulab_free_host_authinfo(get);
		emulab_free_host_authinfo(put);
		free(node);
		return 1;
	}

	if (get != NULL) {
		struct emulab_ha_extra_info *ei = mymalloc(sizeof *ei);
		ei->pid = mystrdup(row[0]);
		ei->gid = mystrdup(row[1]);
		ei->eid = mystrdup(row[2]);
		ei->suid = mystrdup(row[3]);
		get->extra = ei;
	}
	if (put != NULL) {
		struct emulab_ha_extra_info *ei = mymalloc(sizeof *ei);
		ei->pid = mystrdup(row[0]);
		ei->gid = mystrdup(row[1]);
		ei->eid = mystrdup(row[2]);
		ei->suid = mystrdup(row[3]);
		put->extra = ei;
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
	}

	if (put != NULL) {
		struct emulab_ha_extra_info *ei = put->extra;

		if (imageid != NULL) {
			/* Interested in a specific image */
			res = mydb_query("SELECT pid,gid,imagename,path,imageid"
					 " FROM images"
					 " WHERE pid='%s' AND gid='%s'"
					 "  AND pid='%s' AND imagename='%s'"
					 " ORDER BY pid,gid,imagename",
					 5, ei->pid, ei->gid,
					 wantpid, wantname);
		} else {
			/* Find all images that this pid/gid can PUT */
			res = mydb_query("SELECT pid,gid,imagename,path,imageid"
					 " FROM images"
					 " WHERE pid='%s' AND gid='%s'"
					 " ORDER BY pid,gid,imagename",
					 5, ei->pid, ei->gid);
		}

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
			char *imageid;

			row = mysql_fetch_row(res);
			/* XXX ignore rows with null or empty info */
			if (!row[0] || !row[0][0] ||
			    !row[1] || !row[1][0] ||
			    !row[2] || !row[2][0] ||
			    !row[3] || !row[3][0] ||
			    !row[4] || !row[4][0])
				continue;
			imageid = mymalloc(strlen(row[0])+strlen(row[2])+2);
			strcpy(imageid, row[0]);
			strcat(imageid, "/");
			strcat(imageid, row[2]);
			ci = &put->imageinfo[put->numimages];
			ci->imageid = imageid;
			ci->path = mystrdup(row[3]);
			ci->flags = CONFIG_PATH_ISFILE;
			if (stat(ci->path, &sb) == 0) {
				ci->sig = mymalloc(sizeof(time_t));
				*(time_t *)ci->sig = sb.st_mtime;
				ci->flags |= CONFIG_SIG_ISMTIME;
			} else
				ci->sig = NULL;
			ci->get_methods = 0;
			ci->get_options = NULL;
			ci->put_options = NULL;
			ii = mymalloc(sizeof *ii);
			ii->DB_imageid = atoi(row[4]);
			ci->extra = ii;
			put->numimages++;
		}
		mysql_free_result(res);
	}

	if (get != NULL) {
		struct emulab_ha_extra_info *ei = get->extra;

		if (imageid != NULL) {
			/* Interested in a specific image */
			res = mydb_query("SELECT pid,gid,imagename,path,imageid"
					 " FROM images"
					 " WHERE (global=1"
					 "  OR (pid='%s' AND (gid='%s' OR shared=1)))"
					 "  AND pid='%s' AND imagename='%s'"
					 " ORDER BY pid,gid,imagename",
					 5, ei->pid, ei->gid,
					 wantpid, wantname);
		} else {
			/* Find all images that this pid/gid can GET */
			res = mydb_query("SELECT pid,gid,imagename,path,imageid"
					 " FROM images"
					 " WHERE (global=1"
					 "  OR (pid='%s' AND (gid='%s' OR shared=1)))"
					 " ORDER BY pid,gid,imagename",
					 5, ei->pid, ei->gid);
		}
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
			char *imageid;

			row = mysql_fetch_row(res);
			/* XXX ignore rows with null or empty info */
			if (!row[0] || !row[0][0] ||
			    !row[1] || !row[1][0] ||
			    !row[2] || !row[2][0] ||
			    !row[3] || !row[3][0] ||
			    !row[4] || !row[4][0])
				continue;
			imageid = mymalloc(strlen(row[0])+strlen(row[2])+2);
			strcpy(imageid, row[0]);
			strcat(imageid, "/");
			strcat(imageid, row[2]);
			ci = &get->imageinfo[get->numimages];
			ci->imageid = imageid;
			ci->path = mystrdup(row[3]);
			ci->flags = CONFIG_PATH_ISFILE;
			if (stat(ci->path, &sb) == 0) {
				ci->sig = mymalloc(sizeof(time_t));
				*(time_t *)ci->sig = sb.st_mtime;
				ci->flags |= CONFIG_SIG_ISMTIME;
			} else
				ci->sig = NULL;
			set_get_methods(get, get->numimages);
			set_get_options(get, get->numimages);
			ci->put_options = NULL;
			ii = mymalloc(sizeof *ii);
			ii->DB_imageid = atoi(row[4]);
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

	lastpid = strdup("NONE");
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
			lastpid = strdup(row[0]);

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
			if (emulab_get_host_authinfo(&in, NULL, &get, &put)) {
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
	emulab_init,
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

#if 0
/*
 * XXX Gak! Override some logging functions that the tbdb mysql code uses.
 * Otherwise we are going to have log messages going every which way.
 */
void
info(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	log_vwrite(LOG_INFO, fmt, args);
	va_end(args);
}

void
error(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	log_vwrite(LOG_ERR, fmt, args);
	va_end(args);
}

void
warning(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	log_vwrite(LOG_ERR, fmt, args);
	va_end(args);
}

#endif
#endif
