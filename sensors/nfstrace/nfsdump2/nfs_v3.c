/*
 * $Id: nfs_v3.c,v 1.4 2006-02-02 16:16:17 stack Exp $
 *
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

#include	<ctype.h>

#include "nfsrecord.h"

#define	PRINT_STATUS(s, p)	\
	if (p) { if ((s) == NFS3_OK) { fprintf (OutFile, "OK "); } \
		else { fprintf (OutFile, "%x ", (s)); } \
	}


static	char	BigBuf0 [2 * 1024];
static	char	BigBuf1 [2 * 1024];
static	char	BigBuf2 [2 * 1024];
static	char	BigBuf3 [2 * 1024];


int print_diropargs3_x (diropargs3 *d, char *fh_str, char *fn_str);
int print_fh3_x (nfs_fh3 *f, char *fh_str);
int print_size3_x (size3 *s);
int print_sattr3_x (sattr3 *s);
int print_sattrguard3_x (sattrguard3 *s);
int print_nfstime3_x (nfstime3 *t);
int print_access3_x (u_int32_t a);
int print_uint64_x (u_int32_t *p, char *label);
u_int32_t print_createhow3_x (createhow3 *how);
int print_stable_how_x (u_int32_t how);
int print_string (char *str, int len);

static int compute_pct (int n, int d);

/*
 * Print the contents of an NFS RPC call, or at least as much as
 * possible.
 *
 * The op and xid have already been extraced from the RPC header, and
 * p points to the start of the RPC payload (the stuff after the call
 * header).
 */

