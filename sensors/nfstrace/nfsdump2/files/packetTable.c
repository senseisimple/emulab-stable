/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "nfsrecord.h"
#include "packetTable.h"

packetTable_t *ptCreateTable(void)
{
    packetTable_t *retval;
    
    if ((retval = calloc(1, sizeof(packetTable_t))) != NULL) {
	/* ... */
    }
    
    return( retval );
}

void ptDeleteTable(packetTable_t *pt)
{
    if (pt != NULL) {
	ptClearTable(pt);
	free(pt);
	pt = NULL;
    }
}

unsigned int ptHash(u_int32_t host, char *fh, int len)
{
    unsigned int lpc, retval = 0;

    assert(fh != NULL);

    for (lpc = 0; lpc < len; lpc++) {
	retval += fh[lpc];
    }
    retval *= host;

    retval %= PACKET_TABLE_SIZE;
    
    return( retval );
}

packetTrack_t *ptLookupTrack(packetTable_t *pt, u_int32_t host,
			     char *fh, int fh_len, int *hash_out)
{
    packetTrack_t *curr, *retval = NULL;
    int hash;
    
    assert(pt != NULL);
    assert(fh != NULL);
    assert(fh_len < 96);

    hash = ptHash(host, fh, fh_len);
    if (hash_out != NULL)
	*hash_out = hash;
    curr = pt->pt_tracks[hash];
    while ((curr != NULL) &&
	   ((curr->pt_host != host) ||
	    (curr->pt_fh_len != fh_len) ||
	    (memcmp(curr->pt_fh, fh, fh_len) != 0))) {
	curr = curr->pt_next;
    }

    if (curr != NULL)
	retval = curr;

    return( retval );
}

int ptUpdateTrack(packetTable_t *pt, u_int32_t host, char *fh, int fh_len,
		  u_int32_t secs, u_int32_t usecs, u_int32_t version,
		  u_int32_t xid, u_int32_t len, int *hash_out)
{
    packetTrack_t *track;
    int retval = 0;
    
    assert(pt != NULL);
    assert(fh != NULL);

    if ((track = ptLookupTrack(pt, host, fh, fh_len, hash_out)) == NULL) {
	if ((track = calloc(1, sizeof(packetTrack_t))) != NULL) {
	    int hash;

	    track->pt_host = host;
	    track->pt_fh_len = fh_len;
	    track->pt_xid = xid;
	    track->pt_version = version;
	    memcpy(track->pt_fh, fh, fh_len);

	    hash = ptHash(host, fh, fh_len);
	    track->pt_next = pt->pt_tracks[hash];
	    pt->pt_tracks[hash] = track;
	}
    }

    if (track != NULL) {
	track->pt_count += 1;
	track->pt_secs = secs;
	track->pt_usecs = usecs;
	track->pt_bytes += len;
	
	retval = 1;
    }

    return( retval );
}

void ptUpdateTrackAttr(packetTable_t *pt, u_int32_t host, u_int32_t hash,
		       u_int32_t xid, u_int32_t *fa, u_int32_t *fae)
{
    packetTrack_t *track;
    
    assert(pt != NULL);
    assert(fa != NULL);
    assert(fae != NULL);

    track = pt->pt_tracks[hash];
    while ((track != NULL) &&
	   (track->pt_host != host) && (track->pt_xid != xid)) {
	track = track->pt_next;
    }

    if (track != NULL) {
	int len = sizeof(u_int32_t) * (fae - fa);

	if (len > sizeof(track->pt_fattr))
	    len = sizeof(track->pt_fattr);
	memcpy(track->pt_fattr, fa, len);
    }
}

// 1136306953.992281 155.98.33.74.0801 155.98.36.135.03a8 U R3 2f074807  6 read OK ftype 1 mode 1b4 nlink 1 uid 0 gid 1900 size c21 used 1000 rdev e0 rdev2 81e 0028 fsid 38505 fileid 2019012 atime 1136306953.000000 mtime 1136306953.000000 ctime 1136306953.000000 count c21 eof 1 status=0 pl = 330 con = 70 len = 400

// 1136306953.991457 155.98.36.135.03a8 155.98.33.74.0801 U C3 2f074807  6 read     fh b3a39342d2fc3a000c00000012900102a46df21b0000000000000000 off 0 count c8e euid 0 egid 0 con = 102 len = 146

/*
  1136306955.443745 155.98.36.132.0323 155.98.33.74.0801 U C3 1698d900  7 write    fh b3a39342d2fc3a000c00000011900102e53e9b340000000000000000 off 8ef count 6d stable U euid 0 egid 0 con = 102 len = 266
  
  1136306955.444050 155.98.33.74.0801 155.98.36.132.0323 U R3 1698d900  7 write    OK pre-size 8ef pre-mtime 1136306947.000000 pre-ctime 1136306947.000000 ftype 1 mode 1b4 nlink 1 uid 0 gid 1900 size 95c used 1000 rdev e0 rdev2 81e0038 fsid 38505 fileid 2019011 atime 1136306947.000000 mtime 1136306955.000000 ctime 1136306955.000000 count 6d stable U status=0 pl = 132 con = 70 len = 202
*/

void ptDumpTable(FILE *file, packetTable_t *pt, char *proc)
{
    int lpc;
    
    assert(pt != NULL);

    for (lpc = 0; lpc < PACKET_TABLE_SIZE; lpc++) {
	while (pt->pt_tracks[lpc] != NULL) {
	    packetTrack_t *track = pt->pt_tracks[lpc];
	    u_int32_t *p = (u_int32_t *)track->pt_fattr;
	    u_int32_t *e = p + (sizeof(track->pt_fattr) / sizeof(u_int32_t));
	    u_int32_t *r;
	    
	    pt->pt_tracks[lpc] = track->pt_next;

	    fprintf(file, "%u.%.6u ", track->pt_secs, track->pt_usecs);
	    printHostIp (track->pt_host);
	    fprintf (file, ".%.4x ", 0);
	    printHostIp (0);
	    fprintf (file, ".%.4x ", 0);
	    
	    fprintf (file, "U C2 %x 0 %s fh ", 0, proc);
	    r = print_opaque((u_int32_t *)track->pt_fh,
			     (u_int32_t *)(track->pt_fh + track->pt_fh_len),
			     1,
			     track->pt_fh_len / sizeof(u_int32_t));
	    // printf ("%d\n", track->pt_fh_len / sizeof(u_int32_t));
	    // printf ("%p\n", r);

	    fprintf (file, " count %x ", track->pt_bytes);
	    fprintf (file, "pcount %d ", track->pt_count);
	    fprintf (file, "euid -1 egid -1 "); // XXX
	    
	    switch (track->pt_version) {
	    case NFS_VERSION:
		print_fattr2(p, e, 1);
		break;
	    case NFS_V3:
		print_fattr3(p, e, 1);
		break;
	    }
	    
	    fprintf (file, " con = 0 len = 0\n");

	    free(track);
	    track = NULL;
	}
    }
}

void ptClearTable(packetTable_t *pt)
{
    int lpc;
    
    assert(pt != NULL);

    for (lpc = 0; lpc < PACKET_TABLE_SIZE; lpc++) {
	while (pt->pt_tracks[lpc] != NULL) {
	    packetTrack_t *track = pt->pt_tracks[lpc];

	    pt->pt_tracks[lpc] = track->pt_next;
	    free(track);
	    track = NULL;
	}
    }
}
