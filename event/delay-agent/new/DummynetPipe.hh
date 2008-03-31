// DummynetPipe.hh

#ifdef FREEBSD

#ifndef DUMMYNET_PIPE_HH_DELAY_AGENT_1
#define DUMMYNET_PIPE_HH_DELAY_AGENT_1

struct dn_pipe;

class DummynetPipe : public RootPipe
{
public:
  DummynetPipe(std::string const & name);
  virtual ~DummynetPipe();
  virtual void reset(void);
  virtual void resetParameter(Parameter const & newParameter);
private:
  void updateParameter(struct dn_pipe *pipe, Parameter const & parameter);
  void setPipe(struct dn_pipe *pipe);
  struct dn_pipe * getPipe(void);
  int dummynetPipeNumber;
  int dummynetSocket;
};

#endif

#endif
