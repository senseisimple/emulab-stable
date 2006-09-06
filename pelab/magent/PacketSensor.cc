// PacketSensor.cc

#include "lib.h"
#include "PacketSensor.h"
#include "StateSensor.h"

using namespace std;

PacketSensor::PacketSensor(StateSensor const * newState)
  : globalSequence()
  , state(newState)
{
  ackedSize = 0;
  ackedSendTime = Time();
  isRetransmit = false;
}

int PacketSensor::getAckedSize(void) const
{
  if (ackedSendTime == Time())
  {
    logWrite(ERROR, "PacketSensor::getAckedSize() called before localAck()");
  }
  return ackedSize;
}

bool PacketSensor::getIsRetransmit(void) const
{
  return isRetransmit;
}

Time const & PacketSensor::getAckedSendTime(void) const
{
  if (ackedSendTime == Time())
  {
    logWrite(ERROR, "PacketSensor::getAckedSendTime() called before"
             " localAck()");
  }
  return ackedSendTime;
}

void PacketSensor::localSend(PacketInfo * packet)
{
  // Assume this packet is not a retransmit unless proven otherwise
  isRetransmit = false;
  if (state->getState() == StateSensor::ESTABLISHED)
  {
    logWrite(SENSOR_DETAIL,
             "PacketSensor::localSend() for sequence number %u",
             ntohl(packet->tcp->seq));
    unsigned int startSequence = ntohl(packet->tcp->seq);
    if (globalSequence.inSequenceBlock(startSequence))
    {
      /*
       * This packet should be in our list of sent packets - ie. it's a
       * retransmit.
       */
      logWrite(SENSOR_DETAIL,
               "PacketSensor::localSend() found a retransmit");
      isRetransmit = true;
      list<SentPacket>::iterator pos = unacked.begin();
      list<SentPacket>::iterator limit = unacked.end();
      bool done = false;
      for (; pos != limit && !done; ++pos)
      {
        if (pos->inSequenceBlock(startSequence))
        {
          pos->timestamp = packet->packetTime;
          done = true;
        }
      }

      if (!done) {
        logWrite(ERROR, "localSend() unable to find packet record to update.");
      }
    }
    else
    {
      /*
       * Not in the current window of unacked packets - create a new
       * SentPacket record for it
       */
      SentPacket record;
      record.seqStart = startSequence;

      /*
       * Calculate the packet payload size - we have to make sure to take into
       * account IP and TCP option headers
       */
      unsigned int sequenceLength =
        // Total length of the IP part of the packet
        (ntohs(packet->ip->ip_len))
        // Total length of all IP headers (including options)
        - (packet->ip->ip_hl*4)
        // Total length of all TCP headers (including options)
        - (packet->tcp->doff*4);
      // We want to get the sequence number of the last data byte, not the
      // sequence number of the first byte of the next segment
      record.seqEnd = record.seqStart + sequenceLength - 1;
      record.totalLength = packet->packetLength;
      record.timestamp = packet->packetTime;
      logWrite(SENSOR_COMPLETE,
               "PacketSensor::localSend() new record: ss=%u,sl=%u,se=%u,tl=%u",
               record.seqStart, sequenceLength, record.seqEnd,
               record.totalLength);
      if (unacked.empty())
      {
        globalSequence.seqStart = record.seqStart;
        globalSequence.seqEnd = record.seqEnd;
      }
      else
      {
        /*
         * Sanity check - the new packet we're adding should start where the
         * last one left off
         */
        if (record.seqStart != (globalSequence.seqEnd + 1))
        {
          fprintf(stderr,"PacketSensor::localSend() may have missed a "
                   "packet - last seq seen: %u, new seq: %u (lost %d)",
                   globalSequence.seqEnd,record.seqStart,
                   record.seqStart - globalSequence.seqEnd);
        }
        globalSequence.seqEnd = record.seqEnd;
      }
      logWrite(SENSOR_COMPLETE,
               "PacketSensor::localSend(): global start = %u, global end = %u",
               globalSequence.seqStart, globalSequence.seqEnd);
      unacked.push_back(record);
    }

    ackedSize = 0;
    ackedSendTime = Time();
  }
}

