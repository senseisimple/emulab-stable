// Sensor.cc

#include "lib.h"
#include "Sensor.h"

using namespace std;

Sensor::Sensor()
  : sendValid(false)
  , ackValid(false)
{
}

Sensor::~Sensor()
{
}

Sensor * Sensor::getTail(void)
{
  if (next.get() == NULL)
  {
    return this;
  }
  else
  {
    return next->getTail();
  }
}

void Sensor::addNode(auto_ptr<Sensor> node)
{
  if (next.get() == NULL)
  {
    next = node;
  }
  else
  {
    logWrite(ERROR, "Sensor::addNode(): A list tail was asked to add a node.");
  }
}

void Sensor::capturePacket(PacketInfo * packet)
{
  if (packet->packetType == PACKET_INFO_SEND_COMMAND)
  {
    localSend(packet);
  }
  else
  {
    localAck(packet);
  }
  if (next.get() != NULL)
  {
    next->capturePacket(packet);
  }
}

bool Sensor::isSendValid(void) const
{
  return sendValid;
}

bool Sensor::isAckValid(void) const
{
  return ackValid;
}


NullSensor::~NullSensor()
{
}

void NullSensor::localSend(PacketInfo *)
{
  ackValid = false;
  sendValid = true;
  logWrite(SENSOR, "Send received");
}

void NullSensor::localAck(PacketInfo * packet)
{
  sendValid = false;
  ackValid = true;
  logWrite(SENSOR, "Ack received");
  list<Option>::iterator pos = packet->tcpOptions->begin();
  list<Option>::iterator limit = packet->tcpOptions->end();
  for (; pos != limit; ++pos)
  {
    logWrite(SENSOR, "TCP Option: %d", pos->type);
  }
}
