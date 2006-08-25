// CommandOutput.h

// This is the base class which abstracts the messages sent to the monitor.

// To create your own CommandOutput concrete class, overwrite the
// writeMessage() method.

#ifndef COMMAND_OUTPUT_H_STUB_2
#define COMMAND_OUTPUT_H_STUB_2

#include "log.h"
#include "saveload.h"

class CommandOutput
{
public:
  enum
  {
    SENDING_MESSAGE = 0,
    DISCARDING_MESSAGE
  };
  enum PathDirection
  {
    FORWARD_PATH,
    BACKWARD_PATH
  };
public:
  virtual ~CommandOutput() {}
  void eventMessage(std::string const & message, Order const & key,
                    PathDirection dir=FORWARD_PATH)
  {
    if (message.size() <= 0xffff && message.size() > 0)
    {
      Header prefix;
      std::string pathString;
      if (dir == FORWARD_PATH)
      {
        prefix.type = EVENT_FORWARD_PATH;
        pathString = "FORWARD";
      }
      else
      {
        prefix.type = EVENT_BACKWARD_PATH;
        pathString = "BACKWARD";
      }
      prefix.size = message.size();
      prefix.key = key;
      char headerBuffer[Header::headerSize];
      saveHeader(headerBuffer, prefix);
      int result = startMessage(Header::headerSize + message.size());
      if (result == SENDING_MESSAGE)
      {
        writeMessage(headerBuffer, Header::headerSize);
        writeMessage(message.c_str(), message.size());
        endMessage();
      }
      if (result == SENDING_MESSAGE || global::replayArg == REPLAY_LOAD)
      {
        logWrite(COMMAND_OUTPUT, "(%s,%s): %s",
                 key.toString().c_str(), pathString.c_str(), message.c_str());
      }
    }
    else
    {
      logWrite(ERROR, "Event Control Message too big or 0. It was not sent. "
               "Size: %ud", message.size());
    }
  }
protected:
  virtual int startMessage(int size)=0;
  virtual void endMessage(void)=0;
  virtual void writeMessage(char const * message, int count)=0;
};

class NullCommandOutput : public CommandOutput
{
public:
  virtual ~NullCommandOutput() {}
protected:
  virtual int startMessage(int size) { return DISCARDING_MESSAGE; }
  virtual void endMessage(void) {}
  virtual void writeMessage(char const *, int) {}
};

#endif
