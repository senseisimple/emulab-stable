
#ifndef _pilot_connection_h
#define _pilot_connection_h

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "mtp.h"

typedef enum {
    PS_ARRIVED,
    PS_PENDING_POSITION,
    PS_REFINING_POSITION,
    PS_REFINING_ORIENTATION,

    PS_START_WIGGLING,
    PS_WIGGLING,

    PS_MAX
} pilot_state_t;

enum {
    PCB_CONNECTING,
    PCB_CONNECTED,
    PCB_VISION_POSITION,
    PCB_WAYPOINT,
    PCB_CONTACT,
};

enum {
    PCF_CONNECTING = (1L << PCB_CONNECTING),
    PCF_CONNECTED = (1L << PCB_CONNECTED),
    PCF_VISION_POSITION = (1L << PCB_VISION_POSITION),
    PCF_WAYPOINT = (1L << PCB_WAYPOINT),
    PCF_CONTACT = (1L << PCB_CONTACT),
};

struct pilot_connection {
    unsigned long pc_flags;
    pilot_state_t pc_state;
    struct robot_config *pc_robot;
    mtp_handle_t pc_handle;
    struct robot_position pc_actual_pos;
    struct robot_position pc_last_pos;
    struct robot_position pc_waypoint;
    int pc_tries_remaining;
    struct robot_position pc_goal_pos;

    struct obstacle_config pc_obstacles[32];
    unsigned int pc_obstacle_count;
};

#define REL2ABS(_dst, _theta, _rpoint, _apoint) { \
    float _ct, _st; \
    \
    _ct = cosf(_theta); \
    _st = sinf(_theta); \
    (_dst)->x = _ct * (_rpoint)->x - _st * -(_rpoint)->y + (_apoint)->x; \
    (_dst)->y = _ct * (_rpoint)->y + _st * -(_rpoint)->x + (_apoint)->y; \
}

struct pilot_connection *pc_add_robot(struct robot_config *rc);

/**
 * Map the robot ID to a gorobot_conn object.
 *
 * @param robot_id The robot identifier to search for.
 * @return The gorobot_conn that matches the given ID or NULL if no match was
 * found.
 */
struct pilot_connection *pc_find_pilot(int robot_id);

void pc_plot_waypoint(struct pilot_connection *pc);
void pc_wiggle(struct pilot_connection *pc, mtp_wiggle_t mw);
void pc_set_goal(struct pilot_connection *pc, struct robot_position *rp);
void pc_set_actual(struct pilot_connection *pc, struct robot_position *rp);
void pc_override_state(struct pilot_connection *pc, pilot_state_t ps);
void pc_change_state(struct pilot_connection *pc, pilot_state_t ps);
void pc_handle_packet(struct pilot_connection *pc, struct mtp_packet *mp);
void pc_handle_signal(fd_set *rready, fd_set *wready);

/**
 * How close does the robot have to be before it is considered at the intended
 * position.  Measurement is in meters(?).
 */
#define METER_TOLERANCE 0.025

#define WAYPOINT_TOLERANCE 0.25

#define OBSTACLE_BUFFER 0.25

/**
 * How close does the angle have to be before it is considered at the intended
 * angle.
 */
#define RADIAN_TOLERANCE 0.09

/**
 * Maximum number of times to try and refine the position before giving up.
 */
#define MAX_REFINE_RETRIES 3

#define MAX_PILOT_CONNECTIONS 128

#define PILOT_SERVERPORT 2531

struct pilot_connection_data {
    struct pilot_connection pcd_connections[MAX_PILOT_CONNECTIONS];
    unsigned int pcd_connection_count;
    struct mtp_config_rmc *pcd_config;
    mtp_handle_t pcd_emc_handle;
};

extern struct pilot_connection_data pc_data;

#endif
