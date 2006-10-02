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
  if (ackValid)
  {
    return ackedSize;
  }
  else
  {
    logWrite(ERROR,
             "PacketSensor::getAckedSize() called with invalid ack data");
    return 0;
  }
}

bool PacketSensor::getIsRetransmit(void) const
{
  if (sendValid || ackValid)
  {
    return isRetransmit;
  }
  else
  {
    logWrite(ERROR,
             "PacketSensor::getIsRetransmit() called with invalid send or ack "
             "data");
    return false;
  }
}

Time const & PacketSensor::getAckedSendTime(void) const
{
  if (ackValid)
  {
    return ackedSendTime;
  }
  else
  {
    logWrite(ERROR,
             "PacketSensor::getAckedSendTime() called with invalid ack data");
    static Time invalidTime = Time();
    return invalidTime;
  }
}

void PacketSensor::localSend(PacketInfo * packet)
{
  /*
   * Check for window scaling, which is not supported yet by this code. This
   * option is only legal on SACK packets. If we decide to support window
   * scaling in the future, it might be better to move it to some other sensor
   */
  if (packet->tcp->syn) {
    list<Option>::iterator opt;
    for (opt = packet->tcpOptions->begin();
         opt != packet->tcpOptions->end();
         ++opt) {
      if (opt->type == TCPOPT_WINDOW) {
        logWrite(ERROR,"TCP window scaling in use - not supported!");
      }
    }
  }
  // Assume this packet is not a retransmit unless proven otherwise
  ackValid = true;
  isRetransmit = false;
  if (state->isSendValid() && state->getState() == StateSensor::ESTABLISHED)
  {
    // Set it to true, and then set it to false if we encounter an error.
    sendValid = true;
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
          pos->retransmitted = true;
          done = true;
        }
      }

      if (!done) {
        logWrite(ERROR,
                 "localSend() unable to find packet record to update.");
        sendValid = false;
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
       * Calculate the packet payload size - we have to make sure to
       * take into account IP and TCP option headers
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
          logWrite(ERROR,"PacketSensor::localSend() may have missed a "
                  "packet - last seq seen: %u, new seq: %u (lost %d)",
                  globalSequence.seqEnd,record.seqStart,
                  record.seqStart - globalSequence.seqEnd);
        } else {
          logWrite(SENSOR_COMPLETE,"PacketSensor::localSend() looks good: %u %u",
                  record.seqStart, globalSequence.seqEnd);
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
    sendValid = false;
  }
  else
  {
    sendValid = false;
  }
}

