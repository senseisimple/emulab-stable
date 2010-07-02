/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Decayer.h

// This is for an old value which will decay over time. We still want
// to access the old value, but for comparison purposes, we want to
// used the decayed value. For instance, if there is a maximum which
// slowly decreases over time to allow for fundamentally changed
// conditions.

// The decay is proportional to the currently decayed value. If the
// original value is 100 and the decay rate is 0.01, then the decayed
// value would start at 100 then after a single decay(), it would drop
// to 99, then 98.01, etc.

#ifndef DECAYER_H_PELAB_2
#define DECAYER_H_PELAB_2

class Decayer
{
public:
    // A positive rate of decay reduces the decayed value over time
    // (for maximums). A negative rate of decay increases the decayed
    // value over time (for minimums).
    Decayer(int newValue=0, double newRate=0.01);
    // Resets the value to be decayed. The decayRate is constant.
    void reset(int newValue);
    // Get the original value which will be decayed.
    int get(void);
    // Decay a step.
    void decay(void);
    bool operator<(int right) const;
    bool operator<=(int right) const;
    bool operator>(int right) const;
    bool operator>=(int right) const;
    bool operator==(int right) const;
    bool operator!=(int right) const;
    // Default destruction and copy semantics are OK.
private:
    int original;
    double decayed;
    double decayRate;
};

bool operator<(int left, Decayer const & right);
bool operator<=(int left, Decayer const & right);
bool operator>(int left, Decayer const & right);
bool operator>=(int left, Decayer const & right);
bool operator==(int left, Decayer const & right);
bool operator!=(int left, Decayer const & right);

#endif
