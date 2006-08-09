// SensorList.cc

#include "lib.h"
#include "log.h"
#include "SensorList.h"
#include "Sensor.h"
#include "Command.h"

#include "PacketSensor.h"
#include "DelaySensor.h"
#include "MinDelaySensor.h"
#include "MaxDelaySensor.h"
#include "ThroughputSensor.h"

using namespace std;

SensorList::SensorList()
{
  reset();
}

SensorList::SensorList(SensorList const & right)
{
  if (right.head.get() != NULL)
  {
    logWrite(ERROR, "SensorList copy constructed after setup. "
             "This shouldn't have been able to happen. "
             "Sensors have been lost.");
  }
  reset();
}

SensorList & SensorList::operator=(SensorList const & right)
{
  if (this != &right)
  {
    if (right.head.get() != NULL)
    {
      logWrite(ERROR, "SensorList assigned after setup. "
               "This shouldn't have been able to happen. "
               "Sensors have been lost.");
    }
    reset();
  }
  return *this;
}

void SensorList::addSensor(SensorCommand const & newSensor)
{
  // Add dependency type demux here.
  switch(newSensor.type)
  {
  case NULL_SENSOR:
    pushNullSensor();
    break;
  case PACKET_SENSOR:
    pushPacketSensor();
    break;
  case DELAY_SENSOR:
    pushDelaySensor();
    break;
  case MIN_DELAY_SENSOR:
    pushMinDelaySensor();
    break;
  case MAX_DELAY_SENSOR:
    pushMaxDelaySensor();
    break;
  case THROUGHPUT_SENSOR:
    pushThroughputSensor();
    break;
  default:
    logWrite(ERROR,
             "Incorrect sensor type (%d). Ignoring add sensor command.",
             newSensor.type);
    break;
  }
}

Sensor * SensorList::getHead(void)
{
  return head.get();
}

void SensorList::reset(void)
{
  head.reset(NULL);
  tail = NULL;
  // Dependency = NULL here
  depNullSensor = NULL;
  depPacketSensor = NULL;
  depDelaySensor = NULL;
  depThroughputSensor = NULL;
}

void SensorList::pushSensor(std::auto_ptr<Sensor> newSensor)
{
  if (tail != NULL)
  {
    tail->addNode(newSensor);
  }
  else
  {
    head = newSensor;
    tail = head.get();
  }
}

// Add individual pushSensor functions here.

void SensorList::pushNullSensor(void)
{
  // Dependency list

  // Example dependency check
  if (depNullSensor == NULL)
  {
    NullSensor * newSensor = new NullSensor();
    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depNullSensor = newSensor;
  }
}

void SensorList::pushPacketSensor(void)
{
  // Dependency list

  // Example dependency check
  if (depPacketSensor == NULL)
  {
    PacketSensor * newSensor = new PacketSensor();
    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depPacketSensor = newSensor;
  }
}

void SensorList::pushDelaySensor(void)
{
  // Dependency list
  pushPacketSensor();

  // Example dependency check
  if (depDelaySensor == NULL)
  {
    DelaySensor * newSensor = new DelaySensor(depPacketSensor);
    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depDelaySensor = newSensor;
  }
}

void SensorList::pushMinDelaySensor(void)
{
  // Dependency list
  pushDelaySensor();

  std::auto_ptr<Sensor> current(new MinDelaySensor(depDelaySensor));
  pushSensor(current);
}

void SensorList::pushMaxDelaySensor(void)
{
  // Dependency list
  pushDelaySensor();

  std::auto_ptr<Sensor> current(new MaxDelaySensor(depDelaySensor));
  pushSensor(current);
}

void SensorList::pushThroughputSensor(void)
{
  // Dependency list
  pushPacketSensor();

  // Example dependency check
  if (depThroughputSensor == NULL)
  {
    ThroughputSensor * newSensor = new ThroughputSensor(depPacketSensor);
    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depThroughputSensor = newSensor;
  }
}
