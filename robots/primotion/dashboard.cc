/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file dashboard.cc
 *
 * Implementation file for the dashboard class.
 */

#include "config.h"

#include <time.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "aGarciaDefs.tea"

#include "dashboard.hh"

#if !defined(timeradd)
#define timeradd(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;	\
		if ((vvp)->tv_usec >= 1000000) {			\
			(vvp)->tv_sec++;				\
			(vvp)->tv_usec -= 1000000;			\
		}							\
	} while (0)
#endif

#if !defined(timersub)
#define timersub(tvp, uvp, vvp)						\
	do {								\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_usec += 1000000;			\
		}							\
	} while (0)
#endif

dashboard::dashboard(acpGarcia &garcia, FILE *battery_log)
    : db_garcia(garcia),
      db_battery_log(battery_log),
      db_user_led(garcia, "user-led"),
      db_user_button(garcia, "user-button"),
      db_fault(garcia, this),
      db_telemetry_updated(false),
      db_last_left_odometer(0.0),
      db_last_right_odometer(0.0),
      db_battery_warning(NULL),
      db_next_short_tick(0),
      db_next_long_tick(0)
{
    memset(&this->db_telemetry, 0, sizeof(struct mtp_garcia_telemetry));
    memset(&this->db_move_start_time, 0, sizeof(struct timeval));

    this->db_poll_callbacks[DBC_USER_LED] = &this->db_user_led;
    this->db_poll_callbacks[DBC_USER_BUTTON] = &this->db_user_button;
    this->db_poll_callbacks[DBC_FAULT] = &this->db_fault;
}

dashboard::~dashboard()
{
}

void dashboard::startMove()
{
    assert(this->db_move_start_time.tv_sec == 0);
    assert(this->db_move_start_time.tv_usec == 0);
    
    this->db_telemetry.move_count += 1;

    gettimeofday(&this->db_move_start_time, NULL);
}

void dashboard::endMove(float &left_odometer, float &right_odometer)
{
    struct timeval tv, diff, total, grande_total;
    acpValue zvalue(0.0f);

    assert(this->db_move_start_time.tv_sec != 0);
    assert(this->db_move_start_time.tv_usec != 0);

    /* Compute the move time, */
    gettimeofday(&tv, NULL);
    timersub(&tv, &this->db_move_start_time, &diff);
    total.tv_sec = this->db_telemetry.move_time_sec;
    total.tv_usec = this->db_telemetry.move_time_usec;
    timeradd(&total, &diff, &grande_total);
    this->db_telemetry.move_time_sec = grande_total.tv_sec;
    this->db_telemetry.move_time_usec = grande_total.tv_usec;

    /* ... update our global odometers, and */
    left_odometer = this->db_garcia.getNamedValue("distance-left")->
	getFloatVal();
    right_odometer = this->db_garcia.getNamedValue("distance-right")->
	getFloatVal();
    this->db_telemetry.left_odometer += fabs(left_odometer);
    this->db_telemetry.right_odometer += fabs(right_odometer);

    /* ... clear the garcia's per-move odometers. */
    this->db_garcia.setNamedValue("distance-left", &zvalue);
    this->db_garcia.setNamedValue("distance-right", &zvalue);

    /* Clear the start time so we can tell that the move stopped. */
    memset(&this->db_move_start_time, 0, sizeof(struct timeval));
}

void dashboard::dump(FILE *file)
{
    time_t now;

    assert(file != NULL);
    
    time(&now);
    fprintf(file,
	    "Current time: %s"
	    "  Left odometer:\t%f meters\n"
	    "  Right odometer:\t%f meters\n"
	    "  Left velocity:\t%f m/s\n"
	    "  Right velocity:\t%f m/s\n"
	    "  Move count:\t\t%u\n"
	    "  Move time:\t\t%d.%d s\n"
	    "  Battery level:\t%f%%\n"
	    "  Battery voltage:\t%f\n"
	    "  Battery misses:\t%d\n",
	    ctime(&now),
	    this->db_telemetry.left_odometer,
	    this->db_telemetry.right_odometer,
	    this->db_telemetry.left_velocity,
	    this->db_telemetry.right_velocity,
	    this->db_telemetry.move_count,
	    this->db_telemetry.move_time_sec,
	    this->db_telemetry.move_time_usec,
	    this->db_telemetry.battery_level,
	    this->db_telemetry.battery_voltage,
	    this->db_telemetry.battery_misses);
    fflush(file);
}