int nfs_v3_print_call (nfs_pkt_t *record, u_int32_t op, u_int32_t xid,
		u_int32_t *p, u_int32_t payload_len, u_int32_t actual_len,
		nfs_v3_stat_t *stats)
{
	int rc = 0;
	u_int32_t *e, *new_p;
	XDR xdr;
	int got_all = 1;

	/*
	 * This could fail if the alignment is bad.  Otherwise, it's
	 * just a little bit too conservative.
	 */

	xdrmem_create (&xdr, (void *) p, actual_len, XDR_DECODE);

	e = p + (actual_len / 4);

	stats->c3_total++;

	if (op != NFSPROC3_READ && op != NFSPROC3_WRITE)
	    fprintf (OutFile, "C3 %-8x %2x ", xid, op);

	switch (op) {
	case NFSPROC3_NULL :	/* OK */
		stats->c3_null++;
		fprintf (OutFile, "%-8s ", "null");
		break;

	case NFSPROC3_GETATTR : { /* OK */
		GETATTR3args args;

		stats->c3_getattr++;
		bzero ((void *) &args, sizeof (args));
		args.object.data.data_val = BigBuf0;

		fprintf (OutFile, "%-8s ", "getattr");

		if (got_all = xdr_GETATTR3args (&xdr, &args)) {
			print_fh3_x (&args.object, "fh");
		}
		break;
	}

	case NFSPROC3_SETATTR : {	/* OK */
		SETATTR3args args;

		stats->c3_setattr++;
		bzero ((void *) &args, sizeof (args));
		args.object.data.data_val = BigBuf0;

		fprintf (OutFile, "%-8s ", "setattr");

		if (got_all = xdr_SETATTR3args (&xdr, &args)) {
			print_fh3_x (&args.object, "fh");
			print_sattr3_x (&args.new_attributes);
			print_sattrguard3_x (&args.guard);
		}

		break;
	}

	case NFSPROC3_LOOKUP : {	/* OK */
		LOOKUP3args args;

		stats->c3_lookup++;
		bzero ((void *) &args, sizeof (args));
		args.what.dir.data.data_val = BigBuf0;
		args.what.name = BigBuf1;

		fprintf (OutFile, "%-8s ", "lookup");

		if (got_all = xdr_LOOKUP3args (&xdr, &args)) {
			print_diropargs3_x (&(args.what), "fh", "name");
		}

		break;
	}

	case NFSPROC3_ACCESS : {	/* Probably OK */
		ACCESS3args args;

		stats->c3_access++;
		bzero ((void *) &args, sizeof (args));
		args.object.data.data_val = BigBuf0;

		fprintf (OutFile, "%-8s ", "access");

		if (got_all = xdr_ACCESS3args (&xdr, &args)) {
			print_fh3_x (&(args.object), "fh");
			print_access3_x (args.access);
		}

		break;
	}

	case NFSPROC3_READLINK : {
		READLINK3args args;

		stats->c3_readlink++;
		bzero ((void *) &args, sizeof (args));
		args.symlink.data.data_val = BigBuf0;

		fprintf (OutFile, "%-8s ", "readlink");

		if (got_all = xdr_READLINK3args (&xdr, &args)) {
			print_fh3_x (&(args.symlink), "fh");
		}

		break;
	}

	case NFSPROC3_READ : {		/* OK */
		READ3args args;

		stats->c3_read++;
		bzero ((void *) &args, sizeof (args));
		args.file.data.data_val = BigBuf0;

		// fprintf (OutFile, "%-8s ", "read");

		if (got_all = xdr_READ3args (&xdr, &args)) {
			// print_fh3_x (&(args.file), "fh");
			ptUpdateTrack(readTable,
				      record->srcHost,
				      args.file.data.data_val,
				      args.file.data.data_len,
				      record->secs, record->usecs,
				      record->nfsVersion, record->rpcXID,
				      args.count,
				      &record->pthash);

			// print_uint64_x ((u_int32_t *) &args.offset, "off");
			// fprintf (OutFile, "count %x ", args.count);
		}

		break;
	}

	case NFSPROC3_WRITE : {		/* OK */
		nfs_fh3	file;
		offset3 offset;
		count3 count;
		stable_how how;

		stats->c3_write++;
		// fprintf (OutFile, "%-8s ", "write");

		/*
		 * We can't just gulp down the entire args to write,
		 * because they might have an arbitrary amount of data
		 * following.  Therefore, we get the args one field at
		 * a time.
		 */

		bzero ((void *) &file, sizeof (file));
		file.data.data_val = BigBuf0;

		if (got_all = xdr_nfs_fh3 (&xdr, &file)) {
			// print_fh3_x (&file, "fh");
		}
		if (got_all = xdr_offset3 (&xdr, &offset)) {
			// print_uint64_x ((u_int32_t *) &offset, "off");
		}
		if (got_all = xdr_count3 (&xdr, &count)) {
			// fprintf (OutFile, "count %x ", count);
			ptUpdateTrack(writeTable,
				      record->srcHost,
				      file.data.data_val,
				      file.data.data_len,
				      record->secs, record->usecs,
				      record->nfsVersion, record->rpcXID,
				      count,
				      &record->pthash);

			stats->c3_write_b += count;
			if (stats->c3_write_b >= (1024 * 1024)) {
				stats->c3_write_m += stats->c3_write_b / (1024 * 1024);
				stats->c3_write_b %= 1024 * 1024;
			}
		}
		if (got_all = xdr_stable_how (&xdr, &how)) {
			// print_stable_how_x (how);
		}

		break;
	}

	case NFSPROC3_CREATE : {	/* OK */
		CREATE3args args;

		stats->c3_create++;
		fprintf (OutFile, "%-8s ", "create");

		bzero ((void *) &args, sizeof (args));
		args.where.dir.data.data_val = BigBuf0;
		args.where.name = BigBuf1;

		if (got_all = xdr_CREATE3args (&xdr, &args)) {
			print_diropargs3_x (&args.where, "fh", "name");
			print_createhow3_x (&args.how);
		}

		break;
	}

	case NFSPROC3_MKDIR : {
		MKDIR3args args;

		stats->c3_mkdir++;
		fprintf (OutFile, "%-8s ", "mkdir");

		bzero ((void *) &args, sizeof (args));
		args.where.dir.data.data_val = BigBuf0;
		args.where.name = BigBuf1;
		
		if (got_all = xdr_MKDIR3args (&xdr, &args)) {
			print_diropargs3_x (&args.where, "fh", "name");
			print_sattr3_x (&args.attributes);
		}

		break;
	}


	case NFSPROC3_SYMLINK : {
		SYMLINK3args args;

		stats->c3_symlink++;
		fprintf (OutFile, "%-8s ", "symlink");

		bzero ((void *) &args, sizeof (args));
		args.where.dir.data.data_val = BigBuf0;
		args.where.name = BigBuf1;
		args.symlink.symlink_data = BigBuf2;

		if (got_all = xdr_SYMLINK3args (&xdr, &args)) {
			print_diropargs3_x (&args.where, "fh", "name");
			print_sattr3_x (&args.symlink.symlink_attributes);
			fprintf (OutFile, "%s ", "sdata");
			print_string (args.symlink.symlink_data,
					strlen (args.symlink.symlink_data));
		}

		break;
	}

	case NFSPROC3_MKNOD : {
		MKNOD3args args;

		stats->c3_mknod++;
		bzero ((void *) &args, sizeof (args));
		args.where.dir.data.data_val = BigBuf0;
		args.where.name = BigBuf1;

		fprintf (OutFile, "%-8s ", "mknod");

		if (got_all = xdr_MKNOD3args (&xdr, &args)) {
			print_diropargs3_x (&(args.where), "fh", "name");

			/*
			 * There's more, but I'm not going to worry
			 * about it for now.
			 */

		}

		break;
	}

	case NFSPROC3_REMOVE : {	/* OK */
		REMOVE3args args;

		stats->c3_remove++;
		fprintf (OutFile, "%-8s ", "remove");

		bzero ((void *) &args, sizeof (args));
		args.object.dir.data.data_val = BigBuf0;
		args.object.name = BigBuf1;

		if (got_all = xdr_REMOVE3args (&xdr, &args)) {
			print_diropargs3_x (&(args.object), "fh", "name");
		}

		break;
	}

	case NFSPROC3_RMDIR : {
		RMDIR3args args;

		stats->c3_rmdir++;
		fprintf (OutFile, "%-8s ", "rmdir");

		bzero ((void *) &args, sizeof (args));
		args.object.dir.data.data_val = BigBuf0;
		args.object.name = BigBuf1;

		if (got_all = xdr_RMDIR3args (&xdr, &args)) {
			print_diropargs3_x (&(args.object), "fh", "name");
		}

		break;
	}

	case NFSPROC3_RENAME : {
		RENAME3args args;

		stats->c3_rename++;
		fprintf (OutFile, "%-8s ", "rename");

		bzero ((void *) &args, sizeof (args));
		args.from.dir.data.data_val = BigBuf0;
		args.from.name = BigBuf1;
		args.to.dir.data.data_val = BigBuf2;
		args.from.name = BigBuf3;

		if (got_all = xdr_RENAME3args (&xdr, &args)) {
			print_diropargs3_x (&args.from, "fh", "name");
			print_diropargs3_x (&args.to, "fh2", "name2");
		}

		break;
	}

	case NFSPROC3_LINK : {
		LINK3args args;

		stats->c3_link++;
		fprintf (OutFile, "%-8s ", "link");

		bzero ((void *) &args, sizeof (args));
		args.file.data.data_val = BigBuf0;
		args.link.dir.data.data_val = BigBuf1;
		args.link.name = BigBuf2;

		if (got_all = xdr_LINK3args (&xdr, &args)) {
			print_fh3_x (&args.file, "fh");
			print_diropargs3_x (&args.link, "fh2", "name");
		}

		break;
	}

	case NFSPROC3_READDIR : {		/* OK */
		READDIR3args args;

		stats->c3_readdir++;
		fprintf (OutFile, "%-8s ", "readdir");

		bzero ((void *) &args, sizeof (args));
		args.dir.data.data_val = BigBuf0;

		if (got_all = xdr_READDIR3args (&xdr, &args)) {
			print_fh3_x (&args.dir, "fh");
			print_uint64_x ((u_int32_t *) &args.cookie, "cookie");
			fprintf (OutFile, "count %x ", args.count);
		}

		break;
	}

	case NFSPROC3_READDIRPLUS : {
		READDIRPLUS3args args;

		stats->c3_readdirp++;
		fprintf (OutFile, "%-8s ", "readdirp");

		bzero ((void *) &args, sizeof (args));
		args.dir.data.data_val = BigBuf0;

		if (got_all = xdr_READDIRPLUS3args (&xdr, &args)) {
			print_fh3_x (&args.dir, "fh");
			print_uint64_x ((u_int32_t *) &args.cookie, "cookie");
			fprintf (OutFile, "count %x ", args.dircount);
			fprintf (OutFile, "maxcnt %x ", args.maxcount);
		}

		break;
	}

	case NFSPROC3_FSSTAT : {
		FSSTAT3args args;

		stats->c3_fsstat++;
		fprintf (OutFile, "%-8s ", "fsstat");

		bzero ((void *) &args, sizeof (args));
		args.fsroot.data.data_val = BigBuf0;

		if (got_all = xdr_FSSTAT3args (&xdr, &args)) {
			print_fh3_x (&args.fsroot, "fh");
		}

		break;
	}

	case NFSPROC3_FSINFO : {
		FSINFO3args args;

		stats->c3_fsinfo++;
		fprintf (OutFile, "%-8s ", "fsinfo");

		bzero ((void *) &args, sizeof (args));
		args.fsroot.data.data_val = BigBuf0;

		if (got_all = xdr_FSINFO3args (&xdr, &args)) {
			print_fh3_x (&args.fsroot, "fh");
		}


		break;
	}

	case NFSPROC3_PATHCONF : {
		PATHCONF3args args;

		stats->c3_pathconf++;
		fprintf (OutFile, "%-8s ", "pathconf");

		bzero ((void *) &args, sizeof (args));
		args.object.data.data_val = BigBuf0;

		if (got_all = xdr_PATHCONF3args (&xdr, &args)) {
			print_fh3_x (&args.object, "fh");
		}

		break;
	}

	case NFSPROC3_COMMIT : {
		COMMIT3args args;

		stats->c3_commit++;
		fprintf (OutFile, "%-8s ", "commit");

		bzero ((void *) &args, sizeof (args));
		args.file.data.data_val = BigBuf0;

		if (got_all = xdr_COMMIT3args (&xdr, &args)) {
			print_fh3_x (&args.file, "file");
			print_uint64_x ((u_int32_t *) &args.offset, "off");
			fprintf (OutFile, "count %x ", args.count);
		}

		break;
	}

	default :
		fprintf (OutFile, "unknown ");
		rc = -1;

	}

	xdr_destroy (&xdr);

	if (! got_all) {
		fprintf (OutFile, " error 1 ");
	}

	return (rc);
}

