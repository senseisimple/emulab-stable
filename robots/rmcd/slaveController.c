/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file slaveController.c
 *
 * Implementation of the "slave" controller.
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "log.h"
#include "rmcd.h"
#include "pilotConnection.h"
#include "slaveController.h"

static int sc_wiggling(struct slave_controller *sc, mtp_packet_t *mp);

int sc_invariant(struct slave_controller *sc)
{
    assert(sc != NULL);
    assert(sc->sc_pilot != NULL);
    assert(sc->sc_state >= 0);
    assert(sc->sc_state < SS_MAX);

    return 1;
}

void sc_disconnected(struct slave_controller *sc)
{
    assert(sc != NULL);
    assert(sc_invariant(sc));
    assert(sc->sc_pilot->pc_state == PCS_DISCONNECTED);
    
    if (sc->sc_state != SS_IDLE) {
	if (debug > 1) {
	    info("%s dropped connection mid-wiggle\n",
		 sc->sc_pilot->pc_robot->hostname);
	}
    
	mtp_send_packet2(pc_data.pcd_emc_handle,
			 MA_Opcode, MTP_WIGGLE_STATUS,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, sc->sc_pilot->pc_robot->id,
			 MA_Status, MTP_POSITION_STATUS_ERROR,
			 MA_TAG_DONE);

	sc->sc_state = SS_IDLE;
    }
}

/**
 * Pass an update from pilot on to emcd.
 *
 * @param sc An initialized slave controller object.
 * @param mp The update-position packet to handle.
 */
static int sc_pass_update(struct slave_controller *sc, mtp_packet_t *mp)
{
    mtp_status_t ms;
    int retval = 0;
    
    assert(sc != NULL);
    assert(sc_invariant(sc));
    assert((sc->sc_state == SS_START_WIGGLING) ||
	   (sc->sc_state == SS_WIGGLING));
    assert(mp != NULL);

    sc->sc_pilot->pc_flags &= ~PCF_EXPECTING_RESPONSE;

    ms = mp->data.mtp_payload_u.update_position.status;
    switch (ms) {
    case MTP_POSITION_STATUS_ABORTED:
	if (sc->sc_state == SS_START_WIGGLING) {
	    /*
	     * If the COMMAND_STOP aborted something in-progress, we want to
	     * change the status to IDLE so vmcd knows it's OK to proceed.
	     */
	    ms = MTP_POSITION_STATUS_IDLE;
	}
	break;
    default:
	break;
    }
    
    if (debug > 1)
	info("%s wiggle-status %d\n", sc->sc_pilot->pc_robot->hostname, ms);
    
    mtp_send_packet2(pc_data.pcd_emc_handle,
		     MA_Opcode, MTP_WIGGLE_STATUS,
		     MA_Role, MTP_ROLE_RMC,
		     MA_RobotID, sc->sc_pilot->pc_robot->id,
		     MA_Status, ms,
		     MA_TAG_DONE);
    
    if (sc->sc_state == SS_WIGGLING)
	sc->sc_state = SS_IDLE; // all done wiggling for now.
    
    return retval;
}

static int sc_request_report(struct slave_controller *sc, mtp_packet_t *mp)
{
    int retval = 0;
    
    assert(sc != NULL);
    assert(sc_invariant(sc));
    assert(mp != NULL);

    info("%s is obstructed and cannot wiggle!\n",
	 sc->sc_pilot->pc_robot->hostname);
    
    mtp_send_packet2(sc->sc_pilot->pc_handle,
		     MA_Opcode, MTP_REQUEST_REPORT,
		     MA_Role, MTP_ROLE_RMC,
		     MA_RobotID, sc->sc_pilot->pc_robot->id,
		     MA_TAG_DONE);
    
    sc->sc_pilot->pc_flags |= PCF_EXPECTING_RESPONSE;
    sc->sc_pilot->pc_connection_timeout = REPORT_RESPONSE_TIMEOUT;
    
    return retval;
}

