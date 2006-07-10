// CommandOutput.h

// This is the base class which abstracts the messages sent to the monitor.

// To create your own CommandOutput concrete class, overwrite the
// writeMessage() method.

#ifndef COMMAND_OUTPUT_H_STUB_2
#define COMMAND_OUTPUT_H_STUB_2

class CommandOutput
{
public:
  virtual ~CommandOutput() {}
  void eventMessage(std::string const & message, Order const & key)
  {
    if (message.size() <= 0xffff && message.size() > 0)
    {
      writeHeader(EVENT_TO_MONITOR, message.size(), key);
      writeMessage(message.c_str(), message.size());
    }
    else
    {
      logWrite(ERROR, "Event Control Message too big or 0. It was not sent. "
               "Size: %ud", message.size());
    }
  }

private:
  void writeHeader(int type, unsigned short size, Order const & key)
  {
    int bufferSize = sizeof(unsigned char)*2 + sizeof(unsigned short)*3
      + sizeof(unsigned long);
    char buffer[bufferSize];
    char * pos = buffer;

    pos = saveChar(pos, type);
    pos = saveShort(pos, size);
    pos = saveChar(pos, key.transport);
    pos = saveInt(pos, key.ip);
    pos = saveShort(pos, key.localPort);
    pos = saveShort(pos, key.remotePort);
    writeMessage(buffer, bufferSize);
  }

  char * saveChar(char * buffer, unsigned char value)
  {
    memcpy(buffer, &value, sizeof(value));
    return buffer + sizeof(value);
  }

  char * saveShort(char * buffer, unsigned short value)
  {
    unsigned short ordered = htons(value);
    memcpy(buffer, &ordered, sizeof(ordered));
    return buffer + sizeof(ordered);
  }

  char * saveInt(char * buffer, unsigned short value)
  {
    unsigned int ordered = htonl(value);
    memcpy(buffer, &ordered, sizeof(ordered));
    return buffer + sizeof(ordered);
  }
protected:
  virtual void writeMessage(char const * message, int count)=0;
};

#endif
