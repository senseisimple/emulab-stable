/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005, 2006 University of Utah and the Flux Group.
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

#include "garciaUtil.hh"
#include "dashboard.hh"
#include "wheelManager.hh"

#define MAX_WHEELSPEED 1.0f

const float wheelManager::FRONT_RANGER_THRESHOLDS[] = {
    0.27f,
    0.36f
};

const float wheelManager::REAR_RANGER_THRESHOLDS[] = {
    0.189f,
    0.217f,
};

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
    this->sc_wheel_manager.motionStarted(this->sc_behavior);

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

    this->ec_wheel_manager.motionFinished(this->ec_behavior,
					  status,
					  this->ec_callback);

    this->ec_callback = NULL;

//     if (debug) {
//         printf("COMPLETION CALLBACK\n");
//     }

    return aErrNone;
}

wheelManager::wheelManager(acpGarcia &garcia)
    : wm_garcia(garcia),
      wm_last_status(aGARCIA_ERRFLAG_NORMAL),
      wm_speed(DEFAULT_SPEED),
      wm_dashboard(NULL),
      wm_moving_notice(LED_PRI_MOVE, LED_PATTERN_MOVING),
      wm_error_notice(LED_PRI_ERROR, LED_PATTERN_ERROR)
{
}

wheelManager::~wheelManager()
{
}

void wheelManager::setSpeed(float speed)
{
    acpValue av;

    if ((speed < MINIMUM_SPEED) || (speed > MAXIMUM_SPEED))
	speed = DEFAULT_SPEED;

    this->wm_speed = speed;
    av.set(speed);
    this->wm_garcia.setNamedValue("speed", &av);
}

