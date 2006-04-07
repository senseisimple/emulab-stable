/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file pilotClient.cc
 *
 * Implementation file for the pilotClient class.
 */

#include "config.h"

#include <math.h>
#include <sys/time.h>

#include "garciaUtil.hh"
#include "pilotClient.hh"

#define DEFAULT_ROBOT_ACCELERATION 0.4f

/**
 * Callback passed to the wheelManager when performing movements.
 */
class pilotMoveCallback : public wmCallback
{

public:

    pilotMoveCallback(pilotClient::list &pcl, int command_id, float theta);

    /**
     * Callback that will send an MTP_UPDATE_POSITION packet back to the client
     * with the appropriate status value.
     *
     * @param status The completion status of the move.
     */
    virtual void call(int status, float odometer);

private:

    pilotClient::list &pmc_notify_list;

    int pmc_command_id;

    float pmc_theta;
};

pilotMoveCallback::pilotMoveCallback(pilotClient::list &pcl,
				     int command_id,
				     float theta)
    : pmc_notify_list(pcl),
      pmc_command_id(command_id),
      pmc_theta(theta)
{
}

void pilotMoveCallback::call(int status, float odometer)
{
    pilotClient::iterator i;
    mtp_packet_t mp;
    mtp_status_t ms;

    if (debug) {
	fprintf(stderr, "debug: callback status -- %d\n", status);
    }

    switch (status) {
    case aGARCIA_ERRFLAG_NORMAL:
	ms = MTP_POSITION_STATUS_COMPLETE;
	break;
    case aGARCIA_ERRFLAG_ABORT:
    case aGARCIA_ERRFLAG_WONTEXECUTE:
	ms = MTP_POSITION_STATUS_ABORTED;
	break;
    case aGARCIA_ERRFLAG_STALL:
    case aGARCIA_ERRFLAG_FRONTR_LEFT:
    case aGARCIA_ERRFLAG_FRONTR_RIGHT:
    case aGARCIA_ERRFLAG_REARR_LEFT:
    case aGARCIA_ERRFLAG_REARR_RIGHT:
    case aGARCIA_ERRFLAG_SIDER_LEFT:
    case aGARCIA_ERRFLAG_SIDER_RIGHT:
	ms = MTP_POSITION_STATUS_CONTACT;
	break;
    default:
	fprintf(stderr, "error: couldn't complete move %d\n", status);
	ms = MTP_POSITION_STATUS_ERROR;
	break;
    }

    mtp_init_packet(&mp,
		    MA_Opcode, MTP_UPDATE_POSITION,
		    MA_Role, MTP_ROLE_ROBOT,
		    MA_Status, ms,
		    MA_X, cosf(this->pmc_theta) * odometer,
		    MA_Y, sinf(this->pmc_theta) * odometer,
		    MA_Theta, this->pmc_theta,
		    MA_CommandID, this->pmc_command_id,
		    MA_TAG_DONE);

    if (debug > 1) {
	fprintf(stderr, "debug: sending update\n");
	mtp_print_packet(stderr, &mp);
    }

    for (i = this->pmc_notify_list.begin();
	 i != this->pmc_notify_list.end();
	 i++) {
	pilotClient *pc = *i;

	mtp_send_packet(pc->getHandle(), &mp);
    }
}

pilotClient *pilotClient::pc_rmc_client = NULL;

pilotClient::pilotClient(int fd, wheelManager &wm, dashboard &db)
    : pc_wheel_manager(wm), pc_dashboard(db)
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
	    fprintf(stderr, "debug: client disconnected\n");
	}
    }
}




