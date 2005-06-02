/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <stdio.h>

#include "dashboard.hh"
#include "wheelManager.hh"
#include "faultDetection.hh"

faultCallback::~faultCallback()
{
}

faultDetection::faultDetection(acpGarcia &garcia, dashboard *db)
    : fd_garcia(garcia),
      fd_dashboard(db),
      fd_callback(NULL),
      fd_distance_limit(IDLE_DISTANCE_LIMIT),
      fd_velocity_limit(IDLE_VELOCITY_LIMIT),
      fd_fault_count(0)
{
}

faultDetection::~faultDetection()
{
}

bool faultDetection::update(unsigned long now)
{
    struct mtp_garcia_telemetry *mgt;
    unsigned long faults = 0;
    bool retval = true;
    float rv, lv;
    acpValue av;
    
    mgt = this->fd_dashboard->getTelemetry();
    if (fabsf(mgt->left_instant_odometer) > this->fd_distance_limit) {
	fprintf(stderr,
		"fault: left odometer has passed the distance limit"
		"- %.3f > %.3f\n",
		mgt->left_instant_odometer,
		this->fd_distance_limit);
	faults |= FDF_DISTANCE;
    }
    if (fabsf(mgt->right_instant_odometer) > this->fd_distance_limit) {
	fprintf(stderr,
		"fault: right odometer has passed the distance limit"
		"- %.3f\n",
		mgt->right_instant_odometer,
		this->fd_distance_limit);
	faults |= FDF_DISTANCE;
    }

    if (fabsf(mgt->left_velocity) > this->fd_velocity_limit) {
	fprintf(stderr,
		"fault: left velocity has passed the limit - %.3f\n",
		mgt->left_velocity);
	faults |= FDF_VELOCITY;
    }
    if (fabsf(mgt->right_velocity) > this->fd_velocity_limit) {
	fprintf(stderr,
		"fault: right velocity has passed the limit - %.3f\n",
		mgt->right_velocity);
	faults |= FDF_VELOCITY;
    }

    /* XXX Remove the following when continuous motion is added. */
#if 0
    rv = fabsf(mgt->right_velocity);
    lv = fabsf(mgt->left_velocity);
    if ((rv < 0.020) && (lv < 0.020) && (fabsf(lv - rv) > 0.015)) {
	fprintf(stderr,
		"fault: velocity mismatch - %.3f %.3f\n",
		mgt->left_velocity,
		mgt->right_velocity);
	faults |= FDF_VELOCITY_MISMATCH;
    }
#endif


    if (fabsf(mgt->front_ranger_threshold - FRONT_RANGER_THRESHOLD) > 0.01) {
	fprintf(stderr,
		"fault: front-ranger-threshold is invalid %f\n",
		mgt->front_ranger_threshold);
	
	av.set(FRONT_RANGER_THRESHOLD);
	this->fd_garcia.setNamedValue("front-ranger-threshold", &av);
	av.set(1);
	this->fd_garcia.setNamedValue("front-ranger-enable", &av);
    }
    
    if (fabsf(mgt->rear_ranger_threshold - REAR_RANGER_THRESHOLD) > 0.01) {
	fprintf(stderr,
		"fault: rear-ranger-threshold is invalid %f\n",
		mgt->rear_ranger_threshold);
	
	av.set(REAR_RANGER_THRESHOLD);
	this->fd_garcia.setNamedValue("rear-ranger-threshold", &av);
	av.set(1);
	this->fd_garcia.setNamedValue("rear-ranger-enable", &av);
    }
    
    if (faults != 0) {
	this->fd_callback->faultDetected(faults);

	this->fd_fault_count += 1;
	if (this->fd_fault_count >= MAX_FAULTS)
	    retval = false;
    }
    
    return retval;
}
