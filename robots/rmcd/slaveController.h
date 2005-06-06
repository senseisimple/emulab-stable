/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _rmcd_slave_controller_h
#define _rmcd_slave_controller_h

/**
 * @file slaveController.h
 *
 * Header file for the "slave" controller.  The slave controller takes over
 * from the master controller when one of the other daemons, emcd/vmcd, wants
 * to control the robot.  For example, when vmcd needs to wiggle the robot, the
 * commands are dealt with by this controller until the wiggle is completed and
 * RMCD is back in control of the robot's movements.
 */

#include "mtp.h"

/**
 * Forward declaration of the pilot_connection since the controller has a back
 * reference to it and the connection contains an instance of the controller.
 */
struct pilot_connection;

/**
 * Enumeration of the slave controller states.
 */
typedef enum {
    SS_IDLE,		/*< The slave controller is inactive. */
    SS_START_WIGGLING,	/*< Stop the robot in preparation for a wiggle. */
    SS_WIGGLING,	/*< Perform the wiggle. */

    SS_MAX
} slave_state_t;

/**
 * The slave controller structure.  Currently, it just tracks the state of a
 * wiggle.
 */
struct slave_controller {
    struct pilot_connection *sc_pilot;	/*< Reference back to the connection */
    slave_state_t sc_state;
};

/**
 * Check the slave controller object against the following invariants:
 *
 * @li sc_pilot != NULL
 * @li sc_state is a valid slave_state_t value.
 *
 * @param sc An initialized slave controller object.
 * @return True.
 */
int sc_invariant(struct slave_controller *sc);

/**
 * Function called when the pilot connection is lost.  Currently, it checks if
 * a wiggle was in progress and sends an error to emcd, so vmcd can continue.
 * After executing, the controller will always be back in the idle state.
 *
 * @param sc An initialized slave controller object.
 */
void sc_disconnected(struct slave_controller *sc);

/**
 * Dispatches packets received from the pilot being controlled.
 *
 * @param sc An initialized slave controller object.
 * @param mp The packet to dispatch.
 */
int sc_handle_pilot_packet(struct slave_controller *sc, mtp_packet_t *mp);

/**
 * Dispatches packets received from emcd and intended for the pilot being
 * controlled.
 *
 * @param sc An initialized slave controller object.
 * @param mp The packet to dispatch.
 */
int sc_handle_emc_packet(struct slave_controller *sc, mtp_packet_t *mp);

#endif