void PacketSensor::localAck(PacketInfo * packet)
{
  sendValid = false;
  // Right now, we don't know whether or not this ACK is for a retransmitted
  // packet - we will find out later
  isRetransmit = false;
  if (state->isAckValid() && state->getState() == StateSensor::ESTABLISHED)
  {
    // Set it to true, and then set it to false if we encounter an error.
    ackValid = true;

    /*
     * Thanks to SACKs, we might have to worry about more than one range of
     * ACKed packets.
     */
    rangelist ranges;

    /*
     * When we get an ACK, the sequence number is really the next one the peer
     * excects to see: thus, the last sequence number it's ACKing is one less
     * than this.
     */
    uint32_t ack_for = ntohl(packet->tcp->ack_seq) - 1;
    logWrite(SENSOR_DETAIL, "PacketSensor::localAck() for sequence number %u",
             ack_for);
    /*
     * We assume that the ACK field of the packet is acking from the smallest
     * sequence number we know of up to it value
     * XXX: When the ACK is for a sequence number smaller than any we know of,
     * we could assume it is not acking any new packets - but, this requires
     * some more thought about wraparaound, so we'll live with spurious errors
     * for now.
     */
    ranges.push_back(rangepair(globalSequence.seqStart,ack_for));

    /*
     * Now look for SACKs
     */
    list<Option>::iterator opt;
    bool hasSack = false;
    for (opt = packet->tcpOptions->begin();
         opt != packet->tcpOptions->end();
         ++opt) {
      if (opt->type == TCPOPT_SACK) {
        /*
         * Figure out how many regions there are in this option header. There
         * are are two 4-octet sequence numbers for each region
         */
        int num_sacks = opt->length / 8;
        logWrite(SENSOR,"SACK: length = %d num_sacks = %d", opt->length,
                 num_sacks);
        if (opt->length % 8 != 0) {
          logWrite(SENSOR,"Bad SACK header length: %i", (opt->length));
          return;
        }
        const uint32_t *regions = reinterpret_cast<const uint32_t*>(opt->buffer);
        for (int i = 0; i < num_sacks; i++) {
          uint32_t start = ntohl(regions[i*2]);
          /*
           * Like a reguar ACK, the 'end' is the first sequence number *after*
           * the range
           */
          uint32_t end = ntohl(regions[i*2 + 1]) - 1;

          logWrite(SENSOR_DETAIL,"PacketSensor::localAck() found SACK "
                                 "range %u to %u", start, end);

          /*
           * This check is for something I have, believe it or not, actually
           * seen - the SACK is for a range also being ACKed by the packet's
           * ACK field. It looks like the Linux stack can do this after a
           * retransmitted packet (possibly to tell the sender it has recieved
           * two copies?)
           * Anyway, I *think* we don't want to double count these bytes.
           */
          if (end <= ack_for) {
            logWrite(SENSOR_DETAIL,"PacketSensor::localAck() suppressed "
                                   "redundant SACK");
          } else {
            ranges.push_back(rangepair(start, end));
          }
        }

        hasSack = true;
      }
    }

    /*
     * Now, take evry range we know about and handle it
     * XXX: Needs some though wrt wraparound
     */
    ackedSize = 0;
    ackedSendTime = Time();
    rangelist::iterator range;
    for (range = ranges.begin(); range != ranges.end(); range++) {
      uint32_t range_start = range->first;
      uint32_t range_end = range->second;

      logWrite(SENSOR_DETAIL, "PacketSensor::localAck() handling range "
                              "%u to %u", range_start, range_end);

      /*
       * Check to see if the ack is for a seq number smaller than the global
       * sequence - this could mean two things: (1) it's a duplicate ack (in
       * which case it means packet loss or packet re-ordering) (2) we got two
       * (or more) acks out of order
       * No action taken yet other than logging
       * XXX: Needs support for wraparound!
       */
      if (range_end < globalSequence.seqStart)
      {
        if (range_end == (globalSequence.seqStart - 1)) {
          logWrite(SENSOR_DETAIL, "PacketSensor::localAck() detected a "
                                  "duplicate ack");
          continue;
        }
        else
        {
            logWrite(SENSOR_DETAIL, "PacketSensor::localAck() detected an "
                                    "old ack");
            continue;
        }
      }

      /*
       * Now, we find the packets which contain the start and the end of the
       * region being acked. For now, if one of these packets is missing, we
       * just indicate that our value is invalid. In the future, we could try
       * something fancier, like guess the length of the missing packets (from
       * the sequence number).
       * XXX: Again, needs support for wraparound!
       */
      list<SentPacket>::iterator firstPacket = unacked.begin();
      while (firstPacket != unacked.end() &&
             !firstPacket->inSequenceBlock(range_start)) {
        firstPacket++;
      }

      if (firstPacket == unacked.end()) {
        logWrite(ERROR, "Range starts in unknown packet");
        ackValid = false;
        break;
      }

      list<SentPacket>::iterator lastPacket = unacked.begin();
      while (lastPacket != unacked.end() && 
             !lastPacket->inSequenceBlock(range_end)) {
        lastPacket++;
      }

      if (lastPacket == unacked.end()) {
        logWrite(ERROR, "Range ends in unknown packet");
        ackValid = false;
        break;
      }

      /*
       * Now, iterate between the two packets, looking at timestamps and adding
       * up sizes
       */
      list<SentPacket>::iterator pos = firstPacket;
      while (1)
      {
        /*
         * If any packets were retransmitted, then we consider the whole ACK to
         * be for a retransmitted packet. We might be able to make this more
         * fine grained.
         */
        if (pos->retransmitted) {
          isRetransmit = true;
        }
        ackedSize += pos->totalLength;
        /*
         * We want the time for the most recently acked packet, not the one
         * that has the highest sequence number
         */
        if (pos->timestamp > ackedSendTime) {
          ackedSendTime = pos->timestamp;
        }
        /*
         * XXX - should we be looking for packets in the range that we didn't
         * know about?
         */
        if (pos == lastPacket) {
          break;
        } else {
          ++pos;
        }
      }

      /*
       * Now, remove these packets from our list.
       * XXX: This makes the assumption that all acks are on packet boundaries
       * - this seems be be true in practice, but might not be true in theory
       * XXX: In the future, we may want to marked SACKed packets, not totally
       * remove them
       */
      unacked.erase(firstPacket, lastPacket);
      // erase() does not erase the final member of the range, so we have
      // to do that ourselves
      unacked.erase(lastPacket);

      if (unacked.empty())
      {
        globalSequence.seqStart = 0;
        globalSequence.seqEnd = 0;
      }
      else
      {
        globalSequence.seqStart = unacked.front().seqStart;
        globalSequence.seqEnd = unacked.back().seqEnd;
      }
    }

    logWrite(SENSOR_DETAIL,"PacketSensor::localAck() decided on size %i",
        ackedSize);
    if (ackedSize == 0) {
      logWrite(EXCEPTION,"PacketSensor::localAck() declaring ACK invalid");
      ackValid = false;
    }
  }
  else
  {
    ackValid = false;
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
    : seqStart(0), seqEnd(0), totalLength(), timestamp(), retransmitted(false) {
}
