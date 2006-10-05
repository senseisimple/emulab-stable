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
  struct SentPacket;
  std::list<SentPacket> unacked;
  typedef std::list<SentPacket>::iterator SentPacketIterator;
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
    // Has this packet already been SACKed?
    bool sacked;
    // Was this a fake record we generated due to packet loss in libpcap?
    bool fake;

    // Split one sent packet into two at the sequence number indicated. The
    // WARNING: Due to STL lameness, your iterator is invalid after this
    // call!
    static void
      splitPacket(SentPacketIterator, unsigned int, std::list<SentPacket> *);
  };

  /*
   * We use this to keep track of the ranges being ACKed and SACKed
   */
  struct Range 
  {
    Range(unsigned int, unsigned int, bool);
    unsigned int start;
    unsigned int end;
    bool from_sack;
  };
  typedef std::list<Range> rangelist;
private:
  int ackedSize;
  Time ackedSendTime;
  SentPacket globalSequence;
  bool isRetransmit;

  StateSensor const * state;

  // Typically, 4% of a packet is header. It's more, though, if the packet is
  // less than 1500 bytes
  static const float SUPERMAGIC_PACKET_MULTIPLIER = 1.04;
};

#endif
