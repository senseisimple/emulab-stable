/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file wheelManager.cc
 *
 * Implementation file for the wheelManager class.
 */

#include "config.h"

#include <math.h>
#include <errno.h>
#include <stdio.h>

#include "dashboard.hh"
#include "wheelManager.hh"

extern int debug;

/**
 * An "execute" callback for a garcia behavior.
 */
class startCallback : public acpCallback
{

public:

    /**
     * Construct the callback with the given values.
     *
     * @param wm The wheelManager to notify when some motion has started.
     * @param behavior The behavior this callback is attached to.
     */
    startCallback(wheelManager &wm, acpObject *behavior);

    /**
     * Destructor.
     */
    virtual ~startCallback();

    /**
     * Method called when the behavior starts.
     */
    aErr call();
    
private:

    /**
     * The wheelManager to notify when some motion has finished.
     */
    wheelManager &sc_wheel_manager;

    /**
     * The behavior this callback is attached to.
     */
    acpObject *sc_behavior;
    
};

/**
 * A "completion" callback for a garcia behavior.
 */
class endCallback : public acpCallback
{

public:

    /**
     * Construct the callback with the given values.
     *
     * @param wm The wheelManager to notify when some motion has finished.
     * @param behavior The behavior this callback is attached to.
     * @param callback The wheelManager callback that should be triggered.
     */
    endCallback(wheelManager &wm, acpObject *behavior, wmCallback *callback);
    
    /**
     * Destructor.
     */
    virtual ~endCallback();

    /**
     * Method called when the behavior finishes.
     */
    aErr call();
    
private:

    /**
     * The wheelManager to notify when some motion has finished.
     */
    wheelManager &ec_wheel_manager;
    
    /**
     * The behavior this callback is attached to.
     */
    acpObject *ec_behavior;
    
    /**
     * The wheelManager callback that should be triggered.
     */
    wmCallback *ec_callback;
    
};

wmCallback::~wmCallback()
{
}

startCallback::startCallback(wheelManager &wm, acpObject *behavior)
    : sc_wheel_manager(wm), sc_behavior(behavior)
{
    assert(behavior != NULL);
}

startCallback::~startCallback()
{
}

aErr startCallback::call()
{
    this->sc_wheel_manager.motionStarted();
    
    return aErrNone;
}

endCallback::endCallback(wheelManager &wm,
			 acpObject *behavior,
			 wmCallback *callback)
    : ec_wheel_manager(wm),
      ec_behavior(behavior),
      ec_callback(callback)
{
    assert(behavior != NULL);
}

endCallback::~endCallback()
{
}

aErr endCallback::call()
{
    int status;
    
    status = this->ec_behavior->
	getNamedValue("completion-status")->getIntVal();

    this->ec_wheel_manager.motionFinished(status, this->ec_callback);

    this->ec_callback = NULL;
    
    return aErrNone;
}

wheelManager::wheelManager(acpGarcia &garcia)
    : wm_garcia(garcia),
      wm_last_status(aGARCIA_ERRFLAG_NORMAL),
      wm_dashboard(NULL),
      wm_moving_notice(LED_PRI_MOVE, LED_PATTERN_MOVING),
      wm_error_notice(LED_PRI_ERROR, LED_PATTERN_ERROR)
{
}

wheelManager::~wheelManager()
{
}

acpObject *wheelManager::createPivot(float angle, wmCallback *callback)
{
    acpObject *retval = NULL;

    assert(this->invariant());

    /* First, reduce to a single rotation, */
    if (angle > (2 * M_PI)) {
	angle = fmodf(angle, 2 * M_PI);
    }

    /* ... then reduce to the smallest movement. */
    if (angle > M_PI) {
	angle = -((2 * M_PI) - angle);
    } else if (angle < -M_PI) {
	angle = angle + (2 * M_PI);
    }

    if (fabsf(angle) < SMALLEST_PIVOT_ANGLE) {
	errno = EINVAL;
    }
    else {
	acpValue av;

	retval = this->wm_garcia.createNamedBehavior("pivot", NULL);

	av.set(angle);
	retval->setNamedValue("angle", &av);

	av.set(new startCallback(*this, retval));
	retval->setNamedValue("execute-callback", &av);
	
	av.set(new endCallback(*this, retval, callback));
	retval->setNamedValue("completion-callback", &av);
    }
    
    return retval;
}