/*
 * Print the contents of an NFS RPC response.
 *
 * The xid has already been extraced from the RPC header, and the op
 * inferred from it.  p points to the start of the RPC payload (the
 * stuff after the call header).
 */

int nfs_v3_print_resp (nfs_pkt_t *record, u_int32_t op, u_int32_t xid,
		u_int32_t *p, u_int32_t payload_len, u_int32_t actual_len,
		nfsstat3 status,
		nfs_v3_stat_t *stats)
{
	u_int32_t *e, *fh_start;
	u_int32_t *new_p = p;
	u_int32_t count;

	/*
	 * This could fail if the alignment is bad.  Otherwise, it's
	 * just a little bit too conservative.
	 */

	e = p + (actual_len / 4);

	if (op != NFSPROC3_READ && op != NFSPROC3_WRITE)
		fprintf (OutFile, "R3 %8x %2x ", xid, op);

	stats->r3_total++;

	switch (op) {
		case NFSPROC3_NULL :
			stats->r3_null++;
			fprintf (OutFile, "%-8s ", "null");
			break;

		case NFSPROC3_GETATTR :
			stats->r3_getattr++;
			fprintf (OutFile, "%-8s ", "getattr");
			PRINT_STATUS (status, 1);
			if (status == NFS3_OK) {
				new_p = print_fattr3 (p, e, 1);
			}
			break;

		case NFSPROC3_SETATTR :
			stats->r3_setattr++;
			fprintf (OutFile, "%-8s ", "setattr");
			PRINT_STATUS (status, 1);
			if (status == NFS3_OK) {
				new_p = print_wcc_data3 (p, e, 1);
			}
			break;

		case NFSPROC3_LOOKUP :
			stats->r3_lookup++;
			fprintf (OutFile, "%-8s ", "lookup");
			PRINT_STATUS (status, 1);
			if (status == NFS3_OK) {
				fprintf (OutFile, "fh ");
				new_p = print_fh3 (p, e, 1);
				new_p = print_post_op_attr3 (new_p, e, 1);
				new_p = print_post_op_attr3 (new_p, e, 0);
			}
			break;

		case NFSPROC3_ACCESS :
			stats->r3_access++;
			fprintf (OutFile, "%-8s ", "access");
			PRINT_STATUS (status, 1);
			new_p = print_post_op_attr3 (p, e, 0);
			if (status == NFS3_OK) {
				fprintf (OutFile, "acc ");
				new_p = print_uint32 (new_p, e, 1, NULL);
			}
			break;

		case NFSPROC3_READLINK :
			stats->r3_readlink++;
			fprintf (OutFile, "%-8s ", "readlink");
			PRINT_STATUS (status, 1);
			new_p = print_post_op_attr3 (p, e, 0);
			if (status == NFS3_OK) {
				fputs ("name ", OutFile);
				new_p = print_fn3 (new_p, e, 1);
			}
			break;

		case NFSPROC3_READ :
			stats->r3_read++;
			fprintf (OutFile, "%-8s ", "read");
			PRINT_STATUS (status, 1);
			new_p = print_post_op_attr3 (p, e, 1);
			if (status == NFS3_OK) {
				fprintf (OutFile, "count ");
				new_p = print_count3 (new_p, e, 1, &count);

				stats->r3_read_b += count;
				if (stats->r3_read_b >= (1024 * 1024)) {
					stats->r3_read_m += stats->r3_read_b / (1024 * 1024);
					stats->r3_read_b %= 1024 * 1024;
				}

				fprintf (OutFile, "eof ");
				new_p = print_uint32 (new_p, e, 1, NULL);
			}
			break;

		case NFSPROC3_WRITE :
			stats->r3_write++;
			// fprintf (OutFile, "%-8s ", "write");
			// PRINT_STATUS (status, 1);
			// new_p = print_wcc_data3 (p, e, 1);
			p++;
			if ((new_p = print_pre_op_attr3 (p, e, 0))
			    != NULL) {
			    new_p++;
			    ptUpdateTrackAttr(writeTable,
					      record->dstHost,
					      record->pthash,
					      record->rpcXID,
					      new_p,
					      e);
			}
			if (status == NFS3_OK) {
				// fprintf (OutFile, "count ");
				// new_p = print_uint32 (new_p, e, 1, NULL);
				// fprintf (OutFile, "stable ");
				// new_p = print_stable3 (new_p, e, 1);
				/* there's more, but we'll skip it */
			}
			break;

		case NFSPROC3_CREATE :
			stats->r3_create++;
			fprintf (OutFile, "%-8s ", "create");
			PRINT_STATUS (status, 1);
			if (status == NFS3_OK) {
				fputs ("fh ", OutFile);
				new_p = print_post_op_fh3 (p, e, 1);
				new_p = print_post_op_attr3 (new_p, e, 1);
				/* Skip the directory attributes */
			}
			else {
				/* Skip the directory attributes */
			}
			break;

		case NFSPROC3_MKDIR :
			stats->r3_mkdir++;
			fprintf (OutFile, "%-8s ", "mkdir");
			PRINT_STATUS (status, 1);
			if (status == NFS3_OK) {
				fputs ("fh ", OutFile);
				new_p = print_post_op_fh3 (p, e, 1);
				new_p = print_post_op_attr3 (new_p, e, 1);
				/* Skip the directory attributes */
			}
			break;

		case NFSPROC3_SYMLINK :
			stats->r3_symlink++;
			fprintf (OutFile, "%-8s ", "symlink");
			PRINT_STATUS (status, 1);
			if (status == NFS3_OK) {
				fputs ("fh ", OutFile);
				new_p = print_post_op_fh3 (p, e, 1);
				new_p = print_post_op_attr3 (new_p, e, 1);
				/* Skip the directory attributes */
			}
			break;

		case NFSPROC3_MKNOD :
			stats->r3_mknod++;
			fprintf (OutFile, "%-8s ", "mknod");
			PRINT_STATUS (status, 1);
			if (status == NFS3_OK) {
				fputs ("fh ", OutFile);
				new_p = print_post_op_fh3 (p, e, 1);
				new_p = print_post_op_attr3 (new_p, e, 1);
				/* Skip the directory attributes */
			}
			break;

		case NFSPROC3_REMOVE :
			stats->r3_remove++;
			fprintf (OutFile, "%-8s ", "remove");
			PRINT_STATUS (status, 1);
			new_p = print_wcc_data3 (p, e, 1);
			break;

		case NFSPROC3_RMDIR :
			stats->r3_rmdir++;
			fprintf (OutFile, "%-8s ", "rmdir");
			PRINT_STATUS (status, 1);
			new_p = print_wcc_data3 (p, e, 1);
			break;

		case NFSPROC3_RENAME :
			stats->r3_rename++;
			fprintf (OutFile, "%-8s ", "rename");
			PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_LINK :
			stats->r3_link++;
			fprintf (OutFile, "%-8s ", "link");
			PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_READDIR :
			stats->r3_readdir++;
			fprintf (OutFile, "%-8s ", "readdir");
			PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_READDIRPLUS :
			stats->r3_readdirp++;
			fprintf (OutFile, "%-8s ", "readdirp");
			PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_FSSTAT :
			stats->r3_fsstat++;
			fprintf (OutFile, "%-8s ", "fsstat");
			PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_FSINFO :
			stats->r3_fsinfo++;
			fprintf (OutFile, "%-8s ", "fsinfo");
			PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_PATHCONF :
			stats->r3_pathconf++;
			fprintf (OutFile, "%-8s ", "pathconf");
			PRINT_STATUS (status, 1);
			/* skip the rest */
			break;

		case NFSPROC3_COMMIT :
			stats->r3_commit++;
			fprintf (OutFile, "%-8s ", "commit");
			PRINT_STATUS (status, 1);
			new_p = print_wcc_data3 (p, e, 1);
			/* skip the rest */
			break;

		default :
			fprintf (OutFile, "%-8s ", "unknown");
			break;

	}

	if (new_p == NULL) {
	    // fprintf (OutFile, "SHORT PACKET");
		return (-1);
	}

	return (0);
}

