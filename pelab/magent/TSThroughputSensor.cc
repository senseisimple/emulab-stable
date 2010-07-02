/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// TSThroughputSensor.cc

#include "lib.h"
#include "TSThroughputSensor.h"
#include "PacketSensor.h"
#include "StateSensor.h"

using namespace std;

TSThroughputSensor::TSThroughputSensor(PacketSensor const * newPacketHistory,
                                   StateSensor const * newState)
  : lastAckTS(0), packetHistory(newPacketHistory), state(newState),
    deferredBytes(0)
{
}

int TSThroughputSensor::getThroughputInKbps(void) const
{
  if (ackValid)
  {
    return getThroughputInKbps(getLastPeriod(), getLastByteCount());
  }
  else
  {
    logWrite(ERROR,
             "TSThroughputSensor::getThroughputInKbps() "
             "called with invalid data");
    return 0;
  }
}

int TSThroughputSensor::getThroughputInKbps(uint32_t period,
                                            int byteCount)
{
  double kilobits = byteCount * (8.0/1000.0);
  int result = 0;
  if (period != 0)
  {
    result = static_cast<int>(kilobits/(period / 1000.0));
  }
  else
  {
    logWrite(ERROR,
             "TSThroughputSensor::getThroughputInKbps(period, byteCount) "
             "called with a '0' period.");
  }
  return result;
}

uint32_t TSThroughputSensor::getLastPeriod(void) const
{
  if (ackValid)
  {
    return lastPeriod;
  }
  else
  {
    logWrite(ERROR,
             "TSThroughputSensor::getLastPeriod() called with invalid data");
    return 0;
  }
}

int TSThroughputSensor::getLastByteCount(void) const
{
  if (ackValid)
  {
    return lastByteCount;
  }
  else
  {
    logWrite(ERROR,
             "TSThroughputSensor::getLastByteCount() "
             "called with invalid data");
    return 0;
  }
}


void TSThroughputSensor::localSend(PacketInfo *)
{
  ackValid = false;
  sendValid = true;
}

void TSThroughputSensor::localAck(PacketInfo * packet)
{
  sendValid = false;
  if (state->isAckValid() && packetHistory->isAckValid() &&
      state->getState() == StateSensor::ESTABLISHED
      && (packetHistory->getRegionState() == PacketSensor::VALID_REGION
          || packetHistory->getRegionState()
                  == PacketSensor::BEGIN_VALID_REGION))
  {
    if (packetHistory->getRegionState() == PacketSensor::BEGIN_VALID_REGION)
    {
      lastAckTS = 0;
    }
    /*
     * Find the time the other end of this connection says it sent this
     * ACK
     */
    uint32_t currentAckTS = findTcpTimestamp(packet);

    /*
     * It would be nice if we could fall back to regular timing instead of
     * bailing, maybe that's a feature for someday...
     */
    if (currentAckTS == 0) {
      logWrite(ERROR,"TSThroughputSensor::localAck() got a packet without a "
               "timestamp");
      ackValid = false;
      lastPeriod = 1;
      lastByteCount = 0;
      lastAckTS = 0;
      return;
    }

    if (lastAckTS != 0 && currentAckTS < lastAckTS) {
      logWrite(ERROR,"TSThroughputSensor::localAck() got timestamps in reverse "
               "order: o=%u,n=%u",lastAckTS,currentAckTS);
      ackValid = false;
      lastPeriod = 1;
      lastByteCount = 0;
      lastAckTS = 0;
      return;
    }

    if (lastAckTS != 0)
    {
      uint32_t period = currentAckTS - lastAckTS;
      if (period == 0) {
        /*
         * If the period is 0, then the two packets arrived in the same
         * millisecond. If this is the case, we'll just take the bytes acked
         * here and count them as being acked by the next packet, and we'll
         * keep doing this until we get a non-zero period. It would probably be
         * better to count the throughput for these bytes with the previous
         * packet, but that would be a problem since we'd have to delay
         * reporting for every packet until we saw a subsequent non-zero
         * period.
         */
        ackValid = false;
        deferredBytes += packetHistory->getAckedSize();
        logWrite(SENSOR, "TSThroughputSensor::localAck() deferring %i bytes "
                 "due to zero period (%i total)",
                 packetHistory->getAckedSize(), deferredBytes);
      } else {
        ackValid = true;
        /*
         * period is in arbitrary units decided on by the other end - we assume
         * they are in milliseconds
         * XXX: This is a very fragile assumption - it works with Linux on x86,
         * but is not likely to work under other circumstances
         */
        lastPeriod = period;
        lastByteCount = packetHistory->getAckedSize() + deferredBytes;
        logWrite(SENSOR, "TSTHROUGHPUT: %d kbps (period=%i,kbits=%f,deferred=%f)",
                 getThroughputInKbps(), lastPeriod, lastByteCount*(8.0/1000.0),
                 deferredBytes*(8.0/1000));
        deferredBytes = 0;
      }
    }
    else
    {
      lastPeriod = 1;
      lastByteCount = 0;
      ackValid = false;
    }
    lastAckTS = currentAckTS;
  }
  else
  {
    lastPeriod = 1;
    lastByteCount = 0;
    lastAckTS = 0;
    ackValid = false;
  }
}

uint32_t findTcpTimestamp(PacketInfo * packet)
{
  uint32_t result = 0;
  bool done = false;
  list<Option>::iterator opt = packet->tcpOptions->begin();
  list<Option>::iterator limit = packet->tcpOptions->end();
  for (; opt != limit && !done; ++opt)
  {
    if (opt->type == TCPOPT_TIMESTAMP)
    {
      const uint32_t *stamps = reinterpret_cast<const uint32_t*>(opt->buffer);
      /*
       * stamps[0] is TSval (the sending node's timestamp)
       * stamps[1] is TSecr (the timestamp the sending node is echoing)
       */
      result = ntohl(stamps[0]);
      done = true;
    }
  }
  return result;
}
