/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// TrivialCommandOutput.h

#ifndef TRIVIAL_COMMAND_OUTPUT_H_STUB_2
#define TRIVIAL_COMMAND_OUTPUT_H_STUB_2

#include "CommandOutput.h"

class TrivialCommandOutput : public CommandOutput
{
public:
  TrivialCommandOutput();
  virtual ~TrivialCommandOutput();
protected:
  virtual int startMessage(int size);
  virtual void endMessage(void);
  virtual void writeMessage(char const * message, int count);
private:
  void attemptWrite(void);
private:
  std::vector<char> partial;
  size_t start;
  size_t end;
};

#endif
