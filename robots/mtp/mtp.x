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
     * Telemetry from a robot.
     */
    MTP_TELEMETRY		= 60,
    

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
    MTP_POSITION_STATUS_OBSTRUCTED	= 5,
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
};

struct global_bound {
    float horizontal;
    float vertical;
};

struct mtp_config_rmc {
    robot_config robots<>;
    global_bound box;
};

struct mtp_config_vmc {
    robot_config robots<>;
    camera_config cameras<>;
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

struct mtp_update_position {
    int robot_id;
    robot_position position;
    mtp_status_t status;
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
};

struct mtp_command_stop {
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
 case MTP_REQUEST_POSITION:	mtp_request_position	request_position;
 case MTP_REQUEST_ID:		mtp_request_id		request_id;
 case MTP_UPDATE_POSITION:	mtp_update_position	update_position;
 case MTP_UPDATE_ID:		mtp_update_id		update_id;
 case MTP_COMMAND_GOTO:		mtp_command_goto	command_goto;
 case MTP_COMMAND_STOP:		mtp_command_stop	command_stop;
 case MTP_TELEMETRY:		mtp_telemetry		telemetry;
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
