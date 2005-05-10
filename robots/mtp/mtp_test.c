/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#define DEBUG

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "mtp.h"

int main(int argc, char *argv[])
{
  static struct mtp_control mc = {
    42,
    MTP_PP_ERROR_MALLOC,
    "I've come undone"
  };

  static struct robot_config rcr[3] = {
    { 1, "node1" },
    { 2, "node2" },
    { 3, "node3" },
  };

  static struct mtp_config_rmc mcr = {
    3,
    rcr,
    { 10.1, 20.2 }
  };
  
  static struct robot_config rcv[3] = {
    { 4, "vnode4" },
    { 5, "vnode5" },
    { 6, "vnode6" },
  };

  static struct mtp_config_vmc mcv = {
    3,
    rcv
  };
  
  static struct mtp_request_position mrp = {
    4
  };
  
  static struct mtp_update_position mup = {
    3,
    { 5, 25, 0.44, 20.5 },
    MTP_POSITION_STATUS_IDLE
  };
  
  static struct mtp_request_id mri = {
    169,
    { 3.1, 20.2, 69.5, 20.5 }
  };
  
  static struct mtp_update_id mui = {
    30,
    2
  };
  
  static struct mtp_command_goto mcg = {
    5,
    3,
    { 2.2, 13.3, 73.4, 500.04 },
  };
  
  static struct mtp_command_goto mcs = {
    4,
    2,
  };

  static struct mtp_wiggle_request mwr = {
    4,
    1,
  };

  static struct mtp_wiggle_status mws = {
    4,
    4,
  };
  
  int fd, retval = EXIT_SUCCESS;

  if ((fd = open("mtp_stream.dat", O_CREAT | O_TRUNC | O_RDWR, 0666)) < 0) {
    perror("Could not open 'mtp_stream.dat'?");
  }
  else {
    struct mtp_packet *mp;
    int lpc;
    
    assert(mp = mtp_make_packet(MTP_CONTROL_ERROR, MTP_ROLE_EMC, &mc));
    assert(mtp_send_packet(fd,mp) == MTP_PP_SUCCESS);

    assert(mp = mtp_make_packet(MTP_CONFIG_RMC, MTP_ROLE_RMC, &mcr));
    assert(mtp_send_packet(fd,mp) == MTP_PP_SUCCESS);

    assert(mp = mtp_make_packet(MTP_CONFIG_VMC, MTP_ROLE_VMC, &mcv));
    assert(mtp_send_packet(fd,mp) == MTP_PP_SUCCESS);

    assert(mp = mtp_make_packet(MTP_REQUEST_POSITION, MTP_ROLE_VMC, &mrp));
    assert(mtp_send_packet(fd,mp) == MTP_PP_SUCCESS);

    assert(mp = mtp_make_packet(MTP_UPDATE_POSITION, MTP_ROLE_VMC, &mup));
    assert(mtp_send_packet(fd,mp) == MTP_PP_SUCCESS);

    assert(mp = mtp_make_packet(MTP_REQUEST_ID, MTP_ROLE_VMC, &mri));
    assert(mtp_send_packet(fd,mp) == MTP_PP_SUCCESS);

    assert(mp = mtp_make_packet(MTP_UPDATE_ID, MTP_ROLE_VMC, &mui));
    assert(mtp_send_packet(fd,mp) == MTP_PP_SUCCESS);

    assert(mp = mtp_make_packet(MTP_COMMAND_GOTO, MTP_ROLE_VMC, &mcg));
    assert(mtp_send_packet(fd,mp) == MTP_PP_SUCCESS);

    assert(mp = mtp_make_packet(MTP_COMMAND_STOP, MTP_ROLE_VMC, &mcs));
    assert(mtp_send_packet(fd,mp) == MTP_PP_SUCCESS);

    assert(mp = mtp_make_packet(MTP_WIGGLE_REQUEST, MTP_ROLE_VMC, &mwr));
    assert(mtp_send_packet(fd,mp) == MTP_PP_SUCCESS);

    assert(mp = mtp_make_packet(MTP_WIGGLE_REQUEST, MTP_ROLE_VMC, &mws));
    assert(mtp_send_packet(fd,mp) == MTP_PP_SUCCESS);

    lseek(fd,0,SEEK_SET);

    assert(mtp_receive_packet(fd,&mp) == MTP_PP_SUCCESS);
    assert(mp->opcode == MTP_CONTROL_ERROR);
    assert(mp->version == MTP_VERSION);
    assert(mp->role == MTP_ROLE_EMC);
    assert(mp->data.control->id == mc.id);
    assert(mp->data.control->code == mc.code);
    assert(strcmp(mp->data.control->msg, mc.msg) == 0);

    assert(mtp_receive_packet(fd,&mp) == MTP_PP_SUCCESS);
    assert(mp->opcode == MTP_CONFIG_RMC);
    assert(mp->version == MTP_VERSION);
    assert(mp->role == MTP_ROLE_RMC);
    assert(mp->data.config_rmc->num_robots == mcr.num_robots);
    assert(mp->data.config_rmc->box.horizontal == mcr.box.horizontal);
    assert(mp->data.config_rmc->box.vertical == mcr.box.vertical);
    for (lpc = 0; lpc < mcr.num_robots; lpc++) {
      assert(mp->data.config_rmc->robots[lpc].id == mcr.robots[lpc].id);
      assert(strcmp(mp->data.config_rmc->robots[lpc].hostname,
		    mcr.robots[lpc].hostname) == 0);
    }

    assert(mtp_receive_packet(fd,&mp) == MTP_PP_SUCCESS);
    assert(mp->opcode == MTP_CONFIG_VMC);
    assert(mp->version == MTP_VERSION);
    assert(mp->role == MTP_ROLE_VMC);
    assert(mp->data.config_vmc->num_robots == mcv.num_robots);
    for (lpc = 0; lpc < mcv.num_robots; lpc++) {
      assert(mp->data.config_vmc->robots[lpc].id == mcv.robots[lpc].id);
      assert(strcmp(mp->data.config_vmc->robots[lpc].hostname,
		    mcv.robots[lpc].hostname) == 0);
    }

    assert(mtp_receive_packet(fd,&mp) == MTP_PP_SUCCESS);
    assert(mp->opcode == MTP_REQUEST_POSITION);
    assert(mp->version == MTP_VERSION);
    assert(mp->role == MTP_ROLE_VMC);
    assert(mp->data.request_position->robot_id == mrp.robot_id);

    assert(mtp_receive_packet(fd,&mp) == MTP_PP_SUCCESS);
    assert(mp->opcode == MTP_UPDATE_POSITION);
    assert(mp->version == MTP_VERSION);
    assert(mp->role == MTP_ROLE_VMC);
    assert(mp->data.update_position->robot_id == mup.robot_id);
    assert(mp->data.update_position->position.x == mup.position.x);
    assert(mp->data.update_position->position.y == mup.position.y);
    assert(mp->data.update_position->position.theta == mup.position.theta);
    assert(mp->data.update_position->position.timestamp ==
	   mup.position.timestamp);
    assert(mp->data.update_position->status == mup.status);

    assert(mtp_receive_packet(fd,&mp) == MTP_PP_SUCCESS);
    assert(mp->opcode == MTP_REQUEST_ID);
    assert(mp->version == MTP_VERSION);
    assert(mp->role == MTP_ROLE_VMC);
    assert(mp->data.request_id->position.x == mri.position.x);
    assert(mp->data.request_id->position.y == mri.position.y);
    assert(mp->data.request_id->position.theta == mri.position.theta);
    assert(mp->data.request_id->position.timestamp == mri.position.timestamp);
    
    assert(mtp_receive_packet(fd,&mp) == MTP_PP_SUCCESS);
    assert(mp->opcode == MTP_UPDATE_ID);
    assert(mp->version == MTP_VERSION);
    assert(mp->role == MTP_ROLE_VMC);
    assert(mp->data.update_id->robot_id == mui.robot_id);
    
    assert(mtp_receive_packet(fd,&mp) == MTP_PP_SUCCESS);
    assert(mp->opcode == MTP_COMMAND_GOTO);
    assert(mp->version == MTP_VERSION);
    assert(mp->role == MTP_ROLE_VMC);
    assert(mp->data.command_goto->command_id == mcg.command_id);
    assert(mp->data.command_goto->robot_id == mcg.robot_id);
    assert(mp->data.command_goto->position.x == mcg.position.x);
    assert(mp->data.command_goto->position.y == mcg.position.y);
    assert(mp->data.command_goto->position.theta == mcg.position.theta);
    assert(mp->data.command_goto->position.timestamp ==
	   mcg.position.timestamp);
    
    assert(mtp_receive_packet(fd,&mp) == MTP_PP_SUCCESS);
    assert(mp->opcode == MTP_COMMAND_STOP);
    assert(mp->version == MTP_VERSION);
    assert(mp->role == MTP_ROLE_VMC);
    assert(mp->data.command_goto->command_id == mcs.command_id);
    assert(mp->data.command_goto->robot_id == mcs.robot_id);
    
    close(fd);
    fd = -1;
  }
    
  return retval;
}
