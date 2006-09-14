// LeastSquaresThroughput.h

// Algorithm based on the equations at:
//   http://www.shodor.org/succeed/compchem/tools/llsalg.html

#ifndef LEAST_SQUARES_THROUGHPUT_H_STUB_2
#define LEAST_SQUARES_THROUGHPUT_H_STUB_2

#include "Sensor.h"

class ThroughputSensor;
class DelaySensor;

class LeastSquaresThroughput : public Sensor
{
public:
  LeastSquaresThroughput(ThroughputSensor const * newThroughput,
                         DelaySensor const * newDelay);
  virtual ~LeastSquaresThroughput();
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  ThroughputSensor const * throughput;
  DelaySensor const * delay;

  // The number of samples kept at any given time.
  static const int SAMPLE_COUNT = 5;
  // Circular buffers of the last SAMPLE_COUNT samples.
  int throughputSamples[SAMPLE_COUNT];
  int delaySamples[SAMPLE_COUNT];
  // The index of the oldest stored sample.
  int oldest;
  // The total number of samples ever encountered.
  int totalSamples;
};

#endif
