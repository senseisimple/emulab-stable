// DelaySensor.cc

#include "lib.h"
#include "DelaySensor.h"
#include "PacketSensor.h"
#include "StateSensor.h"
#include "Time.h"
#include "TSThroughputSensor.h"

using namespace std;

DelaySensor::DelaySensor(PacketSensor const * newPacketHistory,
                         StateSensor const * newState)
  : state(newState)
{
  lastDelay = 0;
  packetHistory = newPacketHistory;
}

int DelaySensor::getLastDelay(void) const
{
  if (ackValid)
  {
    return lastDelay;
  }
  else
  {
    logWrite(ERROR,
             "DelaySensor::getLastDelay() called with invalid ack data");
    return 0;
  }
}

void DelaySensor::localSend(PacketInfo *)
{
  ackValid = false;
  sendValid = true;
}

void DelaySensor::localAck(PacketInfo * packet)
{
  sendValid = false;
  /*
   * XXX: According to RFC 2988, TCP MUST use Karn's algorithm for RTT
   * calculation, which means that it cannot use retransmitted packets to
   * caculate RTT (since it is ambiguous which of the two packets is being
   * ACKed) unless using TCP timestamps
   */
  if (state->isAckValid() && packetHistory->isAckValid()
      && state->getState() == StateSensor::ESTABLISHED)
  {
    Time diff = packet->packetTime - packetHistory->getAckedSendTime();
    lastDelay = diff.toMilliseconds();
    // Failsafe - if this happens, we need to fix the cause, but in the
    // meantime, let's just make sure that it doesn't make it to the monitor
    if (lastDelay < 0) {
      logWrite(ERROR, "DelaySensor::localAck() Bogus delay %d", lastDelay);
      ackValid = false;
    } else {
      logWrite(SENSOR, "DELAY: %d ms", lastDelay);
      ackValid = true;
    }
  }
  else
  {
    ackValid = false;
  }
}
