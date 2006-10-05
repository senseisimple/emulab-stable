// LeastSquaresThroughput.cc

#include "lib.h"
#include "LeastSquaresThroughput.h"
#include "TSThroughputSensor.h"
#include "DelaySensor.h"
#include "CommandOutput.h"

using namespace std;

const int LeastSquaresThroughput::MAX_SAMPLE_COUNT;
const int LeastSquaresThroughput::DEFAULT_MAX_PERIOD;
const int LeastSquaresThroughput::MAX_LEAST_SQUARES_SAMPLES;

LeastSquaresThroughput::LeastSquaresThroughput(
  TSThroughputSensor const * newThroughput,
  DelaySensor const * newDelay,
  int newMaxPeriod)

  : throughput(newThroughput)
  , delay(newDelay)
  , latest(0)
  , totalSamples(0)
  , maxPeriod(newMaxPeriod)
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
    if (totalSamples > 0)
    {
      latest = (latest + 1) % MAX_SAMPLE_COUNT;
    }

    Ack newAck;
    newAck.size = throughput->getLastByteCount();
    newAck.period = throughput->getLastPeriod();
    newAck.rtt = delay->getLastDelay();
    samples[latest] = newAck;
    ++totalSamples;

    int i = 0;
    int limit = min(MAX_SAMPLE_COUNT, totalSamples);
    int leastSquareCount = 0;
    int byteTotal = 0;
    uint32_t timeTotal = 0;
    int x_i = 0;
    int y_i = 0;
    double numA = 0.0;
    double numB = 0.0;
    double numC = 0.0;
    double denomA = 0.0;
    double denomB = 0.0;
    double denomC = 0.0;
    for (; i < limit && (timeTotal <= static_cast<uint32_t>(maxPeriod)
                         || maxPeriod <= 0); ++i)
    {
      // Add an extra MAX_SAMPLE_COUNT because taking the mod of a
      // negative dividend is undefined (could be negative, could be
      // from division rounding up or down, etc.)
      int index = (latest - i + MAX_SAMPLE_COUNT) % MAX_SAMPLE_COUNT;
      byteTotal += samples[index].size;
      timeTotal += samples[index].period;

      if (i < MAX_LEAST_SQUARES_SAMPLES)
      {
        logWrite(SENSOR_COMPLETE, "LeastSquares: ***Delay sample #%d: %d", i,
                 samples[index].rtt);
        logWrite(SENSOR_COMPLETE, "LeastSquares: Period sample: %d",
                 samples[index].period);
        logWrite(SENSOR_COMPLETE, "LeastSquares: Kilobit sample: %f",
                 samples[index].size*(8.0/1000.0));

        ++leastSquareCount;
        x_i += samples[index].period;
        y_i = samples[index].rtt;
        numA += x_i * y_i;
        numB += x_i;
        numC += y_i;
        denomA += x_i * x_i;
        denomB += x_i;
        denomC += x_i;
      }
    }
    double numD = leastSquareCount;
    double denomD = leastSquareCount;


    // Calculate throughput.
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
    char * bufferFullBuffer;
    if (packet->bufferFull)
    {
      bufferFullBuffer = "Buffer is FULL";
    }
    else
    {
      bufferFullBuffer = "Buffer is EMPTY";
    }
    logWrite(SENSOR, "LeastSquares: %s, cwnd: %d", bufferFullBuffer,
             packet->kernel->tcpi_snd_cwnd);
    logWrite(SENSOR, "LeastSquares: SLOPE: %f TPA: %i LR:%i", slope,
             static_cast<int>(throughputAverage),lastReport);

    if (leastSquareCount >= MAX_LEAST_SQUARES_SAMPLES)
    {
      // We have reversed the points left to right. This means that the
      // line is reflected. So decreasing slope on the reflected-line
      // means that there would have been increasing slope on the
      // original line and therefore increasing delays.
      if (slope < 0.0)
      {
        // The closest linear approximation indicates that buffers are
        // being filled up, which means that the link was saturated
        // over the last MAX_SAMPLE_COUNT samples. So use the average to
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
