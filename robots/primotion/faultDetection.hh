/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _faultDetection_hh
#define _faultDetection_hh

/**
 * @file faultDetection.hh
 *
 * Header file for classes used to detect and act on faults in the robot.
 */

#include <math.h>
#include "garciaUtil.hh"

class dashboard;
class wheelManager;

enum {
    FDB_DISTANCE,
    FDB_VELOCITY,
    FDB_VELOCITY_MISMATCH,
};

enum {
    FDF_DISTANCE = (1L << FDB_DISTANCE),
    FDF_VELOCITY = (1L << FDB_VELOCITY),
    FDF_VELOCITY_MISMATCH = (1L << FDB_VELOCITY_MISMATCH),
};

/**
 * Interface for fault handlers.
 */
class faultCallback
{
    
public:

    /**
     * Destructor.
     */
    virtual ~faultCallback();

    /**
     * Callback triggered when a fault is detected.
     *
     * @param faults Bitmask of FDF_ flags that specify the faults detected.
     */
    virtual void faultDetected(unsigned long faults) = 0;
};

/**
 * Fault detection class that periodically checks the telemetry for anything
 * out of the ordinary.
 */
class faultDetection : public pollCallback
{
    
public:

    /**
     * Maximum number of faults allowed before stopping the program.  Usually a
     * few faults will be detected during normal operation.  So we only stop
     * the world when things are really going wrong.
     */
    static const float MAX_FAULTS = 8;

    /**
     * Desired threshold for the front ranger.
     */
    static const float FRONT_RANGER_THRESHOLD = 0.360f;
    
    /**
     * Desired threshold for the rear ranger.
     */
    static const float REAR_RANGER_THRESHOLD = 0.217f;

    /**
     * Distance limit for when the robot should be idle.  It isn't zero because
     * there is still some play in the wheels while it's stopped.
     *
     * @see IDLE_VELOCITY_LIMIT
     */
    static const float IDLE_DISTANCE_LIMIT = 0.013;

    /**
     * Velocity limit for when the robot should be idle.
     *
     * @see IDLE_DISTANCE_LIMIT
     */
    static const float IDLE_VELOCITY_LIMIT = 0.013;

    /**
     * Construct a fault detector with the given arguments.
     *
     * @param garcia The garcia to monitor.
     * @param db The dashboard to pull telemetry from.
     */
    faultDetection(acpGarcia &garcia, dashboard *db);

    /**
     * Destructor.
     */
    virtual ~faultDetection();

    /**
     * @param fc The callback to trigger when there is a fault.
     */
    void setCallback(faultCallback *fc) { this->fd_callback = fc; };

    /**
     * Set the distance limit, if the odometry value from the telemetry passes
     * this value, a fault will be triggered.
     *
     * @param limit The distance limit in meters.
     */
    void setDistanceLimit(float limit)
    { this->fd_distance_limit = fabsf(limit); };
    
    /**
     * @return The distance limit in meters.
     */
    float getDistanceLimit() { return this->fd_distance_limit; };
    
    /**
     * Set the velocity limit, if the odometry value from the telemetry passes
     * this value, a fault will be triggered.
     *
     * @param limit The velocity limit in meters.
     */
    void setVelocityLimit(float limit)
    { this->fd_velocity_limit = fabsf(limit); };
    
    /**
     * @return The velocity limit in meters.
     */
    float getVelocityLimit() { return this->fd_velocity_limit; };

    /**
     * Check the telemetry for any indications of a fault.
     *
     * @param now The current time in milliseconds.
     */
    virtual bool update(unsigned long now);

private:

    /**
     * The garcia being monitored.
     */
    acpGarcia &fd_garcia;

    /**
     * The dashboard to pull telemetry from.
     */
    dashboard *fd_dashboard;

    /**
     * The callback to trigger when there is a fault.
     */
    faultCallback *fd_callback;

    /**
     * The distance limit.
     */
    float fd_distance_limit;

    /**
     * The velocity limit.
     */
    float fd_velocity_limit;

    /**
     * The number of faults detected so far.
     */
    unsigned int fd_fault_count;
    
};

#endif
