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
    if (dir == FORWARD_PATH)
    {
      genericMessage(EVENT_FORWARD_PATH, message, key);
    }
    else
    {
      genericMessage(EVENT_BACKWARD_PATH, message, key);
    }
  }

  void genericMessage(int type, std::string const & message, Order const & key)
  {
    if (message.size() <= 0xffff && message.size() > 0)
    {
      Header prefix;
      std::string pathString;
      prefix.type = type;
      switch (prefix.type)
      {
      case EVENT_FORWARD_PATH:
        pathString = "EVENT_FORWARD";
        break;
      case EVENT_BACKWARD_PATH:
        pathString = "EVENT_BACKWARD";
        break;
      case TENTATIVE_THROUGHPUT:
        pathString = "TENTATIVE_THROUGHPUT";
        break;
      case AUTHORITATIVE_BANDWIDTH:
        pathString = "AUTHORITATIVE_BANDWIDTH";
        break;
      default:
        pathString = "Unknown Output Command Type";
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
