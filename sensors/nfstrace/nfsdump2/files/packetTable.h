/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _packet_table_h
#define _packet_table_h

#include <stdio.h>
#include <sys/types.h>

#include "nfs_prot.h"

#define PACKET_TABLE_SIZE 31

typedef struct _packetTrack {
    struct _packetTrack *pt_next;
    u_int32_t pt_host;
    u_int32_t pt_xid;
    u_int32_t pt_version;
    char pt_fh[96];
    int pt_fh_len;
    int pt_count;
    u_int32_t pt_secs;
    u_int32_t pt_usecs;
    int pt_bytes;

    char pt_fattr[13 * sizeof(u_int32_t) + 4 * sizeof(u_int64_t)];
} packetTrack_t;

typedef struct _packetTable {
    packetTrack_t *pt_tracks[PACKET_TABLE_SIZE];
} packetTable_t;

packetTable_t *ptCreateTable(void);
void ptDeleteTable(packetTable_t *pt);

unsigned int ptHash(u_int32_t host, char *fh, int fh_len);
packetTrack_t *ptLookupTrack(packetTable_t *pt, u_int32_t host,
			     char *fh, int fh_len, int *hash_out);
int ptUpdateTrack(packetTable_t *pt, u_int32_t host, char *fh, int fh_len,
		  u_int32_t secs, u_int32_t usecs, u_int32_t version,
		  u_int32_t xid, u_int32_t len, int *hash_out);
void ptUpdateTrackAttr(packetTable_t *pt, u_int32_t host, u_int32_t hash,
		       u_int32_t xid, u_int32_t *fa, u_int32_t *fae);
void ptDumpTable(FILE *file, packetTable_t *pt, char *proc);
void ptClearTable(packetTable_t *pt);

#endif