static int sc_process_report(struct slave_controller *sc, mtp_packet_t *mp)
{
    int lpc, compass = 0, retval = 0;
    struct mtp_contact_report *mcr;
    
    assert(sc != NULL);
    assert(sc_invariant(sc));
    assert(mp != NULL);

    sc->sc_pilot->pc_flags &= ~PCF_EXPECTING_RESPONSE;
    mcr = &mp->data.mtp_payload_u.contact_report;

    for (lpc = 0; lpc < mcr->count; lpc++) {
	float local_bearing;

	local_bearing = atan2f(mcr->points[lpc].y, mcr->points[lpc].x);
	compass |= (mtp_compass(local_bearing) & (MCF_EAST|MCF_WEST));
    }

    switch (compass) {
    case MCF_EAST|MCF_WEST:
	info("%s cannot wiggle!\n", sc->sc_pilot->pc_robot->hostname);
	mtp_send_packet2(pc_data.pcd_emc_handle,
			 MA_Opcode, MTP_WIGGLE_STATUS,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, sc->sc_pilot->pc_robot->id,
			 MA_Status, MTP_POSITION_STATUS_ERROR,
			 MA_TAG_DONE);
	break;
	
    case MCF_EAST:
    case MCF_WEST:
	mtp_send_packet2(sc->sc_pilot->pc_handle,
			 MA_Opcode, MTP_COMMAND_GOTO,
			 MA_Role, MTP_ROLE_RMC,
			 MA_X, compass == MCF_EAST ? -0.1 : 0.1,
			 MA_RobotID, sc->sc_pilot->pc_robot->id,
			 MA_CommandID, SLAVE_COMMAND_ID,
			 MA_Speed, 0.1,
			 MA_TAG_DONE);
	
	sc->sc_pilot->pc_flags |= PCF_EXPECTING_RESPONSE;
	sc->sc_pilot->pc_connection_timeout = MOVE_RESPONSE_TIMEOUT;
	break;
	
    case 0:
	mtp_send_packet2(sc->sc_pilot->pc_handle,
			 MA_Opcode, MTP_COMMAND_GOTO,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, sc->sc_pilot->pc_robot->id,
			 MA_CommandID, SLAVE_COMMAND_ID,
			 MA_Theta, M_PI,
			 MA_TAG_DONE);
	
	sc->sc_pilot->pc_flags |= PCF_EXPECTING_RESPONSE;
	sc->sc_pilot->pc_connection_timeout = WIGGLE_RESPONSE_TIMEOUT;
	break;

    default:
	assert(0);
	break;
    }

    return retval;
}

int sc_handle_pilot_packet(struct slave_controller *sc, mtp_packet_t *mp)
{
    int rc, retval = 0;

    assert(sc != NULL);
    assert(sc_invariant(sc));
    assert(sc->sc_pilot->pc_control_mode != PCM_NONE);
    assert(mp != NULL);

    rc = mtp_dispatch(sc, mp,
		      MD_Integer, sc->sc_state,

		      MD_OnInteger, SS_WIGGLING,
		      MD_OnOpcode, MTP_UPDATE_POSITION,
		      MD_OnStatus, MTP_POSITION_STATUS_CONTACT,
		      MD_OnCommandID, SLAVE_COMMAND_ID,
		      MD_Call, sc_request_report,

		      MD_OnOpcode, MTP_CONTACT_REPORT,
		      MD_Call, sc_process_report,

		      /* Pass any updates on to emcd/vmcd. */
		      MD_OR | MD_OnInteger, SS_START_WIGGLING,
		      MD_OnInteger, SS_WIGGLING,
		      MD_OnOpcode, MTP_UPDATE_POSITION,
		      MD_OnCommandID, SLAVE_COMMAND_ID,
		      MD_Call, sc_pass_update,

		      /*
		       * Pass on updates with the MASTER_COMMAND_ID as well.
		       * This should only happen when aborting a running
		       * command.
		       */
		      MD_OnInteger, SS_START_WIGGLING,
		      MD_OnOpcode, MTP_UPDATE_POSITION,
		      MD_OnStatus, MTP_POSITION_STATUS_ABORTED,
		      MD_OnCommandID, MASTER_COMMAND_ID,
		      MD_Call, sc_pass_update,

		      MD_TAG_DONE);
    
    return retval;
}

/**
 * Handle the start of a wiggle.  If we do not have a pilot connection, just
 * send back an error.  Otherwise, tell pilot to stop whatever it is doing at
 * the moment.
 *
 * @param sc An initialized slave controller object.
 * @param mp The wiggle-request packet to handle.
 */
