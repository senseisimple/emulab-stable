// PipeInfo.hh

#ifndef PIPE_INFO_HH_DELAY_AGENT_1
#define PIPE_INFO_HH_DELAY_AGENT_1

#include "Pipe.hh"
#include "EventPipe.hh"
#include "DummynetPipe.hh"

// Each pipe here is uni-directional. Multiple directions are
// accounted for with multiple pipes.
struct PipeInfo
{
  PipeInfo(std::string const & newName,
           std::string const & newEndName,
           bool newIsLan,
           std::string const & newInterface,
           std::string const & newPipeData)
    : name(newName)
    , endName(newEndName)
    , linkName(newName + "-" + newEndName)
    , isLan(newIsLan)
    , parameters(g::defaultParameters)
    , interface(newInterface)
    , pipeData(newPipeData)
#if 0
    , rawPipe(new EventPipe(linkName))
#else
    , rawPipe(new NetlinkPipe(newInterface, newPipeData))
#endif
  {
  }

  // The name of the delay node for this pipe. There may be multiple
  // pipes with the same delay node name. Ex: link0, link1, etc.
  std::string name;
  // The name of the node which is the end point of this pipe. Ex: nodeA, nodeB, etc.
  std::string endName;
  // The name of this pipe as a link name. Ex: link0-nodeA, link1-nodeB, etc.
  std::string linkName;
  bool isLan;
  std::map<Parameter::ParameterType, Parameter> parameters;

  // The interface name on the delay node: Ex: fxp0, fxp1, eth0, etc.
  std::string interface;
  // Implementation-specific data about the pipe. Might be a pipe number, for instance.
  std::string pipeData;
  // rawPipe must follow linkName in declaration order
  Pipe rawPipe;
};

#endif
