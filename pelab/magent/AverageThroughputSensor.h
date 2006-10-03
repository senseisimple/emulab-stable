// AverageThroughputSensor.h

// A throughput sensor which averages the measure over a minimum
// period of time.

#ifndef AVERAGE_THROUGHPUT_SENSOR_H
#define AVERAGE_THROUGHPUT_SENSOR_H

#include "Sensor.h"

class TSThroughputSensor;

class AverageThroughputSensor : public Sensor
{
public:
  AverageThroughputSensor(TSThroughputSensor const * newThroughput);
  virtual ~AverageThroughputSensor();
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  struct Ack
  {
    Ack() : size(0), period(0) {}
    // The size begin acked (in bytes)
    int size;
    // The amount of time to the previous ack.
    uint32_t period;
  };
private:
  TSThroughputSensor const * throughput;

  // The period of time to average over (in milliseconds)
  static const uint32_t AVERAGE_PERIOD = 500;
  enum { MAX_SAMPLE_COUNT = 100 };

  Ack samples[MAX_SAMPLE_COUNT];
  int latest;
  int sampleCount;
};

#endif
