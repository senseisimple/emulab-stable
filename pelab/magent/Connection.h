/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Connection.h

#ifndef CONNECTION_H_STUB_2
#define CONNECTION_H_STUB_2

#include "SensorList.h"

class Time;
class ConnectionModel;
class TrafficModel;
class ConnectionModelCommand;
class TrafficWriteCommand;
class SensorCommand;

class Connection
{
public:
  Connection();
  Connection(Connection const & right);
  Connection & operator=(Connection const & right);

  // Called after the Connection is added to the map. Sets the
  // elabOrder sorting element and sets up the Connection Model. Also
  // sets the transport protocol of this connection (TCP_CONNECTION or
  // UDP_CONNECTION)
  void reset(ElabOrder const & newElab,
             std::auto_ptr<ConnectionModel> newPeer,
             unsigned char transport);
  // Called when the monitor specifies which traffic model to use.
  void setTraffic(std::auto_ptr<TrafficModel> newTraffic);
  // Set a connection model parameter. Things like socket buffer
  // sizes, whether Nagle's algorithm is used, etc.
  void addConnectionModelParam(ConnectionModelCommand const & param);
  // This starts an attempt to connect through the connection
  // model. Called when the monitor notifies of a connect.
  void connect(unsigned int ip);
  // Notifies the traffic model of write information from the monitor.
  void addTrafficWrite(TrafficWriteCommand const & newWrite,
                       std::multimap<Time, Connection *> & schedule);
  // Adds a particular kind of sensor when requested by the monitor.
  void addSensor(SensorCommand const & newSensor);
  // Notifies the sensors of a captured packet.
  void capturePacket(PacketInfo * packet);
  // Allows the connection model to be viewed.
  ConnectionModel const * getConnectionModel(void);
  // Notifies the traffic model that a timer has expired on the
  // scheduler.
  Time writeToConnection(Time const & previousTime);
  // Called just before a connection is removed from the map.
  void cleanup(std::multimap<Time, Connection *> & schedule);
private:
  // There are two kinds of ordering. One is for commands from
  // emulab. One is for the pcap analysis.
  ElabOrder elab;
  PlanetOrder planet;

  std::auto_ptr<ConnectionModel> peer;
  std::auto_ptr<TrafficModel> traffic;
  SensorList measurements;
  bool isConnected;
  bool bufferFull;
  Time nextWrite;
};

#endif
