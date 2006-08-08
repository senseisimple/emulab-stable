// main.cc

#include "lib.h"
#include "log.h"
#include "CommandInput.h"
#include "CommandOutput.h"
#include "Command.h"
#include "Time.h"
#include "Connection.h"
#include "Sensor.h"
#include "TrafficModel.h"

#include "KernelTcp.h"

using namespace std;

namespace global
{
  int connectionModelArg = 0;
  unsigned short peerServerPort = 0;
  unsigned short monitorServerPort = 0;

  int peerAccept = -1;
  string interface;
  auto_ptr<ConnectionModel> connectionModelExemplar;
  list< pair<int, string> > peers;

  map<Order, Connection> connections;
  // A connection is in this map only if it is currently connected.
  map<Order, Connection *> planetMap;

  fd_set readers;
  int maxReader = -1;

  std::auto_ptr<CommandInput> input;
  std::auto_ptr<CommandOutput> output;
}

void processArgs(int argc, char * argv[]);
void init(void);
void mainLoop(void);
void writeToConnections(multimap<Time, Connection *> & schedule);
void addNewPeer(fd_set * readable);
void readFromPeers(fd_set * readable);
void packetCapture(fd_set * readable);

int main(int argc, char * argv[])
{
  processArgs(argc, argv);
  init();
  mainLoop();
  return 0;
}

void processArgs(int argc, char * argv[])
{
  global::connectionModelArg = CONNECTION_MODEL_NULL;
  global::peerServerPort = 3491;
  global::monitorServerPort = 4200;
}

void init(void)
{
  srandom(getpid());
  switch (global::connectionModelArg)
  {
  case CONNECTION_MODEL_NULL:
    ConnectionModelNull::init();
    break;
  case CONNECTION_MODEL_KERNEL:
    KernelTcp::init();
    break;
  default:
    logWrite(ERROR, "connectionModelArg is corrupt at initialization (%d)."
      "Terminating program.", global::connectionModelArg);
  }

  FD_ZERO(&global::readers);
  global::maxReader = -1;

  global::input.reset(new NullCommandInput());
  global::output.reset(new NullCommandOutput());
}

void mainLoop(void)
{
  multimap<Time, Connection *> schedule;

  // Select on file descriptors
  while (true)
  {
    fd_set readable = global::readers;
    Time timeUntilWrite;
    Time now = getCurrentTime();
    multimap<Time, Connection *>::iterator nextWrite = schedule.begin();
    if (nextWrite != schedule.end() && now < nextWrite->first)
    {
      timeUntilWrite = nextWrite->first - now;
    }
    else
    {
      // otherwise we don't want to wait.
      timeUntilWrite = Time();
    }
    int error = select(global::maxReader + 1, &readable, NULL, NULL,
                       timeUntilWrite.getTimeval());
    if (error == -1)
    {
      switch (errno)
      {
      case EBADF:
      case EINVAL:
        logWrite(ERROR, "Select failed do to improper inputs. "
                 "Unable to continue: %s", strerror(errno));
        exit(1);
        break;
      case EINTR:
      case ENOMEM:
      default:
        logWrite(EXCEPTION, "Select failed. We may not be able to continue. "
                 "Retrying: %s", strerror(errno));
        continue;
        break;
      }
    }

    global::input->nextCommand(&readable);
    Command * current = global::input->getCommand();
    while (current != NULL)
    {
      current->run(schedule);
      global::input->nextCommand(&readable);
      current = global::input->getCommand();
    }
    writeToConnections(schedule);
    addNewPeer(&readable);
    readFromPeers(&readable);
    packetCapture(&readable);
  }
}

void writeToConnections(multimap<Time, Connection *> & schedule)
{
  Time now = getCurrentTime();
  bool done = false;
    // Notify any connection which is due, then erase that entry from
    // the schedule and insert another entry at the new time specified
    // by the connection.
  while (! schedule.empty() && !done)
  {
    multimap<Time, Connection *>::iterator pos = schedule.begin();
    if (pos->first < now)
    {
      Connection * current = pos->second;
      Time nextTime = current->writeToConnection(pos->first);
      schedule.erase(pos);
      if (nextTime != Time())
      {
        schedule.insert(make_pair(nextTime, current));
      }
    }
    else
    {
      done = true;
    }
  }
}

void addNewPeer(fd_set * readable)
{
  switch (global::connectionModelArg)
  {
  case CONNECTION_MODEL_NULL:
    ConnectionModelNull::addNewPeer(readable);
    break;
  case CONNECTION_MODEL_KERNEL:
    KernelTcp::addNewPeer(readable);
    break;
  default:
    logWrite(ERROR, "connectionModelArg is corrupt (%d). "
             "No further peers will be accepted.", global::connectionModelArg);
    break;
  }
}

void readFromPeers(fd_set * readable)
{
  switch (global::connectionModelArg)
  {
  case CONNECTION_MODEL_NULL:
    ConnectionModelNull::readFromPeers(readable);
    break;
  case CONNECTION_MODEL_KERNEL:
    KernelTcp::readFromPeers(readable);
    break;
  default:
    logWrite(ERROR, "connectionModelArg is corrupt (%d). "
             "I can no longer read from current peers.",
             global::connectionModelArg);
    break;
  }
}

void packetCapture(fd_set * readable)
{
  switch (global::connectionModelArg)
  {
  case CONNECTION_MODEL_NULL:
    ConnectionModelNull::packetCapture(readable);
    break;
  case CONNECTION_MODEL_KERNEL:
    KernelTcp::packetCapture(readable);
    break;
  default:
    logWrite(ERROR, "connectionModelArg is corrupt (%d). "
             "I can no longer capture packets sent to peers.",
             global::connectionModelArg);
    break;
  }
}

void setDescriptor(int fd)
{
  FD_SET(fd, &(global::readers));
  if (fd > global::maxReader)
  {
    global::maxReader = fd;
  }
}

void clearDescriptor(int fd)
{
  FD_CLR(fd, &(global::readers));
  if (fd > global::maxReader)
  {
    global::maxReader = fd;
  }
}

string ipToString(unsigned int ip)
{
  struct in_addr address;
  address.s_addr = ip;
  return string(inet_ntoa(address));
}
