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
  Sensor();
  virtual ~Sensor();
  Sensor * getTail(void);
  void addNode(std::auto_ptr<Sensor> node);
  void capturePacket(PacketInfo * packet);
  bool isSendValid(void) const;
  bool isAckValid(void) const;
private:
  std::auto_ptr<Sensor> next;
protected:
  virtual void localSend(PacketInfo * packet)=0;
  virtual void localAck(PacketInfo * packet)=0;
protected:
  // This is used for functions which only yield data on a send.
  bool sendValid;
  // This is used for functions which only yield data on an ack.
  bool ackValid;
};

class NullSensor : public Sensor
{
public:
  NullSensor();
  virtual ~NullSensor();
protected:
  virtual void localSend(PacketInfo *);
  virtual void localAck(PacketInfo * packet);
  
  // This is used to detect packets that we see in a different order than
  // the appeared on the wire
  Time lastPacketTime;

};

#endif
