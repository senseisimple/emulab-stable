/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file childProcess.h
 *
 * Header file for the child process accounting functions.
 *
 * NOTE: Most of this was taken from the CPU Broker and tweaked to suit the
 * testbed's needs.
 */

#ifndef _child_process_h
#define _child_process_h

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/*
 * A cpVNode records the resource usage of a particular vnode.
 *
 * vn_Next - Link to next object in the list.
 * vn_Name - The name of the VNode (e.g. node.eid.pid.emulab.net)
 * vn_Usage - The CPU usage since the last sample.
 * vn_MemoryUsage - The Memory usage since the last sample.
 */
struct cpVNode {
    struct cpVNode *vn_Next;
    const char *vn_Name;
    unsigned long long vn_Usage;
    unsigned long long vn_MemoryUsage;
};

/*
 * A cpChildProcess is used to track and record the resource usage of a child
 * process.
 *
 * cp_Next - Link to next object in the list.
 * cp_PID - The process ID of the child.
 * cp_VNode - NULL or a reference to the VNode this process is in.
 * cp_StatusFileName - The "status" file name in "/proc" for this process.
 * cp_MapFileName - The "map" file name in "/proc" for this process.
 * cp_Generation - Generation number, used to garbage collect processes that
 *   do not exist any more.
 * cp_MemoryUsage - The memory used by this process since the last sample.
 * cp_LastUsage - The last recorded usage for this child.  Used to compute
 *   the difference in usage between the current and last sample time.
 */
struct cpChildProcess {
    struct cpChildProcess *cp_Next;
    pid_t cp_PID;
    struct cpVNode *cp_VNode;
    const char *cp_StatusFileName;
    const char *cp_MapFileName;
    unsigned long cp_Generation;
    unsigned long long cp_MemoryUsage;
#if defined(linux) || defined(__FreeBSD__)
    struct timeval cp_LastUsage;
#else
#error "Implement me"
#endif
};

enum {
    CPDB_INITIALIZED,
};

/*
 * Flags for the cpChildProcessData structure.
 *
 * CPDF_INITIALIZED - The global data is initialized.
 */
enum {
    CPDF_INITIALIZED = (1L << CPDB_INITIALIZED),
};

#define CPD_TABLE_SIZE 31

/*
 * Global data for tracking child processes.
 *
 * cpd_Flags - Holds the CPDF_ flags.
 * cpd_CurrentGeneration - The current generation number, this will be compared
 *   against each process' generation number to decide when to garbage collect
 *   cpChildProcess objects.
 * cpd_VNodes - The list of detected vnodes.
 * cpd_Table - Hash table of processes, hashes are based on the PID.
 */
struct cpChildProcessData {
    unsigned long cpd_Flags;
    unsigned long cpd_CurrentGeneration;
    struct cpVNode *cpd_VNodes;
    struct cpChildProcess *cpd_Table[CPD_TABLE_SIZE];
} child_process_data;

/**
 * Initialize the internal accounting data structures.
 *
 * @return True on success, false otherwise.
 */
int cpInitChildProcessData(void);

/**
 * Deinitialize the internal accounting data structures.
 */
void cpKillChildProcessData(void);

/**
 * Garbage collect cpChildProcess objects for processes that do not exist
 * anymore.  Works by incrementing the generation number and then collecting
 * any objects that do not have a matching generation number.
 */
void cpCollectChildProcesses(void);

/**
 * Find the cpChildProcess structure that corresponds to the given ID.
 *
 * @param child_pid The child process to track.
 * @return The cpChildProcess object that corresponds to the given ID or NULL
 * if one has not been created yet.
 */
struct cpChildProcess *cpFindChildProcess(pid_t child_pid);

/**
 * Create a cpChildProcess object for a child that does not have one yet.  Note
 * that this will also create the appropriate cpVNode object if one does not
 * already exist.
 *
 * @callgraph
 *
 * @param child_pid The child process to track.
 * @return The newly created object or NULL if a problem was encountered
 * during creation.
 */
struct cpChildProcess *cpCreateChildProcess(pid_t child_pid);

/**
 * Delete the given cpChildProcess object.
 *
 * @param cp NULL or a previously created cpChildProcess object.
 */
void cpDeleteChildProcess(struct cpChildProcess *cp);

/**
 * Sample the resource usage for a given child process.
 *
 * @param cp A valid cpChildProcess object.
 * @param run_time The amount of time the parent process has been running.
 * @return The CPU usage, in microseconds, for the given child process.
 */
unsigned long long cpSampleUsage(struct cpChildProcess *cp);

#ifdef __cplusplus
}
#endif

#endif
