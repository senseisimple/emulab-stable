// Command.h

#ifndef COMMAND_H_STUB_2
#define COMMAND_H_STUB_2

class Command
{
public:
  virtual void run(std::multimap<Time, Connection *> & schedule)
  {
    std::multimap<ElabOrder, Connection>::iterator pos
      = global::connections.find(key);
    if (pos != global::connections.end())
    {
      runConnect(&(pos->second));
    }
  }
protected:
  virtual void runConnect(Connection * conn)=0;

  // We use a key here and look up the connection only on a run()
  // because some commands delete a connection and we don't want later
  // commands to keep the reference around.
  ElabOrder key;
};

class NewConnectionCommand : public Command
{
public:
  virtual void run(std::multimap<Time, Connection *> &)
  {
    std::multimap<ElabOrder, Connection>::iterator pos
      = global::connections.find(key);
    if (pos == global::connections.end())
    {
      pos = global::connections.insert(make_pair(key, Command()));
      pos->second.reset(elab, connectionModelExemplar.clone());
    }
  }
protected:
  virtual void runConnect(Connection *)
  {
  }
};

class TrafficModelCommand : public Command
{
protected:
  virtual void runConnect(Connection * conn)
  {
  }
};

class ConnectionModelCommand : public Command
{
protected:
  virtual void runConnect(Connection * conn)
  {
    conn->addConnectionModelParam(*this);
  }
public:
  int type;
  unsigned int value;
};

class SensorCommand : public Command
{
protected:
  virtual void runConnect(Connection * conn)
  {
    conn->addSensor(*this);
  }
public:
  int type;
  vector<char> parameters;
};

class ConnectCommand : public Command
{
protected:
  virtual void runConnect(Connection * conn)
  {
    conn->connect();
  }
};

class TrafficWriteCommand : public Command
{
protected:
  virtual void runConnect(Connection * conn)
  {
    conn->addTrafficWrite(*this, schedule);
  }
public:
  unsigned int delta;
  unsigned int size;
};

class DeleteConnectionCommand : public Command
{
public:
  virtual void run(std::multimap<Time, Connection *> & schedule)
  {
    std::multimap<ElabOrder, Connection>::iterator pos
      = global::connections.find(key);
    if (pos != global::connections.end())
    {
      pos->second.cleanup();
      global::connections.erase(pos);
    }
  }
protected:
  virtual void runConnect(Connection * conn)
  {
  }
};

#endif
