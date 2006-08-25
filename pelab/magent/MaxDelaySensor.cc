// MaxDelaySensor.cc

#include "lib.h"
#include "MaxDelaySensor.h"
#include "DelaySensor.h"
#include "CommandOutput.h"
#include "StateSensor.h"

using namespace std;

MaxDelaySensor::MaxDelaySensor(DelaySensor * newDelay, StateSensor * newState,
        MinDelaySensor *newminDelay)
  : maximum(0, 0.01)
  , delay(newDelay)
  , state(newState)
  , mindelay(newminDelay)
{
}

void MaxDelaySensor::localSend(PacketInfo *)
{
}

void MaxDelaySensor::localAck(PacketInfo * packet)
{
  int current = delay->getLastDelay();
  /*
   * We assume that the minimum delay is transmission delay plus the
   * propagation delay. Thus, any additional time is from queueing.
   * Thus, queueingDelay = maxDelay - minDelay
   * BUT - the latency we feed to dummynet is the whole time spend in the
   * dummynet queue, so we have to include the 'forward' part of the min delay
   * in our calculation.
   */
  int minimumDelay = mindelay->getMinDelay();
  int queueingDelay = current - (minimumDelay/2);
  logWrite(SENSOR, "current=%d,min=%d,queueing=%d,saturated=%d", current,
           minimumDelay, queueingDelay, state->isSaturated());
  if (queueingDelay < 0) {
    logWrite(ERROR,"Queueing delay is less than zero!");
  }
  if (queueingDelay > maximum && current != 0) && state->isSaturated())
  {
    ostringstream buffer;
    buffer << "MAXINQ=" << queueingDelay;
    maximum.reset(queueingDelay);
    global::output->eventMessage(buffer.str(), packet->elab);
  }
  else
  {
    maximum.decay();
  }
}
