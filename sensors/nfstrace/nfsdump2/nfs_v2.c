/*
 * $Id: nfs_v2.c,v 1.4 2006-02-02 16:16:17 stack Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <rpc/rpc.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pcap.h>

#include "interface.h"
#include "addrtoname.h"
#include "ethertype.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"
/* #include "nfs.h" */

#include "nfsrecord.h"

static int compute_pct (int n, int d);

#define	PRINT_STATUS(s, p)	\
	if (p) { if ((s) == NFS_OK) { fprintf (OutFile, "OK "); } \
		else { fprintf (OutFile, "%d ", (s)); } \
	}

/*
 * Print the contents of an NFS RPC call, or at least as much as
 * possible.
 *
 * The op and xid have already been extraced from the RPC header, and
 * p points to the start of the RPC payload (the stuff after the call
 * header).
 */

int nfs_v2_print_call (nfs_pkt_t *record, u_int32_t op, u_int32_t xid,
		u_int32_t *p, u_int32_t payload_len, u_int32_t actual_len,
		nfs_v2_stat_t *stats)
{
	u_int32_t *e, *fh_start;
	u_int32_t *new_p = p;
	u_int32_t count;

	/*
	 * This could fail if the alignment is bad.  Otherwise, it's
	 * just a little bit too conservative.
	 */

	e = p + (actual_len / 4);

	stats->c2_total++;

	if (op != NFSPROC_READ && op != NFSPROC_WRITE)
	    fprintf (OutFile, "C2 %-8x %2x ", xid, op);

	switch (op) {
		case NFSPROC_NULL :
			stats->c2_null++;
			fprintf (OutFile, "%-8s ", "null");
			break;

		case NFSPROC_GETATTR :
			stats->c2_getattr++;
			fprintf (OutFile, "%-8s ", "getattr");
			new_p = print_fh2 (p, e, 1, "fh");
			break;

		case NFSPROC_SETATTR :
			stats->c2_setattr++;
			fprintf (OutFile, "%-8s ", "setattr");
			new_p = print_fh2 (p, e, 1, "fh");
			new_p = print_sattr2 (new_p, e, 1);
			break;

		case NFSPROC_ROOT :
			stats->c2_root++;
			fprintf (OutFile, "%-8s ", "root");
			break;

		case NFSPROC_LOOKUP :
			stats->c2_lookup++;
			fprintf (OutFile, "%-8s ", "lookup");
			new_p = print_diropargs2 (p, e, 1, "fh", "fn");
			break;

		case NFSPROC_READLINK :
			stats->c2_readlink++;
			fprintf (OutFile, "%-8s ", "readlink");
			new_p = print_fh2 (p, e, 1, "fh");
			break;

		case NFSPROC_READ :
			stats->c2_read++;
			// fprintf (OutFile, "%-8s ", "read");
			fh_start = p;
			new_p = print_fh2 (p, e, 0, "fh");
			// fprintf (OutFile, "offset ");
			new_p = print_offset2 (new_p, e, 0);
			// fprintf (OutFile, "count ");
			new_p = print_count2 (new_p, e, 0, &count);
			// fprintf (OutFile, "tcount ");
			new_p = print_count2 (new_p, e, 0, NULL);

			if (new_p != NULL) {
			    ptUpdateTrack(readTable,
					  record->srcHost,
					  (char *)fh_start, NFS_FHSIZE,
					  record->secs, record->usecs,
					  record->nfsVersion, record->rpcXID,
					  count,
					  &record->pthash);
			}

			break;

		case NFSPROC_WRITECACHE :
			stats->c2_writecache++;
			fprintf (OutFile, "%-8s ", "writecach");
			break;

		case NFSPROC_WRITE :
			stats->c2_write++;
			// fprintf (OutFile, "%-8s ", "write");
			fh_start = p;
			new_p = print_fh2 (p, e, 0, "fh");
			// fprintf (OutFile, "begoff ");
			new_p = print_offset2 (new_p, e, 0);
			// fprintf (OutFile, "offset ");
			new_p = print_offset2 (new_p, e, 0);
			// fprintf (OutFile, "tcount ");
			new_p = print_count2 (new_p, e, 0, &count);

			if (new_p != NULL) {
			    ptUpdateTrack(writeTable,
					  record->srcHost,
					  (char *)fh_start, NFS_FHSIZE,
					  record->secs, record->usecs,
					  record->nfsVersion, record->rpcXID,
					  count,
					  &record->pthash);
			}

			stats->c2_write_b += count;
			if (stats->c2_write_b >= (1024 * 1024)) {
				stats->c2_write_m += stats->c2_write_b / (1024 * 1024);
				stats->c2_write_b %= 1024 * 1024;
			}

			break;

		case NFSPROC_CREATE :
			stats->c2_create++;
			fprintf (OutFile, "%-8s ", "create");
			new_p = print_diropargs2 (p, e, 1, "fh", "fn");
			new_p = print_sattr2 (new_p, e, 1);
			break;

		case NFSPROC_REMOVE :
			stats->c2_remove++;
			fprintf (OutFile, "%-8s ", "remove");
			new_p = print_diropargs2 (p, e, 1, "fh", "fn");
			break;

		case NFSPROC_RENAME :
			stats->c2_rename++;
			fprintf (OutFile, "%-8s ", "rename");
			new_p = print_diropargs2 (p, e, 1, "fh", "fn");
			new_p = print_diropargs2 (new_p, e, 1, "fh2", "fn2");
			break;

		case NFSPROC_LINK :
			stats->c2_link++;
			fprintf (OutFile, "%-8s ", "link");
			new_p = print_fh2 (p, e, 1, "fh");
			new_p = print_diropargs2 (new_p, e, 1, "fh2", "fn");
			break;

		case NFSPROC_SYMLINK :
			stats->c2_symlink++;
			fprintf (OutFile, "%-8s ", "symlink");
			new_p = print_diropargs2 (p, e, 1, "fh", "fn");
			new_p = print_fn2 (new_p, e, 1, "fn2");
			new_p = print_sattr2 (new_p, e, 1);
			break;

		case NFSPROC_MKDIR :
			stats->c2_mkdir++;
			fprintf (OutFile, "%-8s ", "mkdir");
			new_p = print_diropargs2 (p, e, 1, "fh", "fn");
			new_p = print_sattr2 (new_p, e, 1);
			break;

		case NFSPROC_RMDIR :
			stats->c2_rmdir++;
			fprintf (OutFile, "%-8s ", "rmdir");
			new_p = print_diropargs2 (p, e, 1, "fh", "fn");
			break;

		case NFSPROC_READDIR :
			stats->c2_readdir++;
			fprintf (OutFile, "%-8s ", "readdir");
			new_p = print_fh2 (p, e, 1, "fh");
			new_p = print_cookie2 (new_p, e, 1);
			fprintf (OutFile, "count ");
			new_p = print_count2 (new_p, e, 1, NULL);
			break;

		case NFSPROC_STATFS :
			stats->c2_statfs++;
			fprintf (OutFile, "%-8s ", "statfs");
			new_p = print_fh2 (p, e, 1, "fh");
			break;

		default :
			stats->c2_unknown++;
			fprintf (OutFile, "?? ");
			return (-1);

	}

	if (new_p == NULL) {
		fprintf (OutFile, " error 1 ");
	}

	return (0);
}

