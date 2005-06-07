/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "log.h"
#include "obstacles.h"
#include "pilotConnection.h"

static char *pc_control_mode_strings[] = {
    "none",
    "master",
    "slave",
};

static char *pc_connection_state_strings[] = {
    "disconnected",
    "connecting",
    "connected",
};

char *statsfile = NULL;
int statsfile_fd = -1;
FILE *statsfile_FILE = NULL;

extern int debug;

struct pilot_connection_data pc_data;

static void pc_disconnected(struct pilot_connection *pc)
{
    assert(pc != NULL);
    assert(pc->pc_state != PCS_DISCONNECTED);

    pc->pc_state = PCS_DISCONNECTED;
    sc_disconnected(&pc->pc_slave);
    close(pc->pc_handle->mh_fd);
    FD_CLR(pc->pc_handle->mh_fd, &pc_data.pcd_read_fds);
    FD_CLR(pc->pc_handle->mh_fd, &pc_data.pcd_write_fds);
    mtp_delete_handle(pc->pc_handle);
    pc->pc_handle = NULL;
    pc->pc_connection_timeout = RECONNECT_TIMEOUT;
    pc->pc_control_mode = PCM_NONE;
    pc->pc_flags &= ~PCF_EXPECTING_RESPONSE;
}

static void pc_start_connect(struct pilot_connection *pc)
{
    int fd;
    
    assert(pc != NULL);
    assert(pc->pc_state == PCS_DISCONNECTED);

    info("start connect for %s\n", pc->pc_robot->hostname);

    if ((pc->pc_handle = mtp_create_handle3(pc->pc_robot->hostname,
					    PILOT_SERVERPORT,
					    NULL,
					    1)) == NULL) {
	fatal("robot mtp_create_handle");
    }

    fd = pc->pc_handle->mh_fd;
    FD_SET(fd, &pc_data.pcd_write_fds);
    pc->pc_state = PCS_CONNECTING;
    pc->pc_connection_timeout = CONNECT_TIMEOUT;
}

static void pc_finish_connect(struct pilot_connection *pc)
{
    int rc;
    
    assert(pc != NULL);
    assert(pc->pc_state == PCS_CONNECTING);
    

    info("finish connect for %s\n", pc->pc_robot->hostname);
    
    fcntl(pc->pc_handle->mh_fd, F_SETFL, 0);
    FD_CLR(pc->pc_handle->mh_fd, &pc_data.pcd_write_fds);
    
    if ((rc = mtp_send_packet2(pc->pc_handle,
			       MA_Opcode, MTP_CONTROL_INIT,
			       MA_Role, MTP_ROLE_RMC,
			       MA_Message, "rmcd v0.1",
			       MA_TAG_DONE)) != MTP_PP_SUCCESS) {
	error("could not send init packet to %s %d\n",
	      pc->pc_robot->hostname, rc);
	pc_disconnected(pc);
    }
    else {
	mtp_send_packet2(pc->pc_handle,
			 MA_Opcode, MTP_COMMAND_STOP,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, pc->pc_robot->id,
			 MA_CommandID, MASTER_COMMAND_ID,
			 MA_TAG_DONE);

	pc->pc_control_mode = PCM_MASTER;
	pc->pc_state = PCS_CONNECTED;
	FD_SET(pc->pc_handle->mh_fd, &pc_data.pcd_read_fds);
    }
}

// new 
void pc_zero_stats(struct pilot_connection *pc) {
    assert(pc != NULL);

    // new
    pc->stats.num_retries = 0;
    pc->stats.msg = PC_STATS_MSG_UNDEF;
    bzero(&(pc->stats.command_issue),sizeof(struct timeval));
    bzero(&(pc->stats.command_finish),sizeof(struct timeval));
    bzero(&(pc->stats.start_pos),sizeof(struct timeval));
    bzero(&(pc->stats.end_pos),sizeof(struct timeval));

    return;
}
void pc_stats_start_time(struct pilot_connection *pc) {
    assert(pc != NULL);

    gettimeofday(&(pc->stats.command_issue),NULL);
}
void pc_stats_stop_time(struct pilot_connection *pc) {
    assert(pc != NULL);

    gettimeofday(&(pc->stats.command_finish),NULL);
}
void pc_stats_msg(struct pilot_connection *pc,char *msg) {
    assert(pc != NULL);

    pc->stats.msg = msg;
}
void pc_stats_add_retry(struct pilot_connection *pc) {
    assert(pc != NULL);
    
    ++(pc->stats.num_retries);
}
void pc_stats_start_pos(struct pilot_connection *pc,
			struct robot_position *rp) {
    assert(pc != NULL);
    assert(rp != NULL);
    
    pc->stats.start_pos = *rp;
}
void pc_stats_end_pos(struct pilot_connection *pc,
		      struct robot_position *rp) {
    assert(pc != NULL);
    assert(rp != NULL);

    pc->stats.end_pos = *rp;
}
void pc_print_stats(struct pilot_connection *pc) {
    struct timeval diff;

    if (statsfile_fd < 0 && statsfile != NULL) {
	// open it
	statsfile_fd = open(statsfile,
			    O_WRONLY | O_CREAT | O_TRUNC,
			    S_IRWXU | S_IRGRP);
	if (statsfile_fd == -1) {
	    statsfile_fd = 0;
	}
	else {
	    statsfile_FILE = fdopen(statsfile_fd,"w");
	    if (statsfile_FILE == NULL) {
		statsfile_fd = 0;
	    }
	}
    }
    else if (statsfile_fd < 0) {
	// noop -- can't open file ever
	;
    }
    else {
	timersub(&(pc->stats.command_finish),
		 &(pc->stats.command_issue),
		 &diff);
	fprintf(statsfile_FILE,
		"%s: time=%f,num_retries=%d,"
		"goal_pos(x=%f,y=%f,theta=%f,time=%f),"
		"end_pos=(x=%f,y=%f,theta=%f,time=%f)\n",
		pc->pc_robot->hostname,
		//pc->stats.msg,
		(float)(diff.tv_sec)+diff.tv_usec/1000000.0f,
		pc->stats.num_retries,
		pc->stats.start_pos.x,
		pc->stats.start_pos.y,
		pc->stats.start_pos.theta,
		pc->stats.start_pos.timestamp,
		pc->stats.end_pos.x,
		pc->stats.end_pos.y,
		pc->stats.end_pos.theta,
		pc->stats.end_pos.timestamp);
	fflush(statsfile_FILE);
    }
}

