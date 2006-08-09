// TrivialCommandOutput.cc

#include "lib.h"
#include "TrivialCommandOutput.h"
#include "CommandInput.h"

using namespace std;

TrivialCommandOutput::TrivialCommandOutput()
  : start(0)
  , end(0)
{
}

TrivialCommandOutput::~TrivialCommandOutput()
{
}

int TrivialCommandOutput::startMessage(int size)
{
  attemptWrite();
  if (start == end && global::input->getMonitorSocket() != -1)
  {
    partial.resize(size);
    start = 0;
    end = 0;
    return SENDING_MESSAGE;
  }
  else
  {
    return DISCARDING_MESSAGE;
  }
}

void TrivialCommandOutput::endMessage(void)
{
  attemptWrite();
}

void TrivialCommandOutput::writeMessage(char const * message, int count)
{
  if (end + count <= partial.size())
  {
    memcpy(& partial[end], message, count);
    end += count;
  }
}

void TrivialCommandOutput::attemptWrite(void)
{
  if (start < end)
  {
    int fd = global::input->getMonitorSocket();
    if (fd == -1)
    {
      logWrite(ROBUST, "Unable to write because there is no connection to "
               "the monitor");
      return;
    }
    int error = send(fd, & partial[start], end - start, 0);
    if (error > 0)
    {
      start += error;
    }
    else if (error == -1)
    {
      switch(errno)
      {
      case EBADF:
      case EINVAL:
        logWrite(ERROR, "Problem with descriptor on command channel: %s",
                 strerror(errno));
        break;
      case EAGAIN:
      case EINTR:
        break;
      default:
        logWrite(EXCEPTION, "Failed write on command channel: %s",
                 strerror(errno));
        break;
      }
    }
    else
    {
      // error == 0. This means that the connection has gone away.
      global::input->disconnect();
      partial.resize(0);
      start = 0;
      end = 0;
      logWrite(ROBUST, "Command channel was closed from the other side.");
    }
  }
}
