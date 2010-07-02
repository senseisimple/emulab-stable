/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// ThroughputSensor.cc

#include "lib.h"
#include "ThroughputSensor.h"
#include "PacketSensor.h"
#include "StateSensor.h"

using namespace std;

ThroughputSensor::ThroughputSensor(PacketSensor const * newPacketHistory,
                                   StateSensor const * newState)
  : throughputInKbps(0)
  , maxThroughput(0)
  , packetHistory(newPacketHistory)
  , state(newState)
{
}

int ThroughputSensor::getThroughputInKbps(void) const
{
  if (ackValid)
  {
    return throughputInKbps;
  }
  else
  {
    logWrite(ERROR,
             "ThroughputSensor::getThroughputInKbps() "
             "called with invalid data");
    return 0;
  }
}

void ThroughputSensor::localSend(PacketInfo *)
{
  ackValid = false;
  sendValid = true;
}

void ThroughputSensor::localAck(PacketInfo * packet)
{
  sendValid = false;
  if (state->isAckValid() && packetHistory->isAckValid() &&
      state->getState() == StateSensor::ESTABLISHED)
  {
    ackValid = true;
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
      ackValid = false;
    }
    lastAckTime = currentAckTime;
  }
  else
  {
    throughputInKbps = 0;
    lastAckTime = Time();
    ackValid = false;
  }
}
