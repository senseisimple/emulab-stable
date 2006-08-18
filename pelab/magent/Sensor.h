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
  bool isLinkSaturated(PacketInfo * packet);
public:
  virtual ~Sensor();
  Sensor * getTail(void);
  void addNode(std::auto_ptr<Sensor> node);
  void captureSend(PacketInfo * packet);
  void captureAck(PacketInfo * packet);
private:
  std::auto_ptr<Sensor> next;
protected:
  virtual void localSend(PacketInfo * packet)=0;
  virtual void localAck(PacketInfo * packet)=0;
};

class NullSensor : public Sensor
{
public:
  virtual ~NullSensor();
protected:
  virtual void localSend(PacketInfo *);
  virtual void localAck(PacketInfo * packet);
};

#endif
