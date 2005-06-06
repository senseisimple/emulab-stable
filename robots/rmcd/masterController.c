/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file masterController.c
 *
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "log.h"
#include "rmcd.h"
#include "rclip.h"
#include "obstacles.h"
#include "pathPlanning.h"
#include "pilotConnection.h"
#include "masterController.h"

struct master_controller_data mc_data;

/**
 * Do a fuzzy comparison of two values.
 *
 * @param x1 The first value.
 * @param x2 The second value.
 * @param tol The amount of tolerance to take into account when doing the
 * comparison.
 */
#define cmp_fuzzy(x1, x2, tol)				\
  ((((x1) - (tol)) < (x2)) && (x2 < ((x1) + (tol))))

#define STATE_TOL 0.04f
#define STATE_ATOL 0.5f
    
int mc_invariant(struct master_controller *mc)
{
    assert(mc != NULL);
    
    return 1;
}

static int mc_set_goal(struct master_controller *mc, mtp_packet_t *mp)
{
    int retval = 0;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));
    assert(mp != NULL);

    mc->mc_pause_time = 0;
    mc->mc_flags &= ~(MCF_CONTACT);
    mc->mc_tries_remaining = mc_data.mcd_max_refine_retries;
    mc->mc_plan.pp_goal_pos = mp->data.mtp_payload_u.command_goto.position;
    mc->mc_plan.pp_speed = mp->data.mtp_payload_u.command_goto.speed;

    ob_rem_obstacle(mc->mc_self_obstacle);
    mc->mc_self_obstacle = NULL;
    
    switch (mc->mc_pilot->pc_control_mode) {
    case PCM_NONE:
	break;
    case PCM_MASTER:
	mtp_send_packet2(mc->mc_pilot->pc_handle,
			 MA_Opcode, MTP_COMMAND_STOP,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, mc->mc_pilot->pc_robot->id,
			 MA_CommandID, MASTER_COMMAND_ID,
			 MA_TAG_DONE);
	
	mc->mc_pilot->pc_flags |= PCF_EXPECTING_RESPONSE;
	mc->mc_pilot->pc_connection_timeout = STOP_RESPONSE_TIMEOUT;
	
	pc_zero_stats(mc->mc_pilot);
	pc_stats_start_pos(mc->mc_pilot,&(mc->mc_plan.pp_goal_pos));
	pc_stats_start_time(mc->mc_pilot);
	break;
    case PCM_SLAVE:
	if (debug > 1) {
	    info("%s is wiggling, waiting for new position\n",
		 mc->mc_pilot->pc_robot->hostname);
	}
	
	mtp_send_packet2(pc_data.pcd_emc_handle,
			 MA_Opcode, MTP_REQUEST_POSITION,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, mc->mc_pilot->pc_robot->id,
			 MA_TAG_DONE);
	
	// don't print here -- cause the previous move, if any, finished 
	// more or less successfully...
	pc_zero_stats(mc->mc_pilot);
	pc_stats_start_pos(mc->mc_pilot,&(mc->mc_plan.pp_goal_pos));
	break;
    }
    
    return retval;
}

static int mc_set_actual(struct master_controller *mc, mtp_packet_t *mp)
{
    int retval = 0;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));
    assert(mp != NULL);

    mc->mc_plan.pp_actual_pos =
	mp->data.mtp_payload_u.update_position.position;

    // set the current "final" stats pos
    pc_stats_end_pos(mc->mc_pilot,&(mc->mc_plan.pp_actual_pos));
    
    return retval;
}

static int mc_request_report(struct master_controller *mc, mtp_packet_t *mp)
{
    int retval = 0;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));
    assert(mp != NULL);

    mtp_send_packet2(mc->mc_pilot->pc_handle,
		     MA_Opcode, MTP_REQUEST_REPORT,
		     MA_Role, MTP_ROLE_RMC,
		     MA_RobotID, mc->mc_pilot->pc_robot->id,
		     MA_TAG_DONE);
    
    mc->mc_pilot->pc_flags |= PCF_EXPECTING_RESPONSE;
    mc->mc_pilot->pc_connection_timeout = REPORT_RESPONSE_TIMEOUT;
    
    return retval;
}