int print_diropargs3_x (diropargs3 *d, char *fh_str, char *fn_str)
{

	print_fh3_x (&(d->dir), fh_str);
	fprintf (OutFile, "%s ", fn_str);
	print_string (d->name, strlen (d->name));

	return (0);
}

int print_fh3_x (nfs_fh3 *f, char *fh_str)
{
	u_int32_t i;

	fprintf (OutFile, "%s ", fh_str);
	for (i = 0; i < f->data.data_len; i++) {
		fprintf (OutFile, "%.2x", 0xff & f->data.data_val [i]);
	}
	fprintf (OutFile, " ");

	return (0);
}

u_int32_t *print_fh3 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t i;
	u_int32_t fh_len;
	u_int32_t n_words;

	if (p == NULL) {
		return (NULL);
	}

	/*
	 * Gotta be at least two words-- one for the length, and one
	 * for some actual data.  (usually it's quite a bit longer,
	 * however...).
	 */

	if (e < (p + 2)) {
		return (NULL);
	}

	fh_len = (u_int32_t) ntohl (*p++);

	n_words = fh_len / 4;

	if (fh_len & 3) {
		n_words++;
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

u_int32_t *print_o_nfstime3 (u_int32_t *p, u_int32_t *e, int print,
		char *label)
{
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}

	if (e < p) {
		return (NULL);
	}

	switch (ntohl (*new_p++)) {
		case DONT_CHANGE :
			break;
		case SET_TO_SERVER_TIME :
			break;
		case SET_TO_CLIENT_TIME :
			if (print) fprintf (OutFile, "%s ", label);
			new_p = print_nfstime3 (new_p, e, print);
			break;
	}

	return (new_p);
}

