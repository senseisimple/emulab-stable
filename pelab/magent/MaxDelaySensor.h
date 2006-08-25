// MaxDelaySensor.h

#ifndef MAX_DELAY_SENSOR_H_STUB_2
#define MAX_DELAY_SENSOR_H_STUB_2

#include "Sensor.h"
#include "Decayer.h"

class DelaySensor;
class StateSensor;

class MaxDelaySensor : public Sensor
{
public:
  MaxDelaySensor(DelaySensor * newDelay, StateSensor * newState);
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  Decayer maximum;
  DelaySensor * delay;
  StateSensor * state;
};

#endif
