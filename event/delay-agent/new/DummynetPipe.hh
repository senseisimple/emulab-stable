// DummynetPipe.hh

// A pipe which implements the system by generating events for the old delay agent.

// TEVC example:
// /usr/testbed/bin/tevc -e project/experiment now agent-name MODIFY BANDWIDTH=3455
// or DELAY=34
// or PLR=0.1

#ifndef DUMMYNET_PIPE_HH_DELAY_AGENT_1
#define DUMMYNET_PIPE_HH_DELAY_AGENT_1

class DummynetPipe : public RootPipe
{
public:
  // 'name' will be prefixed with "new-" plus the original agent's
  // name. To call the old agent, simply remove the prefix.
  DummynetPipe(std::string const & name);
  virtual ~DummynetPipe();
  virtual void reset(void);
  virtual void resetParameter(Parameter const & newParameter);
private:
  virtual updateParameter(struct dn_pipe *pipe, Parameter const & parameter);
  virtual void setPipe(struct dn_pipe *pipe);
  virtual struct dn_pipe *getPipe(void);
  int dummynetPipeNumber;
  int dummynetSocket;
};

#endif
