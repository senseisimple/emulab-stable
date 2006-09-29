// MaxDelaySensor.cc

#include "lib.h"
#include "MaxDelaySensor.h"
#include "DelaySensor.h"
#include "CommandOutput.h"
#include "StateSensor.h"

using namespace std;

MaxDelaySensor::MaxDelaySensor(DelaySensor const * newDelay,
                               StateSensor const * newState,
                               MinDelaySensor const * newminDelay,
                               PacketSensor const * newpacketSensor)
  : maximum(0, 0.01)
  , delay(newDelay)
  , state(newState)
  , mindelay(newminDelay)
  , packetsensor(newpacketSensor)
  , lastreported(-1)
{
}

void MaxDelaySensor::localSend(PacketInfo * packet)
{
  ackValid = false;
  /*
   * If we detect packet loss, report the maximum delay we've recently seen on
   * the forward path
   */
  if (packetsensor->isSendValid() && packetsensor->getIsRetransmit()) {
    if (maximum.get() <= 0) {
      logWrite(ERROR,"maxDelaySensor::localSend() got bogus max %i",
                      maximum.get());
      sendValid = false;
      return;
    }
    if (lastreported != maximum.get()) {
      logWrite(SENSOR,"MaxDelaySensor::localSend() reporting new max %d",
               maximum.get());
      ostringstream buffer;
      buffer << "MAXINQ=" << maximum.get();
      global::output->eventMessage(buffer.str(), packet->elab);
      lastreported = maximum.get();
    } else {
      logWrite(SENSOR_DETAIL,"MaxDelaySensor::localSend() suppressing max %d",
               maximum.get());
    }
    sendValid = true;
  }
  else
  {
    sendValid = false;
  }
}

void MaxDelaySensor::localAck(PacketInfo * packet)
{
  sendValid = false;

  /*
   * Only try to make this calculation on established connections
   */
  if (! state->isAckValid() || ! delay->isAckValid()
      || ! mindelay->isAckValid()
      ||  state->getState() != StateSensor::ESTABLISHED) {
    ackValid = false;
    return;
  }

  /*
   * We assume that the minimum delay is transmission delay plus the
   * propagation delay. Thus, any additional time is from queueing.
   * Thus, queueingDelay = maxDelay - minDelay
   * BUT - the latency we feed to dummynet is the whole time spend in the
   * dummynet queue, so we have to include the 'forward' part of the min delay
   * in our calculation.
   */
  int current = delay->getLastDelay();
  int minimumDelay = mindelay->getMinDelay();
  int queueingDelay = current - (minimumDelay/2);
  logWrite(SENSOR, "current=%d,min=%d,queueing=%d,saturated=%d", current,
           minimumDelay, queueingDelay, state->isSaturated());
  if (queueingDelay < 0) {
    logWrite(ERROR,"Queueing delay is less than zero!");
    ackValid = false;
    return;
  }
  /*
   * Keep track of the maximum delay on the forward path that we've seen
   * recently. If we discover any packet loss, we'll report the delay seen for
   * by this packet
   */
  if ((queueingDelay > maximum) && (current != 0))
  {
    maximum.reset(queueingDelay);
  }
  else
  {
    maximum.decay();
  }
  ackValid = true;
}
