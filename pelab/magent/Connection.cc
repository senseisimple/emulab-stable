/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
  logWrite(CONNECTION, "Connection created");
}

Connection::Connection(Connection const & right)
  : measurements(right.measurements)
  , isConnected(right.isConnected)
  , bufferFull(right.bufferFull)
  , nextWrite(right.nextWrite)
{
  logWrite(CONNECTION, "Connection copy constructed");
  if (right.peer.get() != NULL)
  {
    peer = right.peer->clone();
  }
  if (right.traffic.get() != NULL)
  {
    traffic = right.traffic->clone();
  }
}

Connection & Connection::operator=(Connection const & right)
{
  logWrite(CONNECTION, "Connection assigned");
  if (this != &right)
  {
    if (right.peer.get() != NULL)
    {
      peer = right.peer->clone();
    }
    if (right.traffic.get() != NULL)
    {
      traffic = right.traffic->clone();
    }
    measurements = right.measurements;
    isConnected = right.isConnected;
    bufferFull = right.bufferFull;
    nextWrite = right.nextWrite;
  }
  return *this;
}

void Connection::reset(ElabOrder const & newElab,
                       std::auto_ptr<ConnectionModel> newPeer,
                       unsigned char transport)
{
  logWrite(CONNECTION, "Peer added to connection");
  elab = newElab;
  peer = newPeer;
  planet.transport = transport;
  switch (transport)
  {
  case TCP_CONNECTION:
    planet.remotePort = global::peerServerPort;
    break;
  case UDP_CONNECTION:
    planet.remotePort = global::peerUdpServerPort;
    break;
  default:
    logWrite(ERROR, "Invalid transport protocol %d, "
             "defaulting to TCP_CONNECTION", transport);
    planet.transport = TCP_CONNECTION;
    planet.remotePort = global::peerServerPort;
    break;
  }
}

void Connection::setTraffic(std::auto_ptr<TrafficModel> newTraffic)
{
  logWrite(CONNECTION, "Traffic Model added to connection");
  traffic = newTraffic;
}

void Connection::addConnectionModelParam(ConnectionModelCommand const & param)
{
  logWrite(CONNECTION, "Connection Model Parameter added to connection");
  if (peer.get() != NULL)
  {
    peer->addParam(param);
  }
  else
  {
    logWrite(ERROR, "Connection model has not been initialized before an "
             "addConnectionModelParam call");
  }
}

void Connection::connect(unsigned int ip)
{
  logWrite(CONNECTION, "Connection connected");
  planet.ip = ip;
  if (peer.get() != NULL)
  {
    // planet is modified by ConnectionModel::connect()
    peer->connect(planet);
    isConnected = peer->isConnected();
    if (isConnected)
    {
      logWrite(PCAP,"Inserting key: %s",
               planet.toString().c_str());
      global::planetMap[planet] = this;
    }
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
  logWrite(CONNECTION, "Sensor added to connection");
  measurements.addSensor(newSensor);
}

void Connection::capturePacket(PacketInfo * packet)
{
  Sensor * head = measurements.getHead();
  packet->elab = elab;
  packet->bufferFull = bufferFull;
  if (head != NULL && isConnected)
  {
    head->capturePacket(packet);
  }
  replayWritePacket(packet);
}

ConnectionModel const * Connection::getConnectionModel(void)
{
  return peer.get();
}

Time Connection::writeToConnection(Time const & previousTime)
{
  WriteResult result;
  result.planet.transport = planet.transport;
  result.planet.ip = planet.ip;
  result.planet.remotePort = planet.remotePort;
  if (isConnected)
  {
    result.planet.localPort = planet.localPort;
  }
  traffic->writeToPeer(peer.get(), previousTime, result);
  if (!isConnected && result.isConnected)
  {
    planet = result.planet;
    logWrite(PCAP,"Inserting key: %s",
             planet.toString().c_str());
    global::planetMap[planet] = this;
  }
  else if (isConnected && result.isConnected
           && planet != result.planet)
  {
    logWrite(CONNECTION, "OldKey: %s", planet.toString().c_str());
    global::planetMap[planet] = NULL;
    planet = result.planet;
    logWrite(PCAP,"Inserting key: %s",
             planet.toString().c_str());
    global::planetMap[planet] = this;
  }
  isConnected = result.isConnected;
  bufferFull = result.bufferFull;
  nextWrite = result.nextWrite;
  return result.nextWrite;
}

void Connection::cleanup(std::multimap<Time, Connection *> & schedule)
{
  logWrite(CONNECTION, "Connection cleanup");
  if (isConnected)
  {
    global::planetMap[planet] = NULL;
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
