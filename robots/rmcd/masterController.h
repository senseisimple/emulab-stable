/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _rmcd_master_controller_h
#define _rmcd_master_controller_h

/**
 * @file masterController.h
 *
 * Header file for the "master" controller.  The master controller is used when
 * RMCD is in sole control of the robot's movements.  This controller is
 * responsible for communicating with the pilot, calling the path planner, and
 * constructing dynamic obstacles as they are detected.
 */

#include "mtp.h"
#include "pathPlanning.h"

/**
 * Forward declaration of the pilot_connection since the controller has a back
 * reference to it and the connection contains an instance of the controller.
 */
struct pilot_connection;

enum {
  MCB_CONTACT,
};

enum {
  MCF_CONTACT = (1L << MCB_CONTACT),	/*< Robot made contact with an
					  obstacle. */
};

/**
 *
 */
struct master_controller {
  struct pilot_connection *mc_pilot;
  unsigned long mc_flags;
  unsigned long mc_pause_time;
  int mc_tries_remaining;
  unsigned int mc_waypoint_tries;
  struct path_plan mc_plan;
  struct obstacle_node *mc_self_obstacle;
};

#define DEFAULT_PAUSE_TIME 10
#define MAX_CONTACT_DISTANCE 0.36

/**
 * Dispatch a packet received from the pilot.  We expect to only receive
 * position update packets and contact reports if an obstacle was detected.
 *
 * @param mc An initialized master controller object.
 * @param mp The packet to dispatch.
 */
int mc_handle_pilot_packet(struct master_controller *mc, mtp_packet_t *mp);

/**
 * Dispatch a packet received from emcd for this robot.  We expect to only
 * receive vision position updates and goto commands.
 *
 * @param mc An initialized master controller object.
 * @param mp The packet to dispatch.
 */
int mc_handle_emc_packet(struct master_controller *mc, mtp_packet_t *mp);

/**
 * Handle the switch from the slave controller to the master.
 *
 * @param mc An initialized master controller object.
 */
int mc_handle_switch(struct master_controller *mc);

/**
 * Handle a timer tick.  Currently, this function is used to restart a
 * controller that was paused for some amount of time.
 *
 * @param mc An initialized master controller object.
 */
int mc_handle_tick(struct master_controller *mc);

struct master_controller_data {
  /**
   * Maximum number of times to try and refine the position before giving up.
   */
  unsigned int mcd_max_refine_retries;
    
  /**
   * How close does the angle have to be before it is considered at the
   * intended angle.
   */
  float mcd_radian_tolerance; 
    
  /**
   * How close does the robot have to be before it is considered at the
   * intended position.  Measurement is in meters(?).
   */
  float mcd_meter_tolerance;
};

/**
 *
 */
extern struct master_controller_data mc_data;





void mc_nlctr_getstates(struct robot_position_states *robotcp,
			struct robot_position *goalpos,
			struct robot_position *robotpos);
void mc_nlctr_controller(float *Vl, float *Vr, struct robot_position_states *robotcp);




#endif
