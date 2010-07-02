/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// CommandInput.h

// This is the abstract base class for various kinds of control
// input. Each input is broken down into one of a number of
// commands. 'nextCommand' attempts to read enough bytes to make up
// the next command while 'getCommand' returns the current command or
// NULL if the previous 'nextCommand' had failed to acquire enough
// bytes.

#ifndef COMMAND_INPUT_H_STUB_2
#define COMMAND_INPUT_H_STUB_2

#include "Command.h"

class CommandInput
{
public:
  virtual ~CommandInput() {}
  virtual Command * getCommand(void)
  {
    return currentCommand.get();
  }
  virtual void nextCommand(fd_set * readable)=0;
  virtual int getMonitorSocket(void)=0;
  virtual void disconnect(void)=0;
protected:
  std::auto_ptr<Command> currentCommand;
};

class NullCommandInput : public CommandInput
{
public:
  virtual ~NullCommandInput() {}
  virtual void nextCommand(fd_set *) {}
  virtual int getMonitorSocket(void) { return -1; }
  virtual void disconnect(void) {}
};

#endif
