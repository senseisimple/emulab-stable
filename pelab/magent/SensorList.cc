/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
#include "TSThroughputSensor.h"
#include "EwmaThroughputSensor.h"
#include "LeastSquaresThroughput.h"

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
  case TSTHROUGHPUT_SENSOR:
    pushTSThroughputSensor();
    break;
  case EWMA_THROUGHPUT_SENSOR:
    pushEwmaThroughputSensor();
    break;
  case LEAST_SQUARES_THROUGHPUT:
    pushLeastSquaresThroughput();
    break;
  case AVERAGE_THROUGHPUT_SENSOR:
    pushAverageThroughputSensor();
    break;
  case UDP_PACKET_SENSOR:
    pushUdpPacketSensor();
    break;
  case UDP_THROUGHPUT_SENSOR:
    pushUdpThroughputSensor();
    break;
  case UDP_MINDELAY_SENSOR:
    pushUdpMinDelaySensor();
    break;
  case UDP_MAXDELAY_SENSOR:
    pushUdpMaxDelaySensor();
    break;
  case UDP_RTT_SENSOR:
    pushUdpRttSensor();
    break;
  case UDP_LOSS_SENSOR:
    pushUdpLossSensor();
    break;
  case UDP_AVG_THROUGHPUT_SENSOR:
    pushUdpAvgThroughputSensor();
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
  depTSThroughputSensor = NULL;
  depMinDelaySensor = NULL;
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

  if (depMinDelaySensor == NULL) {
      logWrite(SENSOR, "Adding MinDelaySensor");
      MinDelaySensor * newSensor = new MinDelaySensor(depDelaySensor);
      std::auto_ptr<Sensor> current(newSensor);
      pushSensor(current);

      // Dependency set
      depMinDelaySensor = newSensor;
  }

}

void SensorList::pushMaxDelaySensor(void)
{
  // Dependency list
  pushMinDelaySensor();
  pushDelaySensor();
  pushStateSensor();
  pushPacketSensor();

  logWrite(SENSOR, "Adding MaxDelaySensor");
  std::auto_ptr<Sensor> current(new MaxDelaySensor(depDelaySensor,
                                                   depStateSensor,
                                                   depMinDelaySensor,
                                                   depPacketSensor));
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

void SensorList::pushTSThroughputSensor(void)
{
  // Dependency list
  pushStateSensor();
  pushPacketSensor();

  // Example dependency check
  if (depTSThroughputSensor == NULL)
  {
    logWrite(SENSOR, "Adding TSThroughputSensor");
    TSThroughputSensor * newSensor = new TSThroughputSensor(depPacketSensor,
                                                            depStateSensor);
    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depTSThroughputSensor = newSensor;
  }
}

void SensorList::pushEwmaThroughputSensor(void)
{
  // Dependency list
  pushStateSensor();
  pushThroughputSensor();

  logWrite(SENSOR, "Adding EwmaThroughputSensor");
  std::auto_ptr<Sensor> current(new EwmaThroughputSensor(depThroughputSensor,
                                                         depStateSensor));
  pushSensor(current);
}

void SensorList::pushLeastSquaresThroughput(void)
{
  // Dependency list
  pushTSThroughputSensor();
  pushDelaySensor();

  logWrite(SENSOR, "Adding LeastSquaresThroughput");
  std::auto_ptr<Sensor> current(new LeastSquaresThroughput(depTSThroughputSensor,
                                                           depDelaySensor));
  pushSensor(current);
}

void SensorList::pushAverageThroughputSensor(void)
{
  // Dependency list
  pushTSThroughputSensor();
  pushDelaySensor();

  logWrite(SENSOR, "Adding AverageThroughputSensor");
  std::auto_ptr<Sensor> current(
    new LeastSquaresThroughput(depTSThroughputSensor, depDelaySensor,
                               LeastSquaresThroughput::DEFAULT_MAX_PERIOD));
  pushSensor(current);
}

void SensorList::pushUdpPacketSensor()
{
  pushNullSensor();

  // Example dependency check
  if (depUdpPacketSensor == NULL)
  {
    logWrite(SENSOR, "Adding UdpPacketSensor");
    UdpPacketSensor * newSensor = new UdpPacketSensor();

    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depUdpPacketSensor = newSensor;
  }

}
void SensorList::pushUdpThroughputSensor()
{
  pushUdpPacketSensor();

  // Example dependency check
  if (depUdpThroughputSensor == NULL)
  {
    logWrite(SENSOR, "Adding UdpThroughputSensor");
    UdpThroughputSensor * newSensor = new UdpThroughputSensor(depUdpPacketSensor);

    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depUdpThroughputSensor = newSensor;
  }

}
void SensorList::pushUdpMinDelaySensor()
{
  pushUdpPacketSensor();

  // Example dependency check
  if (depUdpMinDelaySensor == NULL)
  {
    logWrite(SENSOR, "Adding UdpMinDelaySensor");
    UdpMinDelaySensor * newSensor = new UdpMinDelaySensor(depUdpPacketSensor);

    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depUdpMinDelaySensor = newSensor;
  }

}
void SensorList::pushUdpMaxDelaySensor()
{
  pushUdpPacketSensor();
  pushUdpMinDelaySensor();

  // Example dependency check
  if (depUdpMaxDelaySensor == NULL)
  {
    logWrite(SENSOR, "Adding UdpMaxDelaySensor");
    UdpMaxDelaySensor * newSensor = new UdpMaxDelaySensor(depUdpPacketSensor, depUdpMinDelaySensor);

    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depUdpMaxDelaySensor = newSensor;
  }

}

void SensorList::pushUdpRttSensor()
{
  pushUdpPacketSensor();

  // Example dependency check
  if (depUdpRttSensor == NULL)
  {
    logWrite(SENSOR, "Adding UdpRttSensor");
    UdpRttSensor * newSensor = new UdpRttSensor(depUdpPacketSensor);

    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depUdpRttSensor = newSensor;
  }

}

void SensorList::pushUdpLossSensor()
{
  pushUdpPacketSensor();
  pushUdpRttSensor();

  // Example dependency check
  if (depUdpLossSensor == NULL)
  {
    logWrite(SENSOR, "Adding UdpLossSensor");
    UdpLossSensor * newSensor = new UdpLossSensor(depUdpPacketSensor, depUdpRttSensor);

    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depUdpLossSensor = newSensor;
  }

}

void SensorList::pushUdpAvgThroughputSensor()
{
  pushUdpPacketSensor();
  pushUdpLossSensor();

  // Example dependency check
  if (depUdpAvgThroughputSensor == NULL)
  {
    logWrite(SENSOR, "Adding UdpAvgThroughputSensor");
    UdpAvgThroughputSensor * newSensor = new UdpAvgThroughputSensor(depUdpPacketSensor, depUdpLossSensor);

    std::auto_ptr<Sensor> current(newSensor);
    pushSensor(current);

    // Example dependency set
    depUdpAvgThroughputSensor = newSensor;
  }

}
