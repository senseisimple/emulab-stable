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

int StateSensor::getState(void)
{
  return state;
}

void StateSensor::localSend(PacketInfo * packet)
{
  if (packet->tcp->syn && state == INITIAL)
  {
    state = AFTER_SYN;
    logWrite(SENSOR, "State change to AFTER_SYN");
  }
  else if (packet->tcp->syn)
  {
    logWrite(ERROR, "Sent a SYN packet out of order");
  }
  else if (! packet->tcp->syn && state == AFTER_SYN_ACK)
  {
    state = BEFORE_ESTABLISHED;
    logWrite(SENSOR, "State change to BEFORE_ESTABLISHED");
  }
  else if (state == BEFORE_ESTABLISHED)
  {
    state = ESTABLISHED;
    logWrite(SENSOR, "State change to ESTABLISHED");
  }
}

void StateSensor::localAck(PacketInfo * packet)
{
  if (packet->tcp->syn && packet->tcp->ack && state == AFTER_SYN)
  {
    state = AFTER_SYN_ACK;
    logWrite(SENSOR, "State change to AFTER_SYN_ACK");
  }
  else if (packet->tcp->syn && packet->tcp->ack)
  {
    logWrite(ERROR, "Received a SYNACK packet out of order");
  }
  else if (state == BEFORE_ESTABLISHED)
  {
    state = ESTABLISHED;
    logWrite(SENSOR, "State change to ESTABLISHED");
  }
}
