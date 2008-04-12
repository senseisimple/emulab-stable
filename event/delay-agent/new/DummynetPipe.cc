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
#ifdef LINUX

// NetlinkPipe.cc

#include "lib.hh"
#include "DummynetPipe.hh"

extern "C"
{
#include <sys/types.h>
#include <sys/socket.h>

#include <netlink/list.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/route.h>
#include <netlink/route/link.h>
#include <netlink/route/class.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/sch/plr.h>
#include <netlink/route/sch/delay.h>
#include <netlink/route/sch/htb.h>
}

using namespace std;

NetlinkPipe::NetlinkPipe(std::string const & iface, std::string const & pipeno)
{
	interfaceName = iface;
	pipeNumber = pipeno;
	nl_handle = NULL;
	class_cache = NULL;
	qdisc_cache = NULL;
	
	init();
	/* test(); */
}

void NetlinkPipe::test(void)
{
	Parameter testParam(Parameter::LOSS, 50000);

	updateParameter(testParam);
}

int NetlinkPipe::init(void)
{
	struct nl_cache *link_cache;
	int handle;
	string str;

	link_cache = NULL;

	cerr << "Got pipe number " << pipeNumber << endl;
	handle = hexStringToInt(pipeNumber);
	cerr << "handle: " << handle << endl;

	nl_handle = nl_handle_alloc();
	if (nl_handle == NULL) {
		cerr << "Unable to allocate nl_handle" << endl;
		return -1;
	}

	if (nl_connect(nl_handle, NETLINK_ROUTE) < 0) {
		cerr << "Unable to allocate Netlink socket" << endl;
		return -1;
	}
	link_cache = rtnl_link_alloc_cache(nl_handle);
	if (link_cache == NULL) {
		cerr << "Unable to allocate link cache" << endl;
		return -1;
	}

	ifindex = rtnl_link_name2i(link_cache, interfaceName.c_str());
	if (ifindex == RTNL_LINK_NOT_FOUND) {
		cerr << "Unable to translate link name to ifindex" << endl;
		return -1;
	}
	nl_cache_free(link_cache);

	qdisc_cache = rtnl_qdisc_alloc_cache(nl_handle);
	if (qdisc_cache == NULL) {
		cerr << "Unable to allocate qdisc cache" << endl;
		return -1;
	}

	plrHandle = handle << 16;
	delayHandle = (handle + 0x10) << 16;
	htbHandle = (handle + 0x20) << 16;

	class_cache = rtnl_class_alloc_cache(nl_handle, ifindex);
	if (class_cache == NULL) {
		cerr << "Unable to allocate class cache" << endl;
		return -1;
	}

	htbClassHandle = htbHandle + 1;


	return 0;
}

NetlinkPipe::~NetlinkPipe()
{
	if (qdisc_cache)
		nl_cache_free(qdisc_cache);

	if (class_cache)
		nl_cache_free(class_cache);

	if (nl_handle)
		nl_close(nl_handle);
}

void NetlinkPipe::reset(void)
{
	map<Parameter::ParameterType, Parameter>::iterator pos = g::defaultParameters.begin();
	map<Parameter::ParameterType, Parameter>::iterator limit = g::defaultParameters.end();

	for (; pos != limit; ++pos)
	{
		updateParameter(pos->second);
	}
}

void NetlinkPipe::resetParameter(Parameter const & newParameter)
{
	updateParameter(newParameter);
}

void NetlinkPipe::updateParameter(Parameter const & newParameter)
{
	struct rtnl_qdisc *qdisc;
	struct rtnl_class *htbClass;

	qdisc = NULL;

	switch(newParameter.getType()) {
		case Parameter::BANDWIDTH:
				htbClass = rtnl_class_get(class_cache, htbClassHandle);
				if (htbClass == NULL) {
					cerr << "Couldn't find htb class " << htbClassHandle << endl;
					return;
				}
				rtnl_htb_set_rate(htbClass, newParameter.getValue());
				rtnl_htb_set_ceil(htbClass, newParameter.getValue());
				rtnl_class_change(nl_handle, htbClass, NULL);
				rtnl_class_put(htbClass);
                                break;
		case Parameter::DELAY:
				qdisc = rtnl_qdisc_get(qdisc_cache, ifindex, delayHandle);
				if (qdisc == NULL) {
					cerr << "Couldn't find delay qdisc " << delayHandle << endl;
					return;
				}
				rtnl_delay_set_delay(qdisc, newParameter.getValue());
				rtnl_qdisc_change(nl_handle, qdisc, NULL);
				rtnl_qdisc_put(qdisc);
                                break;
		case Parameter::LOSS:
				qdisc = rtnl_qdisc_get(qdisc_cache, ifindex, plrHandle);
				if (qdisc == NULL) {
					cerr << "Couldn't find plr qdisc " << plrHandle << endl;
					return;
				}
				rtnl_plr_set_plr(qdisc, newParameter.getValue());
				rtnl_qdisc_change(nl_handle, qdisc, NULL);
				rtnl_qdisc_put(qdisc);
                                break;
		default:
				break;
	}
}

#endif
