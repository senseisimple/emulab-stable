// CircularTraffic.cc

#include "lib.h"
#include "log.h"
#include "CircularTraffic.h"
#include "Command.h"
#include "ConnectionModel.h"

using namespace std;

CircularTraffic::CircularTraffic()
{
  begin = 0;
  usedCount = 0;
  current = 0;
  writes.resize(DEFAULT_SIZE);
}

CircularTraffic::~CircularTraffic()
{
}

auto_ptr<TrafficModel> CircularTraffic::clone(void)
{
  auto_ptr<CircularTraffic> result;
  result->begin = begin;
  result->usedCount = usedCount;
  result->current = current;
  result->writes = writes;
  auto_ptr<TrafficModel> model(result.release());
  return model;
}

Time CircularTraffic::addWrite(TrafficWriteCommand const & newWrite,
                               Time const & deadline)
{
  if (usedCount < static_cast<int>(writes.size()))
  {
    writes[usedCount] = newWrite;
    ++usedCount;
  }
  else
  {
    writes[begin] = newWrite;
    begin = (begin + 1) % writes.size();
  }
  if (deadline == Time())
  {
    return getCurrentTime() + writes[current].delta;
  }
  else
  {
    return Time();
  }
}

void CircularTraffic::writeToPeer(ConnectionModel * peer,
                                  Time const & previousTime,
                                  WriteResult & result)
{
  if (usedCount > 0)
  {
    current = (current + 1) % usedCount;
    peer->writeMessage(writes[current].size, result);
    result.nextWrite = previousTime + writes[current].delta;
    result.isConnected = peer->isConnected();
  }
  else
  {
    logWrite(ERROR, "writeToPeer() called without addWrite() being "
             "called first. This should be impossible.");
    result.isConnected = peer->isConnected();
    result.bufferFull = false;
    result.nextWrite = Time();
  }
}
