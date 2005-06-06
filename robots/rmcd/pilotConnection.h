/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _pilot_connection_h
#define _pilot_connection_h

/**
 * @file pilotConnection.h
 *
 * Header file for the pilotConnection component which manages the network
 * connection between RMCD and the pilot daemons on the robots.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "mtp.h"
#include "slaveController.h"
#include "masterController.h"

/**
 * Number of seconds to wait for a response to a COMMAND_STOP.  The response
 * should come back almost immediately since there is no movement.
 */
#define STOP_RESPONSE_TIMEOUT 5

/**
 * Number of seconds to wait for a response to a REQUEST_REPORT.  The response
 * should come back almost immediately since there is no movement.
 */
#define REPORT_RESPONSE_TIMEOUT 2

/**
 * XXX
 */
#define MOVE_RESPONSE_TIMEOUT (2 * 60)

/**
 * Number of seconds to wait for a response to a wiggle movement.
 */
#define WIGGLE_RESPONSE_TIMEOUT 15

enum {
    UNUSED_COMMAND_ID,

    MASTER_COMMAND_ID,	/*< Command ID used by the master controller. */
    SLAVE_COMMAND_ID,	/*< Command ID used by the slave controller. */
};

/**
 * Indicates the state of the connection to the pilot daemon.
 */
typedef enum {
    PCS_DISCONNECTED,	/*< There is no connection open yet. */
    PCS_CONNECTING,	/*< The connect() is in progress. */
    PCS_CONNECTED,	/*< The connect() succeeded. */
} pilot_connection_state_t;

/**
 * Indicates which component is in control of the robot.  Any packets received
 * will be distributed to the component specified by these values.
 */
typedef enum {
    PCM_NONE,	/*< No connection, noone is in control. */
    PCM_MASTER,	/*< The master controller is in control. */
    PCM_SLAVE,	/*< The slave controller is in control. */
} pilot_control_mode_t;

#define PC_STATS_MSG_SUCCESS "Success"
#define PC_STATS_MSG_FAILURE "Failure"
#define PC_STATS_MSG_UNDEF   "Undef"

struct pc_stats {
    char *msg;
    struct timeval command_issue;
    struct timeval command_finish;
    int num_retries;
    struct robot_position start_pos;
    struct robot_position end_pos;
};

enum {
    PCB_EXPECTING_RESPONSE,
};

enum {
    /**
     * Indicates that a response is expected from the pilot.  The
     * pc_connection_timeout field contains the number of seconds to wait for
     * the response before the pilot is considered unresponsive and the
     * connection is dropped.
     */
    PCF_EXPECTING_RESPONSE = (1L << PCB_EXPECTING_RESPONSE),
};

/**
 * Structure used to track connections to the pilot daemons.
 *
 * @field pc_state The state of the connection to the robot.
 * @field pc_connection_timeout The number of seconds to wait before
 * considering a pilot to be unresponsive.
 * @field pc_robot The specification of the robot the pilot is in control of.
 * @field pc_handle The MTP connection to the pilot.
 * @field pc_flags Contains any PCF_ flags.
 * @field pc_control_mode Indicates which component is in control of the robot.
 * @field pc_slave The slave controller state.
 * @field pc_master The master controller state.
 * @field pc_stats Statistics.
 */
struct pilot_connection {
    pilot_connection_state_t pc_state;
    unsigned int pc_connection_timeout;
    struct robot_config *pc_robot;
    mtp_handle_t pc_handle;
    unsigned long pc_flags;
    pilot_control_mode_t pc_control_mode;
    struct slave_controller pc_slave;
    struct master_controller pc_master;
    // new for logging
    struct pc_stats stats;
};

/**
 * Add a robot to the set of currently maintained pilot connections.
 *
 * @param rc The robot configuration.
 * @return The newly created pilot_connection object or NULL if there was a
 * failure.
 */
struct pilot_connection *pc_add_robot(struct robot_config *rc);

/**
 * Print the current state of the world.  Called when SIGINFO is received.
 */
void pc_dump_info(void);

/**
 * Find and return the pilot_connection matching a given robot ID.
 *
 * @param robot_id The unique identifier of the robot to search for.
 * @return The pilot_connection matching the given robot identifier or NULL if
 * no match was found.
 */
struct pilot_connection *pc_find_pilot(int robot_id);

struct pilot_connection *
pc_find_pilot_by_location(struct robot_position *rp,
			  float tolerance,
			  struct pilot_connection *pc_ignore);

/**
 * Handle a packet received from EMCD.
 *
 * @param pc
 * @param mp
 */
void pc_handle_emc_packet(struct pilot_connection *pc, mtp_packet_t *mp);

/**
 *
 */
void pc_handle_pilot_packet(struct pilot_connection *pc, mtp_packet_t *mp);

/**
 *
 */
void pc_handle_signal(fd_set *rready, fd_set *wready);

/**
 * @param current_time
 */
void pc_handle_timeout(struct timeval *current_time);

// new
void pc_print_stats(struct pilot_connection *pc);
void pc_zero_stats(struct pilot_connection *pc);
void pc_stats_start_time(struct pilot_connection *pc);
void pc_stats_stop_time(struct pilot_connection *pc);
void pc_stats_add_retry(struct pilot_connection *pc);
void pc_stats_start_pos(struct pilot_connection *pc,
			struct robot_position *rp);
void pc_stats_end_pos(struct pilot_connection *pc,
		      struct robot_position *rp);
void pc_stats_msg(struct pilot_connection *pc, char *msg);

#define MAX_PILOT_CONNECTIONS 128

#define PILOT_SERVERPORT 2531

#define CONNECT_TIMEOUT 5
#define RECONNECT_TIMEOUT 5

struct pilot_connection_data {
    struct pilot_connection pcd_connections[MAX_PILOT_CONNECTIONS];
    unsigned int pcd_connection_count;
    struct mtp_config_rmc *pcd_config;
    mtp_handle_t pcd_emc_handle;
    
    fd_set pcd_read_fds;
    fd_set pcd_write_fds;
};

extern struct pilot_connection_data pc_data;

#endif