u_int32_t *print_o_uint32 (u_int32_t *p, u_int32_t *e, int print,
		char *label)
{
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}

	if (e < p) {
		return (NULL);
	}

	if (*new_p++) {
		fprintf (OutFile, "%s ", label);
		new_p = print_uint32 (new_p, e, print, NULL);
	}

	return (new_p);
}

u_int32_t *print_o_uint64 (u_int32_t *p, u_int32_t *e, int print,
		char *label)
{
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}

	if (e < p) {
		return (NULL);
	}

	if (*new_p++) {
		fprintf (OutFile, "%s ", label);
		new_p = print_uint64 (new_p, e, print);
	}

	return (new_p);
}


u_int32_t *print_nfstime3 (u_int32_t *p, u_int32_t *e, int print)
{

	if (p == NULL) {
		return (NULL);
	}

	if (e < (p + 2)) {
		return (NULL);
	}

	if (print) {

		/*
		 * For NFS v3, time is measured in nanoseconds.  This
		 * is cool in it's own way, but few system clocks can
		 * come close to actually measuring things at this
		 * granularity, and it's not really needed anyway. 
		 * Therefore, we round down to microseconds.
		 */

		u_int32_t	secs	= ntohl (p [0]);
		u_int32_t	usecs	= ntohl (p [1]) / 1000;

		fprintf (OutFile, "%u.%.6u ", secs, usecs);
	}

	return (p + 2);
}