static int mc_plot(struct master_controller *mc, mtp_packet_t *mp)
{
    float distance, theta;
    int retval = 0;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));
    assert(mp != NULL);

    mtp_polar(&mc->mc_plan.pp_actual_pos,
	      &mc->mc_plan.pp_goal_pos,
	      &distance,
	      &theta);

    if ((mc->mc_tries_remaining <= 0) ||
	(distance < mc_data.mcd_meter_tolerance)) {

	if (cmp_fuzzy(mc->mc_plan.pp_actual_pos.theta,
		      mc->mc_plan.pp_goal_pos.theta,
		      mc_data.mcd_radian_tolerance)) {
	    mc->mc_pause_time = ~0;
	    assert(mc->mc_self_obstacle == NULL);
	    mc->mc_self_obstacle = ob_add_robot(&mc->mc_plan.pp_actual_pos,
						mc->mc_pilot->pc_robot->id);
	    mtp_send_packet2(pc_data.pcd_emc_handle,
			     MA_Opcode, MTP_UPDATE_POSITION,
			     MA_Role, MTP_ROLE_RMC,
			     MA_Position, &mc->mc_plan.pp_actual_pos,
			     MA_RobotID, mc->mc_pilot->pc_robot->id,
			     MA_Status, MTP_POSITION_STATUS_COMPLETE,
			     MA_TAG_DONE);

	    pc_stats_stop_time(mc->mc_pilot);
	    pc_print_stats(mc->mc_pilot);
	}
	else {
	    mtp_send_packet2(mc->mc_pilot->pc_handle,
			     MA_Opcode, MTP_COMMAND_GOTO,
			     MA_Role, MTP_ROLE_RMC,
			     MA_RobotID, mc->mc_pilot->pc_robot->id,
			     MA_CommandID, MASTER_COMMAND_ID,
			     MA_Theta, (mc->mc_plan.pp_goal_pos.theta -
					mc->mc_plan.pp_actual_pos.theta),
			     MA_TAG_DONE);
	    
	    mc->mc_pilot->pc_flags |= PCF_EXPECTING_RESPONSE;
	    mc->mc_pilot->pc_connection_timeout = WIGGLE_RESPONSE_TIMEOUT;
	}
    }
    else {
	struct robot_position *rp = NULL, _rp;
 
	switch (pp_plot_waypoint(&mc->mc_plan)) {
	case PPC_NO_WAYPOINT:
	    rp = mtp_world2local(&_rp,
				 &mc->mc_plan.pp_actual_pos,
				 &mc->mc_plan.pp_goal_pos);
	    mc->mc_tries_remaining -= 1;

	    pc_stats_add_retry(mc->mc_pilot);

	    break;
	case PPC_WAYPOINT:
	    rp = mtp_world2local(&_rp,
				 &mc->mc_plan.pp_actual_pos,
				 &mc->mc_plan.pp_waypoint);
	    mc->mc_tries_remaining = mc_data.mcd_max_refine_retries;
	    break;
	case PPC_BLOCKED:
	case PPC_GOAL_IN_OBSTACLE:
	    mc->mc_pause_time = DEFAULT_PAUSE_TIME;
	    assert(mc->mc_self_obstacle == NULL);
	    mc->mc_self_obstacle = ob_add_robot(&mc->mc_plan.pp_actual_pos,
						mc->mc_pilot->pc_robot->id);
	    break;
	}

	if (rp != NULL) {
   
	    mtp_send_packet2(mc->mc_pilot->pc_handle,
			     MA_Opcode, MTP_COMMAND_GOTO,
			     MA_Role, MTP_ROLE_RMC,
			     MA_RobotID, mc->mc_pilot->pc_robot->id,
			     MA_CommandID, MASTER_COMMAND_ID,
			     MA_Position, rp,
			     MA_Speed, mc->mc_plan.pp_speed,
			     MA_TAG_DONE);
	    
	    mc->mc_pilot->pc_flags |= PCF_EXPECTING_RESPONSE;
	    mc->mc_pilot->pc_connection_timeout = MOVE_RESPONSE_TIMEOUT;
	}
    }

    return retval;
}


