// EwmaThroughputSensor.cc

#include "lib.h"
#include "EwmaThroughputSensor.h"
#include "ThroughputSensor.h"
#include "CommandOutput.h"

using namespace std;

EwmaThroughputSensor::EwmaThroughputSensor(
  ThroughputSensor * newThroughputSource)
  : throughput(0.0)
  , throughputSource(newThroughputSource)
{
}

void EwmaThroughputSensor::localSend(PacketInfo * packet)
{
}

void EwmaThroughputSensor::localAck(PacketInfo * packet)
{
  int latest = throughputSource->getThroughputInKbps();
  if (latest != 0)
  {
    if (throughput == 0.0)
    {
      throughput = latest;
    }
    else
    {
      static const double alpha = 0.1;
      throughput = throughput*(1.0-alpha) + latest*alpha;
    }
    ostringstream buffer;
    buffer << setiosflags(ios::fixed | ios::showpoint) << setprecision(0);
    buffer << "bandwidth=" << throughput;
    global::output->eventMessage(buffer.str(), packet->elab,
                                 CommandOutput::FORWARD_PATH);
  }
}
