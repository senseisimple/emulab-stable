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
  // The default max period to use.
  static const int DEFAULT_MAX_PERIOD = 500;
public:
  LeastSquaresThroughput(TSThroughputSensor const * newThroughput,
                         DelaySensor const * newDelay,
                         int newMaxPeriod = 0);
  virtual ~LeastSquaresThroughput();
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  struct Ack
  {
    Ack() : size(0), period(0), rtt(0) {}
    int size; // in bytes
    uint32_t period; // in milliseconds
    int rtt; // in milliseconds
  };
private:
  TSThroughputSensor const * throughput;
  DelaySensor const * delay;

  // The number of samples kept at any given time.
  static const int MAX_SAMPLE_COUNT = 100;
  // Circular buffer of the last MAX_SAMPLE_COUNT samples.
  Ack samples[MAX_SAMPLE_COUNT];
  // The maximum number of samples used for least squares analysis.
  static const int MAX_LEAST_SQUARES_SAMPLES = 5;

  // The index of the latest stored sample.
  int latest;
  // The total number of samples ever encountered.
  int totalSamples;
  // The maximum amount of time to look backwards in milliseconds. If
  // this number is <= 0 then all available samples are used.
  int maxPeriod;

  // The last number reported to the monitor in kbps.
  // Only send bandwidth if it is different than this number.
  // Only send throughput if it is > this number.
  int lastReport;
};

#endif
