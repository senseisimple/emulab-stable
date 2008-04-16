// DummynetPipe.cc

#ifdef FREEBSD

extern "C"
{
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/mbuf.h>
#include <unistd.h>
/*
#include <netinet/ip_compat.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_fw.h>
#include <netinet/ip_dummynet.h>
*/


#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_fw.h>
#include <net/route.h> /* def. of struct route */
#include <netinet/ip_dummynet.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
}

#include "lib.hh"
#include "DummynetPipe.hh"


#if __FreeBSD_version >= 601000
#define DN_PIPE_NEXT(p) ((p)->next.sle_next)
#else
#define DN_PIPE_NEXT(p) ((p)->next)
#endif

using namespace std;

DummynetPipe::DummynetPipe(std::string const & pipeno)
{
  cerr << "Creating Dummynet Pipe" << endl;
  dummynetPipeNumber = stringToInt(pipeno);

  dummynetSocket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
  if (dummynetSocket < 0)
  {
    cerr << "Can't create raw socket" << endl;
  }
}

DummynetPipe::~DummynetPipe()
{
  close(dummynetSocket);
}

void DummynetPipe::reset(void)
{
  char *data;
  struct dn_pipe *pipe;

  data = getAllPipes();
  if (data == NULL) {
    cerr << "Couldn't get dummynet pipes" << endl;
    return;
  }
  pipe = findPipe(data);
  if (pipe == NULL)
  {
    cerr << "Couldn't find pipe " << dummynetPipeNumber << endl;
    return;
  }

  map<Parameter::ParameterType, Parameter>::iterator pos = g::defaultParameters.begin();
  map<Parameter::ParameterType, Parameter>::iterator limit = g::defaultParameters.end();

  for (; pos != limit; ++pos)
  {
    updateParameter(pipe, pos->second);
  }

  setPipe(pipe);
}

void DummynetPipe::updateParameter(struct dn_pipe* pipe, Parameter const & newParameter)
{
  switch (newParameter.getType())
  {
  case Parameter::LINK_UP:
    if (newParameter.getValue() == 0)
    {
      // Link down
      pipe->fs.plr = 0x7fffffff;
    }
    else
    {
      // Link up
      pipe->fs.plr = 0;
    }
    break;
  case Parameter::BANDWIDTH:
    pipe->bandwidth = newParameter.getValue() * 1000;
    break;
  case Parameter::DELAY:
    pipe->delay = newParameter.getValue();
    break;
  case Parameter::LOSS:
    // TODO: Loss
    break;
  case Parameter::NOTHING:
    break;
  }
}

void DummynetPipe::resetParameter(Parameter const & newParameter)
{
  char *data;
  struct dn_pipe *pipe;

  data = getAllPipes();
  if (data == NULL) {
    cerr << "Couldn't get dummynet pipes" << endl;
    return;
  }
  pipe = findPipe(data);
  if (pipe == NULL)
  {
    cerr << "Couldn't find pipe " << dummynetPipeNumber << endl;
    return;
  }

  updateParameter(pipe, newParameter);

  setPipe(pipe);
}

char * DummynetPipe::callGetsockopt(char * data, size_t * count)
{
  if (getsockopt(dummynetSocket, IPPROTO_IP, IP_DUMMYNET_GET,
                 data, count) < 0)
  {
    cerr << "Error in getsockopt\n" << endl;
    return NULL;
  }
  else
  {
    return data;
  }
}

char * DummynetPipe::getAllPipes(void)
{
  static vector<char> data(sizeof(struct dn_pipe), '\0');
  char * result = & data[0];
  size_t num_bytes = data.size();
  result = callGetsockopt(& data[0], &num_bytes);
  while (num_bytes >= data.size() && result != NULL)
  {
    data.resize(data.size()*2 + 200);
    num_bytes = data.size();
    result = callGetsockopt(& data[0], &num_bytes);
  }
  return result;
}

struct dn_pipe * DummynetPipe::findPipe(char *data)
{
  struct dn_pipe *p, *pipe;
  int l;

  pipe = NULL;
  p = (struct dn_pipe *) data;

  while ( DN_PIPE_NEXT(p) == (struct dn_pipe *)DN_IS_PIPE )
  {
    if (dummynetPipeNumber == p->pipe_nr) {
      pipe = p;
      break;
    }

    l = p->fs.rq_elements * sizeof(struct dn_flow_queue) +
        sizeof(struct dn_pipe);

    p = (struct dn_pipe *)((char *)p + l);

  }

  return pipe;
}



void DummynetPipe::setPipe(struct dn_pipe *pipe)
{
  if (setsockopt(dummynetSocket, IPPROTO_IP, IP_DUMMYNET_CONFIGURE, pipe, sizeof(*pipe))
      < 0)
    cerr << "IP_DUMMYNET_CONFIGURE setsockopt failed" << endl;
}

#endif