/*
 * Print the contents of an NFS RPC response.
 *
 * The op and xid have already been extraced from the RPC header, and
 * p points to the start of the RPC payload (the stuff after the call
 * header).
 */

int nfs_v2_print_resp (nfs_pkt_t *record, u_int32_t op, u_int32_t xid,
		u_int32_t *p, u_int32_t payload_len, u_int32_t actual_len,
		nfsstat status, nfs_v2_stat_t *stats)
{
	u_int32_t *e;
	u_int32_t *new_p = p;
	u_int32_t count;

	/*
	 * This could fail if the alignment is bad.  Otherwise, it's
	 * just a little bit too conservative.
	 */

	e = p + (actual_len / 4);

	if (op != NFSPROC_READ && op != NFSPROC_WRITE)
	    fprintf (OutFile, "R2 %-8x %2x ", xid, op);

	stats->r2_total++;

	switch (op) {
		case NFSPROC_NULL :
			stats->r2_null++;
			fprintf (OutFile, "%-8s ", "null");
			break;

		case NFSPROC_GETATTR :
			stats->r2_getattr++;
			fprintf (OutFile, "%-8s ", "getattr");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				new_p = print_fattr2 (p, e, 1);
			}
			break;

		case NFSPROC_SETATTR :
			stats->r2_setattr++;
			fprintf (OutFile, "%-8s ", "setattr");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				new_p = print_fattr2 (p, e, 1);
			}
			break;

		case NFSPROC_ROOT :
			stats->r2_root++;
			fprintf (OutFile, "%-8s ", "root");
			break;

		case NFSPROC_LOOKUP :
			stats->r2_lookup++;
			fprintf (OutFile, "%-8s ", "lookup");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				new_p = print_diropokres2 (p, e, 1);
			}
			break;

		case NFSPROC_READLINK :
			stats->r2_readlink++;
			fprintf (OutFile, "%-8s ", "readlink");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				new_p = print_fn2 (p, e, 1, "fn");
			}
			break;

		case NFSPROC_READ :
			stats->r2_read++;
			fprintf (OutFile, "%-8s ", "read");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				new_p = print_fattr2 (p, e, 1);
				fprintf (OutFile, "count ");
				new_p = print_count2 (new_p, e, 1, &count);

				stats->r2_read_b += count;
				if (stats->r2_read_b >= (1024 * 1024)) {
					stats->r2_read_m += stats->r2_read_b / (1024 * 1024);
					stats->r2_read_b %= 1024 * 1024;
				}
			}
			break;

		case NFSPROC_WRITECACHE :
			stats->r2_writecache++;
			fprintf (OutFile, "%-8s ", "writecac");
			break;

		case NFSPROC_WRITE :
			stats->r2_write++;
			// fprintf (OutFile, "%-8s ", "write");
			// PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
			    ptUpdateTrackAttr(writeTable,
					      record->dstHost,
					      record->pthash,
					      record->rpcXID,
					      p,
					      e);
				// new_p = print_fattr2 (p, e, 1);
			}
			break;

		case NFSPROC_CREATE :
			stats->r2_create++;
			fprintf (OutFile, "%-8s ", "create");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				new_p = print_diropokres2 (p, e, 1);
			}
			break;

		case NFSPROC_REMOVE :
			stats->r2_remove++;
			fprintf (OutFile, "%-8s ", "remove");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				fprintf (OutFile, "nfsstat ");
				new_p = print_uint32 (new_p, e, 1, NULL);
			}
			break;

		case NFSPROC_RENAME :
			stats->r2_rename++;
			fprintf (OutFile, "%-8s ", "rename");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				fprintf (OutFile, "nfsstat ");
				new_p = print_uint32 (new_p, e, 1, NULL);
			}
			break;

		case NFSPROC_LINK :
			stats->r2_link++;
			fprintf (OutFile, "%-8s ", "link");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				fprintf (OutFile, "nfsstat ");
				new_p = print_uint32 (new_p, e, 1, NULL);
			}
			break;

		case NFSPROC_SYMLINK :
			stats->r2_symlink++;
			fprintf (OutFile, "%-8s ", "symlink");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				fprintf (OutFile, "nfsstat ");
				new_p = print_uint32 (new_p, e, 1, NULL);
			}
			break;

		case NFSPROC_MKDIR :
			stats->r2_mkdir++;
			fprintf (OutFile, "%-8s ", "mkdir");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				new_p = print_diropokres2 (p, e, 1);
			}
			break;

		case NFSPROC_RMDIR :
			stats->r2_rmdir++;
			fprintf (OutFile, "%-8s ", "rmdir");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				fprintf (OutFile, "nfsstat ");
				new_p = print_uint32 (new_p, e, 1, NULL);
			}
			break;

		case NFSPROC_READDIR :
			stats->r2_readdir++;
			fprintf (OutFile, "%-8s ", "readdir");
			PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC_STATFS :
			stats->r2_statfs++;
			fprintf (OutFile, "%-8s ", "statfs");
			PRINT_STATUS (status, 1);
			if (status == NFS_OK) {
				new_p = print_statfsokres2 (p, e, 1);
			}
			/* skip the rest */
			break;

		default :
			stats->r2_unknown++;
			fprintf (OutFile, "?? ");
			return (-1);

	}

	if (new_p == NULL) {
		fprintf (OutFile, "+/+");
	}

	return (0);
}

