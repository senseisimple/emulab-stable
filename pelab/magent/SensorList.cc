// SensorList.cc

#include "lib.h"
#include "log.h"
#include "SensorList.h"
#include "Sensor.h"
#include "Command.h"

#include "StateSensor.h"
#include "PacketSensor.h"
#include "DelaySensor.h"
#include "MinDelaySensor.h"
#include "MaxDelaySensor.h"
#include "ThroughputSensor.h"
#include "EwmaThroughputSensor.h"

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
  case STATE_SENSOR:
    pushStateSensor();
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
  case EWMA_THROUGHPUT_SENSOR:
    pushEwmaThroughputSensor();
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
  depStateSensor = NULL;
  depPacketSensor = NULL;
  depDelaySensor = NULL;
  depThroughputSensor = NULL;
}

void SensorList::pushSensor(std::auto_ptr<Sensor> newSensor)
{
  if (tail != NULL)
  {
    tail->addNode(newSensor);
    tail = tail->getTail();
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
    logWrite(SENSOR, "Adding NullSensor");
    NullSensor * newSensor = new NullSensor();
    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depNullSensor = newSensor;
  }
}

void SensorList::pushStateSensor(void)
{
  // Dependency list
  pushNullSensor();

  // Dependency check
  if (depStateSensor == NULL)
  {
    logWrite(SENSOR, "Adding StateSensor");
    StateSensor * newSensor = new StateSensor();
    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Dependency set
    depStateSensor = newSensor;
  }
}

void SensorList::pushPacketSensor(void)
{
  // Dependency list
  pushStateSensor();

  // Example dependency check
  if (depPacketSensor == NULL)
  {
    logWrite(SENSOR, "Adding PacketSensor");
    PacketSensor * newSensor = new PacketSensor(depStateSensor);
    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depPacketSensor = newSensor;
  }
}

void SensorList::pushDelaySensor(void)
{
  // Dependency list
  pushStateSensor();
  pushPacketSensor();

  // Example dependency check
  if (depDelaySensor == NULL)
  {
    logWrite(SENSOR, "Adding DelaySensor");
    DelaySensor * newSensor = new DelaySensor(depPacketSensor, depStateSensor);
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

  logWrite(SENSOR, "Adding MinDelaySensor");
  std::auto_ptr<Sensor> current(new MinDelaySensor(depDelaySensor));
  pushSensor(current);
}

void SensorList::pushMaxDelaySensor(void)
{
  // Dependency list
  pushDelaySensor();
  pushStateSensor();

  logWrite(SENSOR, "Adding MaxDelaySensor");
  std::auto_ptr<Sensor> current(new MaxDelaySensor(depDelaySensor,
                                                   depStateSensor));
  pushSensor(current);
}

void SensorList::pushThroughputSensor(void)
{
  // Dependency list
  pushStateSensor();
  pushPacketSensor();

  // Example dependency check
  if (depThroughputSensor == NULL)
  {
    logWrite(SENSOR, "Adding ThroughputSensor");
    ThroughputSensor * newSensor = new ThroughputSensor(depPacketSensor,
                                                        depStateSensor);
    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depThroughputSensor = newSensor;
  }
}

void SensorList::pushEwmaThroughputSensor(void)
{
  // Dependency list
  pushThroughputSensor();

  logWrite(SENSOR, "Adding EwmaThroughputSensor");
  std::auto_ptr<Sensor> current(new EwmaThroughputSensor(depThroughputSensor));
  pushSensor(current);
}
