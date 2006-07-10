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

#ifndef SENSOR_LIST_H_STUB_2
#define SENSOR_LIST_H_STUB_2

class Sensor;
class SensorCommand;

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
  // Functions for creating sensors and pushing them onto the list.
  void pushSensor(std::auto_ptr<Sensor> newSensor);
private:
  // Dependency pointers.
};

#endif
