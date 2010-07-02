/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// MinDelaySensor.cc

#include "lib.h"
#include "MinDelaySensor.h"
#include "DelaySensor.h"
#include "CommandOutput.h"

using namespace std;

MinDelaySensor::MinDelaySensor(DelaySensor const * newDelay)
  : minimum(1000000, -0.01), minDelay(1000000), lastreported(-1)
{
  delay = newDelay;
}

int MinDelaySensor::getMinDelay(void) const
{
  if (ackValid)
  {
    return minDelay;
  }
  else
  {
    logWrite(ERROR,
             "MinDelaySensor::getMinDelay() called with invalid ack data");
    return 0;
  }
}

void MinDelaySensor::localSend(PacketInfo *)
{
  ackValid = false;
  sendValid = true;
}

void MinDelaySensor::localAck(PacketInfo * packet)
{
  sendValid = false;
  if (delay->isAckValid())
  {
    int current = delay->getLastDelay();
    int oneway = current / 2;
    if (current < minimum && oneway != lastreported)
    {
      minDelay = current;
      ostringstream buffer;
      buffer << "delay=" << oneway;
      minimum.reset(current);
      global::output->eventMessage(buffer.str(), packet->elab,
                                   CommandOutput::FORWARD_PATH);
      global::output->eventMessage(buffer.str(), packet->elab,
                                   CommandOutput::BACKWARD_PATH);
      lastreported = oneway;
    }
    else
    {
      minimum.decay();
    }
    logWrite(SENSOR_DETAIL,"MinDelaySensor::localAck() sez: cur=%i one=%i min=%i last=%i",
            current, oneway, minimum.get(), lastreported);
    ackValid = true;
  }
  else
  {
    ackValid = false;
  }
}
