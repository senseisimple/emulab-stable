/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _robot_object_h
#define _robot_object_h

#include "mtp.h"
#include "listNode.h"

enum {
    RB_WIGGLING,
};

enum {
    RF_WIGGLING = (1L << RB_WIGGLING),
};

struct robot_object {
    struct lnMinNode ro_link;
    unsigned long ro_flags;
    char *ro_name;
    int ro_id;
    struct timeval ro_lost_timestamp;
};

struct robot_object *roFindRobot(struct lnMinList *list, int id);

#endif
