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
  else if (packet->packetType == PACKET_INFO_ACK_COMMAND)
  {
    localAck(packet);
  } else {
    logWrite(ERROR,"Sensor::capturePacket() got unexpected packet type %d",
             packet->packetType);
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

void NullSensor::localSend(PacketInfo * packet)
{
  ackValid = false;
  sendValid = true;
  logWrite(SENSOR, "----------------------------------------");
  logWrite(SENSOR, "Send received: Time: %f", packet->packetTime.toDouble());
}

void NullSensor::localAck(PacketInfo * packet)
{
  sendValid = false;
  ackValid = true;
  logWrite(SENSOR, "----------------------------------------");
  logWrite(SENSOR, "Ack received: Time: %f", packet->packetTime.toDouble());
  list<Option>::iterator pos = packet->tcpOptions->begin();
  list<Option>::iterator limit = packet->tcpOptions->end();
  for (; pos != limit; ++pos)
  {
    logWrite(SENSOR, "TCP Option: %d", pos->type);
  }
}
