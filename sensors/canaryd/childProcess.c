/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file childProcess.c
 *
 * Implementation file for the child process accounting functions.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/time.h>
#include <sys/param.h>

#include "childProcess.h"

/**
 * The file name format for the process' memory map and statistics in '/proc'.
 */
#if defined(__FreeBSD__)
static char *PROC_STAT_FORMAT = "/proc/%d/status";
static char *PROC_MAP_FORMAT = "/proc/%d/map";
#endif

/**
 * Global data for child processes.
 */
struct cpChildProcessData child_process_data;

int cpInitChildProcessData(void)
{
    int lpc, retval = 1;

    for( lpc = 0; lpc < CPD_TABLE_SIZE; lpc++ )
    {
	child_process_data.cpd_Table[lpc] = NULL;
    }
    child_process_data.cpd_Flags |= CPDF_INITIALIZED;
    return( retval );
}

void cpKillChildProcessData(void)
{
    /* XXX implement me */
}

void cpCollectChildProcesses(void)
{
    int lpc;

    /* Increment the generation number and */
    child_process_data.cpd_CurrentGeneration += 1;

    /* ... then collect any objects that do not match. */
    for( lpc = 0; lpc < CPD_TABLE_SIZE; lpc++ )
    {
	struct cpChildProcess *cp, **prev, *next;
	
	prev = &child_process_data.cpd_Table[lpc];
	cp = child_process_data.cpd_Table[lpc];
	while( cp != NULL )
	{
	    next = cp->cp_Next;
	    
	    if( cp->cp_Generation != child_process_data.cpd_CurrentGeneration )
	    {
		*prev = cp->cp_Next;
		cpDeleteChildProcess(cp);
	    }
	    else
	    {
		prev = &cp->cp_Next;
	    }
	    cp = next;
	}
    }
}

/**
 * Find a cpVNode with the given name.
 *
 * @param name The name of the vnode.
 * @return The matching cpVNode object or NULL.
 */
struct cpVNode *cpFindVNode(const char *name)
{
    struct cpVNode *vn, *retval = NULL;

    vn = child_process_data.cpd_VNodes;
    while( (vn != NULL) && (retval == NULL) )
    {
	if( strcmp(vn->vn_Name, name) == 0 )
	{
	    retval = vn;
	}
	vn = vn->vn_Next;
    }
    return( retval );
}

struct cpChildProcess *cpFindChildProcess(pid_t child_pid)
{
    struct cpChildProcess *curr, *retval = NULL;
    int hash = child_pid % CPD_TABLE_SIZE;

    curr = child_process_data.cpd_Table[hash];
    while( (curr != NULL) && (retval == NULL) )
    {
	if( curr->cp_PID == child_pid )
	{
	    retval = curr;
	}
	curr = curr->cp_Next;
    }
    return( retval );
}

struct cpChildProcess *cpCreateChildProcess(pid_t child_pid)
{
    struct cpChildProcess *retval;

    /* Allocate the structure and the /proc file name in one chunk. */
    if( (retval = calloc(1,
			 sizeof(struct cpChildProcess) +
			 strlen(PROC_STAT_FORMAT) +
			 16 + /* extra space for the PID */
			 strlen(PROC_MAP_FORMAT) +
			 16 +
			 1)) != NULL )
    {
	int rc, hash;
	FILE *file;

	retval->cp_PID = child_pid;

	/* Generate the "status" file name, */
	rc = snprintf((char *)(retval + 1),
		      strlen(PROC_STAT_FORMAT) + 16 + 1,
		      PROC_STAT_FORMAT,
		      child_pid);
	retval->cp_StatusFileName = (const char *)(retval + 1);

	/* ... the "map" file name, */
	snprintf((char *)retval->cp_StatusFileName + rc + 1,
		 strlen(PROC_MAP_FORMAT) + 16 + 1,
		 PROC_MAP_FORMAT,
		 child_pid);
	retval->cp_MapFileName = (char *)retval->cp_StatusFileName + rc + 1;

	/* ... and make the process part of the current generation. */
	retval->cp_Generation = child_process_data.cpd_CurrentGeneration;

	/* Check the status file to see if the process is in a vnode, then */
	if( (file = fopen(retval->cp_StatusFileName, "r")) != NULL )
	{
	    char buffer[1024], vname[MAXHOSTNAMELEN];

	    fgets(buffer, sizeof(buffer), file);
	    sscanf(buffer,
		   "%*s %*u %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s "
		   "%s",
		   vname);
	    fclose(file);

	    if( strcmp(vname, "-") == 0 )
	    {
		/* Not in a vnode. */
	    }
	    else if( (retval->cp_VNode = cpFindVNode(vname)) == NULL )
	    {
		struct cpVNode *vn;

		/* In a new vnode. */
		if( (vn = calloc(1,
				 sizeof(struct cpVNode) +
				 strlen(vname) + 1)) != NULL )
		{
		    strcpy((char *)(vn + 1), vname);
		    vn->vn_Name = (const char *)(vn + 1);
		    
		    vn->vn_Next = child_process_data.cpd_VNodes;
		    child_process_data.cpd_VNodes = vn;
		}
		retval->cp_VNode = vn;
	    }
	}

	/* ... add the process to the hash table. */
	hash = child_pid % CPD_TABLE_SIZE;
	retval->cp_Next = child_process_data.cpd_Table[hash];
	child_process_data.cpd_Table[hash] = retval;
    }
    return( retval );
}