int print_size3_x (size3 *s)
{
	u_int32_t hi, lo;
	u_int32_t *p = (u_int32_t *) s;

	/* This might be endian-specific. -DJE */
	hi = p [1];
	lo = p [0];

	if (hi != 0) {
		fprintf (OutFile, "size %x%08x ", hi, lo);
	}
	else {
		fprintf (OutFile, "size %x ", lo);
	}

	return (0);
}

int print_sattr3_x (sattr3 *s)
{

	if (s->mode.set_it) {
		fprintf (OutFile, "mode %x ", s->mode.set_mode3_u.mode);
	}
	if (s->uid.set_it) {
		fprintf (OutFile, "uid %d ", s->uid.set_uid3_u.uid);
	}
	if (s->gid.set_it) {
		fprintf (OutFile, "gid %d ", s->gid.set_gid3_u.gid);
	}
	if (s->size.set_it) {
		print_uint64_x ((u_int32_t *) &(s->size.set_size3_u.size),
				"size");
	}
	if (s->atime.set_it == SET_TO_CLIENT_TIME) {
		fprintf (OutFile, "atime ");
		print_nfstime3_x (&(s->atime.set_atime_u.atime));
	}
	else if (s->atime.set_it == SET_TO_SERVER_TIME) {
		fprintf (OutFile, "atime SERVER ");
	}

	if (s->mtime.set_it == SET_TO_CLIENT_TIME) {
		fprintf (OutFile, "mtime ");
		print_nfstime3_x (&(s->mtime.set_mtime_u.mtime));
	}
	else if (s->mtime.set_it == SET_TO_SERVER_TIME) {
		fprintf (OutFile, "mtime SERVER ");
	}

	return (0);
}

int print_sattrguard3_x (sattrguard3 *s)
{

	if (s->check) {
		fprintf (OutFile, "ctime ");
		print_nfstime3_x (&(s->sattrguard3_u.obj_ctime));
	}

	return (0);
}

int print_nfstime3_x (nfstime3 *t)
{
	/*
	 * For NFS v3, time is measured in nanoseconds.  This
	 * is cool in it's own way, but few system clocks can
	 * come close to actually measuring things at this
	 * granularity, and it's not really needed anyway. 
	 * Therefore, we round down to microseconds.
	 */

	fprintf (OutFile, "%u.%.6u ", t->seconds, t->nseconds / 1000);

	return (0);
}

u_int32_t *print_sattr3 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}

	/*
	 * This is ugly, but unavoidable.  The whole structure
	 * is composed of variable-length substructures.  You
	 * have to pluck apart the whole thing in order to
	 * find each particular piece.
	 */

	new_p = print_o_uint32 (new_p, e, print, "mode");	/* mode */
	new_p = print_o_uint32 (new_p, e, print, "uid");	/* uid */
	new_p = print_o_uint32 (new_p, e, print, "gid");	/* gid */
	new_p = print_o_uint64 (new_p, e, print, "size");	/* size */

	new_p = print_o_nfstime3 (new_p, e, print, "atime");
	new_p = print_o_nfstime3 (new_p, e, print, "mtime");

	return (new_p);
}

u_int32_t *print_sattrguard3 (u_int32_t *p, u_int32_t *e, int print)
{

	return (print_o_nfstime3 (p, e, print, "ctime"));
}


u_int32_t *print_fn3 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t s_len, tot_len;
	u_char *str;

	if ((p == NULL) || (e < (p + 1))) {
		return (NULL);
	}

	tot_len = s_len = ntohl (*p++);
	str = (u_char *) p;

	/*
	 * &&& Making assumptions about sizeof (u_int32_t) == 4, in
	 * order to do some bit fidding.
	 */

	if (tot_len & 3) {
		tot_len &= ~3;
		tot_len += 4;
	}

	if (e < (u_int32_t *) (str + tot_len)) {
		fprintf (OutFile, " \"...\" ");
		return (NULL);
	}

	if (print) {
		print_string ((char *) str, s_len);
	}

	return ((u_int32_t *) (str + tot_len));
}

int print_string (char *str, int len)
{
	unsigned int i;
	int c;

	fprintf (OutFile, "\"");
	for (i = 0; i < len; i++) {
		c = str [i];

		if (c == '"' || c == '\\' || isspace (c)) { 
			fprintf (OutFile, "\\%.2x", 0xff & c);
		}
		else {
			fprintf (OutFile, "%c", str [i]);
		}
	}
	fprintf (OutFile, "\" ");

	return (0);
}


int print_access3_x (u_int32_t a)
{

	fprintf (OutFile, "acc %x ", a);

	return (0);
}


u_int32_t *print_uint32 (u_int32_t *p, u_int32_t *e, int print, u_int32_t *val)
{

	if ((p == NULL) || (e < p)) {
		return (NULL);
	}

	if (print) {
		fprintf (OutFile, "%x ", ntohl (*p));
	}

	if (val != NULL) {
		*val = ntohl (*p);
	}

	return (p + 1);
}

