// MaxDelaySensor.cc

#include "lib.h"
#include "MaxDelaySensor.h"
#include "DelaySensor.h"
#include "CommandOutput.h"
#include "StateSensor.h"

using namespace std;

MaxDelaySensor::MaxDelaySensor(DelaySensor * newDelay, StateSensor * newState)
  : maximum(0, 0.01)
  , delay(newDelay)
  , state(newState)
{
}

void MaxDelaySensor::localSend(PacketInfo *)
{
}

void MaxDelaySensor::localAck(PacketInfo * packet)
{
  int current = delay->getLastDelay();
  logWrite(SENSOR, "current=%d,saturated=%d", current, state->isSaturated());
  if (current > maximum && current != 0 && state->isSaturated())
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
