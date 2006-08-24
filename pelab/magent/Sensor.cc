// Sensor.cc

#include "lib.h"
#include "Sensor.h"

using namespace std;

bool Sensor::isLinkSaturated(PacketInfo * packet)
{
  return packet->bufferFull &&
    // and *not* in slow start
    !(packet->kernel->tcpi_snd_cwnd < packet->kernel->tcpi_snd_ssthresh
      || (static_cast<unsigned int>(htons(packet->tcp->window))
          << packet->kernel->tcpi_rcv_wscale)
      <= (packet->kernel->tcpi_unacked * 1448));
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

NullSensor::~NullSensor()
{
}

void NullSensor::localSend(PacketInfo *)
{
  logWrite(SENSOR, "Send received");
}

void NullSensor::localAck(PacketInfo * packet)
{
  logWrite(SENSOR, "Ack received");
  list<Option>::iterator pos = packet->tcpOptions->begin();
  list<Option>::iterator limit = packet->tcpOptions->end();
  for (; pos != limit; ++pos)
  {
    logWrite(SENSOR, "TCP Option: %d", pos->type);
  }
}
