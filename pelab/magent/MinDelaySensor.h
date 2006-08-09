// MinDelaySensor.h

// This sensor keeps track of the lowest delay seen recently. The
// lowest value it has decays slowly as new measurements come in.

#ifndef MIN_DELAY_SENSOR_H_STUB_2
#define MIN_DELAY_SENSOR_H_STUB_2

#include "Sensor.h"
#include "Decayer.h"

class DelaySensor;

class MinDelaySensor : public Sensor
{
public:
  MinDelaySensor(DelaySensor * newDelay);
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  Decayer minimum;
  DelaySensor * delay;
};

#endif