u_int32_t *print_fh2 (u_int32_t *p, u_int32_t *e, int print, char *str)
{

	if (print && (str != NULL)) {
		fprintf (OutFile, "%s ", str);
	}

	return (print_opaque (p, e, print,
			NFS_FHSIZE / sizeof (u_int32_t)));
}

u_int32_t *print_sattr2 (u_int32_t *p, u_int32_t *e, int print)
{

	if (p == NULL) {
		return (NULL);
	}

	if (e < (p + 8)) {
		return (NULL);
	}

	if (print) {
		if (ntohl (p [0]) != -1) {
			fprintf (OutFile, "mode %x ", ntohl (p [0]));
		}
		if (ntohl (p [1]) != -1) {
			fprintf (OutFile, "uid %d ", ntohl (p [1]));
		}
		if (ntohl (p [2]) != -1) {
			fprintf (OutFile, "gid %d ", ntohl (p [2]));
		}
		if (ntohl (p [3]) != -1) {
			fprintf (OutFile, "size %d ", ntohl (p [3]));
		}

		/*
		 * Playing fast and loose with casting-- peeking into
		 * the first word of the nfstime struct to see if it's
		 * -1. 
		 */

		if (ntohl (p [4]) != -1) {
			fprintf (OutFile, "%s", "atime ");
			print_nfstime2 (&p [4], e, print);
		}

		if (ntohl (p [6]) != -1) {
			fprintf (OutFile, "%s", "mtime ");
			print_nfstime2 (&p [6], e, print);
		}
	}

	return (p);
}

