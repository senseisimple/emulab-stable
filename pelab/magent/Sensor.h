// Sensor.h

// This is the abstract base class for all of the individual
// measurement code, filters, and others. It includes code which
// allows a Sensor to represent a polymorphic list.

#ifndef SENSOR_H_STUB_2
#define SENSOR_H_STUB_2

#include "log.h"

class Sensor
{
public:
  virtual ~Sensor() {}
  Sensor * getTail(void)
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
  void addNode(std::auto_ptr<Sensor> node)
  {
    next = node;
  }
  void captureSend(PacketInfo * packet)
  {
    localSend(packet);
    if (next.get() != NULL)
    {
      next->captureSend(packet);
    }
  }
  void captureAck(PacketInfo * packet)
  {
    localAck(packet);
    if (next.get() != NULL)
    {
      next->captureAck(packet);
    }
  }
private:
  std::auto_ptr<Sensor> next;
protected:
  virtual void localSend(PacketInfo * packet)=0;
  virtual void localAck(PacketInfo * packet)=0;
};

class NullSensor : public Sensor
{
public:
  virtual ~NullSensor() {}
protected:
  virtual void localSend(PacketInfo *)
  {
    logWrite(SENSOR, "Send received");
  }
  virtual void localAck(PacketInfo *)
  {
    logWrite(SENSOR, "Ack received");
  }
};

#endif