struct pilot_connection *pc_add_robot(struct robot_config *rc)
{
    struct pilot_connection *retval;

    assert(rc != NULL);
    assert(rc->id >= 0);
    assert(rc->hostname != NULL);
    assert(strlen(rc->hostname) > 0);

    retval = &pc_data.pcd_connections[pc_data.pcd_connection_count];
    pc_data.pcd_connection_count += 1;

    retval->pc_state = PCS_DISCONNECTED;
    retval->pc_control_mode = PCM_NONE;
    retval->pc_robot = rc;
    retval->pc_slave.sc_pilot = retval;
    retval->pc_master.mc_pilot = retval;
    retval->pc_master.mc_plan.pp_robot = rc;

    // new 
    pc_zero_stats(retval);

    if (debug > 1) {
	info("debug: connecting to %s\n", rc->hostname);
    }

    pc_start_connect(retval);
    
    return retval;
}

void pc_dump_info(void)
{
    int lpc;

    info("info: pilot list\n");
    for (lpc = 0; lpc < pc_data.pcd_connection_count; lpc++) {
	struct pilot_connection *pc;
	
	pc = &pc_data.pcd_connections[lpc];
	info("  %s: state=%s; flags=0x%x; mode=%s; timeout=%d;\n"
	     "    flags: %x; pause: %lu; self_obst: %d\n"
	     "    actual: %.2f %.2f %.2f\n"
	     "    waypt:  %.2f %.2f %.2f\n"
	     "    goal:   %.2f %.2f %.2f\n"
	     "    obst:   %.2f %.2f %.2f %.2f\n",
	     pc->pc_robot->hostname,
	     pc_connection_state_strings[pc->pc_state],
	     pc->pc_flags,
	     pc_control_mode_strings[pc->pc_control_mode],
	     pc->pc_connection_timeout,
	     pc->pc_master.mc_flags,
	     pc->pc_master.mc_pause_time,
	     pc->pc_master.mc_self_obstacle != NULL ?
	     pc->pc_master.mc_self_obstacle->on_expanded.id : -1,
	     pc->pc_master.mc_plan.pp_actual_pos.x,
	     pc->pc_master.mc_plan.pp_actual_pos.y,
	     pc->pc_master.mc_plan.pp_actual_pos.theta,
	     pc->pc_master.mc_plan.pp_waypoint.x,
	     pc->pc_master.mc_plan.pp_waypoint.y,
	     pc->pc_master.mc_plan.pp_waypoint.theta,
	     pc->pc_master.mc_plan.pp_goal_pos.x,
	     pc->pc_master.mc_plan.pp_goal_pos.y,
	     pc->pc_master.mc_plan.pp_goal_pos.theta,
	     pc->pc_master.mc_plan.pp_obstacle.xmin,
	     pc->pc_master.mc_plan.pp_obstacle.ymin,
	     pc->pc_master.mc_plan.pp_obstacle.xmax,
	     pc->pc_master.mc_plan.pp_obstacle.ymax);
    }
}

struct pilot_connection *pc_find_pilot(int robot_id)
{
    struct pilot_connection *retval = NULL;
    int lpc;
    
    assert(robot_id >= 0);

    for (lpc = 0; (lpc < pc_data.pcd_connection_count) && !retval; lpc++) {
	struct pilot_connection *pc = &pc_data.pcd_connections[lpc];

	if (pc->pc_robot->id == robot_id) {
	    retval = pc;
	}
    }
    