bool pilotClient::handlePacket(mtp_packet_t *mp, list &notify_list)
{
    struct mtp_command_startnull *mcsn;
    struct mtp_command_goto *mcg;
    struct mtp_command_stop *mcs;
    struct mtp_command_wheels *mcw;
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
		    fprintf(stderr,
			    "debug: rmc client connected - %s\n",
			    mp->data.mtp_payload_u.init.msg);
		}

		pilotClient::pc_rmc_client = this;

		retval = true;
	    }
	    break;
	case MTP_ROLE_EMULAB:
	    if (debug) {
		fprintf(stderr,
			"debug: emulab client connected - %s\n",
			mp->data.mtp_payload_u.init.msg);
	    }

	    retval = true;
	    break;
	}
	break;

    case MTP_COMMAND_GOTO:
	if (this->pc_role == MTP_ROLE_RMC) {
	    pilotClient::iterator i;
	    struct mtp_packet cmp;

	    mcg = &mp->data.mtp_payload_u.command_goto;

	    cmp = *mp;
	    cmp.role = MTP_ROLE_ROBOT;
	    for (i = notify_list.begin(); i != notify_list.end(); i++) {
		pilotClient *pc = *i;

		if (pc->getRole() == MTP_ROLE_EMULAB) {
		    mtp_send_packet(pc->getHandle(), &cmp);
		}
	    }

	    /* Figure out if they want us to do a move or a pivot. */
	    if ((mcg->position.x != 0.0) || (mcg->position.y != 0.0)) {
		acpValue av;
		float theta;

		if (debug) {
		    fprintf(stderr,
			    "debug: move to %f %f\n",
			    mcg->position.x,
			    mcg->position.y);
		}

		this->pc_wheel_manager.setSpeed(mcg->speed);

		/* XXX Need to figure out the real theta and send that back. */
		theta = atan2f(mcg->position.y, mcg->position.x);
		this->pc_wheel_manager.
		    setDestination(mcg->position.x,
				   mcg->position.y,
				   new pilotMoveCallback(notify_list,
							 mcg->command_id,
							 theta));
	    }
	    else {
		if (debug) {
		    fprintf(stderr,
			    "debug: reorient to %f\n",
			    mcg->position.theta);
		}

		this->pc_wheel_manager.setSpeed(mcg->speed);
		this->pc_wheel_manager.setOrientation(
						      mcg->position.theta,
						      new pilotMoveCallback(notify_list,
									    mcg->command_id,
									    mcg->position.theta));
	    }

	    retval = true;
	}
	break;

    case MTP_COMMAND_STOP:
	if (this->pc_role == MTP_ROLE_RMC) {
	    struct mtp_packet cmp, rmp;
	    pilotClient::iterator i;
	    bool send_idle = false;

	    mcs = &mp->data.mtp_payload_u.command_stop;

	    if (debug) {
		fprintf(stderr, "debug: full stop\n");
	    }

	    if (!this->pc_wheel_manager.stop()) {
		mtp_init_packet(&rmp,
				MA_Opcode, MTP_UPDATE_POSITION,
				MA_Role, MTP_ROLE_ROBOT,
				MA_Status, MTP_POSITION_STATUS_IDLE,
				MA_CommandID, mcs->command_id,
				MA_TAG_DONE);

		send_idle = true;
	    }

	    cmp = *mp;
	    cmp.role = MTP_ROLE_ROBOT;
	    for (i = notify_list.begin(); i != notify_list.end(); i++) {
		pilotClient *pc = *i;

		if (pc->getRole() == MTP_ROLE_EMULAB)
		    mtp_send_packet(pc->getHandle(), &cmp);
		if (send_idle)
		    mtp_send_packet(pc->getHandle(), &rmp);
	    }

	    retval = true;
	}
	break;
	/* DAN */
    case MTP_COMMAND_STARTNULL:
	if (this->pc_role == MTP_ROLE_RMC) {
	    mcsn = &mp->data.mtp_payload_u.command_startnull;
	                
	    /* start NULL command */
	    this->pc_wheel_manager.
		startNULL(mcsn->acceleration, 
			  new pilotMoveCallback(notify_list,
						mcsn->command_id, 0.0f));
            
	    retval = true;
	}
	break;
    case MTP_COMMAND_WHEELS:
	if (this->pc_role == MTP_ROLE_RMC) {

	    mcw = &mp->data.mtp_payload_u.command_wheels;
	    if (debug > 1) {
		fprintf(stderr,
			"debug: set wheels to: %f %f\n",
			mcw->vleft,
			mcw->vright);
	    }
	    this->pc_wheel_manager.setWheels(mcw->vleft, mcw->vright);
	    retval = true;
            
	}
	break;


    case MTP_REQUEST_REPORT:
	{
	    struct mtp_garcia_telemetry *mgt;
	    struct contact_point points[8];
	    pilotClient::iterator i;
	    struct mtp_packet cmp;
	    int count = 0;

	    mgt = this->pc_dashboard.getTelemetry();

	    /* Compute "contacts" based on the sensor reports. */
	    if ((mgt->front_ranger_left != 0.0) &&
		(mgt->front_ranger_right != 0.0)) {
		float min_range;

		if (mgt->front_ranger_left < mgt->front_ranger_right)
		    min_range = mgt->front_ranger_left;
		else
		    min_range = mgt->front_ranger_right;
		points[count].x = cos(0.0) * min_range;
		points[count].y = sin(0.0) * min_range;
		count += 1;
	    }
	    else if (mgt->front_ranger_left != 0.0) {
		/*
		 * The front sensors are angled out a bit, the 0.40 angle seems
		 * to work well enough.
		 */
		points[count].x = cos(0.40) * mgt->front_ranger_left;
		points[count].y = sin(0.40) * mgt->front_ranger_left;
		count += 1;
	    }
	    else if (mgt->front_ranger_right != 0.0) {
		points[count].x = cos(-0.40) * mgt->front_ranger_right;
		points[count].y = sin(-0.40) * mgt->front_ranger_right;
		count += 1;
	    }

	    if (mgt->side_ranger_left != 0.0) {
		points[count].x = cos(M_PI_2) * mgt->side_ranger_left;
		points[count].y = sin(M_PI_2) * mgt->side_ranger_left;
		count += 1;
	    }

	    if (mgt->side_ranger_right != 0.0) {
		points[count].x = cos(-M_PI_2) * mgt->side_ranger_right;
		points[count].y = sin(-M_PI_2) * mgt->side_ranger_right;
		count += 1;
	    }

	    if ((mgt->rear_ranger_left != 0.0) &&
		(mgt->rear_ranger_right != 0.0)) {
		float min_range;

		if (mgt->rear_ranger_left < mgt->rear_ranger_right)
		    min_range = mgt->rear_ranger_left;
		else
		    min_range = mgt->rear_ranger_right;
		points[count].x = cos(M_PI) * min_range;
		points[count].y = sin(M_PI) * min_range;
		count += 1;
	    }
	    else if (mgt->rear_ranger_left != 0.0) {
		/*
		 * XXX This computation is probably wrong...  We should have a
		 * constant offset for Y and set X to be the ranger value plus
		 * some constant.  I don't remember exactly what the ranger
		 * value means for these (i.e. if it is based off the center
		 * of the robot or not).
		 */
		points[count].x = cos(M_PI - 0.40) * mgt->rear_ranger_left;
		points[count].y = sin(M_PI - 0.40) * mgt->rear_ranger_left;
		count += 1;
	    }
	    else if (mgt->rear_ranger_right != 0.0) {
		points[count].x = cos(-M_PI + 0.40) * mgt->rear_ranger_right;
		points[count].y = sin(-M_PI + 0.40) * mgt->rear_ranger_right;
		count += 1;
	    }
	    else if (mgt->stall_contact && count == 0) {
		points[count].x = (mgt->stall_contact < 0) ? -0.20 : 0.15;
		points[count].y = 0;
		count += 1;
	    }

	    mtp_init_packet(&cmp,
			    MA_Opcode, MTP_CONTACT_REPORT,
			    MA_Role, MTP_ROLE_ROBOT,
			    MA_ContactPointCount, count,
			    MA_ContactPoints, points,
			    MA_TAG_DONE);

	    for (i = notify_list.begin(); i != notify_list.end(); i++) {
		pilotClient *pc = *i;

		mtp_send_packet(pc->getHandle(), &cmp);
	    }

	    retval = true;
	}
	break;

    default:
	fprintf(stderr, "warning: unhandled opcode - %d\n", mp->data.opcode);
	break;
    }

    return retval;
}
