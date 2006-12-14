/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
  currentRegionState = INVALID_REGION;
  lastAckTime = Time();
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

PacketSensor::RegionState PacketSensor::getRegionState(void) const
{
  if (ackValid || sendValid)
  {
    return currentRegionState;
  }
  else
  {
    logWrite(ERROR,
             "PacketSensor::getRegionState() called "
             "with invalid ack or send data");
    return INVALID_REGION;
  }
}

void PacketSensor::localSend(PacketInfo * packet)
{
  ackValid = false;
  if (currentRegionState == BEGIN_VALID_REGION)
  {
    currentRegionState = VALID_REGION;
  }
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
  /*
   * Bail out now if there is no data in this packet
   */
  if (sequenceLength == 0) {
    ackValid = false;
    sendValid = false;
    logWrite(SENSOR_DETAIL,"PacketSensor::localSend() skipping a non-data packet");
    return;
  }

  // Assume this packet is not a retransmit unless proven otherwise
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

      // We want to get the sequence number of the last data byte, not the
      // sequence number of the first byte of the next segment
      record.seqEnd = record.seqStart + sequenceLength - 1;
      record.totalLength = packet->packetLength;
      record.timestamp = packet->packetTime;
      logWrite(SENSOR_DETAIL,
               "PacketSensor::localSend() new record: ss=%u,sl=%u,se=%u,tl=%u",
               record.seqStart, sequenceLength, record.seqEnd,
               record.totalLength);
      if (unacked.empty())
      {
        globalSequence.seqStart = record.seqStart;
        globalSequence.seqEnd = record.seqEnd;
        if (packet->packetTime > lastAckTime)
        {
          // Only invalidate if this packet happened after the last
          // ack packet. This is necessary because of the re-ordering
          // problem.
          currentRegionState = INVALID_REGION;
        }
      }
      else
      {
        /*
         * Sanity check - the new packet we're adding should start where the
         * last one left off
         */
        if (record.seqStart != (globalSequence.seqEnd + 1))
        {
          LOG_TYPE type;
          if (record.seqStart - globalSequence.seqEnd > 15000) {
            type = ERROR;
          } else {
            type = EXCEPTION;
          }
          logWrite(type,"PacketSensor::localSend() may have missed a "
                  "packet - last seq seen: %u, new seq: %u (lost %d)",
                  globalSequence.seqEnd,record.seqStart,
                  record.seqStart - globalSequence.seqEnd);
          /*
           * Put a dummy packet into the record. We multiply the data size by a
           * constant factor that takes into account the header size for typical
           * packets.
           */

          /* The following is false. This is more common than
           * expected, and can lead to RTT estimates that are much
           * lower than reality. Therefore, set the time to Time() and
           * then avoid such packets when determining the latest time
           * (and if there are only such packets being acked, then set
           * ackValid to false. */

          /* We put the current packet's timestamp on the fake packet.
           * This is pretty safe - the worst it can do is slightly
           * underestimate RTT - but, since the current packet is definitely
           * newer, our RTT estimates will almost always use its timestamp. The
           * only case that we'll underestimate is when some ACK only ACKs fake
           * packets, which is hopefully rare, and the error should be small
           */
          SentPacket fake;
          fake.seqStart = globalSequence.seqEnd + 1;
          fake.seqEnd = record.seqStart - 1;
          fake.totalLength = static_cast<int>((fake.seqEnd - fake.seqStart)
              * SUPERMAGIC_PACKET_MULTIPLIER);
          fake.fake = true;
          fake.timestamp = Time(); //packet->packetTime;
          unacked.push_back(fake);

          /*
           * Calculate the packet payload size - we have to make sure to
           * take into account IP and TCP option headers
           */
          logWrite(SENSOR_COMPLETE,
                   "PacketSensor::localSend() fake record: ss=%u,sl=%u,se=%u,tl=%u",
                   fake.seqStart, (fake.seqEnd - fake.seqStart), fake.seqEnd,
                   fake.totalLength);

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
  lastAckTime = packet->packetTime;
  if (currentRegionState == BEGIN_VALID_REGION)
  {
    currentRegionState = VALID_REGION;
  }
  // Right now, we don't know whether or not this ACK is for a retransmitted
  // packet - we will find out later
  isRetransmit = false;

  // Ignore SYN
  if (packet->tcp->syn) {
      logWrite(SENSOR_DETAIL, "PacketSensor::localAck() Skipping SYN");
      ackValid = false;
      return;
  }

  // Ignore FIN
  if (packet->tcp->fin) {
      logWrite(SENSOR_DETAIL, "PacketSensor::localAck() Skipping FIN");
      ackValid = false;
      return;
  }

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
    logWrite(SENSOR_COMPLETE, "PacketSensor::localAck() globalSequnce is %u "
            "to %u", globalSequence.seqStart, globalSequence.seqEnd);
    /*
     * We assume that the ACK field of the packet is acking from the smallest
     * sequence number we know of up to it value
     * XXX: When the ACK is for a sequence number smaller than any we know of,
     * we could assume it is not acking any new packets - but, this requires
     * some more thought about wraparaound, so we'll live with spurious errors
     * for now.
     */
    if ((globalSequence.seqStart == 0) && (globalSequence.seqEnd == 0)) {
      /*
       * In this case, we know of no outstanding packets
       */
      logWrite(EXCEPTION, "PacketSensor::localAck(): Got an ACK, but I don't "
                          "know of any outstanding packets");
    } else if (ack_for == (globalSequence.seqStart - 1)) {
      logWrite(SENSOR, "PacketSensor::localAck() detected a "
                       "duplicate ack for %u",ack_for);
    } else if (ack_for < globalSequence.seqStart) {
      logWrite(EXCEPTION, "PacketSensor::localAck(): Got an ACK below the "
                          "globalRange (%u < %u)",ack_for,
                          globalSequence.seqStart);
    } else {
        /*
         * Okay, this looks like a legit ACK!
         */
        ranges.push_back(Range(globalSequence.seqStart,ack_for,false));
    }

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
        logWrite(SENSOR_DETAIL,"SACK: length = %d num_sacks = %d", opt->length,
                 num_sacks);
        if (opt->length % 8 != 0) {
          logWrite(ERROR,"Bad SACK header length: %i", (opt->length));
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
            ranges.push_back(Range(start, end, true));
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
    bool isRegionValid = true;
    rangelist::iterator range;
    for (range = ranges.begin(); range != ranges.end(); range++) {
      uint32_t range_start = range->start;
      uint32_t range_end = range->end;

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
          logWrite(EXCEPTION, "PacketSensor::localAck() detected an "
                              "old ack");
          continue;
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
        isRegionValid = false;
        ackValid = false;
        break;
      }

      bool split = false;
      if (firstPacket->seqStart != range_start) {
        /*
         * We have to split up this SentPacket so that we can ACK only part of
         * it
         */
        logWrite(SENSOR_DETAIL, "PacketSensor::localAck(): Range does not "
            "start on packet boundary: %u (%u to %u, tl %u)",
            range_start, firstPacket->seqStart, firstPacket->seqEnd,
            firstPacket->totalLength);
        SentPacket::splitPacket(firstPacket, range_start, &unacked);
        split = true;
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

      if (lastPacket->seqEnd != range_end) {
        /*
         * We have to split up this SentPacket so that we can ACK only part of
         * it
         */
        logWrite(SENSOR_DETAIL, "PacketSensor::localAck(): Range does not "
            "end on packet boundary: %u (%u to %u, tl %u)",
            range_end, lastPacket->seqStart, lastPacket->seqEnd,
            lastPacket->totalLength);
        SentPacket::splitPacket(lastPacket, range_end +1, &unacked);
        split = true;
      }

      if (split) {
        /*
         * Wow, the STL list is *lame*. Insertion invalidates the
         * iterators (WHY?), so we have to find the packets again
         */
        firstPacket = unacked.begin();
        while (firstPacket != unacked.end() &&
               !firstPacket->inSequenceBlock(range_start)) {
          firstPacket++;
        }
        if (firstPacket == unacked.end() ||
            firstPacket->seqStart != range_start) {
          logWrite(ERROR,"PacketSensor::localack(): Internal error - starting "
              "packet not found");
          ackValid = false;
          continue;
        }
        lastPacket = unacked.begin();
        while (lastPacket != unacked.end() &&
               !lastPacket->inSequenceBlock(range_end)) {
          lastPacket++;
        }
        if (lastPacket == unacked.end() ||
            lastPacket->seqEnd != range_end) {
          logWrite(ERROR,"PacketSensor::localack(): Internal error - ending "
              "packet not found");
          ackValid = false;
          continue;
        }
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

        /*
         * Don't count up bytes that have already been SACKed
         */
        if (!pos->sacked) {
            ackedSize += pos->totalLength;
        }

        /*
         * If this range is a SACK, mark the packet as being SACKed so that we
         * won't count it again.
         */
        if (range->from_sack) {
            pos->sacked = true;
        }

        /*
         * We want the time for the most recently acked packet, not the one
         * that has the highest sequence number.
         */
        if (pos->timestamp > ackedSendTime) {
          ackedSendTime = pos->timestamp;
        }

        /*
         * If any of the packets are fake, that means that there have
         * been some packets dropped by libpcap. This means, that we
         * know that it is invalid.
         */
        if (pos->fake)
        {
          isRegionValid = false;
        }

        if (pos == lastPacket) {
          break;
        } else {
          ++pos;
        }
      }

      /*
       * Now, remove these packets from our list, but not if this was a SACK,
       * since we are likely to see future ACKs or SACKs for the same packet
       */
      if (!range->from_sack) {
        unacked.erase(firstPacket, lastPacket);
        // erase() does not erase the final member of the range, so we have
        // to do that ourselves
        unacked.erase(lastPacket);
      }

      if (unacked.empty())
      {
        globalSequence.seqStart = 0;
        globalSequence.seqEnd = 0;
        // We don't want to start in INVALID_REGION yet. Since some
        // packets are re-ordered, we may have gotten all available
        // acks, but not the sends which were actually earlier.
      }
      else
      {
        globalSequence.seqStart = unacked.front().seqStart;
        globalSequence.seqEnd = unacked.back().seqEnd;
      }
    }

    if (isRegionValid)
    {
      if (currentRegionState == INVALID_REGION)
      {
        currentRegionState = BEGIN_VALID_REGION;
      }
      else
      {
        currentRegionState = VALID_REGION;
      }
    }
    else
    {
      currentRegionState = INVALID_REGION;
    }

    if (ackedSendTime == Time())
    {
      logWrite(EXCEPTION, "PacketSensor::localAck() received an ack for only "
               "artificial packets: declaring ACK invalid");
      ackValid = false;
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
  logWrite(SENSOR, "REGION: %d", currentRegionState);
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
    : seqStart(0), seqEnd(0), totalLength(), timestamp(), retransmitted(false),
      sacked(0), fake(false) {
}

void PacketSensor::SentPacket::splitPacket(PacketSensor::SentPacketIterator it,
      unsigned int split_seqnum, std::list<SentPacket> *unacked) {

  // Save these away so that we can use them in the message below
  unsigned int old_start = it->seqStart;
  unsigned int old_end = it->seqEnd;
  unsigned int old_length = it->seqEnd - it->seqStart;
  unsigned int old_tl = it->totalLength;

  SentPacket newpacket = *it;
  newpacket.seqEnd = split_seqnum - 1;
  it->seqStart = split_seqnum;
  /*
   * Figure out what percent of the totalLength each part accounted for so that
   * we can split it fairly.
   */
  double firstpct =
                  (static_cast<double>(newpacket.seqEnd) -
                   static_cast<double>(newpacket.seqStart)) /
                  static_cast<double>(old_length);
  double secondpct =
                   (static_cast<double>(it->seqEnd) -
                    static_cast<double>(it->seqStart)) /
                   static_cast<double>(old_length);
  newpacket.totalLength = static_cast<int>(firstpct * old_tl);
  it->totalLength = static_cast<int>(secondpct * old_tl);
  // The new packet goes before the old packet
  PacketSensor::SentPacketIterator newit;
  newit = unacked->insert(it,newpacket);
  logWrite(SENSOR_DETAIL, "PacketSensor::splitPacket(): Split %u to %u "
      "(tl %i) into %u to %u (tl %i) and %u to %u (tl %i)",
      old_start, old_end, old_tl, newit->seqStart, newit->seqEnd,
      newit->totalLength, it->seqStart, it->seqEnd, it->totalLength);
}

PacketSensor::Range::Range(unsigned int _start, unsigned int _end, bool _sack) {
    start = _start;
    end = _end;
    from_sack = _sack;
}
