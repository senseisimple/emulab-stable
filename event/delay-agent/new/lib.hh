// lib.hh

#ifndef LIB_HH_DELAY_AGENT_1
#define LIB_HH_DELAY_AGENT_1

#include <cassert>

#include <string>
#include <map>
#include <list>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

#include "event.h"

#define TBDB_OBJECTTYPE_LINK    "LINK"
#define TBDB_EVENTTYPE_UP       "UP"
#define TBDB_EVENTTYPE_DOWN     "DOWN"
#define TBDB_EVENTTYPE_MODIFY   "MODIFY"
#define TBDB_EVENTTYPE_RESET    "RESET"
#define TBDB_EVENTTYPE_COMPLETE "COMPLETE"

enum { EVENT_BUFFER_SIZE = 50 };

int stringToInt(std::string const & val);
int hexStringToInt(std::string const & val);
std::string intToString(int val);

void resetCallback(event_handle_t handle,
                   event_notification_t notification, void *data);
void linkCallback(event_handle_t handle,
                  event_notification_t notification, void *data);

#include "Parameter.hh"

namespace g
{
  extern std::string experimentName;
  extern bool debug;

  extern std::map<Parameter::ParameterType, Parameter> defaultParameters;
}

#include "PipeInfo.hh"

namespace g
{
  extern std::multimap<std::string, std::list<PipeInfo>::iterator> pipes;
  extern std::list<PipeInfo> pipeVault;
}

#endif
