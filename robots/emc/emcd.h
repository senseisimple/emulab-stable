
#ifndef __EMCD_H__
#define __EMCD_H__

struct emc_robot_config {
  int id;
  char *hostname;
  char *vname;
  struct in_addr ia;
  int token;
};

struct rmc_client {
  int sock_fd;
  struct robot_list *position_list;
};

struct vmc_client {
  int sock_fd;
  struct robot_list *position_list;
};

#define EMC_SERVER_PORT 2525

#define EMC_UPDATE_HZ (2 * 1000)

#endif
