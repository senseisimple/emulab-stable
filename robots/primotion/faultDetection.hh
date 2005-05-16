/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _faultDetection_hh
#define _faultDetection_hh

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

class faultCallback
{
    
public:

    /**
     * Destructor.
     */
    virtual ~faultCallback();
    
    virtual void faultDetected(unsigned long faults) = 0;
};

class faultDetection : public pollCallback
{
    
public:

    static const float MAX_FAULTS = 8;
    
    static const float FRONT_RANGER_THRESHOLD = 0.360f;
    
    static const float REAR_RANGER_THRESHOLD = 0.217f;

    static const float IDLE_DISTANCE_LIMIT = 0.013;

    static const float IDLE_VELOCITY_LIMIT = 0.013;
    
    faultDetection(acpGarcia &garcia, dashboard *db);
    virtual ~faultDetection();
    
    void setCallback(faultCallback *fc) { this->fd_callback = fc; };

    void setDistanceLimit(float limit)
    { this->fd_distance_limit = fabsf(limit); };
    float getDistanceLimit() { return this->fd_distance_limit; };
    void setVelocityLimit(float limit)
    { this->fd_velocity_limit = fabsf(limit); };
    float getVelocityLimit() { return this->fd_velocity_limit; };
    
    virtual bool update(unsigned long now);

private:

    acpGarcia &fd_garcia;
    dashboard *fd_dashboard;
    faultCallback *fd_callback;

    float fd_distance_limit;
    float fd_velocity_limit;

    unsigned int fd_fault_count;
    
};

#endif