void PacketSensor::localAck(PacketInfo * packet)
{
  // Right now, we don't know whether or not this ACK is for a retransmitted
  // packet (we could tell, but there doesn't seem to be a need yet)
  isRetransmit = false;
  if (state->getState() == StateSensor::ESTABLISHED)
  {
    /*
     * When we get an ACK, the sequence number is really the next one the peer
     * excects to see: thus, the last sequence number it's ACKing is one less
     * than this.
     * Note: This should handle wraparound properly
     */
    uint32_t ack_for = ntohl(packet->tcp->ack_seq) - 1;

    logWrite(SENSOR_DETAIL, "PacketSensor::localAck() for sequence number %u",
             ack_for);

    /*
     * Check to see if the ack is for a seq number smaller than the global
     * sequence - this could mean two things: (1) it's a duplicate ack (in
     * which case it means packet loss or packet re-ordering) (2) we got two
     * (or more) acks out of order
     * No action taken yet other than logging
     * XXX: Needs support for wraparound!
     */
    if (ack_for < globalSequence.seqStart)
    {
      if (ack_for == (globalSequence.seqStart - 1)) {
        logWrite(SENSOR_DETAIL, "PacketSensor::localAck() detected a "
                                "duplicate ack");
        return;
      } else {
        logWrite(SENSOR_DETAIL, "PacketSensor::localAck() detected an "
                                "old ack");
      }
    }

    list<SentPacket>::iterator pos = unacked.begin();
    list<SentPacket>::iterator limit = unacked.end();
    bool found = false;
    ackedSize = 0;

    /*
     * Make sure this packet doesn't have a SACK option, in which case our
     * calculation is wrong.
     */
    list<Option>::iterator opt;
    for (opt = packet->tcpOptions->begin();
         opt != packet->tcpOptions->end();
         ++opt) {
      if (opt->type == TCPOPT_SACK) {
        logWrite(ERROR,"Packet has a SACK option!");
      }
    }

    while (pos != limit && !found)
    {
      found = pos->inSequenceBlock(ack_for);
      /*
       * XXX: Assumes that SACK is not in use - assumes that this ACK
       * is for all sequence numbers up to the one it's ACKing
       */
      ackedSize += pos->totalLength;
      if (found)
      {
        ackedSendTime = pos->timestamp;
      }
      if (pos != limit)
      {
        ++pos;
      }
    }
    if (!found)
    {
      logWrite(ERROR, "Could not find the ack sequence number in list "
               "of unacked packets.");
      ackedSize = 0;
      ackedSendTime = Time();
      return;
    }
    unacked.erase(unacked.begin(), pos);
    if (unacked.empty())
    {
      globalSequence.seqStart = 0;
      globalSequence.seqEnd = 0;
    }
    else
    {
      globalSequence.seqStart = unacked.front().seqStart;
    }

    logWrite(SENSOR_DETAIL, "PacketSensor::localAck() decided on size %u",
             ackedSize);
  }
}

bool PacketSensor::SentPacket::inSequenceBlock(unsigned int sequence)
{
  logWrite(SENSOR_COMPLETE,
           "PacketSensor::inSequenceBlock(): Is %u between %u and %u?",
           sequence, seqStart, seqEnd);
  bool result = false;
  if (seqStart < seqEnd)
  {
    result = sequence >= seqStart && sequence <= seqEnd;
  }
  else if (seqStart > seqEnd)
  {
    /*
     * This handles the sequence number wrapping around
     */
    result = sequence >= seqStart || sequence <= seqEnd;
  }
  if (result)
  {
    logWrite(SENSOR_COMPLETE, "Yes!");
  }
  else
  {
    logWrite(SENSOR_COMPLETE, "No!");
  }
  return result;
}

PacketSensor::SentPacket::SentPacket()
    : seqStart(0), seqEnd(0), totalLength(), timestamp() {
}