static int mc_nlwrapper(struct master_controller *mc, mtp_packet_t *mp) {
    /* DAN */
  
    float Vleft, Vright;
    robot_position_states rstates;
    
    
    /* get states from position data */
    mc_nlctr_getstates(&rstates,
		       &mc->mc_plan.pp_goal_pos,
		       &mc->mc_plan.pp_actual_pos);
      
    if (debug > 1) {  
	info("Current states (e,alpha,theta): %f %f %f\n",
	     rstates.e,
	     rstates.alpha,
	     rstates.theta);
    }
    
    /* if states are close to zero, send MTP_POSITION_STATUS_COMPLETE */
    if ((fabsf(rstates.alpha) < STATE_ATOL && fabsf(rstates.theta)) &&
	fabsf(rstates.e) < STATE_TOL) {
      
	info("Robot is at destination (%f)\n", rstates.e);
        
    /* tell EMCD */
	mtp_send_packet2(pc_data.pcd_emc_handle,
			 MA_Opcode, MTP_UPDATE_POSITION,
			 MA_Role, MTP_ROLE_RMC,
			 MA_Position, &mc->mc_plan.pp_actual_pos,
			 MA_RobotID, mc->mc_pilot->pc_robot->id,
			 MA_Status, MTP_POSITION_STATUS_COMPLETE,
			 MA_TAG_DONE);
        
	/* tell robot to STOP */
	mtp_send_packet2(mc->mc_pilot->pc_handle,
			 MA_Opcode, MTP_COMMAND_STOP,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, mc->mc_pilot->pc_robot->id,
			 MA_CommandID, MASTER_COMMAND_ID,
			 MA_TAG_DONE);
      
    } else {
	/* run controller */
	mc_nlctr_controller(&Vleft, &Vright, &rstates);
    
	if (debug > 1) {
	    info("Wheel speeds (L/R): %f %f\n", Vleft, Vright);
	}
    
	/* send to robot */
	mtp_send_packet2(mc->mc_pilot->pc_handle,
			 MA_Opcode, MTP_COMMAND_WHEELS,
			 MA_Role, MTP_ROLE_RMC,
			 MA_CommandID, MASTER_COMMAND_ID,
			 MA_RobotID, mc->mc_pilot->pc_robot->id,
			 MA_vleft, (double)(Vleft),
			 MA_vright, (double)(Vright),
			 MA_TAG_DONE);
    }
   
    
    return 0;
  
}




int mc_handle_emc_packet(struct master_controller *mc, mtp_packet_t *mp)
{
    int rc, retval = 0;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));
    assert(mp != NULL);

    rc = mtp_dispatch(mc, mp,
		      MD_Integer, mc->mc_flags,

		      MD_OnOpcode, MTP_COMMAND_GOTO,
		      MD_Call, mc_set_goal,

		      /*
		       * Always update the position before calling anything
		       * else.
		       */
		      MD_OnOpcode, MTP_UPDATE_POSITION,
		      MD_AlsoCall, mc_set_actual,

		      MD_OnOpcode, MTP_UPDATE_POSITION,
		      MD_OnStatus, MTP_POSITION_STATUS_MOVING,
		      MD_Call, mc_nlwrapper,

		      /* The sensors fired, get a report before moving. */
		      MD_OnFlags, MCF_CONTACT,
		      MD_OnOpcode, MTP_UPDATE_POSITION,
		      MD_Call, mc_request_report,

		      MD_OnOpcode, MTP_UPDATE_POSITION,
		      MD_Call, mc_plot,

		      MD_TAG_DONE);

    return retval;
}

static int mc_update_flags(struct master_controller *mc, mtp_packet_t *mp)
{
    mtp_status_t ms;
    int retval = 0;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));
    assert(mp != NULL);

    ms = mp->data.mtp_payload_u.update_position.status;
    
    switch (ms) {
    case MTP_POSITION_STATUS_CONTACT:
	mc->mc_flags |= MCF_CONTACT;
	break;

    default:
	mc->mc_flags &= ~MCF_CONTACT;
	break;
    }

    return retval;
}

