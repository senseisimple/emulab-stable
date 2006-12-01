/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// EwmaThroughputSensor.cc

#include "lib.h"
#include "EwmaThroughputSensor.h"
#include "ThroughputSensor.h"
#include "CommandOutput.h"
#include "StateSensor.h"

using namespace std;

EwmaThroughputSensor::EwmaThroughputSensor(
  ThroughputSensor const * newThroughputSource,
  StateSensor const * newState)
  : maxThroughput(0)
  , bandwidth(0.0)
  , throughputSource(newThroughputSource)
  , state(newState)
{
}

void EwmaThroughputSensor::localSend(PacketInfo * packet)
{
  ackValid = false;
  sendValid = true;
}

void EwmaThroughputSensor::localAck(PacketInfo * packet)
{
  sendValid = false;
  if (throughputSource->isAckValid() && state->isAckValid())
  {
    int latest = throughputSource->getThroughputInKbps();
    if (state->isSaturated())
    {
      // The link is saturated, so we know that the throughput
      // measurement is the real bandwidth.
      if (bandwidth == 0.0)
      {
        bandwidth = latest;
      }
      else
      {
        static const double alpha = 0.1;
        bandwidth = bandwidth*(1.0-alpha) + latest*alpha;
      }
      // We have got an actual bandwidth measurement, so reset
      // maxThroughput accordingly.
      maxThroughput = static_cast<int>(bandwidth);
      ostringstream buffer;
      buffer << static_cast<int>(bandwidth);
      global::output->genericMessage(AUTHORITATIVE_BANDWIDTH, buffer.str(),
                                     packet->elab);
    }
    else
    {
      // The link isn't saturated, so we don't know whether this
      // throughput measurement represents real bandwidth or not.
      if (latest > maxThroughput)
      {
        maxThroughput = latest;
        // Send out a tentative number
        ostringstream buffer;
        buffer << maxThroughput;
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
