/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __EMCD_H__
#define __EMCD_H__

#include "mtp.h"

enum {
  ERB_HAS_GOAL,
};

enum {
  ERF_HAS_GOAL = (1L << ERB_HAS_GOAL),
};

struct emc_robot_config {
  int id;
  char *hostname;
  char *vname;
  struct in_addr ia;
  int token;
  float init_x;
  float init_y;
  float init_theta;
  struct robot_position last_update_pos;
  struct robot_position last_goal_pos;
  unsigned long flags;
};

struct rmc_client {
  mtp_handle_t handle;
  struct robot_list *position_list;
};

struct vmc_client {
  mtp_handle_t handle;
  struct robot_list *position_list;
};

#define EMC_SERVER_PORT 2525

#define EMC_UPDATE_HZ (333)

#endif
