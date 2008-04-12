// lib.cc

#include "lib.hh"

#include <netlink/netlink.h>
#include <netlink/route/link.h>

using namespace std;

namespace g
{
  string experimentName;
  bool debug = false;

  map<Parameter::ParameterType, Parameter> defaultParameters;

  multimap<std::string, std::list<PipeInfo>::iterator> pipes;
  list<PipeInfo> pipeVault;
}

int stringToInt(std::string const & val)
{
  int result = 0;
  istringstream stream(val);
  stream >> result;
  return result;
}

int hexStringToInt(std::string const & val)
{
  int result = 0;
  istringstream stream(val);
  stream >> std::hex >> result;
  return result;
}

string intToString(int val)
{
  ostringstream stream;
  stream << val;
  return stream.str();
}

