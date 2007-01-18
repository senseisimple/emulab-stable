/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// SensorList.h

// This is the class for managing sensors and their dependencies. When
// adding a sensor, add a new entry in the 'addSensor()' switch table
// for the sensor type constant. Add a 'pushXXX()' function for the
// sensor. Call all the 'push' functions for any dependency before
// creating the new sensor. If there are new dependencies, add them to
// the list of dependency pointers, add the pointer copies to the
// assignment operator, nulls to the default constructor, and add a
// check to each associated push function to see if that pointer is
// non-NULL.

// ASSUMPTION: Functions called through the dependency declarations
// are queries only. They do not change the sensor
// involved. Therefore, declare them const and any reference to them
// const and any query-functions in them const.

#ifndef SENSOR_LIST_H_STUB_2
#define SENSOR_LIST_H_STUB_2

#include "Sensor.h"
#include "UdpPacketSensor.h"
#include "UdpThroughputSensor.h"
#include "UdpMinDelaySensor.h"
#include "UdpMaxDelaySensor.h"

class SensorCommand;

class NullSensor;
class StateSensor;
class PacketSensor;
class DelaySensor;
class MinDelaySensor;
class MaxDelaySensor;
class ThroughputSensor;
class TSThroughputSensor;
class EwmaThroughputSensor;
class LeastSquaresThroughput;


// Udp - CHANGES - Begin
class UdpPacketSensor;
class UdpThroughputSensor;
class UdpMaxDelaySensor;
class UdpMinDelaySensor;
// Udp - CHANGES - End

class SensorList
{
public:
  SensorList();
  SensorList(SensorList const & right);
  SensorList & operator=(SensorList const & right);

  void addSensor(SensorCommand const & newSensor);
  Sensor * getHead(void);
private:
  std::auto_ptr<Sensor> head;
  Sensor * tail;
private:
  void reset(void);
  void pushSensor(std::auto_ptr<Sensor> newSensor);
  // Functions for creating sensors and pushing them onto the list.
  void pushNullSensor(void);
  void pushStateSensor(void);
  void pushPacketSensor(void);
  void pushDelaySensor(void);
  void pushMinDelaySensor(void);
  void pushMaxDelaySensor(void);
  void pushThroughputSensor(void);
  void pushTSThroughputSensor(void);
  void pushEwmaThroughputSensor(void);
  void pushLeastSquaresThroughput(void);
  void pushAverageThroughputSensor(void);

  // Udp - CHANGES - Begin
  void pushUdpPacketSensor(void);
  void pushUdpThroughputSensor(void);
  void pushUdpMinDelaySensor(void);
  void pushUdpMaxDelaySensor(void);
  // Udp - CHANGES - End

private:
  // Example dependency
  NullSensor const * depNullSensor;
  // Dependency pointers.
  StateSensor const * depStateSensor;
  PacketSensor const * depPacketSensor;
  DelaySensor const * depDelaySensor;
  MinDelaySensor const * depMinDelaySensor;
  ThroughputSensor const * depThroughputSensor;
  TSThroughputSensor const * depTSThroughputSensor;

  // Udp - CHANGES - Begin
  UdpPacketSensor const * depUdpPacketSensor;
  UdpThroughputSensor const * depUdpThroughputSensor;
  UdpMinDelaySensor const * depUdpMinDelaySensor;
  UdpMaxDelaySensor const * depUdpMaxDelaySensor;
  // Udp - CHANGES - End

};

#endif