u_int32_t *print_uint64 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t hi, lo;

	if ((p == NULL) || (e < (p + 1))) {
		return (NULL);
	}

	if (print) {
		hi = ntohl (p [0]);
		lo = ntohl (p [1]);

		if (hi != 0) {
			fprintf (OutFile, "%x%08x ", hi, lo);
		}
		else {
			fprintf (OutFile, "%x ", lo);
		}
	}

	return (p + 2);
}

int print_uint64_x (u_int32_t *p, char *label)
{
	u_int32_t hi, lo;

#ifdef WORDS_BIGENDIAN
	hi = p [0];
	lo = p [1];
#else
	hi = p [1];
	lo = p [0];
#endif

	if (label != NULL) {
		fprintf (OutFile, "%s ", label);
	}

	if (hi != 0) {
		fprintf (OutFile, "%x%08x ", hi, lo);
	}
	else {
		fprintf (OutFile, "%x ", lo);
	}

	return (0);
}

u_int32_t *print_diropargs3  (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t *new_p;

	new_p = print_fh3 (p, e, print);
	if (print) {
		fprintf (OutFile, "name ");
	}
	new_p = print_fn3 (new_p, e, print);

	return (new_p);
}

u_int32_t *print_symlink3 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t *new_p;

	new_p = print_sattr3 (p, e, print);
	new_p = print_nfspath3 (new_p, e, print);

	return (new_p);
}

int print_stable_how_x (u_int32_t how)
{
	u_char c;

	switch (how) {
		case UNSTABLE	: c = 'U'; break;
		case DATA_SYNC	: c = 'D'; break;
		case FILE_SYNC	: c = 'F'; break;
		default		: c = '?'; break;
	}

	fprintf (OutFile, "stable %c ", c);

	return (0);
}

u_int32_t *print_stable3 (u_int32_t *p, u_int32_t *e, int print)
{

	if (p == NULL) {
		return (NULL);
	}

	if (p < e) {
		if (print) {
			u_char c;

			switch (ntohl (p [0])) {
				case 0 : c = 'U'; break;
				case 1 : c = 'D'; break;
				case 2 : c = 'F'; break;
				default : c = '?'; break;
			}

			fprintf (OutFile, "%c ", c);
		}
		return (p + 1);
	}
	else {
		return (NULL);
	}
}

u_int32_t *print_mknoddata3 (u_int32_t *p, u_int32_t *e, int print)
{

	if (p == NULL) {
		return (NULL);
	}

	if (e < p) {
		return (NULL);
	}

	print_uint32 (p, e, print, NULL);

	switch (ntohl (*p++)) {

		case NF3BLK :
			if (print) fprintf (OutFile, "BLK ");
			p = print_sattr3 (p, e, print);
			p = print_uint32 (p, e, print, NULL);
			p = print_uint32 (p, e, print, NULL);
			break;

		case NF3CHR :
			if (print) fprintf (OutFile, "CHR ");
			p = print_sattr3 (p, e, print);
			p = print_uint32 (p, e, print, NULL);
			p = print_uint32 (p, e, print, NULL);
			break;

		case NF3SOCK :
			if (print) fprintf (OutFile, "SOCK ");
			p = print_sattr3 (p, e, print);
			break;

		case NF3FIFO :
			if (print) fprintf (OutFile, "FIFO ");
			p = print_sattr3 (p, e, print);
			break;

		default :
			if (print) fprintf (OutFile, "??? ");
			break;

	}

	return (p);
}

u_int32_t print_createhow3_x (createhow3 *how)
{

	switch (how->mode) {
		case UNCHECKED :
			fprintf (OutFile, "how U ");
			print_sattr3_x (&(how->createhow3_u.obj_attributes));
			break;

		case GUARDED :
			fprintf (OutFile, "how G ");
			print_sattr3_x (&(how->createhow3_u.obj_attributes));
			break;

		case EXCLUSIVE :
			fprintf (OutFile, "how X ");
			break;
	}

	return (0);
}

u_int32_t *print_fattr3 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}

	if (e < p) {
		return (NULL);
	}

	if (print) fprintf (OutFile, "ftype ");
	new_p = print_uint32 (new_p, e, print, NULL);	/* ftype */
	if (print) fprintf (OutFile, "mode ");
	new_p = print_uint32 (new_p, e, print, NULL);	/* mode */
	if (print) fprintf (OutFile, "nlink ");
	new_p = print_uint32 (new_p, e, print, NULL);	/* nlink */
	if (print) fprintf (OutFile, "uid ");
	new_p = print_uint32 (new_p, e, print, NULL);	/* uid */
	if (print) fprintf (OutFile, "gid ");
	new_p = print_uint32 (new_p, e, print, NULL);	/* gid */
	if (print) fprintf (OutFile, "size ");
	new_p = print_uint64 (new_p, e, print);	/* size */
	if (print) fprintf (OutFile, "used ");
	new_p = print_uint64 (new_p, e, print);	/* used */

	if (print) fprintf (OutFile, "rdev ");
	new_p = print_uint32 (new_p, e, print, NULL);	/* specdata 1 */
	if (print) fprintf (OutFile, "rdev2 ");
	new_p = print_uint32 (new_p, e, print, NULL);	/* specdata 2 */

	if (print) fprintf (OutFile, "fsid ");
	new_p = print_uint64 (new_p, e, print);	/* fsid */
	if (print) fprintf (OutFile, "fileid ");
	new_p = print_uint64 (new_p, e, print);	/* fileid */


	if (print) fprintf (OutFile, "atime ");
	new_p = print_nfstime3 (new_p, e, print);	/* atime */
	if (print) fprintf (OutFile, "mtime ");
	new_p = print_nfstime3 (new_p, e, print);	/* mtime */
	if (print) fprintf (OutFile, "ctime ");
	new_p = print_nfstime3 (new_p, e, print);	/* ctime */

	return (new_p);
}

