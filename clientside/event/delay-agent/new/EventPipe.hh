// EventPipe.hh

// A pipe which implements the system by generating events for the old delay agent.

// TEVC example:
// /usr/testbed/bin/tevc -e project/experiment now agent-name MODIFY BANDWIDTH=3455
// or DELAY=34
// or PLR=0.1

#ifndef EVENT_PIPE_HH_DELAY_AGENT_1
#define EVENT_PIPE_HH_DELAY_AGENT_1

#include "RootPipe.hh"

class EventPipe : public RootPipe
{
public:
  // 'name' will be prefixed with "new-" plus the original agent's
  // name. To call the old agent, simply remove the prefix.
  EventPipe(std::string const & name);
  virtual ~EventPipe();
  virtual void reset(void);
  virtual void resetParameter(Parameter const & newParameter);
private:
  std::string agentName;
};

#endif
