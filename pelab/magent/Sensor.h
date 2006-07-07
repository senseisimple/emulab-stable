// Sensor.h

// This is the abstract base class for all of the individual
// measurement code, filters, and others. It includes code which
// allows a Sensor to represent a polymorphic list.

#ifndef SENSOR_H_STUB_2
#define SENSOR_H_STUB_2

class Sensor
{
public:
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
      next=node;
  }
  void captureSend(Time const & packetTime, struct tcp_info const * kernel,
                   struct tcphdr const * tcp, Order const & elab,
                   bool bufferFull)
  {
      localSend(packetTime, kernel, tcp, elab, bufferFull);
      if (next.get() != NULL)
      {
          next->captureSend(packetTime, kernel, tcp, elab, bufferFull);
      }
  }
  void captureAck(Time const & packetTime, struct tcp_info * kernel,
                  struct tcphdr * tcp, Order const & elab,
                  bool bufferFull)
  {
      localAck(packetTime, kernel, tcp, elab, bufferFull);
      if (next.get() != NULL)
      {
          next->captureAck(packetTime, kernel, tcp, elab, bufferFull);
      }
  }
  std::auto_ptr<Sensor> clone(void) const;
  {
      std::auto_ptr<Sensor> result(localClone());
      if (next.get() != NULL)
      {
          result.next = next->clone();
      }
      return result;
  }
private:
  std::auto_ptr<Sensor> next;
protected:
  virtual void localSend(Time const & packetTime,
                         struct tcp_info const * kernel,
                         struct tcphdr const * tcp, Order const & elab,
                         bool bufferFull)=0;
  virtual void localAck(Time const & packetTime,
                        struct tcp_info const * kernel,
                        struct tcphdr const * tcp, Order const & elab,
                        bool bufferFull)=0;
  virtual std::auto_ptr<Sensor> clone(void) const=0;
};

#endif
