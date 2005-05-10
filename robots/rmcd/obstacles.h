/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _obstacles_h
#define _obstacles_h

#include "mtp.h"
#include "rclip.h"

int ob_find_obstacle(struct obstacle_config *oc,
		     struct mtp_config_rmc *mcr,
		     rc_line_t line);

#endif
