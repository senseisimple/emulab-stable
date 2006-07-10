// Connection.cc

#include "lib.h"
#include "log.h"
#include "Connection.h"
#include "Time.h"
#include "ConnectionModel.h"
#include "TrafficModel.h"
#include "Sensor.h"

using namespace std;

Connection::Connection()
  : isConnected(false)
  , bufferFull(false)
{
}

Connection::Connection(Connection const & right)
  : peer(right.peer->clone())
  , traffic(right.traffic->clone())
  , measurements(right.measurements)
  , isConnected(right.isConnected)
  , bufferFull(right.bufferFull)
  , nextWrite(right.nextWrite)
{
}

Connection & Connection::operator=(Connection const & right)
{
  if (this != &right)
  {
    peer = right.peer->clone();
    traffic = right.traffic->clone();
    measurements = right.measurements;
    isConnected = right.isConnected;
    bufferFull = right.bufferFull;
    nextWrite = right.nextWrite;
  }
  return *this;
}

void Connection::reset(Order const & newElab,
                       std::auto_ptr<ConnectionModel> newPeer)
{
  elab = newElab;
  peer = newPeer;
}

void Connection::setTraffic(std::auto_ptr<TrafficModel> newTraffic)
{
  traffic = newTraffic;
}

void Connection::connect(void)
{
  if (peer.get() != NULL)
  {
    peer->connect();
  }
  else
  {
    logWrite(ERROR,
            "Connection model has not been initialized before a connect call");
  }
}

void Connection::addTrafficWrite(TrafficWriteCommand const & newWrite,
                                 multimap<Time, Connection *> & schedule)
{
  if (traffic.get() != NULL)
  {
    Time temp = traffic->addWrite(newWrite, nextWrite);
    if (temp != Time() && nextWrite == Time())
    {
      nextWrite = temp;
      schedule.insert(make_pair(nextWrite, this));
    }
  }
  else
  {
    logWrite(ERROR,
             "Traffic write was added before the traffic model was set");
  }
}

void Connection::addSensor(SensorCommand const & newSensor)
{
  measurements.addSensor(newSensor);
}

void Connection::captureSend(Time & packetTime, struct tcp_info * kernel,
                             struct tcphdr * tcp)
{
  Sensor * head = measurements.getHead();
  if (head != NULL && isConnected)
  {
    head->captureSend(packetTime, kernel, tcp, elab, bufferFull);
  }
}

void Connection::captureAck(Time & packetTime, struct tcp_info * kernel,
                            struct tcphdr * tcp)
{
  Sensor * head = measurements.getHead();
  if (head != NULL && isConnected)
  {
    head->captureAck(packetTime, kernel, tcp, elab, bufferFull);
  }
}

Time Connection::writeToConnection(Time const & previousTime)
{
  WriteResult result = traffic->writeToPeer(peer.get(), previousTime);
  if (!isConnected && result.isConnected)
  {
    planet = result.planet;
    global::planetMap.insert(make_pair(planet, this));
  }
  else if (isConnected && result.isConnected
           && planet != result.planet)
  {
    global::planetMap.erase(planet);
    planet = result.planet;
    global::planetMap.insert(make_pair(planet, this));
  }
  isConnected = result.isConnected;
  bufferFull = result.bufferFull;
  nextWrite = result.nextWrite;
  return result.nextWrite;
}

void Connection::cleanup(std::multimap<Time, Connection *> & schedule)
{
  if (isConnected)
  {
    global::planetMap.erase(planet);
  }
  if (nextWrite != Time())
  {
    std::multimap<Time, Connection *>::iterator pos
      = schedule.lower_bound(nextWrite);
    std::multimap<Time, Connection *>::iterator limit
      = schedule.upper_bound(nextWrite);
    for (; pos != limit; ++pos)
    {
      if (pos->second == this)
      {
        schedule.erase(pos);
        break;
      }
    }
  }
}
