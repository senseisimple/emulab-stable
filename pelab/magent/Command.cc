/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Command.cc

#include "lib.h"
#include "Command.h"
#include "Sensor.h"
#include "Connection.h"
#include "ConnectionModel.h"
#include "TrafficModel.h"
#include "CircularTraffic.h"

using namespace std;

void NewConnectionCommand::run(std::multimap<Time, Connection *> &)
{
  logWrite(COMMAND_INPUT, "Running NEW_CONNECTION_COMMAND: %s",
           key.toString().c_str());
  std::map<Order, Connection>::iterator pos
    = global::connections.find(key);
  if (pos == global::connections.end())
  {
    pos = global::connections.insert(make_pair(key, Connection())).first;
    pos->second.reset(key, global::connectionModelExemplar->clone());
  }
}

void NewConnectionCommand::runConnect(Connection *,
                          std::multimap<Time, Connection *> &)
{
}

//-----------------------

void TrafficModelCommand::runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &)
{
  logWrite(COMMAND_INPUT, "Running TRAFFIC_MODEL_COMMAND: %s",
           key.toString().c_str());
  std::auto_ptr<TrafficModel> model(new CircularTraffic());
  conn->setTraffic(model);
}

//-----------------------

void ConnectionModelCommand::runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &)
{
  logWrite(COMMAND_INPUT, "Running CONNECTION_MODEL_COMMAND: %s",
           key.toString().c_str());
  conn->addConnectionModelParam(*this);
}

//-----------------------

void SensorCommand::runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &)
{
  logWrite(COMMAND_INPUT, "Running SENSOR_COMMAND: %s",
           key.toString().c_str());
  conn->addSensor(*this);
}

//-----------------------

void ConnectCommand::runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &)
{
  logWrite(COMMAND_INPUT, "Running CONNECT_COMMAND: %s",
           key.toString().c_str());
  conn->connect();
}

//-----------------------

void TrafficWriteCommand::runConnect(Connection * conn,
                          std::multimap<Time, Connection *> & schedule)
{
//  logWrite(COMMAND_INPUT, "Running TRAFFIC_WRITE_COMMAND");
  conn->addTrafficWrite(*this, schedule);
}

//-----------------------

void DeleteConnectionCommand::run(std::multimap<Time, Connection *> & schedule)
{
  logWrite(COMMAND_INPUT, "Running DELETE_CONNECTION_COMMAND: %s",
           key.toString().c_str());
  std::map<Order, Connection>::iterator pos
    = global::connections.find(key);
  if (pos != global::connections.end())
  {
    pos->second.cleanup(schedule);
    global::connections.erase(pos);
  }
}

void DeleteConnectionCommand::runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &)
{
}
