// CircularTraffic.cc

#include "lib.h"
#include "log.h"
#include "CircularTraffic.h"
#include "Command.h"
#include "ConnectionModel.h"

using namespace std;

CircularTraffic::CircularTraffic()
{
  current = writes.end();
}

CircularTraffic::~CircularTraffic()
{
}

auto_ptr<TrafficModel> CircularTraffic::clone(void)
{
  auto_ptr<CircularTraffic> result;
  result->current = current;
  result->writes = writes;
  result->nextWriteSize = nextWriteSize;
  auto_ptr<TrafficModel> model(result.release());
  return model;
}

Time CircularTraffic::addWrite(TrafficWriteCommand const & newWrite,
                               Time const & deadline)
{
  Time now = getCurrentTime();
  writes.push_back(newWrite);
  writes.back().localTime = now;
  // XXX: end() is not necessarily a constant value. Though this
  // should work, if there is erratic behaviour, here is a good place
  // to check.
  if (current == writes.end())
  {
    current = writes.begin();
  }

  if (deadline == Time())
  {
    // If there is no current deadline, return the next one.
    nextWriteSize = writes.back().size;
    return now + min(writes.back().delta,
                     static_cast<unsigned int>(EXPIRATION_TIME));
  }
  else
  {
    // Otherwise, it doesn't matter what we return.
    return Time();
  }
}

void CircularTraffic::writeToPeer(ConnectionModel * peer,
                                  Time const & previousTime,
                                  WriteResult & result)
{
  if (current != writes.end())
  {
    // Write a cached message of the specified size.
    peer->writeMessage(nextWriteSize, result);

    // Iterate over any stale writes, removing them as we go along.
    // First, start out with one past the current write.
    current = advance(current);
    // next is used to cache the next value in case we need to delete current.
    std::list<TrafficWriteCommand>::iterator next = advance(current);
    // Unless we find a recent write, we don't schedule another write.
    result.nextWrite = Time();
    bool done = false;
    Time now = getCurrentTime();
    while (current != writes.end() && !done)
    {
      if ((now - current->localTime).toMilliseconds() < EXPIRATION_TIME)
      {
        // The current write is recent. Use it.
        result.nextWrite = previousTime + current->delta;
        nextWriteSize = current->size;
        done = true;
      }
      else
      {
        // The current write is stale.
        if (current == next)
        {
          // Special case when our list is of size 1, we just nuke the
          // list and give up.
          writes.clear();
          current = writes.end();
          nextWriteSize = 0;
        }
        else
        {
          // Common case. list is > size 2. We erase the current
          // node. Then we advance it using next.
          writes.erase(current);
          current = next;
          next = advance(next);
        }
      }
    }
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

list<TrafficWriteCommand>::iterator CircularTraffic::advance(
  list<TrafficWriteCommand>::iterator old)
{
  list<TrafficWriteCommand>::iterator result = old;
  if (result == writes.end())
  {
    result = writes.begin();
  }
  else
  {
    ++result;
  }
  return result;
}
