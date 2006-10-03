// LeastSquaresThroughput.h

// Algorithm based on the equations at:
//   http://www.shodor.org/succeed/compchem/tools/llsalg.html

#ifndef LEAST_SQUARES_THROUGHPUT_H_STUB_2
#define LEAST_SQUARES_THROUGHPUT_H_STUB_2

#include "Sensor.h"

class TSThroughputSensor;
class DelaySensor;

class LeastSquaresThroughput : public Sensor
{
public:
  LeastSquaresThroughput(TSThroughputSensor const * newThroughput,
                         DelaySensor const * newDelay);
  virtual ~LeastSquaresThroughput();
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  TSThroughputSensor const * throughput;
  DelaySensor const * delay;

  // The number of samples kept at any given time.
  static const int SAMPLE_COUNT = 50;
  // Circular buffers of the last SAMPLE_COUNT samples.
  //   The number of bytes in each sample. Used for average throughput
  //   calculation.
  int byteSamples[SAMPLE_COUNT];
  //   The delta time of each sample. This is the difference between
  //   the time of the ack at that sample and the time of the ack at
  //   the previous sample (in milliseconds).
  uint32_t timeSamples[SAMPLE_COUNT];
//  int throughputSamples[SAMPLE_COUNT];
  int delaySamples[SAMPLE_COUNT];
  // The index of the oldest stored sample.
  int oldest;
  // The total number of samples ever encountered.
  int totalSamples;

  // The last number reported to the monitor in kbps.
  // Only send bandwidth if it is different than this number.
  // Only send throughput if it is > this number.
  int lastReport;
};

#endif
