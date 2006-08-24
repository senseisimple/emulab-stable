// PacketSensor.cc

#include "lib.h"
#include "PacketSensor.h"
#include "StateSensor.h"

using namespace std;

PacketSensor::PacketSensor(StateSensor * newState)
  : globalSequence()
  , state(newState)
{
  ackedSize = 0;
  ackedSendTime = Time();
}

int PacketSensor::getAckedSize(void) const
{
  if (ackedSendTime == Time())
  {
    logWrite(ERROR, "PacketSensor::getAckedSize() called before localAck()");
  }
  return ackedSize;
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
  if (state->getState() == StateSensor::ESTABLISHED)
  {
    logWrite(SENSOR,
             "PacketSensor::localSend() for sequence number %u",
             ntohl(packet->tcp->seq));
    unsigned int startSequence = ntohl(packet->tcp->seq);
    if (globalSequence.inSequenceBlock(startSequence))
    {
      logWrite(SENSOR,
               "PacketSensor::localSend() within globalSequence");
      /*
       * This packet should be in our list of sent packets - ie. it's a
       * retransmit.
       */
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
      logWrite(SENSOR,
               "PacketSensor::localSend() new record: ss=%u,sl=%u,se=%u,tl=%u",
               record.seqStart, sequenceLength, record.seqEnd,
               record.totalLength);
      globalSequence.seqEnd = record.seqEnd;
      if (unacked.empty())
      {
        globalSequence.seqStart = record.seqStart;
        globalSequence.seqEnd = record.seqEnd;
      }
      logWrite(SENSOR,
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
  if (state->getState() == StateSensor::ESTABLISHED)
  {
    list<SentPacket>::iterator pos = unacked.begin();
    list<SentPacket>::iterator limit = unacked.end();
    bool found = false;
    ackedSize = 0;

    /*
     * When we get an ACK, the sequence number is really the next one the peer
     * excects to see: thus, the last sequence number it's ACKing is one less
     * than this.
     * Note: This should handle wraparound properly
     */
    uint32_t ack_for = ntohl(packet->tcp->ack_seq) - 1;
    logWrite(SENSOR, "PacketSensor::localAck() for sequence number %u",
             ack_for);

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

    logWrite(SENSOR, "PacketSensor::localAck() decided on size %u",
             ackedSize);
  }
}

bool PacketSensor::SentPacket::inSequenceBlock(unsigned int sequence)
{
  logWrite(SENSOR,
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
    logWrite(SENSOR, "Yes!");
  }
  else
  {
    logWrite(SENSOR, "No!");
  }
  return result;
}

PacketSensor::SentPacket::SentPacket()
    : seqStart(0), seqEnd(0), totalLength(), timestamp() {
}