acpObject *wheelManager::createMove(float distance, wmCallback *callback)
{
    acpObject *retval = NULL;

    assert(this->invariant());

    if (fabsf(distance) < SMALLEST_MOVE_DISTANCE) {
	errno = EINVAL;
    }
    else {
	acpValue av;

	retval = this->wm_garcia.createNamedBehavior("move", NULL);

	av.set(distance);
	retval->setNamedValue("distance", &av);

	av.set(new startCallback(*this, retval));
	retval->setNamedValue("execute-callback", &av);
	
	av.set(new endCallback(*this, retval, callback));
	retval->setNamedValue("completion-callback", &av);
    }
    
    return retval;
}

void wheelManager::setDestination(float x, float y, wmCallback *callback)
{
    acpObject *move, *pivot;
    float angle, distance;
    
    angle = atan2f(y, x);
    distance = hypot(x, y);

    /*
     * Check if we can make the move by backing up instead of turning all the
     * way around and moving forward.
     */
    if (fabsf(angle) > M_PI_2) {
	if (angle >= 0.0)
	    angle -= M_PI;
	else
	    angle += M_PI;
	distance = -distance;
    }
    
    if ((move = this->createMove(distance, callback)) == NULL) {
	/* Skipping everything. */
	if (callback != NULL) {
	    callback->call(aGARCIA_ERRFLAG_WONTEXECUTE);
	    
	    delete callback;
	    callback = NULL;
	}
    }
    else {
	if ((pivot = this->createPivot(angle)) == NULL) {
	    /* Skipping pivot. */
	}
	else {
	    this->wm_garcia.queueBehavior(pivot);
	    pivot = NULL;
	}
	
	this->wm_garcia.queueBehavior(move);
	move = NULL;
    }
}

void wheelManager::setOrientation(float orientation, wmCallback *callback)
{
    acpObject *pivot;

    if ((pivot = this->createPivot(orientation, callback)) == NULL) {
	if (callback != NULL) {
	    callback->call(aGARCIA_ERRFLAG_WONTEXECUTE);
	    
	    delete callback;
	    callback = NULL;
	}
    }
    else {
	this->wm_garcia.queueBehavior(pivot);
	pivot = NULL;
    }
}

void wheelManager::stop(void)
{
    this->wm_garcia.flushQueuedBehaviors();
}

void wheelManager::motionStarted(void)
{
    if (debug) {
	fprintf(stderr, "debug: motion started\n");
    }
    
    if ((this->wm_last_status != aGARCIA_ERRFLAG_NORMAL) &&
	(this->wm_last_status != aGARCIA_ERRFLAG_ABORT)) {
	if (debug) {
	    fprintf(stderr, "debug: clear error LED\n");
	}
	
	this->wm_dashboard->remUserLEDClient(&this->wm_error_notice);
	this->wm_last_status = 0;
    }

    this->wm_dashboard->addUserLEDClient(&this->wm_moving_notice);
    this->wm_dashboard->startMove(this->wm_garcia);
}

void wheelManager::motionFinished(int status, wmCallback *callback)
{
    if (debug) {
	fprintf(stderr, "debug: motion finished -- %d\n", status);
    }
    
    if (status != aGARCIA_ERRFLAG_WONTEXECUTE) {
	if ((status != aGARCIA_ERRFLAG_NORMAL) &&
	    (status != aGARCIA_ERRFLAG_ABORT)) {
	    if (debug) {
		fprintf(stderr, "debug: set error LED\n");
	    }
	    
	    this->wm_dashboard->addUserLEDClient(&this->wm_error_notice);
	}
	
	this->wm_dashboard->endMove(this->wm_garcia);
	this->wm_dashboard->remUserLEDClient(&this->wm_moving_notice);
	
	this->wm_last_status = status;
    }
    
    if (callback != NULL) {
	callback->call(this->wm_last_status);
	
	delete callback;
	callback = NULL;
    }
}