    return retval;
}

struct pilot_connection *
pc_find_pilot_by_location(struct robot_position *rp,
			  float tolerance,
			  struct pilot_connection *pc_ignore)
{
    struct pilot_connection *retval = NULL;
    int lpc;
    
    assert(rp != NULL);
    assert(tolerance > 0.0);

    for (lpc = 0; (lpc < pc_data.pcd_connection_count) && !retval; lpc++) {
	struct pilot_connection *pc = &pc_data.pcd_connections[lpc];
	float r, theta;
	
	mtp_polar(rp, &pc->pc_master.mc_plan.pp_actual_pos, &r, &theta);
	if ((r < tolerance) && (pc != pc_ignore))
	    retval = pc;
    }
    
    return retval;
}

void pc_handle_emc_packet(struct pilot_connection *pc, struct mtp_packet *mp)
{
    assert(pc != NULL);
    assert(mp != NULL);

    sc_handle_emc_packet(&pc->pc_slave, mp);
    if (pc->pc_slave.sc_state == SS_IDLE)
	mc_handle_emc_packet(&pc->pc_master, mp);
    else
	pc->pc_control_mode = PCM_SLAVE;
}

void pc_handle_pilot_packet(struct pilot_connection *pc, struct mtp_packet *mp)
{
    assert(pc != NULL);
    assert(mp != NULL);
    
    if (debug > 1) {
	info("%s pilot packet: ", pc->pc_robot->hostname);
	mtp_print_packet(stderr, mp);
    }

    switch (pc->pc_control_mode) {
    case PCM_NONE:
	assert(0);
	break;
    case PCM_SLAVE:
	sc_handle_pilot_packet(&pc->pc_slave, mp);
	if (pc->pc_slave.sc_state == SS_IDLE) {
	    pc->pc_control_mode = PCM_MASTER;
	    mc_handle_switch(&pc->pc_master);
	}
	break;
    case PCM_MASTER:
	mc_handle_pilot_packet(&pc->pc_master, mp);
	break;
    }
}

void pc_handle_signal(fd_set *rready, fd_set *wready)
{
    int lpc;
    
    assert(rready != NULL);
    assert(wready != NULL);

    for (lpc = 0; lpc < pc_data.pcd_connection_count; lpc++) {
	struct pilot_connection *pc = &pc_data.pcd_connections[lpc];
	mtp_handle_t mh = pc->pc_handle;

	if ((mh != NULL) && FD_ISSET(mh->mh_fd, rready)) {
	    assert(pc->pc_state == PCS_CONNECTED);
	    
	    do {
		struct mtp_packet mp;
		int rc;
		
		if ((rc = mtp_receive_packet(mh, &mp)) != MTP_PP_SUCCESS) {
		    pc_disconnected(pc);
		    mh = NULL;
		}
		else {
		    pc_handle_pilot_packet(pc, &mp);
		}
	    } while ((pc->pc_state == PCS_CONNECTED) &&
		     (mh != NULL) && (mh->mh_remaining > 0));
	}
	if ((mh != NULL) && FD_ISSET(mh->mh_fd, wready)) {
	    assert(pc->pc_state == PCS_CONNECTING);
	    
	    pc_finish_connect(pc);
	}
    }
}

void pc_handle_timeout(struct timeval *current_time)
{
    int lpc;

    assert(current_time != NULL);

    for (lpc = 0; lpc < pc_data.pcd_connection_count; lpc++) {
	struct pilot_connection *pc = &pc_data.pcd_connections[lpc];

	switch (pc->pc_state) {
	case PCS_DISCONNECTED:
	    pc->pc_connection_timeout -= 1;
	    info("check %d\n", pc->pc_connection_timeout);
	    if (pc->pc_connection_timeout == 0) {
		info("retrying connection for %s\n", pc->pc_robot->hostname);
		pc_start_connect(pc);
	    }
	    break;
	case PCS_CONNECTING:
	    pc->pc_connection_timeout -= 1;
	    info("ccheck %d\n", pc->pc_connection_timeout);
	    if (pc->pc_connection_timeout == 0) {
		info("dropping connection for %s\n", pc->pc_robot->hostname);
		pc_disconnected(pc);
	    }
	    break;
	case PCS_CONNECTED:
	    if (pc->pc_flags & PCF_EXPECTING_RESPONSE) {
		info("dcheck %d\n", pc->pc_connection_timeout);
		pc->pc_connection_timeout -= 1;
		if (pc->pc_connection_timeout == 0) {
		    info("dropping connection for %s\n",
			 pc->pc_robot->hostname);
		    pc_disconnected(pc);
		}
	    }
	    break;
	}
	
	switch (pc->pc_control_mode) {
	case PCM_NONE:
	    break;
	case PCM_SLAVE:
	    break;
	case PCM_MASTER:
	    mc_handle_tick(&pc->pc_master);
	    break;
	}
    }

    ob_tick();
}
