// DelaySensor.h

#ifndef DELAY_SENSOR_H_STUB_2
#define DELAY_SENSOR_H_STUB_2

#include "Sensor.h"

class PacketSensor;

class DelaySensor : public Sensor
{
public:
  DelaySensor(PacketSensor * newPacketHistory);
  int getLastDelay(void) const;
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  int lastDelay;
  PacketSensor * packetHistory;
};

#endif
