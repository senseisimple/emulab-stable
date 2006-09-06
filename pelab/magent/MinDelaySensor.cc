// MinDelaySensor.cc

#include "lib.h"
#include "MinDelaySensor.h"
#include "DelaySensor.h"
#include "CommandOutput.h"

using namespace std;

MinDelaySensor::MinDelaySensor(DelaySensor * newDelay)
  : minimum(1000000, -0.01), minDelay(1000000), lastreported(-1)
{
  delay = newDelay;
}

void MinDelaySensor::localSend(PacketInfo *)
{
}

void MinDelaySensor::localAck(PacketInfo * packet)
{
  int current = delay->getLastDelay();
  int oneway = current / 2;
  if (current < minimum && current != 0 && oneway != lastreported)
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
}
