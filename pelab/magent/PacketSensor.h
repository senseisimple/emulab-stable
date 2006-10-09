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
  // An INVALID_REGION indicates that either a period of idleness in
  // the connection has occurred (all packets have been acked at some
  // point, and the next ack has not yet been reached). Or a packet
  // loss has occurred and no ack has yet acked only valid packets.

  // A BEGIN_VALID_REGION indicates the first packet of a region. This
  // can be used as the starting barrier for difference calculations
  // in timestamps, for instance. This is set on the first valid ack
  // packet. On the next packet, the state is changed to VALID_REGION

  // A VALID_REGION indicates that this packet represents a packet in
  // the stream when no funny business with packet loss or connection
  // idleness has been detected.
  enum RegionState
  {
    INVALID_REGION = 0,
    BEGIN_VALID_REGION = 1,
    VALID_REGION = 2
  };
public:
  PacketSensor(StateSensor const * newState);
  // Get the size of the acknowledgement in bytes.
  int getAckedSize(void) const;
  // Find out if the packet in question is a retransmission
  bool getIsRetransmit(void) const;
  // Get the time of the last packet sent which was acked.
  Time const & getAckedSendTime(void) const;
  RegionState getRegionState(void) const;
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
  RegionState currentRegionState;
  Time lastAckTime;

  StateSensor const * state;

  // Typically, 4% of a packet is header. It's more, though, if the packet is
  // less than 1500 bytes
  static const float SUPERMAGIC_PACKET_MULTIPLIER = 1.04;
};

#endif
