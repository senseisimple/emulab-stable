/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file mtp.x
 *
 * XDR specification for the data structures used in the MTP protocol.  This
 * file will be fed into rpcgen(1) to create "mtp_xdr.h" and "mtp_xdr.c".
 */

#ifdef RPC_HDR
%#include <rpc/types.h>
%#include <rpc/xdr.h>
%
%#ifdef __cplusplus
%extern "C" {
%#endif
#endif

/**
 * Current version number for the protocol.
 *
 * @see mtp_packet
 */
const MTP_VERSION	= 0x02;

/**
 * Opcodes for each message type.
 *
 * @see mtp_payload
 */
enum mtp_opcode_t {
    /**
     * An error message.
     */
    MTP_CONTROL_ERROR		= 11,

    /**
     * Unused.
     */
    MTP_CONTROL_NOTIFY		= 12,

    /**
     * Initialize a client connection with a server.
     */
    MTP_CONTROL_INIT		= 13,

    /**
     * Unused.
     */
    MTP_CONTROL_CLOSE		= 14,


    /**
     * VMC configuration packet, sent in response to an MTP_CONTROL_INIT.
     */
    MTP_CONFIG_VMC		= 20,

    /**
     * RMC configuration packet, sent in response to an MTP_CONTROL_INIT.
     */
    MTP_CONFIG_RMC		= 21,

    /**
     * VMC client configuration packet.
     */
    MTP_CONFIG_VMC_CLIENT	= 22,


    /**
     * Request the current position of a given robot.
     */
    MTP_REQUEST_POSITION	= 30,

    /**
     * Request the identity of a robot at a given position.
     */
    MTP_REQUEST_ID		= 31,


    /**
     * Update the current position of a robot, can be sent in response to an
     * MTP_REQUEST_POSITION or spontaneously.
     */
    MTP_UPDATE_POSITION		= 40,

    /**
     * Update the identity of a robot at a position, sent in response to an
     * MTP_REQUEST_ID.
     */
    MTP_UPDATE_ID		= 41,


    /**
     * Move the robot to a specific position or orientation.
     */
    MTP_COMMAND_GOTO		= 50,

    /**
     * Stop the robot from moving any further.
     */
    MTP_COMMAND_STOP		= 51,

    /**
     * Send left/right wheel speeds directly to robot
     */
     MTP_COMMAND_WHEELS		= 52,

    /**
     * Start NULL primitive on robot
     */
     MTP_COMMAND_STARTNULL	= 53,

    /**
     * Build a trajectory
     */
    MTP_COMMAND_BUILD_TRAJ = 54,

    /**
     * Start tracking a trajectory
     */
    MTP_COMMAND_TRACK_TRAJ = 55,

    /**
     * Telemetry from a robot.
     */
    MTP_TELEMETRY		= 60,

    /**
     * Snapshot the state of a daemon.
     */
    MTP_SNAPSHOT		= 61,

    /**
     * vmcd tells emcd to wiggle a robot (for id purposes).
     */
    MTP_WIGGLE_REQUEST		= 70,

    /**
     * the result of a wiggle.
     */
    MTP_WIGGLE_STATUS		= 71,

    MTP_REQUEST_REPORT		= 80,

    MTP_CONTACT_REPORT		= 81,

    MTP_CREATE_OBSTACLE		= 90,
    MTP_UPDATE_OBSTACLE		= 91,
    MTP_REMOVE_OBSTACLE		= 92,

    MTP_OPCODE_MAX
};

/**
 * The "role" of the daemon that is sending a packet.
 *
 * @see mtp_packet
 */
enum mtp_role_t {
    MTP_ROLE_VMC	= 1,
    MTP_ROLE_EMC	= 2,
    MTP_ROLE_RMC	= 3,
    MTP_ROLE_EMULAB	= 4,
    MTP_ROLE_ROBOT	= 5,

    MTP_ROLE_MAX
};

/**
 * The status of the robot whose position is being updated.
 *
 * @see mtp_update_position
 */
enum mtp_status_t {
    MTP_POSITION_STATUS_UNKNOWN		= -1,
    MTP_POSITION_STATUS_IDLE		= 1,
    MTP_POSITION_STATUS_MOVING		= 2,
    MTP_POSITION_STATUS_ERROR		= 3,
    MTP_POSITION_STATUS_COMPLETE	= 4,
    MTP_POSITION_STATUS_CONTACT		= 5,
    MTP_POSITION_STATUS_ABORTED		= 6,
    MTP_POSITION_STATUS_CYCLE_COMPLETE	= 32
};

struct mtp_control {
    int id;
    int code;
    string msg<>;
};

struct robot_config {
    int id;
    string hostname<>;
};

struct camera_config {
    string hostname<>;
    int port;
    float x;
    float y;
    float width;
    float height;
    float fixed_x;
    float fixed_y;
};

struct obstacle_config {
    int id;
    float xmin;
    float ymin;
    float zmin;
    float xmax;
    float ymax;
    float zmax;
};

struct dyn_obstacle_config {
    int robot_id;
    obstacle_config config;
};

struct box {
    float x;
    float y;
    float width;
    float height;
};

struct mtp_config_rmc {
    robot_config robots<>;
    box bounds<>;
    obstacle_config obstacles<>;
};

struct mtp_config_vmc {
    robot_config robots<>;
    camera_config cameras<>;
};

struct mtp_config_vmc_client {
    float fixed_x;
    float fixed_y;
};

struct mtp_request_position {
    int robot_id;
};

struct robot_position {
    float x;
    float y;
    float theta;
    double timestamp;
};


struct robot_position_states {
   float e;
   float alpha;
   float theta;
   double timestamp;
};


struct mtp_update_position {
    int robot_id;
    robot_position position;
    mtp_status_t status;
    int command_id;
};

struct mtp_request_id {
    int request_id;
    robot_position position;
};

struct mtp_update_id {
    int request_id;
    int robot_id;
};

struct mtp_command_goto {
    int command_id;
    int robot_id;
    robot_position position;
    float speed;
};

struct mtp_command_stop {
    int command_id;
    int robot_id;
};

struct mtp_command_wheels {
    int command_id;
    int robot_id;
    float vleft;
    float vright;
};

struct mtp_command_startnull {
    int command_id;
    int robot_id;
    float acceleration;
};

struct mtp_command_build_traj {
    int command_id;
    int robot_id;
};

struct mtp_command_track_traj {
    int command_id;
    int robot_id;
};

/**
 * The different types of robots that support telemetry.
 */
enum mtp_robot_type_t {
    MTP_ROBOT_GARCIA		= 1	/*< Acroname Garcia. */
};

struct mtp_garcia_telemetry {
    int robot_id;
    float battery_level;
    float battery_voltage;
    int battery_misses;
    float left_odometer;
    float right_odometer;
    float left_instant_odometer;
    float right_instant_odometer;
    float left_velocity;
    float right_velocity;
    unsigned int move_count;
    int move_time_sec;
    int move_time_usec;
    int down_ranger_left;
    int down_ranger_right;
    float front_ranger_left;
    float front_ranger_right;
    float front_ranger_threshold;
    float rear_ranger_left;
    float rear_ranger_right;
    float rear_ranger_threshold;
    float side_ranger_left;
    float side_ranger_right;
    float side_ranger_threshold;
    float speed;
    int status;
    int idle;
    int user_button;
    int user_led;
    int stall_contact;
};

enum mtp_wiggle_t {
    MTP_WIGGLE_START	= 1,
    /* turn 180deg from current pos */
    MTP_WIGGLE_180_R	= 10,
    /* turn 180deg from current pos, then 180deg in opposite direction */
    MTP_WIGGLE_180_R_L	= 11,
    /* turn 360deg from current pos */
    MTP_WIGGLE_360_R	= 12,
    /* turn 360deg from current pos, then 360deg in opposite direction */
    MTP_WIGGLE_360_R_L	= 13
};

struct mtp_wiggle_request {
    int robot_id;
    mtp_wiggle_t wiggle_type;
};

struct mtp_wiggle_status {
    int robot_id;
    mtp_status_t status;
};

struct contact_point {
    float x;
    float y;
};

struct mtp_contact_report {
    int count;
    contact_point points[8];
};

union mtp_telemetry switch (mtp_robot_type_t type) {
 case MTP_ROBOT_GARCIA:		mtp_garcia_telemetry	garcia;
};

union mtp_payload switch (mtp_opcode_t opcode) {
 case MTP_CONTROL_ERROR:	mtp_control		error;
 case MTP_CONTROL_NOTIFY:	mtp_control		notify;
 case MTP_CONTROL_INIT:		mtp_control		init;
 case MTP_CONTROL_CLOSE:	mtp_control		close;
 case MTP_CONFIG_RMC:		mtp_config_rmc		config_rmc;
 case MTP_CONFIG_VMC:		mtp_config_vmc		config_vmc;
 case MTP_CONFIG_VMC_CLIENT:	mtp_config_vmc_client	config_vmc_client;
 case MTP_REQUEST_POSITION:	mtp_request_position	request_position;
 case MTP_REQUEST_ID:		mtp_request_id		request_id;
 case MTP_UPDATE_POSITION:	mtp_update_position	update_position;
 case MTP_UPDATE_ID:		mtp_update_id		update_id;
 case MTP_COMMAND_GOTO:		mtp_command_goto	command_goto;
 case MTP_COMMAND_STOP:		mtp_command_stop	command_stop;
 case MTP_COMMAND_WHEELS:	mtp_command_wheels	command_wheels;
 case MTP_COMMAND_STARTNULL:	mtp_command_startnull	command_startnull;
 case MTP_COMMAND_BUILD_TRAJ:   mtp_command_build_traj  command_build_traj;
 case MTP_COMMAND_TRACK_TRAJ:   mtp_command_track_traj  command_track_traj;
 case MTP_TELEMETRY:		mtp_telemetry		telemetry;
 case MTP_WIGGLE_REQUEST:	mtp_wiggle_request	wiggle_request;
 case MTP_WIGGLE_STATUS:	mtp_wiggle_status	wiggle_status;
 case MTP_REQUEST_REPORT:	mtp_request_position	request_report;
 case MTP_CONTACT_REPORT:	mtp_contact_report	contact_report;
 case MTP_CREATE_OBSTACLE:	dyn_obstacle_config	create_obstacle;
 case MTP_UPDATE_OBSTACLE:	obstacle_config		update_obstacle;
 case MTP_REMOVE_OBSTACLE:	int			remove_obstacle;
 case MTP_SNAPSHOT:		int			snapshot;
};

/**
 * The packet structure used for the MTP protocol, contains a simple header and
 * the payload with the actual message data.
 */
struct mtp_packet {
    short vers;
    short role;

    mtp_payload data;
};

#ifdef JAVA_RPC
program FOO {
    version FIRST_DEMO_VERSION {
        void nullcall(void) = 0;
    } = 1;
} = 0x20049679;
#endif

#ifdef RPC_HDR
%#ifdef __cplusplus
%}
%#endif
#endif
