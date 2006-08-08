// CommandOutput.h

// This is the base class which abstracts the messages sent to the monitor.

// To create your own CommandOutput concrete class, overwrite the
// writeMessage() method.

#ifndef COMMAND_OUTPUT_H_STUB_2
#define COMMAND_OUTPUT_H_STUB_2

class CommandOutput
{
public:
  enum
  {
    SENDING_MESSAGE = 0,
    DISCARDING_MESSAGE
  }
public:
  virtual ~CommandOutput() {}
  void eventMessage(std::string const & message, Order const & key,
                    char direction)
  {
    if (message.size() <= 0xffff && message.size() > 0)
    {
      Header prefix;
      prefix.type = EVENT_TO_MONITOR;
      prefix.size = message.size();
      prefix.key = key;
      char headerBuffer[Header::headerSize];
      saveHeader(headerBuffer, prefix);
      int result = startMessage(Header::headerSize + sizeof(direction)
                                + message.size());
      if (result == SENDING_MESSAGE)
      {
        writeMessage(headerBuffer, Header::headerSize);
        writeMessage(&direction, sizeof(direction));
        writeMessage(message.c_str(), message.size());
        finishMessage();
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