u_int32_t *print_nfstime2 (u_int32_t *p, u_int32_t *e, int print)
{

	if ((p == NULL) || (e < p + 2)) {
		return (NULL);
	}

	if (print) {
		fprintf (OutFile, "%d.%-6d ", ntohl (p [0]), ntohl (p [1]));
	}

	return (p + 2);

}

u_int32_t *print_fn2 (u_int32_t *p, u_int32_t *e, int print, char *str)
{

	if (print && (str != NULL)) {
		fprintf (OutFile, "%s ", str);
	}

	return (print_fn3 (p, e, print));
}

u_int32_t *print_diropargs2  (u_int32_t *p, u_int32_t *e, int print,
		char *fh_str, char *fn_str)
{
	u_int32_t *new_p;

	new_p = print_fh2 (p, e, print, fh_str);
	new_p = print_fn2 (new_p, e, print, fn_str);

	return (new_p);
}

/*
u_int32_t *print_symlink2 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t *new_p;

	new_p = print_diropargs2 (p, e, print);
	new_p = print_fn2 (new_p, e, print);
	new_p = print_sattr2 (new_p, e, print);

	return (new_p);
}
*/

u_int32_t *print_cookie2 (u_int32_t *p, u_int32_t *e, int print)
{

	if (print) fprintf (OutFile, "cookie ");

	return (print_opaque (p, e, print,
			NFS_COOKIESIZE / sizeof (u_int32_t)));
}


u_int32_t *print_opaque (u_int32_t *p, u_int32_t *e, int print, int n_words)
{
	u_int32_t i;

	if (p == NULL) {
		return (NULL);
	}

	if (e < (p + n_words)) {
		return (NULL);
	}

	if (print) {
		for (i = 0; i < n_words; i++) {
			fprintf (OutFile, "%.8x", ntohl (p [i]));
		}
		fprintf (OutFile, " ");
	}

	return (p + n_words);
}

u_int32_t *print_fattr2 (u_int32_t *p, u_int32_t *e, int print)
{

	if (p == NULL) {
		return (NULL);
	}

	if (e < (p + 17)) {
		return (NULL);
	}

	if (print) {
		fprintf (OutFile, "ftype %x ", ntohl (p [0]));	/* ftype */
		fprintf (OutFile, "mode %x ", ntohl (p [1]));	/* mode */
		fprintf (OutFile, "nlink %x ", ntohl (p [2]));	/* nlink */
		fprintf (OutFile, "uid %x ", ntohl (p [3]));	/* uid */
		fprintf (OutFile, "gid %x ", ntohl (p [4]));	/* gid */
		fprintf (OutFile, "size %x ", ntohl (p [5]));	/* size */
		fprintf (OutFile, "blksize %x ", ntohl (p [6]));	/* blksize */
		fprintf (OutFile, "rdev %x ", ntohl (p [7]));	/* rdev */
		fprintf (OutFile, "blocks %x ", ntohl (p [8]));	/* blocks */
		fprintf (OutFile, "fsid %x ", ntohl (p [9]));	/* fsid */
		fprintf (OutFile, "fileid %x ", ntohl (p [10]));	/* fileid */

		fprintf (OutFile, "atime ");
		print_nfstime2 (&p [11], e, print);	/* atime */
		fprintf (OutFile, " mtime ");
		print_nfstime2 (&p [13], e, print);	/* mtime */
		fprintf (OutFile, " ctime ");
		print_nfstime2 (&p [15], e, print);	/* ctime */
		fprintf (OutFile, " ");

	}

	return (p + 17);
}