static int mc_pause(struct master_controller *mc, mtp_packet_t *mp)
{
    int retval = 0;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));
    assert(mp != NULL);

    mc->mc_pilot->pc_flags &= ~PCF_EXPECTING_RESPONSE;
    mc->mc_pause_time = DEFAULT_PAUSE_TIME;
    if (mc->mc_self_obstacle == NULL)
	mc->mc_self_obstacle = ob_add_robot(&mc->mc_plan.pp_actual_pos,
					    mc->mc_pilot->pc_robot->id);
    
    return retval;
}

static int mc_request_position(struct master_controller *mc, mtp_packet_t *mp)
{
    int retval = 0;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));

    mc->mc_pause_time = 0;
    ob_rem_obstacle(mc->mc_self_obstacle);
    mc->mc_self_obstacle = NULL;
    mc->mc_pilot->pc_flags &= ~PCF_EXPECTING_RESPONSE;
    mtp_send_packet2(pc_data.pcd_emc_handle,
		     MA_Opcode, MTP_REQUEST_POSITION,
		     MA_Role, MTP_ROLE_RMC,
		     MA_RobotID, mc->mc_pilot->pc_robot->id,
		     MA_TAG_DONE);
    
    return retval;
}

static int mc_process_report(struct master_controller *mc, mtp_packet_t *mp)
{
    int lpc, compass = 0, retval = 0;
    struct mtp_contact_report *mcr;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));
    assert(mp != NULL);

    mc->mc_pilot->pc_flags &= ~PCF_EXPECTING_RESPONSE;
    mcr = &mp->data.mtp_payload_u.contact_report;

    for (lpc = 0; lpc < mcr->count; lpc++) {
	struct pilot_connection *pc;
	struct robot_position rp;
	struct contact_point cp;
	float local_bearing;

	local_bearing = atan2f(mcr->points[lpc].y, mcr->points[lpc].x);
	compass |= (mtp_compass(local_bearing) & (MCF_EAST|MCF_WEST));

	ob_obstacle_location(&cp,
			     &mc->mc_plan.pp_actual_pos,
			     &mcr->points[lpc]);
	rp.x = cp.x;
	rp.y = cp.y;
	pc = pc_find_pilot_by_location(&rp, 0.40, mc->mc_pilot);
	info("loc %p %u\n", pc, pc != NULL ? pc->pc_master.mc_pause_time : 0);
	if (pc == NULL) {
	    ob_found_obstacle(&mc->mc_plan.pp_actual_pos, &cp);
	}
	else if (pc->pc_master.mc_self_obstacle == NULL) {
	    compass = MCF_EAST|MCF_WEST;
	}
    }

    switch (compass) {
    case MCF_EAST|MCF_WEST:
	info("%s cannot move!\n", mc->mc_pilot->pc_robot->hostname);
	mc->mc_pause_time = DEFAULT_PAUSE_TIME;
	mc->mc_self_obstacle = ob_add_robot(&mc->mc_plan.pp_actual_pos,
					    mc->mc_pilot->pc_robot->id);
	break;
    case MCF_EAST:
    case MCF_WEST:
	info("%s detected an obstacle to the %s\n",
	     mc->mc_pilot->pc_robot->hostname,
	     MTP_COMPASS_STRING(compass));
	mtp_send_packet2(mc->mc_pilot->pc_handle,
			 MA_Opcode, MTP_COMMAND_GOTO,
			 MA_Role, MTP_ROLE_RMC,
			 MA_X, compass == MCF_EAST ? -0.1 : 0.1,
			 MA_RobotID, mc->mc_pilot->pc_robot->id,
			 MA_CommandID, MASTER_COMMAND_ID,
			 MA_Speed, 0.1,
			 MA_TAG_DONE);
	break;
    case 0:
	info("%s's obstacle disappeared\n", mc->mc_pilot->pc_robot->hostname);
	mc->mc_flags &= ~MCF_CONTACT;
	mc_request_position(mc, NULL);
	break;

    default:
	assert(0);
	break;
    }

    return retval;
}

