// ThroughputSensor.cc

#include "lib.h"
#include "ThroughputSensor.h"
#include "PacketSensor.h"

using namespace std;

ThroughputSensor::ThroughputSensor(PacketSensor * newPacketHistory)
  : throughputInKbps(0)
  , packetHistory(newPacketHistory)
{
  throughputInKbps = 0;
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
  Time currentAckTime = packet->packetTime;
  if (lastAckTime != Time() && currentAckTime != lastAckTime)
  {
    // period is in seconds.
    double period = (currentAckTime - lastAckTime).toMilliseconds() / 1000.0;
    double kilobits = packetHistory->getAckedSize() * (8.0/1000.0);
    throughputInKbps = static_cast<int>(kilobits/period);
  }
  lastAckTime = currentAckTime;
}
