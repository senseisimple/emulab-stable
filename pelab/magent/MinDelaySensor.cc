// MinDelaySensor.cc

#include "lib.h"
#include "MinDelaySensor.h"
#include "DelaySensor.h"
#include "CommandOutput.h"

using namespace std;

MinDelaySensor::MinDelaySensor(DelaySensor * newDelay)
  : minimum(1000000, -0.01), minDelay(1000000)
{
  delay = newDelay;
}

void MinDelaySensor::localSend(PacketInfo *)
{
}

void MinDelaySensor::localAck(PacketInfo * packet)
{
  int current = delay->getLastDelay();
  if (current < minimum && current != 0)
  {
    minDelay = current;
    ostringstream buffer;
    buffer << "delay=" << minDelay/2;
    minimum.reset(current);
    global::output->eventMessage(buffer.str(), packet->elab,
                                 CommandOutput::FORWARD_PATH);
    global::output->eventMessage(buffer.str(), packet->elab,
                                 CommandOutput::BACKWARD_PATH);
  }
  else
  {
    minimum.decay();
  }
}
