// DummynetPipe.cc

#ifdef FREEBSD

#include "lib.hh"
#include "DummynetPipe.hh"

extern "C"
{
#include <sys/types.h>
#include <sys/socket.h>
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

#if __FreeBSD_version >= 601000
#define DN_PIPE_NEXT(p) ((p)->next.sle_next)
#else
#define DN_PIPE_NEXT(p) ((p)->next)
#endif

using namespace std;

DummynetPipe::DummynetPipe(std::string const & pipeno)
{
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
  struct dn_pipe *pipe;

  pipe = getPipe();
  if (pipe == NULL)
  {
    cerr << "Couldn't find pipe for " << dummynetPipeNumber << endl;
    return;
  }

  map<Parameter::ParameterType, Parameter>::iterator pos = g::defaultParameters.begin();
  map<Parameter::ParameterType, Parameter>::iterator limit = g::defaultParameters.end();

  for (; pos != limit; ++pos)
  {
    updateParameter(pipe, pos->second);
  }

  setPipe(pipe);

  free(pipe);
}

void DummynetPipe::updateParameter(struct dn_pipe* pipe, Parameter const & newParameter)
{
  switch (newParameter.getType())
  {
  case Parameter::BANDWIDTH:
    pipe->bandwidth = newParameter.getValue();
    break;
  case Parameter::DELAY:
    pipe->delay = newParameter.getValue();
    break;
  default:
    // TODO: Handle UP and DOWN events
    break;
  }
}

void DummynetPipe::resetParameter(Parameter const & newParameter)
{
  struct dn_pipe *pipe;

  pipe = getPipe();
  if (pipe == NULL)
  {
    return;
  }

  updateParameter(pipe, newParameter);

  setPipe(pipe);

  free(pipe);
}

char * callGetsockopt(char * data, size_t * count)
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
  static vector<char> data(sizeof(struct dn_pipe));
  char * result = & data[0];
  size_t num_bytes = 0;
  result = callGetsockopt(& data[0], &num_bytes);
  while (num_bytes >= data.size() && result != NULL)
  {
    data.resize(data.size()*2 + 200);
    result = callGetsockopt(& data[0], &num_bytes);
  }
  return result;
}

struct dn_pipe * DummynetPipe::getPipe(void)
{

  // Find the pipe we care about
  // Pass this pipe to the thing which should deal with it.

  size_t num_bytes = sizeof(struct dn_pipe);
  size_t num_alloc = sizeof(struct dn_pipe);
  void * data = NULL;
  void * next = NULL;
  struct dn_pipe * p = NULL;
  struct dn_pipe * pipe = NULL;
  struct dn_flow_queue * q = NULL;
  int l;

  data = malloc(num_bytes);
  if(data == NULL)
  {
    cerr << "malloc: cant allocate memory" << endl;
    return 0;
  }

  while (num_bytes >= num_alloc)
  {
    num_alloc = num_alloc * 2 + 200;
    num_bytes = num_alloc ;
    if ((data = realloc(data, num_bytes)) == NULL)
    {
      cerr << "cant alloc memory" << endl;
      return 0;
    }

    if (getsockopt(dummynetSocket, IPPROTO_IP, IP_DUMMYNET_GET,
                   data, &num_bytes) < 0)
    {
      cerr << "error in getsockopt" << endl;
      return 0;
    }

  }

  next = data;
  p = (struct dn_pipe *) data;
  pipe = NULL;

  for ( ; num_bytes >= sizeof(*p) ; p = (struct dn_pipe *)next )
  {
    if ( DN_PIPE_NEXT(p) != (struct dn_pipe *)DN_IS_PIPE )
      break ;

    l = sizeof(*p) + p->fs.rq_elements * sizeof(*q) ;
    next = (void*)((char *)p  + l);
    num_bytes -= l ;
    if (dummynetPipeNumber != p->pipe_nr)
      continue;

    q = (struct dn_flow_queue *)(p+1) ;


#if 0
    /* grab pipe delay and bandwidth */
    link_map[l_index].getParams(p_index).setDelay(p->delay);
    link_map[l_index].getParams(p_index).setBandwidth(p->bandwidth);

    /* get flow set parameters*/
    get_flowset_params( &(p->fs), l_index, p_index);

    /* get dynamic queue parameters*/
    get_queue_params( &(p->fs), l_index, p_index);
#endif
  }

  if ((num_bytes < sizeof(*p) || (DN_PIPE_NEXT(p) != (struct dn_pipe *)DN_IS_PIPE))) {
    return NULL;
  }

  pipe = (struct dn_pipe*)malloc(l);
  if (pipe == NULL) {
    cerr << "can't allocate memory" << endl;
    return NULL;
  }

  memcpy(pipe, p, l);
  free(data);

  return pipe;
}



void DummynetPipe::setPipe(struct dn_pipe *pipe)
{
  if (setsockopt(dummynetSocket, IPPROTO_IP, IP_DUMMYNET_CONFIGURE, pipe, sizeof(*pipe))
      < 0)
    cerr << "IP_DUMMYNET_CONFIGURE setsockopt failed" << endl;
}

#endif
