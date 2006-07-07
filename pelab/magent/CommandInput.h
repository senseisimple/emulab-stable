// CommandInput.h

// This is the abstract base class for various kinds of control
// input. Each input is broken down into one of a number of
// commands. 'nextCommand' attempts to read enough bytes to make up
// the next command while 'getCommand' returns the current command or
// NULL if the previous 'nextCommand' had failed to acquire enough
// bytes.

#ifndef COMMAND_INPUT_H_STUB_2
#define COMMAND_INPUT_H_STUB_2

class Command;

class CommandInput
{
public:
  virtual Command * getCommand(void)
  {
    return currentCommand.get();
  }
  virtual void nextCommand(void)=0;
protected:
  std::auto_ptr<Command> currentCommand;
};

#endif
