// EwmaThroughputSensor.h

#ifndef EWMA_THROUGHPUT_SENSOR_H_STUB_2
#define EWMA_THROUGHPUT_SENSOR_H_STUB_2

#include "Sensor.h"

class ThroughputSensor;

class EwmaThroughputSensor : public Sensor
{
public:
  EwmaThroughputSensor(ThroughputSensor * newThroughputSource);
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  double throughput;
  ThroughputSensor * throughputSource;
};

#endif
