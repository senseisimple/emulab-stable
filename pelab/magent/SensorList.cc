// SensorList.cc

#include "lib.h"
#include "log.h"
#include "SensorList.h"
#include "Sensor.h"
#include "Command.h"

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
    pushNullSensor(newSensor);
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

void reset(void)
{
  head.reset(NULL);
  tail = NULL;
  // Dependency = NULL here
  depNullSensor = NULL;
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

void SensorList::pushNullSensor(SensorCommand const &)
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
