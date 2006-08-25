// MaxDelaySensor.h

#ifndef MAX_DELAY_SENSOR_H_STUB_2
#define MAX_DELAY_SENSOR_H_STUB_2

#include "Sensor.h"
#include "StateSensor.h"
#include "MinDelaySensor.h"
#include "Decayer.h"

class DelaySensor;
class StateSensor;

class MaxDelaySensor : public Sensor
{
public:
  MaxDelaySensor(DelaySensor * newDelay, StateSensor * newState,
                 MinDelaySensor * newminDelay);
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  Decayer maximum;
  DelaySensor * delay;
  StateSensor * state;
  MinDelaySensor * mindelay;
};

#endif
