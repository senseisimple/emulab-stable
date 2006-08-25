// DelaySensor.cc

#include "lib.h"
#include "DelaySensor.h"
#include "PacketSensor.h"
#include "StateSensor.h"
#include "Time.h"

using namespace std;

DelaySensor::DelaySensor(PacketSensor * newPacketHistory,
                         StateSensor * newState)
  : state(newState)
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
  if (state->getState() == StateSensor::ESTABLISHED)
  {
    Time diff = packet->packetTime - packetHistory->getAckedSendTime();
    lastDelay = diff.toMilliseconds();
    logWrite(SENSOR, "DELAY: %d ms", lastDelay);
  }
}