static int sc_start_wiggling(struct slave_controller *sc, mtp_packet_t *mp)
{
    int retval = 0;
    
    assert(sc != NULL);
    assert(sc_invariant(sc));
    assert(mp != NULL);

    if (sc->sc_pilot->pc_control_mode == PCM_NONE) {
	if (debug > 1)
	    info("%s is not connected\n", sc->sc_pilot->pc_robot->hostname);
    
	mtp_send_packet2(pc_data.pcd_emc_handle,
			 MA_Opcode, MTP_WIGGLE_STATUS,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, sc->sc_pilot->pc_robot->id,
			 MA_Status, MTP_POSITION_STATUS_ERROR,
			 MA_TAG_DONE);
    }
    else {
	if (debug > 1)
	    info("start wiggle for %s\n", sc->sc_pilot->pc_robot->hostname);
	
	mtp_send_packet2(sc->sc_pilot->pc_handle,
			 MA_Opcode, MTP_COMMAND_STOP,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, sc->sc_pilot->pc_robot->id,
			 MA_CommandID, SLAVE_COMMAND_ID,
			 MA_TAG_DONE);

	sc->sc_pilot->pc_flags |= PCF_EXPECTING_RESPONSE;
	sc->sc_pilot->pc_connection_timeout = STOP_RESPONSE_TIMEOUT;
	
	sc->sc_state = SS_START_WIGGLING;
    }
    
    return retval;
}

/**
 * Handle the actual wiggle.  If we do not have a pilot connection, just send
 * back an error.  Otherwise, tell pilot to pivot 180 degrees in one direction
 * or the other, based on the SCF_REVERSE flag.
 *
 * @param sc An initialized slave controller object.
 * @param mp The wiggle-request packet to handle.
 */
static int sc_wiggling(struct slave_controller *sc, mtp_packet_t *mp)
{
    int retval = 0;
    
    assert(sc != NULL);
    assert(sc_invariant(sc));
    assert(mp != NULL);

    if (sc->sc_pilot->pc_control_mode == PCM_NONE) {
	if (debug > 1)
	    info("%s is not connected\n", sc->sc_pilot->pc_robot->hostname);
	
	mtp_send_packet2(pc_data.pcd_emc_handle,
			 MA_Opcode, MTP_WIGGLE_STATUS,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, sc->sc_pilot->pc_robot->id,
			 MA_Status, MTP_POSITION_STATUS_ERROR,
			 MA_TAG_DONE);
    }
    else {
	assert(sc->sc_pilot->pc_state == PCS_CONNECTED);
	assert(sc->sc_pilot->pc_control_mode == PCM_SLAVE);
	
	if (debug > 1)
	    info("wiggling %s\n", sc->sc_pilot->pc_robot->hostname);
	
	mtp_send_packet2(sc->sc_pilot->pc_handle,
			 MA_Opcode, MTP_COMMAND_GOTO,
			 MA_Role, MTP_ROLE_RMC,
			 MA_RobotID, sc->sc_pilot->pc_robot->id,
			 MA_CommandID, SLAVE_COMMAND_ID,
			 MA_Theta, M_PI,
			 MA_TAG_DONE);
	
	sc->sc_pilot->pc_flags |= PCF_EXPECTING_RESPONSE;
	sc->sc_pilot->pc_connection_timeout = WIGGLE_RESPONSE_TIMEOUT;
	
	sc->sc_state = SS_WIGGLING;
    }
    
    return retval;
}

int sc_handle_emc_packet(struct slave_controller *sc, mtp_packet_t *mp)
{
    int rc, retval = 0;

    assert(sc != NULL);
    assert(sc_invariant(sc));
    assert(mp != NULL);

    rc = mtp_dispatch(sc, mp,
		      MD_Integer, sc->sc_state,
		      
		      MD_OnInteger, SS_IDLE,
		      MD_OnOpcode, MTP_WIGGLE_REQUEST,
		      MD_OnWiggleType, MTP_WIGGLE_START,
		      MD_Call, sc_start_wiggling,
		    
		      MD_OnInteger, SS_START_WIGGLING,
		      MD_OnOpcode, MTP_WIGGLE_REQUEST,
		      MD_OnWiggleType, MTP_WIGGLE_180_R,
		      MD_Call, sc_wiggling,

		      MD_Return,
		      
		      MD_TAG_DONE);
    
    return retval;
}
