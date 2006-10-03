// AverageThroughputSensor.cc

#include "lib.h"
#include "AverageThroughputSensor.h"
#include "TSThroughputSensor.h"
#include "CommandOutput.h"

using namespace std;

AverageThroughputSensor::AverageThroughputSensor(
  TSThroughputSensor const * newThroughput)
  : throughput(newThroughput)
  , latest(0)
  , sampleCount(0)
{
}

AverageThroughputSensor::~AverageThroughputSensor()
{
}

void AverageThroughputSensor::localSend(PacketInfo * packet)
{
  ackValid = false;
  sendValid = true;
}

void AverageThroughputSensor::localAck(PacketInfo * packet)
{
  sendValid = false;
  if (throughput->isAckValid())
  {
    Ack newAck;
    newAck.size = throughput->getLastByteCount();
    newAck.period = throughput->getLastPeriod();
    if (sampleCount > 0)
    {
      // If the buffer is not empty, then we want to put the new ack
      // in the slot after latest.
      latest = (latest + 1) % MAX_SAMPLE_COUNT;
    }
    // Otherwise, latest just points to the first ack and we want to
    // fill it.
    ++sampleCount;
    samples[latest] = newAck;

    int byteSum = 0;
    uint32_t periodSum = 0;
    int i = 0;
    int limit = min(static_cast<int>(MAX_SAMPLE_COUNT), sampleCount);
    for (; i < limit && periodSum < AVERAGE_PERIOD; ++i)
    {
      // Add an extra MAX_SAMPLE_COUNT because taking the mod of a
      // negative dividend is undefined (could be negative, could be
      // from division rounding up or down, etc.)
      int index = (latest - i + MAX_SAMPLE_COUNT) % MAX_SAMPLE_COUNT;
      byteSum += samples[index].size;
      periodSum += samples[index].period;
    }

    int result = throughput->getThroughputInKbps(periodSum, byteSum);
    logWrite(SENSOR, "AVERAGE_THROUGHPUT: %d, period %u, kilobits %f",
             result, periodSum, byteSum*(8.0/1000.0));

    ackValid = true;
  }
}