int mc_handle_pilot_packet(struct master_controller *mc, mtp_packet_t *mp)
{
    int rc, retval = 0;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));
    assert(mp != NULL);

    rc = mtp_dispatch(mc, mp,

		      /* Ignore any packets not sent by this controller. */
		      MD_OnCommandID, SLAVE_COMMAND_ID,
		      MD_Return,

		      /* Always update some flags before other calls. */
		      MD_OnOpcode, MTP_UPDATE_POSITION,
		      MD_AlsoCall, mc_update_flags,

		      MD_OnOpcode, MTP_UPDATE_POSITION,
		      MD_OnStatus, MTP_POSITION_STATUS_ERROR,
		      MD_Call, mc_pause,

		      MD_OnOpcode, MTP_UPDATE_POSITION,
		      MD_Call, mc_request_position,

		      MD_OnOpcode, MTP_CONTACT_REPORT,
		      MD_Call, mc_process_report,

		      MD_TAG_DONE);

    return retval;
}

int mc_handle_switch(struct master_controller *mc)
{
    int retval = 0;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));

    retval = mc_request_position(mc, NULL);

    return retval;
}

int mc_handle_tick(struct master_controller *mc)
{
    int retval = 0;
    
    assert(mc != NULL);
    assert(mc_invariant(mc));

    if (mc->mc_pause_time > 0) {
	mc->mc_pause_time -= 1;

	if (mc->mc_pause_time == 0)
	    retval = mc_request_position(mc, NULL);
    }

    return retval;
}





void mc_nlctr_getstates(struct robot_position_states *robotcp,
                        struct robot_position *goalpos,
                        struct robot_position *robotpos) {
    /* calculate robot polar position states from current and goal positions
   * The goal position is the origin, with the current position offset
   */
  
    struct robot_position_states robotcp_out;
    float theta0;
    float xdiff, ydiff;
   
    
    assert(robotcp != NULL);
    assert(goalpos != NULL);
    assert(robotpos != NULL);
     
    
    
    xdiff = goalpos->x - robotpos->x;
    ydiff = -(goalpos->y - robotpos->y);
    
    theta0 = atan2(ydiff, xdiff);
    
     
     
    robotcp_out.e = sqrt(pow(xdiff,2) + pow(ydiff,2));
    robotcp_out.theta = theta0  - goalpos->theta;
    robotcp_out.alpha = theta0 - robotpos->theta;
    robotcp_out.timestamp = robotpos->timestamp;
      
    *robotcp = robotcp_out;
     
}


void mc_nlctr_controller(float *Vl, float *Vr, struct robot_position_states *robotcp) {
  
    
  
    float u_max = 0.4f; /* u saturation */
    float C_u, C_omega; /* controller outputs */
          
    /* controller parameters: */
    float K_gamma = 1.0f, K_h = 1.0f, K_k = 1.0f;
          
    float K_radius = 0.0889f;
    // float C_max = 26.25f;
    float C_max = 13.125f;
  
    
    assert(robotcp != NULL);
    
    
    /* controller: */
    C_u = u_max * tanh(K_gamma * cos(robotcp->alpha) * robotcp->e);
            
    if (0 == robotcp->alpha) {
	C_omega = 0.0f;
    }
    else {
	C_omega = K_k * robotcp->alpha + K_gamma *
	    ((cos(robotcp->alpha)*sin(robotcp->alpha)) / robotcp->alpha) *
	    (robotcp->alpha + K_h * robotcp->theta);
        
	if (C_omega > C_max) {
	    C_omega = C_max;
	}
	if (C_omega < -C_max) {
	    C_omega = -C_max;
	}
    }
    /* end controller */
            
    /* wheel velocity translator: */
    *Vl = C_u - K_radius * C_omega;
    *Vr = C_u + K_radius * C_omega;
    /* end wheel velocity translator */
  
}
