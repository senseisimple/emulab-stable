// PacketSensor.cc

#include "lib.h"
#include "PacketSensor.h"

using namespace std;

PacketSensor::PacketSensor()
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
  unsigned int startSequence = ntohl(packet->tcp->seq);
  if (globalSequence.inSequenceBlock(startSequence))
  {
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
  }
  else
  {
    SentPacket record;
    record.seqStart = ntohl(startSequence);
    unsigned int sequenceLength = packet->packetLength -
      sizeof(struct ether_header) - IP_HL(packet->ip)*4 -
      sizeof(struct tcphdr);
    record.seqEnd = record.seqStart + sequenceLength;
    record.totalLength = packet->packetLength;
    record.timestamp = packet->packetTime;
    globalSequence.seqEnd = record.seqEnd;
    if (unacked.empty())
    {
      globalSequence.seqStart = record.seqStart;
      globalSequence.seqEnd = record.seqEnd;
    }
    unacked.push_back(record);
  }

  ackedSize = 0;
  ackedSendTime = Time();
}

void PacketSensor::localAck(PacketInfo * packet)
{
  list<SentPacket>::iterator pos = unacked.begin();
  list<SentPacket>::iterator limit = unacked.end();
  bool found = false;
  ackedSize = 0;

  while (pos != limit && !found)
  {
    found = pos->inSequenceBlock(ntohl(packet->tcp->ack_seq));
    ackedSize += pos->totalLength;
    if (found)
    {
      ackedSendTime = pos->timestamp;
    }
    else
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
}

bool PacketSensor::SentPacket::inSequenceBlock(unsigned int sequence)
{
  if (seqStart < seqEnd)
  {
    return sequence >= seqStart && sequence < seqEnd;
  }
  else if (seqStart > seqEnd)
  {
    return sequence >= seqStart || sequence < seqEnd;
  }
  else
  {
    return false;
  }
}
