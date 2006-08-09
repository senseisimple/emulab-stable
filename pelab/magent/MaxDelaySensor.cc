// MaxDelaySensor.cc

#include "lib.h"
#include "MaxDelaySensor.h"
#include "DelaySensor.h"
#include "CommandOutput.h"

using namespace std;

MaxDelaySensor::MaxDelaySensor(DelaySensor * newDelay)
  : maximum(0, 0.01)
{
  delay = newDelay;
}

void MaxDelaySensor::localSend(PacketInfo *)
{
}

void MaxDelaySensor::localAck(PacketInfo * packet)
{
  int current = delay->getLastDelay();
  if (current > maximum && current != 0)
  {
    ostringstream buffer;
    buffer << "MAXINQ=" << current;
    maximum.reset(current);
    global::output->eventMessage(buffer.str(), packet->elab);
  }
  else
  {
    maximum.decay();
  }
}