u_int32_t *print_post_op_attr3 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}
	if (e < p) {
		return (NULL);
	}

	if (*p) {
		return (print_fattr3 (p + 1, e, print));
	}
	else {
		return (p + 1);
	}
}

u_int32_t *print_post_op_fh3 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}
	if (e < p) {
		return (NULL);
	}

	if (*p) {
		return (print_fh3 (p + 1, e, print));
	}
	else {
		return (p + 1);
	}
}

u_int32_t *print_wcc_data3 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}
	if (e < p) {
		return (NULL);
	}

	if (*new_p++) {
		new_p = print_pre_op_attr3 (new_p, e, print);
	}
	if (*new_p++) {
		new_p = print_fattr3 (new_p, e, print);
	}

	return (new_p);
}

u_int32_t *print_pre_op_attr3 (u_int32_t *p, u_int32_t *e, int print)
{
	u_int32_t *new_p = p;

	if (p == NULL) {
		return (NULL);
	}

	if (e < p) {
		return (NULL);
	}

	if (print) {
		fprintf (OutFile, "pre-size ");
	}
	new_p = print_uint64 (new_p, e, print);

	if (print) {
		fprintf (OutFile, "pre-mtime ");
	}
	new_p = print_nfstime3 (new_p, e, print);

	if (print) {
		fprintf (OutFile, "pre-ctime ");
	}
	new_p = print_nfstime3 (new_p, e, print);

	return (new_p);
}

void nfs_v3_stat_init (nfs_v3_stat_t *p)
{

	memset ((void *) p, 0, sizeof (nfs_v3_stat_t));
}

void nfs_v3_stat_print (nfs_v3_stat_t *p, FILE *out)
{

	fprintf (out, "v3 %-8s  %12d %12d\n", "total",
			p->c3_total, p->r3_total);	

	fprintf (out, "v3 %-8s  %12d %12d %% %2d  %10.2fM\n", "read",
			p->c3_read, p->r3_read,
			compute_pct (p->c3_read, p->c3_total),
			(double) p->r3_read_m + (p->r3_read_b / (1024.0 * 1024.0)));

	fprintf (out, "v3 %-8s  %12d %12d %% %2d  %10.2fM\n", "write",
			p->c3_write, p->r3_write,
			compute_pct (p->c3_write, p->c3_total),
			(double) p->c3_write_m + (p->c3_write_b / (1024.0 * 1024.0)));

	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "getattr", p->c3_getattr, p->r3_getattr,
			compute_pct (p->c3_getattr, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "lookup", p->c3_lookup, p->r3_lookup,
			compute_pct (p->c3_lookup, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "readdir", p->c3_readdir, p->r3_readdir,
			compute_pct (p->c3_readdir, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "fsstat", p->c3_fsstat, p->r3_fsstat,
			compute_pct (p->c3_fsstat, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "access", p->c3_access, p->r3_access,
			compute_pct (p->c3_access, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "setattr", p->c3_setattr, p->r3_setattr,
			compute_pct (p->c3_setattr, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "remove", p->c3_remove, p->r3_remove,
			compute_pct (p->c3_remove, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "create", p->c3_create, p->r3_create,
			compute_pct (p->c3_create, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "readlink", p->c3_readlink, p->r3_readlink,
			compute_pct (p->c3_readlink, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "mkdir", p->c3_mkdir, p->r3_mkdir,
			compute_pct (p->c3_mkdir, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "symlink", p->c3_symlink, p->r3_symlink,
			compute_pct (p->c3_symlink, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "rmdir", p->c3_rmdir, p->r3_rmdir,
			compute_pct (p->c3_rmdir, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "rename", p->c3_rename, p->r3_rename,
			compute_pct (p->c3_rename, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "link", p->c3_link, p->r3_link,
			compute_pct (p->c3_link, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "pathconf", p->c3_pathconf, p->r3_pathconf,
			compute_pct (p->c3_pathconf, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "fsinfo", p->c3_fsinfo, p->r3_fsinfo,
			compute_pct (p->c3_fsinfo, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "mknod", p->c3_mknod, p->r3_mknod,
			compute_pct (p->c3_mknod, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "null", p->c3_null, p->r3_null,
			compute_pct (p->c3_null, p->c3_total));
	fprintf (out, "v3 %-8s  %12d %12d %% %2d\n", "commit", p->c3_commit, p->r3_commit,
			compute_pct (p->c3_commit, p->c3_total));


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
 * end of nfs_v3.c
 */

