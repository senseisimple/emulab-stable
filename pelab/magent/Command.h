/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Command.h

#ifndef COMMAND_H_STUB_2
#define COMMAND_H_STUB_2

#include "Connection.h"

class Command
{
public:
  virtual void run(std::multimap<Time, Connection *> & schedule)
  {
    std::map<ElabOrder, Connection>::iterator pos
      = global::connections.find(key);
    if (pos != global::connections.end())
    {
      runConnect(&(pos->second), schedule);
    }
  }
  virtual ~Command() {}
protected:
  virtual void runConnect(Connection * conn,
                          std::multimap<Time, Connection *> & schedule)=0;

public:
  // We use a key here and look up the connection only on a run()
  // because some commands delete a connection and we don't want later
  // commands to keep the reference around.
  ElabOrder key;
};

class NewConnectionCommand : public Command
{
public:
  NewConnectionCommand() : transport(TCP_CONNECTION) {}
  virtual void run(std::multimap<Time, Connection *> &);
protected:
  virtual void runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &);
public:
  unsigned char transport;
};

class TrafficModelCommand : public Command
{
protected:
  virtual void runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &);
};

class ConnectionModelCommand : public Command
{
protected:
  virtual void runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &);
public:
  int type;
  int value;
};

class SensorCommand : public Command
{
protected:
  virtual void runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &);
public:
  int type;
//  std::vector<char> parameters;
};

class ConnectCommand : public Command
{
public:
  ConnectCommand() : ip(0) {}
protected:
  virtual void runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &);
public:
  unsigned int ip;
};

class TrafficWriteCommand : public Command
{
protected:
  virtual void runConnect(Connection * conn,
                          std::multimap<Time, Connection *> & schedule);
public:
  unsigned int delta;
  unsigned int size;
  Time localTime;
};

class DeleteConnectionCommand : public Command
{
public:
  virtual void run(std::multimap<Time, Connection *> & schedule);
protected:
  virtual void runConnect(Connection * conn,
                          std::multimap<Time, Connection *> &);
};

#endif
