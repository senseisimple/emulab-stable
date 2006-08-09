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
  std::auto_ptr<TrafficModel> model(new CircularTraffic());
  conn->setTraffic(model);
}

//-----------------------

void ConnectionModelCommand::runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &)
{
  conn->addConnectionModelParam(*this);
}

//-----------------------

void SensorCommand::runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &)
{
  conn->addSensor(*this);
}

//-----------------------

void ConnectCommand::runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &)
{
  conn->connect();
}

//-----------------------

void TrafficWriteCommand::runConnect(Connection * conn,
                          std::multimap<Time, Connection *> & schedule)
{
  conn->addTrafficWrite(*this, schedule);
}

//-----------------------

void DeleteConnectionCommand::run(std::multimap<Time, Connection *> & schedule)
{
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
