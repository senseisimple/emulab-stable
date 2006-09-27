// LeastSquaresThroughput.cc

#include "lib.h"
#include "LeastSquaresThroughput.h"
#include "TSThroughputSensor.h"
#include "DelaySensor.h"
#include "CommandOutput.h"

using namespace std;

LeastSquaresThroughput::LeastSquaresThroughput(
  TSThroughputSensor const * newThroughput,
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
//    throughputSamples[oldest] = throughput->getThroughputInKbps();
    byteSamples[oldest] = throughput->getLastByteCount();
    timeSamples[oldest] = throughput->getLastPeriod();
    delaySamples[oldest] = delay->getLastDelay();
    oldest = (oldest + 1) % SAMPLE_COUNT;
    ++totalSamples;
    if (totalSamples >= SAMPLE_COUNT)
    {
      int i = 0;
//      double throughputAverage = 0;
      int byteTotal = 0;
      uint32_t timeTotal = 0;
      int x_i = 0;
      int y_i = 0;
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
//        throughputAverage += throughputSamples[index];
        byteTotal += byteSamples[index];
        timeTotal += timeSamples[index];

        logWrite(SENSOR_DETAIL, "LeastSquares: ***Delay sample #%d: %d", i,
                 delaySamples[index]);
        logWrite(SENSOR_DETAIL, "LeastSquares: Period sample: %d",
                 timeSamples[index]);
        logWrite(SENSOR_DETAIL, "LeastSquares: Kilobit sample: %f",
                 byteSamples[index]*(8.0/1000.0));

        x_i += timeSamples[index];
        y_i = delaySamples[index];
        numA += x_i * y_i;
        numB += x_i;
        numC += y_i;
        denomA += x_i * x_i;
        denomB += x_i;
        denomC += x_i;
      }
      // Calculate throughput.
//      throughputAverage /= SAMPLE_COUNT;
      logWrite(SENSOR, "LeastSquares: timeTotal: %d, kilobitTotal: %f",
               timeTotal, byteTotal*(8.0/1000.0));
      double throughputAverage = throughput->getThroughputInKbps(timeTotal,
                                                                 byteTotal);

      double num = (numA * numD) - (numB * numC);
      double denom = (denomA * denomD) - (denomB * denomC);

      // Theoretically denom cannot be 0 because our x values are
      // sample numbers which monotonically increase.
      double slope = 0.0;
      if (fabs(denom) > 0.00001)
      {
        slope = num/denom;
      }

      logWrite(SENSOR, "LeastSquares: SLOPE: %f TPA: %i LR:%i", slope,
               static_cast<int>(throughputAverage),lastReport);

      if (slope > 0.0)
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
