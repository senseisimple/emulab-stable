// EwmaThroughputSensor.h

#ifndef EWMA_THROUGHPUT_SENSOR_H_STUB_2
#define EWMA_THROUGHPUT_SENSOR_H_STUB_2

#include "Sensor.h"

class ThroughputSensor;
class StateSensor;

class EwmaThroughputSensor : public Sensor
{
public:
  EwmaThroughputSensor(ThroughputSensor const * newThroughputSource,
                       StateSensor const * newState);
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  // The maximum throughput or latest bandwidth number
  int maxThroughput;
  // And EWMA of the last several solid bandwidth measurements
  double bandwidth;
  ThroughputSensor const * throughputSource;
  StateSensor const * state;
};

#endif
