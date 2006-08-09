// DelaySensor.cc

#include "lib.h"
#include "DelaySensor.h"
#include "PacketSensor.h"
#include "Time.h"

using namespace std;

DelaySensor::DelaySensor(PacketSensor * newPacketHistory)
{
  lastDelay = 0;
  packetHistory = newPacketHistory;
}

int DelaySensor::getLastDelay(void) const
{
  return lastDelay;
}

void DelaySensor::localSend(PacketInfo *)
{
}

void DelaySensor::localAck(PacketInfo * packet)
{
  Time diff = packet->packetTime - packetHistory->getAckedSendTime();
  lastDelay = diff.toMilliseconds();
}
