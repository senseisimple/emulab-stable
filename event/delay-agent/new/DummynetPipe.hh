// DummynetPipe.hh

#ifndef DUMMYNET_PIPE_HH_DELAY_AGENT_1
#define DUMMYNET_PIPE_HH_DELAY_AGENT_1

struct dn_pipe;

class DummynetPipe : public RootPipe
{
public:
  DummynetPipe(std::string const & newPipeNumber);
  virtual ~DummynetPipe();
  virtual void reset(void);
  virtual void resetParameter(Parameter const & newParameter);
private:
  char * callGetsockopt(char * data, size_t * count);
  void updateParameter(struct dn_pipe *pipe, Parameter const & parameter);
  void setPipe(struct dn_pipe *pipe);
  char * getAllPipes(void);
  struct dn_pipe * findPipe(char * data);
  int dummynetPipeNumber;
  int dummynetSocket;
};

#endif
