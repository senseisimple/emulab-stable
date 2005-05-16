/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _robot_object_h
#define _robot_object_h

#include "mtp.h"
#include "listNode.h"

typedef enum {
    ROS_MIN,
    
    ROS_KNOWN,
    ROS_UNKNOWN,
    ROS_STARTED_WIGGLING,
    ROS_WIGGLE_QUEUE,
    ROS_WIGGLING,
    ROS_LOST,

    ROS_MAX
} ro_status_t;

struct robot_object {
    struct lnMinNode ro_link;
    struct robot_object *ro_next;
    ro_status_t ro_status;
    char *ro_name;
    int ro_id;
    struct timeval ro_lost_timestamp;
};

void roInit(void);

void roMoveRobot(struct robot_object *ro, ro_status_t new_status);
struct robot_object *roDequeueRobot(ro_status_t old_status,
				    ro_status_t new_status);
struct robot_object *roFindRobot(int id);

struct robot_data {
    struct lnMinList rd_lists[ROS_MAX];
    struct robot_object *rd_all;
};

extern struct robot_data ro_data;
extern char *ro_status_names[];

#endif