acpObject *wheelManager::createPivot(float angle, wmCallback *callback)
{
    acpObject *retval = NULL;

    assert(this->invariant());

    /* First, reduce to a single rotation, */
    if (angle > (2 * M_PI)) {
	angle = fmodf(angle, 2 * M_PI);
    }

    angle = floorf(angle * 1000.0) / 1000.0;

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

    distance = floorf(distance * 1000.0) / 1000.0;

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
    struct mtp_garcia_telemetry *mgt;
    float diff, angle, distance;
    acpObject *move, *pivot;
    threshold_level_t tl;
    acpValue av;

    angle = atan2f(y, x);
    distance = hypot(x, y);

    /*
     * Check if we can make the move by backing up instead of turning all the
     * way around and moving forward.
     */
    if ((distance <= 0.60f) && fabsf(angle) > M_PI_2) {
	if (angle >= 0.0)
	    angle -= M_PI;
	else
	    angle += M_PI;
	distance = -distance;
    }

    /* For small distances, drop the sensor thresholds so we can maneuver. */
    if (distance <= 0.60f)
	tl = THRESH_LOW;
    else
	tl = THRESH_HIGH;
    av.set(FRONT_RANGER_THRESHOLDS[tl]);
    this->wm_garcia.setNamedValue("front-ranger-threshold", &av);
    av.set(REAR_RANGER_THRESHOLDS[tl]);
    this->wm_garcia.setNamedValue("rear-ranger-threshold", &av);

    mgt = this->wm_dashboard->getTelemetry();
    diff = fabsf(mgt->rear_ranger_left - mgt->rear_ranger_right);
    /*
     * If the rear rangers detect an obstacle that is not perpendicular to the
     * robot, we might need to pivot in the opposite direction so the tail
     * doesn't hit the obstacle.
     */
    if (diff > 0.08f) {
	/*
	 * Make sure the direction we want to pivot is clear, otherwise, pivot
	 * the opposite direction.
	 */
	if ((mgt->rear_ranger_right == 0.0f) ||
	    (mgt->rear_ranger_left < mgt->rear_ranger_right)) {
	    if (angle < 0.0f) {
		angle += M_PI;
		distance = -distance;
	    }
	}
	else {
	    if (angle > 0.0f) {
		angle -= M_PI;
		distance = -distance;
	    }
	}
    }

    if ((move = this->createMove(distance, callback)) == NULL) {
	/* Skipping everything. */
	if (callback != NULL) {
	    callback->call(aGARCIA_ERRFLAG_WONTEXECUTE, 0);

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
	this->wm_moving = true;
    }
}

void wheelManager::setOrientation(float orientation, wmCallback *callback)
{
    acpObject *pivot;

    if ((pivot = this->createPivot(orientation, callback)) == NULL) {
	if (callback != NULL) {
	    callback->call(aGARCIA_ERRFLAG_WONTEXECUTE, 0);

	    delete callback;
	    callback = NULL;
	}
    }
    else {
	this->wm_garcia.queueBehavior(pivot);
	pivot = NULL;
	this->wm_moving = true;
    }
}


void wheelManager::startNULL(float accel, wmCallback *callback) {
    /* Start the NULL primitive only if the garcia is idle */

    assert(this->invariant());

    if (this->wm_garcia.getNamedValue("idle")->getBoolVal()) {

	acpValue av;
	acpObject *nullb = NULL;
	
	av.set(FRONT_RANGER_THRESHOLDS[THRESH_HIGH]);
	this->wm_garcia.setNamedValue("front-ranger-threshold", &av);
	av.set(REAR_RANGER_THRESHOLDS[THRESH_HIGH]);
	this->wm_garcia.setNamedValue("rear-ranger-threshold", &av);
	
	nullb = this->wm_garcia.createNamedBehavior("null", NULL);

	av.set((float)(accel)); /* Do I really need to force it to a float? */
	nullb->setNamedValue("acceleration", &av);

	av.set(new startCallback(*this, nullb));
	nullb->setNamedValue("execute-callback", &av);

	av.set(new endCallback(*this, nullb, callback));
	nullb->setNamedValue("completion-callback", &av);

	this->wm_garcia.queueBehavior(nullb);    
	nullb = NULL;
	
	this->wm_moving = true;
    }


}

void wheelManager::setWheels(float vl, float vr) {

    acpValue av_L;
    acpValue av_R;

    float maxspeed = 0.0f;

    if (!this->wm_moving)
	return;
    
    if (fabsf(vl) >= fabsf(vr)) {
	maxspeed = fabsf(vl) * 100.0;
    }
    else {
	maxspeed = fabsf(vr) * 100.0;
    }

    if (fabsf(vl) < 0.01) {
	vl = 0.0f;
    }

    if (fabsf(vr) < 0.01) {
	vr = 0.0f;
    }

    if (vl > MAX_WHEELSPEED) {
	vl = MAX_WHEELSPEED;
    }
    if (vl < -MAX_WHEELSPEED) {
	vl = -MAX_WHEELSPEED;
    }

    if (vr > MAX_WHEELSPEED) {
	vr = MAX_WHEELSPEED;
    }
    if (vr < -MAX_WHEELSPEED) {
	vr = -MAX_WHEELSPEED;
    }

    av_L.set((float)(vl));
    av_R.set((float)(vr));

    this->wm_garcia.setNamedValue("damped-speed-left", &av_L);
    this->wm_garcia.setNamedValue("damped-speed-right", &av_R);
    
    /* handle fault detection */
    this->wm_dashboard->setDistanceLimit(10000.0f);
    this->wm_dashboard->setVelocityLimit(maxspeed);
}



bool wheelManager::stop(void)
{
    acpValue av;

    av.set((float)(0.0));
    this->wm_garcia.setNamedValue("damped-speed-left", &av);
    this->wm_garcia.setNamedValue("damped-speed-right", &av);


    this->wm_garcia.flushQueuedBehaviors();

    if (debug) {
	fprintf(stderr, "debug: STOP WHEELS\n");
    }

    return this->wm_moving;
}

void wheelManager::motionStarted(acpObject *behavior)
{
    float distance;
    acpValue *av;

    if (debug) {
	fprintf(stderr, "debug: motion started\n");
    }

    if ((av = behavior->getNamedValue("distance")) != NULL)
	distance = av->getFloatVal();
    else
	distance = 1.0f; // XXX pivot, just assume a meter for now.
    this->wm_dashboard->setDistanceLimit(distance * 1.1);
    this->wm_dashboard->setVelocityLimit(this->wm_speed * 1.1);

    if ((this->wm_last_status != aGARCIA_ERRFLAG_NORMAL) &&
	(this->wm_last_status != aGARCIA_ERRFLAG_ABORT)) {
	if (debug) {
	    fprintf(stderr, "debug: clear error LED\n");
	}

	this->wm_dashboard->remUserLEDClient(&this->wm_error_notice);
	this->wm_last_status = 0;
    }

    this->wm_dashboard->addUserLEDClient(&this->wm_moving_notice);
    this->wm_dashboard->startMove();
}

void wheelManager::motionFinished(acpObject *behavior,
				  int status,
				  wmCallback *callback)
{
    float odometer = 0.0f;

    if (debug) {
	fprintf(stderr, "debug: motion finished with status %d\n", status);
    }

    if (status == aGARCIA_ERRFLAG_STALL) {
	float distance = behavior->getNamedValue("distance")->getFloatVal();

    if (debug) {
        fprintf(stderr, "debug: STALL at %f\n", distance);
    }

	this->wm_dashboard->getTelemetry()->stall_contact =
	    (distance < 0) ? -1 : 1;
    }
    else {
	this->wm_dashboard->getTelemetry()->stall_contact = 0;
    }

    if (status != aGARCIA_ERRFLAG_WONTEXECUTE) {
	float left_odometer, right_odometer;

	if ((status != aGARCIA_ERRFLAG_NORMAL) &&
	    (status != aGARCIA_ERRFLAG_ABORT)) {
	    if (debug) {
		fprintf(stderr, "debug: set error LED\n");
	    }

	    this->wm_dashboard->addUserLEDClient(&this->wm_error_notice);
	}

	this->wm_dashboard->endMove(left_odometer, right_odometer);
	/*
	 * If both wheels moved the same direction it was a straight move,
	 * otherwise, it was a pivot.
	 */
	if ((left_odometer / fabsf(left_odometer)) ==
	    (right_odometer / fabsf(right_odometer))) {
	    printf(" %f %f -- %f %f\n",
		   left_odometer, right_odometer,
		   left_odometer / left_odometer,
		   right_odometer / right_odometer);
	    odometer = left_odometer;
	}

	this->wm_dashboard->remUserLEDClient(&this->wm_moving_notice);

	this->wm_last_status = status;
    }

    this->wm_dashboard->setVelocityLimit(faultDetection::IDLE_VELOCITY_LIMIT);
    this->wm_dashboard->setDistanceLimit(faultDetection::IDLE_DISTANCE_LIMIT);

    if (callback != NULL) {
	this->wm_moving = false; // XXX This assumes callbacks on the last move

	callback->call(this->wm_last_status, odometer);

	delete callback;
	callback = NULL;
    }
}
