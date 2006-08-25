// ThroughputSensor.cc

#include "lib.h"
#include "ThroughputSensor.h"
#include "PacketSensor.h"
#include "StateSensor.h"

using namespace std;

ThroughputSensor::ThroughputSensor(PacketSensor * newPacketHistory,
                                   StateSensor * newState)
  : throughputInKbps(0)
  , maxThroughput(0)
  , packetHistory(newPacketHistory)
  , state(newState)
{
}

int ThroughputSensor::getThroughputInKbps(void) const
{
  return throughputInKbps;
}

void ThroughputSensor::localSend(PacketInfo *)
{
}

void ThroughputSensor::localAck(PacketInfo * packet)
{
  if (state->getState() == StateSensor::ESTABLISHED)
  {
    Time currentAckTime = packet->packetTime;
    if (lastAckTime != Time() && currentAckTime != lastAckTime)
    {
      // period is in seconds.
      double period = (currentAckTime - lastAckTime).toMilliseconds() / 1000.0;
      double kilobits = packetHistory->getAckedSize() * (8.0/1000.0);
      int latest = static_cast<int>(kilobits/period);
      if (state->isSaturated() || latest > maxThroughput)
      {
        throughputInKbps = latest;
        maxThroughput = latest;
        logWrite(SENSOR, "THROUGHPUT: %d kbps", throughputInKbps);
      }
    }
    else
    {
      throughputInKbps = 0;
    }
    lastAckTime = currentAckTime;
  }
  else
  {
    throughputInKbps = 0;
    lastAckTime = Time();
  }
}
