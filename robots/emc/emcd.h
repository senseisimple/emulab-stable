
#ifndef __EMCD_H__
#define __EMCD_H__

struct rmc_client {
  int sock_fd;
  struct robot_list *position_list;
};

struct vmc_client {
  int sock_fd;
  struct robot_list *position_list;
};

#define EMC_SERVER_PORT 2525

#endif