bool dashboard::update(unsigned long now)
{
    bool retval = true;
    float lo, ro;
    int lpc;

    /*
     * Note, these queries need to come first so that we can get an accurate
     * velocity measure.  Otherwise, the time difference from the queries below
     * screw things up.
     */
    this->db_telemetry.left_instant_odometer = lo =
	this->db_garcia.getNamedValue("distance-left")->getFloatVal();
    this->db_telemetry.right_instant_odometer = ro =
	this->db_garcia.getNamedValue("distance-right")->getFloatVal();

    /*
     * The odometers are reset to zero when a new move is executed, so we
     * need to detect that and reset our idea of the last odometer reading.
     */
    if (fabsf(lo) < fabsf(this->db_last_left_odometer))
	this->db_last_left_odometer = 0.0f;
    if (fabsf(ro) < fabsf(this->db_last_right_odometer))
	this->db_last_right_odometer = 0.0f;

    this->db_telemetry.left_velocity = (lo - this->db_last_left_odometer) *
	(1000.0f / (float)(now - this->db_last_tick));
    this->db_telemetry.right_velocity = (ro - this->db_last_right_odometer) *
	(1000.0f / (float)(now - this->db_last_tick));
#if 0
    printf("%f %f %d -- %f %f\n",
	   lo, ro, now - this->db_last_tick,
	   this->db_telemetry.left_velocity,
	   this->db_telemetry.right_velocity);
#endif

    this->db_last_left_odometer = lo;
    this->db_last_right_odometer = ro;
    
    if (now > this->db_next_long_tick) {
	float battVoltage;

	battVoltage = this->db_garcia.getNamedValue("battery-voltage")->
	    getFloatVal();
	if (battVoltage <= 0.0) {
	    /* XXX Bad read of the battery, try again in a little bit. */
	    this->db_telemetry.battery_misses += 1;
	    this->db_next_long_tick = now + 1000;
	}
	else {
	    this->db_telemetry.battery_voltage = battVoltage;
	    this->db_telemetry.battery_level =
		this->db_garcia.getNamedValue("battery-level")->
		getFloatVal() * 100.0;

	    if (this->db_telemetry.battery_level < BATTERY_LOW_THRESHOLD) {
		if (this->db_battery_warning == NULL) {
		    /*
		     * First time the battery level dropped below the
		     * threshold, turn on the LED.
		     */
		    this->db_battery_warning =
			new ledClient(LED_PRI_BATTERY, LED_PATTERN_BATTERY);
		    this->addUserLEDClient(this->db_battery_warning);
		}
	    }
	    else if ((this->db_telemetry.
		      battery_level > BATTERY_HIGH_THRESHOLD) &&
		     (this->db_battery_warning != NULL)) {
		this->remUserLEDClient(this->db_battery_warning);
		
		delete this->db_battery_warning;
		this->db_battery_warning = NULL;
	    }

	    this->db_next_long_tick = now + LONG_UPDATE_INTERVAL;

	    fprintf(this->db_battery_log,
		    "%lu %f %f %d %f %f %u %d,%d\n",
		    now,
		    this->db_telemetry.battery_level,
		    this->db_telemetry.battery_voltage,
		    this->db_telemetry.battery_misses,
		    this->db_telemetry.left_odometer,
		    this->db_telemetry.right_odometer,
		    this->db_telemetry.move_count,
		    this->db_telemetry.move_time_sec,
		    this->db_telemetry.move_time_usec);
	    fflush(this->db_battery_log);
	}

	this->db_telemetry.speed =
	    this->db_garcia.getNamedValue("speed")->getFloatVal();
	
	this->db_telemetry.side_ranger_threshold = this->db_garcia.
	    getNamedValue("side-ranger-threshold")->getFloatVal();
	
	this->db_telemetry.front_ranger_threshold = this->db_garcia.
	    getNamedValue("front-ranger-threshold")->getFloatVal();
	
	this->db_telemetry.rear_ranger_threshold = this->db_garcia.
	    getNamedValue("rear-ranger-threshold")->getFloatVal();
    }

    if (now > this->db_next_short_tick) {
#if 0
	this->db_telemetry.down_ranger_left =
	    this->db_garcia.getNamedValue("down-ranger-left")->getIntVal();
	this->db_telemetry.down_ranger_right =
	    this->db_garcia.getNamedValue("down-ranger-right")->getIntVal();
#endif
	
	this->db_telemetry.front_ranger_left =
	    this->db_garcia.getNamedValue("front-ranger-left")->getFloatVal();
	this->db_telemetry.front_ranger_right =
	    this->db_garcia.getNamedValue("front-ranger-right")->getFloatVal();
	
	this->db_telemetry.rear_ranger_left =
	    this->db_garcia.getNamedValue("rear-ranger-left")->getFloatVal();
	this->db_telemetry.rear_ranger_right =
	    this->db_garcia.getNamedValue("rear-ranger-right")->getFloatVal();
	
	this->db_telemetry.side_ranger_left =
	    this->db_garcia.getNamedValue("side-ranger-left")->getFloatVal();
	this->db_telemetry.side_ranger_right =
	    this->db_garcia.getNamedValue("side-ranger-right")->getFloatVal();

#if 0
	this->db_telemetry.status =
	    this->db_garcia.getNamedValue("status")->getIntVal();
	
	this->db_telemetry.idle =
	    this->db_garcia.getNamedValue("idle")->getIntVal();
#endif
	
	this->db_next_short_tick = now + SHORT_UPDATE_INTERVAL;

	this->db_telemetry_updated = true;
    }

    for (lpc = 0; lpc < DBC_MAX; lpc++) {
	retval = this->db_poll_callbacks[lpc]->update(now) && retval;
    }

    this->db_last_tick = now;
    
    return retval;
}
