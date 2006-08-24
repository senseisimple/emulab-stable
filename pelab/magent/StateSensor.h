// StateSensor.h

#ifndef STATE_SENSOR_H_STUB_2
#define STATE_SENSOR_H_STUB_2

#include "Sensor.h"

class StateSensor : public Sensor
{
public:
  enum
  {
    INITIAL,
    AFTER_SYN,
    AFTER_SYN_ACK,
    // This doesn't correspond to any real TCP state. This is so that
    // later sensors on the same ack won't see it as part of an
    // established connection.
    BEFORE_ESTABLISHED,
    ESTABLISHED
  };
public:
  StateSensor();
  virtual ~StateSensor();
  int getState(void);
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  int state;
};

#endif
