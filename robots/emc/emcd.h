
#ifndef __EMCD_H__
#define __EMCD_H__

#include "mtp.h"

struct emc_robot_config {
  int id;
  char *hostname;
  char *vname;
  struct in_addr ia;
  int token;
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

#define EMC_UPDATE_HZ (2 * 1000)

#endif
