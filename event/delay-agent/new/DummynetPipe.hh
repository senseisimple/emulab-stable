// DummynetPipe.hh

#ifdef FREEBSD

#ifndef DUMMYNET_PIPE_HH_DELAY_AGENT_1
#define DUMMYNET_PIPE_HH_DELAY_AGENT_1

class DummynetPipe : public RootPipe
{
public:
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

#endif
