/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// StateSensor.h

#ifndef STATE_SENSOR_H_STUB_2
#define STATE_SENSOR_H_STUB_2

#include "Sensor.h"

class StateSensor : public Sensor
{
public:
  enum
  {
    INITIAL,
    ESTABLISHED
  };
public:
  StateSensor();
  virtual ~StateSensor();
  int getState(void) const;
  bool isSaturated(void) const;
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  void calculateSaturated(PacketInfo * packet);
private:
  int state;
  bool saturated;
};

#endif
