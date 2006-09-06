// MaxDelaySensor.h

#ifndef MAX_DELAY_SENSOR_H_STUB_2
#define MAX_DELAY_SENSOR_H_STUB_2

#include "Sensor.h"
#include "StateSensor.h"
#include "MinDelaySensor.h"
#include "Decayer.h"
#include "PacketSensor.h"

class DelaySensor;
class StateSensor;
class PacketSensor;

class MaxDelaySensor : public Sensor
{
public:
  MaxDelaySensor(DelaySensor * newDelay, StateSensor * newState,
                 MinDelaySensor * newminDelay, PacketSensor * newpacketSensor);
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  Decayer maximum;
  DelaySensor * delay;
  StateSensor * state;
  MinDelaySensor * mindelay;
  PacketSensor * packetsensor;

  // The last delay we reported to the monitor
  int lastreported;
};

#endif
