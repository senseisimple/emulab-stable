// StateSensor.cc

#include "lib.h"
#include "StateSensor.h"

using namespace std;

StateSensor::StateSensor()
  : state(INITIAL)
{
  logWrite(SENSOR, "State change to INITIAL");
}

StateSensor::~StateSensor()
{
}

int StateSensor::getState(void) const
{
  if (sendValid || ackValid)
  {
    return state;
  }
  else
  {
    logWrite(ERROR,
             "StateSensor::getState() called with invalid data");
    return INITIAL;

  }
}

bool StateSensor::isSaturated(void) const
{
  if (sendValid || ackValid)
  {
    return saturated;
  }
  else
  {
    logWrite(ERROR,
             "StateSensor::isSaturated() called with invalid data");
    return false;
  }
}

void StateSensor::localSend(PacketInfo * packet)
{
  ackValid = false;
  sendValid = true;
  if (packet->tcp->syn && state == INITIAL)
  {
    state = AFTER_SYN;
    logWrite(SENSOR, "State change to AFTER_SYN");
  }
  else if (packet->tcp->syn)
  {
    logWrite(ERROR, "Sent a SYN packet out of order");
    sendValid = false;
  }
  else if (! packet->tcp->syn && state == AFTER_SYN_ACK)
  {
    state = ESTABLISHED;
    logWrite(SENSOR, "State change to ESTABLISHED");
  }
  calculateSaturated(packet);
}

void StateSensor::localAck(PacketInfo * packet)
{
  sendValid = false;
  ackValid = true;
  if (packet->tcp->syn && packet->tcp->ack && state == AFTER_SYN)
  {
    state = AFTER_SYN_ACK;
    logWrite(SENSOR, "State change to AFTER_SYN_ACK");
  }
  else if (packet->tcp->syn && packet->tcp->ack)
  {
    logWrite(ERROR, "Received a SYNACK packet out of order");
    ackValid = false;
  }
  calculateSaturated(packet);
}

void StateSensor::calculateSaturated(PacketInfo * packet)
{
  unsigned int snd_cwnd = packet->kernel->tcpi_snd_cwnd;
  unsigned int snd_ssthresh = packet->kernel->tcpi_snd_ssthresh;
  unsigned int window = (static_cast<unsigned int>(htons(packet->tcp->window))
    << packet->kernel->tcpi_rcv_wscale);
  unsigned int unacked = packet->kernel->tcpi_unacked * 1448;
  logWrite(SENSOR, "stateEstablished=%d,bufferFull=%d", state == ESTABLISHED,
           packet->bufferFull);
  logWrite(SENSOR, "snd_cwnd=%u,snd_ssthresh=%u,window=%u,unacked=%u",
           snd_cwnd, snd_ssthresh, window, unacked);
  saturated = (state == ESTABLISHED
               && packet->bufferFull
               // and *not* in slow start
               && !(snd_cwnd < snd_ssthresh || window <= unacked));
}
