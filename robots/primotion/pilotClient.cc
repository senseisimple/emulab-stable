/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file pilotClient.cc
 *
 * Implementation file for the pilotClient class.
 */

#include "config.h"

#include "pilotClient.hh"

/**
 * Callback passed to the wheelManager when performing movements.
 */
class pilotMoveCallback : public wmCallback
{

public:

    /**
     * Callback that will send an MTP_UPDATE_POSITION packet back to the client
     * with the appropriate status value.
     *
     * @param status The completion status of the move.
     */
    virtual void call(int status);

};

void pilotMoveCallback::call(int status)
{
    mtp_packet_t mp;
    mtp_status_t ms;
    
    switch (status) {
    case aGARCIA_ERRFLAG_NORMAL:
	ms = MTP_POSITION_STATUS_COMPLETE;
	break;
    case aGARCIA_ERRFLAG_ABORT:
    case aGARCIA_ERRFLAG_WONTEXECUTE:
	ms = MTP_POSITION_STATUS_ABORTED;
	break;
    default:
	ms = MTP_POSITION_STATUS_ERROR;
	break;
    }

    mtp_init_packet(&mp,
		    MA_Opcode, MTP_UPDATE_POSITION,
		    MA_Role, MTP_ROLE_ROBOT,
		    MA_Status, ms,
		    MA_TAG_DONE);

    if (pilotClient::pc_rmc_client != NULL) {
	mtp_send_packet(pilotClient::pc_rmc_client->getHandle(), &mp);
    }
}

pilotClient *pilotClient::pc_rmc_client = NULL;

pilotClient::pilotClient(int fd, wheelManager &wm)
    : pc_wheel_manager(wm)
{
    assert(fd > 0);

    this->pc_handle = mtp_create_handle(fd);
    this->pc_role = (mtp_role_t)0;
}

pilotClient::~pilotClient()
{
    close(this->pc_handle->mh_fd);
    mtp_delete_handle(this->pc_handle);
    this->pc_handle = NULL;

    if (pc_rmc_client == this) {
	if (debug) {
	    fprintf(stderr, "debug: rmc client disconnected\n");
	}
	
	this->pc_wheel_manager.stop();
	pc_rmc_client = NULL;
    }
    else {
	if (debug) {
	    fprintf(stderr, "debug: emulab client disconnected\n");
	}
    }
}

bool pilotClient::handlePacket(mtp_packet_t *mp)
{
    struct mtp_command_goto *mcg;
    struct mtp_control *mc;
    bool retval = false;

    assert(mp != NULL);

    if (debug > 1) {
	fprintf(stderr, "received: ");
	mtp_print_packet(stderr, mp);
    }

    switch (mp->data.opcode) {
    case MTP_CONTROL_INIT:
	mc = &mp->data.mtp_payload_u.init;
	this->pc_role = (mtp_role_t)mp->role;
	switch (mp->role) {
	case MTP_ROLE_RMC:
	    if (pilotClient::pc_rmc_client == NULL) {
		if (debug) {
		    fprintf(stderr, "debug: rmc client connected\n");
		}
		
		pilotClient::pc_rmc_client = this;
		
		retval = true;
	    }
	    break;
	case MTP_ROLE_EMULAB:
	    if (debug) {
		fprintf(stderr, "debug: emulab client connected\n");
	    }
	    
	    retval = true;
	    break;
	}
	break;
	
    case MTP_COMMAND_GOTO:
	if (this->pc_role == MTP_ROLE_RMC) {
	    mcg = &mp->data.mtp_payload_u.command_goto;
	    if ((mcg->position.x != 0.0) || (mcg->position.y != 0.0)) {
		if (debug) {
		    fprintf(stderr,
			    "debug: move to %f %f\n",
			    mcg->position.x,
			    mcg->position.y);
		}
		
		this->pc_wheel_manager.
		    setDestination(mcg->position.x,
				   mcg->position.y,
				   new pilotMoveCallback());
	    }
	    else {
		if (debug) {
		    fprintf(stderr,
			    "debug: reorient to %f\n",
			    mcg->position.theta);
		}
		
		this->pc_wheel_manager.setOrientation(mcg->position.theta,
						      new pilotMoveCallback());
	    }
	    
	    retval = true;
	}
	break;

    case MTP_COMMAND_STOP:
	if (this->pc_role == MTP_ROLE_RMC) {
	    struct mtp_packet rmp;

	    if (debug) {
		fprintf(stderr, "debug: full stop\n");
	    }
	    
	    this->pc_wheel_manager.stop();
	    mtp_init_packet(&rmp,
			    MA_Opcode, MTP_UPDATE_POSITION,
			    MA_Role, MTP_ROLE_ROBOT,
			    MA_Status, MTP_POSITION_STATUS_IDLE,
			    MA_TAG_DONE);
	    mtp_send_packet(this->pc_handle, &rmp);

	    retval = true;
	}
	break;

    default:
	fprintf(stderr, "warning: unhandled opcode - %d\n", mp->data.opcode);
	break;
    }
    
    return retval;
}
