/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file pilotButtonCallback.cc
 *
 * Implementation file for the pilotButtonCallback class.
 */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "pilotButtonCallback.hh"

pilotButtonCallback::pilotButtonCallback(dashboard &db, wheelManager &wm)
    : pbc_dashboard(db),
      pbc_wheel_manager(wm),
      pbc_command_notice(LED_PRI_USER, LED_PATTERN_COMMAND_MODE)
{
    this->pbc_dashboard.setUserButtonCallback(this);
}

pilotButtonCallback::~pilotButtonCallback()
{
}

bool pilotButtonCallback::shortCommandClick(acpGarcia &garcia,
					    unsigned long now)
{
    ledClient *lc;

    lc = new ledClient(LED_PRI_USER, LED_PATTERN_ACK, now + 3 * 1000);
    this->pbc_dashboard.addUserLEDClient(lc);

    this->pbc_dashboard.dump(stdout);

    {
	acpValue pivotAngle;
	acpObject *ao;

	garcia.flushQueuedBehaviors();

	pivotAngle.set(-0.33f);
	ao = garcia.createNamedBehavior("pivot", "pivot1");
	ao->setNamedValue("angle", &pivotAngle);
	garcia.queueBehavior(ao);
	
	pivotAngle.set(0.66f);
	ao = garcia.createNamedBehavior("pivot", "pivot2");
	ao->setNamedValue("angle", &pivotAngle);
	garcia.queueBehavior(ao);
	
	pivotAngle.set((float)(-1.5 * M_PI));
	ao = garcia.createNamedBehavior("pivot", "pivot3");
	ao->setNamedValue("angle", &pivotAngle);
	garcia.queueBehavior(ao);
	
	pivotAngle.set((float)(3.5 * M_PI));
	ao = garcia.createNamedBehavior("pivot", "pivot4");
	ao->setNamedValue("angle", &pivotAngle);
	garcia.queueBehavior(ao);

	pivotAngle.set(-0.33f);
	ao = garcia.createNamedBehavior("pivot", "pivot5");
	ao->setNamedValue("angle", &pivotAngle);
	garcia.queueBehavior(ao);

	this->pbc_wheel_manager.setDestination(-0.25, -0.25);
	
	this->pbc_wheel_manager.setDestination(0.25, -0.25);
    }
    
    return true;
}

bool pilotButtonCallback::commandMode(acpGarcia &garcia,
				      unsigned long now,
				      bool on)
{
    if (on)
	this->pbc_dashboard.addUserLEDClient(&this->pbc_command_notice);
    else
	this->pbc_dashboard.remUserLEDClient(&this->pbc_command_notice);
    
    return true;
}

bool pilotButtonCallback::longCommandClick(acpGarcia &garcia,
					    unsigned long now)
{
    ledClient *lc;

    lc = new ledClient(LED_PRI_USER, LED_PATTERN_ACK, now + 3 * 1000);
    this->pbc_dashboard.addUserLEDClient(lc);

    if (getuid() != 0) {
	fprintf(stderr, "error: cannot reboot, not running as root.\n");
    }
    else {
	system("/sbin/reboot");
    }
    
    return false;
}

bool pilotButtonCallback::shortClick(acpGarcia &garcia, unsigned long now)
{
    ledClient *lc;

    lc = new ledClient(LED_PRI_USER, LED_PATTERN_ACK, now + 3 * 1000);
    this->pbc_dashboard.addUserLEDClient(lc);
    
    if (getuid() != 0) {
	fprintf(stderr, "error: cannot shutdown, not running as root.\n");
    }
    else {
	system("/sbin/shutdown -h now");
    }
    
    return false;
}
