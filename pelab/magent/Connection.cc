// Connection.cc

#include "lib.h"
#include "Connection.h"
#include "Time.h"
#include "ConnectionModel.h"
#include "TrafficModel.h"
#include "Sensor.h"

using namespace std;

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

void Connection::addTrafficWrite(TrafficWriteCommand const & newWrite
                            std::multimap<Time, Connection *> const & schedule)
{
  if (traffic.get() != NULL)
  {
    Time nextTime = traffic->addWrite(newWrite);
    if (nextTime != Time())
    {
      schedule.insert(make_pair(nextTime, this));
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
    planet = result;
    global::planetMap.insert(make_pair(planet, this));
  }
  isConnected = result.isConnected;
  bufferFull = result.bufferFull;
  return result.nextWrite;
}

void cleanup(void)
{
  if (isConnected)
  {
    global::planetMap.erase(planet);
  }
}
