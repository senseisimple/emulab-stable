// ThroughputSensor.h

// For every ack, this calculates the

#ifndef THROUGHPUT_SENSOR_H_STUB_2
#define THROUGHPUT_SENSOR_H_STUB_2

#include "Sensor.h"

class PacketSensor;
class StateSensor;

class ThroughputSensor : public Sensor
{
public:
  ThroughputSensor(PacketSensor * newPacketHistory, StateSensor * newState);
  int getThroughputInKbps(void) const;
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  int throughputInKbps;
  int maxThroughput;
  Time lastAckTime;
  PacketSensor * packetHistory;
  StateSensor * state;
};

#endif
