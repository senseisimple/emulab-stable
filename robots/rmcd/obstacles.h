
#ifndef _obstacles_h
#define _obstacles_h

#include "mtp.h"
#include "rclip.h"

struct obstacle_config *ob_find_obstacle(struct mtp_config_rmc *mcr,
					 rc_line_t line);

#endif