void cpDeleteChildProcess(struct cpChildProcess *cp)
{
    if( cp != NULL )
    {
	free(cp);
	cp = NULL;
    }
}

#if defined(__FreeBSD__)
unsigned long long cpSampleUsage(struct cpChildProcess *cp)
{
    static char unused_string[1024];
    
    int retval = 0;
    FILE *file;

    /* Get CPU statistics first, then */
    if( (file = fopen(cp->cp_StatusFileName, "r")) != NULL )
    {
	struct timeval utime, stime, accum, usage;
	char buffer[1024];
	int unused_int;
	
	memset(&utime, 0, sizeof(struct timeval));
	memset(&stime, 0, sizeof(struct timeval));
	memset(&accum, 0, sizeof(struct timeval));
	fgets(buffer, sizeof(buffer), file);
	sscanf(buffer,
	       "%s %d %d %d %d %d,%d %s %d,%d %ld,%ld %ld,%ld",
	       unused_string,
	       &unused_int,
	       &unused_int,
	       &unused_int,
	       &unused_int,
	       &unused_int,
	       &unused_int,
	       unused_string,
	       &unused_int,
	       &unused_int,
	       &utime.tv_sec,
	       &utime.tv_usec,
	       &stime.tv_sec,
	       &stime.tv_usec);
	timeradd(&utime, &stime, &accum);
	timersub(&accum, &cp->cp_LastUsage, &usage);
	cp->cp_LastUsage = accum;
	retval += (usage.tv_sec * 1000000) + usage.tv_usec;
	cp->cp_VNode->vn_Usage += retval;
	fclose(file);
    }

    /* ... do the memory stats. */
    if( (file = fopen(cp->cp_MapFileName, "r")) != NULL )
    {
	unsigned long long total_memory = 0;
	char buffer[16384];
	int rc;

	if( (rc = fread(buffer, 1, sizeof(buffer), file)) > 0 )
	{
	    char *line = buffer;
	    
	    do {
		int resident, private_resident, ref_count, shadow_count, flags;
		void *start, *end, *obj;
		char *next_line;
		char type[32];

		next_line = strchr(line, '\n');
		*next_line = '\0';
		sscanf(line,
		       "%p %p %d %d %p %*s %d %d %x %*s %*s %s",
		       &start,
		       &end,
		       &resident,
		       &private_resident,
		       &obj,
		       &ref_count,
		       &shadow_count,
		       &flags,
		       type);

		/* XXX Not sure which ones to count. */
		if( ((strcmp(type, "default") == 0) && (ref_count <= 5)) ||
		    ((strcmp(type, "vnode") == 0) && (ref_count <= 2)) )
		{
		    total_memory += ((char *)end) - ((char *)start);
		}

		line = next_line + 1;
	    } while( line < &buffer[rc] );
	}

	cp->cp_MemoryUsage = total_memory;
	cp->cp_VNode->vn_MemoryUsage += total_memory;
	
	fclose(file);
    }
    
    return( retval );
}
#endif
