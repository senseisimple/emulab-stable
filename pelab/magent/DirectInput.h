/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// DirectInput.h

#ifndef DIRECT_INPUT_H_STUB_2
#define DIRECT_INPUT_H_STUB_2

#include "CommandInput.h"
#include "saveload.h"

class DirectInput : public CommandInput
{
public:
  DirectInput();
  virtual ~DirectInput();
  virtual void nextCommand(fd_set * readable);
  virtual int getMonitorSocket(void);
  virtual void disconnect(void);
  int checksum(void);
private:
  enum MonitorState
  {
    ACCEPTING,
    HEADER,
    BODY
  };
private:
  MonitorState state;
  int monitorAccept;
  int monitorSocket;
  int index;
  char headerBuffer[Header::headerSize];
  Header commandHeader;
  enum { bodyBufferSize = 0xffff };
  char bodyBuffer[bodyBufferSize];
};

#endif