u_int32_t *print_diropokres2 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t *new_p;

	if (p == NULL) {
		return (NULL);
	}

	if (e < p) {
		return (NULL);
	}

	new_p = print_fh2 (p, e, print, "fh");
	new_p = print_fattr2 (new_p, e, print);

	return (new_p);
}

u_int32_t *print_statfsokres2 (u_int32_t *p, u_int32_t *e, int print)
{

	if (p == NULL) {
		return (NULL);
	}

	if (e < (p + 5)) {
		return (NULL);
	}

	if (print) {
		fprintf (OutFile, "tsize %x ", ntohl (p [0]));	/* tsize */
		fprintf (OutFile, "bsize %x ", ntohl (p [1]));	/* bsize */
		fprintf (OutFile, "blocks %x ", ntohl (p [2]));	/* blocks */
		fprintf (OutFile, "bfree %x ", ntohl (p [3]));	/* bfree */
		fprintf (OutFile, "bavail %x ", ntohl (p [4]));	/* bavail */

	}

	return (p + 5);
}

void nfs_v2_stat_init (nfs_v2_stat_t *p)
{

	memset ((void *) p, 0, sizeof (nfs_v2_stat_t));
}

void nfs_v2_stat_print (nfs_v2_stat_t *p, FILE *out)
{

	fprintf (out, "v2 %-8s  %12d %12d\n", "total",
			p->c2_total, p->r2_total);	

	fprintf (out, "v2 %-8s  %12d %12d %% %2d  %10.2fM\n", "read",
			p->c2_read, p->r2_read,
			compute_pct (p->c2_read, p->c2_total),
			(double) p->r2_read_m + (p->r2_read_b / (1024.0 * 1024.0)));

	fprintf (out, "v2 %-8s  %12d %12d %% %2d  %10.2fM\n", "write",
			p->c2_write, p->r2_write,
			compute_pct (p->c2_write, p->c2_total),
			(double) p->c2_write_m + (p->c2_write_b / (1024.0 * 1024.0)));

	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "getattr", p->c2_getattr, p->r2_getattr,
			compute_pct (p->c2_getattr, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "lookup", p->c2_lookup, p->r2_lookup,
			compute_pct (p->c2_lookup, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "setattr", p->c2_setattr, p->r2_setattr,
			compute_pct (p->c2_setattr, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "create", p->c2_create, p->r2_create,
			compute_pct (p->c2_create, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "remove", p->c2_remove, p->r2_remove,
			compute_pct (p->c2_remove, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "readdir", p->c2_readdir, p->r2_readdir,
			compute_pct (p->c2_readdir, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "readlink", p->c2_readlink, p->r2_readlink,
			compute_pct (p->c2_readlink, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "rename", p->c2_rename, p->r2_rename,
			compute_pct (p->c2_rename, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "link", p->c2_link, p->r2_link,
			compute_pct (p->c2_link, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "symlink", p->c2_symlink, p->r2_symlink,
			compute_pct (p->c2_symlink, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "mkdir", p->c2_mkdir, p->r2_mkdir,
			compute_pct (p->c2_mkdir, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "rmdir", p->c2_rmdir, p->r2_rmdir,
			compute_pct (p->c2_rmdir, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "statfs", p->c2_statfs, p->r2_statfs,
			compute_pct (p->c2_statfs, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "null", p->c2_null, p->r2_null,
			compute_pct (p->c2_null, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "root", p->c2_root, p->r2_root,
			compute_pct (p->c2_root, p->c2_total));
	fprintf (out, "v2 %-8s  %12d %12d %% %2d\n", "wrcache", p->c2_writecache, p->r2_writecache,
			compute_pct (p->c2_writecache, p->c2_total));

}

static int compute_pct (int n, int d)
{

	if (n == 0 || d == 0) {
		return (0);
	}
	else {
		return ((n * 100) / d);
	}
}

/*
 * end of nfs_v2.c
 */

