// TSThroughputSensor.cc

#include "lib.h"
#include "TSThroughputSensor.h"
#include "PacketSensor.h"
#include "StateSensor.h"

using namespace std;

TSThroughputSensor::TSThroughputSensor(PacketSensor const * newPacketHistory,
                                   StateSensor const * newState)
  : lastAckTS(0), packetHistory(newPacketHistory), state(newState)
{
}

int TSThroughputSensor::getThroughputInKbps(void) const
{
  if (ackValid)
  {
    return throughputInKbps;
  }
  else
  {
    logWrite(ERROR,
             "ThroughputSensor::getThroughputInKbps() "
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
      state->getState() == StateSensor::ESTABLISHED)
  {
    /*
     * Find the time the other end of this connection says it sent this
     * ACK
     */
    uint32_t currentAckTS = 0;
    list<Option>::iterator opt;
    for (opt = packet->tcpOptions->begin();
         opt != packet->tcpOptions->end();
         ++opt) {
      if (opt->type == TCPOPT_TIMESTAMP) {
        const uint32_t *stamps = reinterpret_cast<const uint32_t*>(opt->buffer);
        /*
         * stamps[0] is TSval (the sending node's timestamp)
         * stamps[1] is TSecr (the timestamp the sending node is echoing)
         */
        currentAckTS = htonl(stamps[0]);
      }
    }
    
    /*
     * It would be nice if we could fall back to regular timing instead of
     * bailing, maybe that's a feature for someday...
     */
    if (currentAckTS == 0) {
      logWrite(ERROR,"TSThroughputSensor::localAck() got a packet without a "
                     "timestamp");
      ackValid = false;
      throughputInKbps = 0;
      lastAckTS = 0;
      return;
    }

    if (lastAckTS != 0 && currentAckTS < lastAckTS) {
      logWrite(ERROR,"TSThroughputSensor::localAck() got timestamps in reverse "
                     "order: o=%u,n=%u",lastAckTS,currentAckTS);
      ackValid = false;
      throughputInKbps = 0;
      lastAckTS = 0;
      return;
    }

    if (lastAckTS != 0)
    {
      ackValid = true;
      /*
       * period is in arbitrary units decided on by the other end - we assume
       * they are in milliseconds XXX: Verify this
       */
      int period = currentAckTS - lastAckTS;
      double kilobits = packetHistory->getAckedSize() * (8.0/1000.0);
      int latest = static_cast<int>(kilobits/(period / 1000.0));
      throughputInKbps = latest;
      maxThroughput = latest;
      logWrite(SENSOR, "TSTHROUGHPUT: %d kbps (period=%i,kbits=%f)",
          latest,period,kilobits);
    }
    else
    {
      throughputInKbps = 0;
      ackValid = false;
    }
    lastAckTS = currentAckTS;
  }
  else
  {
    throughputInKbps = 0;
    lastAckTS = 0;
    ackValid = false;
  }
}
