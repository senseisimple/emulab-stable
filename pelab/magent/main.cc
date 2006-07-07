// main.cc

#include "lib.h"

using namespace std;

void writeToConnections(multimap<Time, Connection *> const & schedule);
void addNewPeer(fd_set * readable);
void readFromPeers(fd_set * readable);

void mainLoop(void)
{
  multimap<Time, Connection *> schedule;

  // Select on file descriptors
  while (true)
  {
    fd_set readable = global::readers;
    select();

    global::input->nextCommand(readable);
    Command * current = global::input->getCommand();
    while (current != NULL)
    {
      current->run(schedule);
      global::input->nextCommand(readable);
      current = global::input->getCommand();
    }
    writeToConnections(schedule);
    addNewPeer(readable);
    readFromPeers(readable);
    packetCapture(readable);
  }
}

void writeToConnections(multimap<Time, Connection *> const & schedule)
{
  Time now = getCurrentTime();
  bool done = false;
    // Notify any connection which is due, then erase that entry from
    // the schedule and insert another entry at the new time specified
    // by the connection.
  while (! schedule.empty && !done)
  {
    multimap<Time, Connection *>::iterator pos = schedule.begin();
    if (pos->first < now)
    {
      Connection * current = pos->second;
      Time nextTime = current->writeConnection(pos->first);
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
  case CONNECTION_MODEL_KERNEL:
    kernelTcp_addNewPeer(readable);
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
  case CONNECTION_MODEL_KERNEL:
    kernelTcp_readFromPeers(readable);
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
  case CONNECTION_MODEL_KERNEL:
    kernelTcp_packetCapture(readable);
    break;
  default:
    logWrite(ERROR, "connectionModelArg is corrupt (%d). "
             "I can no longer capture packets sent to peers.",
             global::connectionModelArg);
    break;
  }
}

void addDescriptor(int fd)
{
  FD_SET(fd, &(global::readers));
  if (fd > global::maxReader)
  {
    global::maxReader = fd;
  }
}
