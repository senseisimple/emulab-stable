// LeastSquaresThroughput.cc

#include "lib.h"
#include "LeastSquaresThroughput.h"
#include "ThroughputSensor.h"
#include "DelaySensor.h"
#include "CommandOutput.h"

using namespace std;

LeastSquaresThroughput::LeastSquaresThroughput(
  ThroughputSensor const * newThroughput,
  DelaySensor const * newDelay)

  : throughput(newThroughput)
  , delay(newDelay)
  , oldest(0)
  , totalSamples(0)
  , lastReport(0)
{
}

LeastSquaresThroughput::~LeastSquaresThroughput()
{
}

void LeastSquaresThroughput::localSend(PacketInfo * packet)
{
  ackValid = false;
  sendValid = true;
}

void LeastSquaresThroughput::localAck(PacketInfo * packet)
{
  sendValid = false;
  if (throughput->isAckValid() && delay->isAckValid())
  {
    throughputSamples[oldest] = throughput->getThroughputInKbps();
    delaySamples[oldest] = delay->getLastDelay();
    oldest = (oldest + 1) % SAMPLE_COUNT;
    ++totalSamples;
    if (totalSamples >= SAMPLE_COUNT)
    {
      int i = 0;
      double throughputAverage = 0;
      double numA = 0.0;
      double numB = 0.0;
      double numC = 0.0;
      double numD = SAMPLE_COUNT;
      double denomA = 0.0;
      double denomB = 0.0;
      double denomC = 0.0;
      double denomD = SAMPLE_COUNT;
      for (; i < SAMPLE_COUNT; ++i)
      {
        int index = (oldest + i) % SAMPLE_COUNT;
        throughputAverage += throughputSamples[index];

        logWrite(SENSOR_DETAIL, "LeastSquares: Delay sample #%d: %d", i,
                 delaySamples[index]);
        numA += i * delaySamples[index];
        numB += i;
        numC += delaySamples[index];
        denomA += i * i;
        denomB += i;
        denomC += i;
      }
      throughputAverage /= SAMPLE_COUNT;

      double num = (numA * numD) - (numB * numC);
      double denom = (denomA * denomD) - (denomB * denomC);

      // Theoretically denom cannot be 0 because our x values are
      // sample numbers which monotonically increase.
      double slope = num/denom;

      logWrite(SENSOR, "LeastSquares: SLOPE: %f", slope);

      if (slope > 0)
      {
        // The closest linear approximation indicates that buffers are
        // being filled up, which means that the link was saturated
        // over the last SAMPLE_COUNT samples. So use the average to
        // yield a result.
        if (static_cast<int>(throughputAverage) != lastReport)
        {
          lastReport = static_cast<int>(throughputAverage);
          ostringstream buffer;
          buffer << static_cast<int>(throughputAverage);
          global::output->genericMessage(AUTHORITATIVE_BANDWIDTH, buffer.str(),
                                         packet->elab);
        }
      }
      else
      {
        // The buffers are not being filled up. So we just have a
        // tentative throughput measurement.
        if (static_cast<int>(throughputAverage) > lastReport)
        {
          lastReport = static_cast<int>(throughputAverage);
          ostringstream buffer;
          buffer << static_cast<int>(throughputAverage);
          global::output->genericMessage(TENTATIVE_THROUGHPUT, buffer.str(),
                                         packet->elab);
        }
      }
      ackValid = true;
    }
    else
    {
      ackValid = false;
    }
  }
  else
  {
    ackValid = false;
  }
}
