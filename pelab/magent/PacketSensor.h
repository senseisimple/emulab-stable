// PacketSensor.h

// This sensor keeps track of unacknowledged packets.

#ifndef PACKET_SENSOR_H_STUB_2
#define PACKET_SENSOR_H_STUB_2

#include "Sensor.h"
#include "Time.h"
#include "lib.h"

class StateSensor;

class PacketSensor : public Sensor
{
public:
  PacketSensor(StateSensor const * newState);
  // Get the size of the acknowledgement in bytes.
  int getAckedSize(void) const;
  // Find out if the packet in question is a retransmission
  bool getIsRetransmit(void) const;
  // Get the time of the last packet sent which was acked.
  Time const & getAckedSendTime(void) const;
protected:
  virtual void localSend(PacketInfo * packet);
  virtual void localAck(PacketInfo * packet);
private:
  struct SentPacket
  {
    SentPacket();
    bool inSequenceBlock(unsigned int sequence);

    unsigned int seqStart;
    unsigned int seqEnd;
    // This is the total length including all headers.
    unsigned int totalLength;
    Time timestamp;
    // Has this packet been retransmitted?
    bool retransmitted;
  };

  typedef std::pair<uint32_t, uint32_t> rangepair;
  typedef std::list<rangepair> rangelist;
private:
  int ackedSize;
  Time ackedSendTime;
  std::list<SentPacket> unacked;
  SentPacket globalSequence;
  bool isRetransmit;

  StateSensor const * state;
};

#endif
