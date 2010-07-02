// NetlinkPipe.hh

// A pipe which implements the system by generating events for the old delay agent.

// TEVC example:
// /usr/testbed/bin/tevc -e project/experiment now agent-name MODIFY BANDWIDTH=3455
// or DELAY=34
// or PLR=0.1

#ifndef NETLINK_PIPE_HH_DELAY_AGENT_1
#define NETLINK_PIPE_HH_DELAY_AGENT_1

#include <stdint.h>

class NetlinkPipe : public RootPipe
{
public:
  // 'name' will be prefixed with "new-" plus the original agent's
  // name. To call the old agent, simply remove the prefix.
  NetlinkPipe(std::string const & iface, std::string const & pipeno);
  virtual ~NetlinkPipe();
  virtual void reset(void);
  virtual void resetParameter(Parameter const & newParameter);
private:
  virtual void updateParameter(Parameter const & newParameter);
  virtual int init(void);
  virtual void test(void);

  std::string interfaceName;
  std::string pipeNumber;
  int ifindex;
  struct nl_handle *nl_handle;
  struct nl_cache *qdisc_cache, *class_cache;
  uint32_t plrHandle, delayHandle, htbHandle;
  uint32_t htbClassHandle;
};

#endif
