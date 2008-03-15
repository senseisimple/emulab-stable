// linkCallback.cc

#include "lib.hh"
#include "PipeInfo.hh"

using namespace std;

void handleEvent(event_handle_t handle,
                 event_notification_t notification,
                 string const & type,
                 list<PipeInfo>::iterator pipe);
void handleModify(event_handle_t handle,
                  event_notification_t notification,
                  list<PipeInfo>::iterator pipe);
Parameter parseArg(string const & arg);
void changeParameter(list<PipeInfo>::iterator pipe, Parameter const & newParameter);


void linkCallback(event_handle_t handle,
                  event_notification_t notification,
                  void *data)
{
  cerr << "EVENT: Received new event" << endl;
  char name[EVENT_BUFFER_SIZE];
  char type[EVENT_BUFFER_SIZE];

  if (event_notification_get_string(handle, notification, "OBJNAME", name, EVENT_BUFFER_SIZE) == 0)
  {
    cerr << "EVENT: Could not get the object name" << endl;
    return;
  }

  if (event_notification_get_string(handle, notification, "EVENTTYPE", type, EVENT_BUFFER_SIZE) == 0)
  {
    cerr << "EVENT: Could not get the event type" << endl;
    return;
  }

  multimap<string, list<PipeInfo>::iterator>::iterator pos = g::pipes.lower_bound(name);
  multimap<string, list<PipeInfo>::iterator>::iterator limit = g::pipes.upper_bound(name);
  for (; pos != limit; ++pos)
  {
    handleEvent(handle, notification, type, pos->second);
  }
}

void handleEvent(event_handle_t handle,
                 event_notification_t notification,
                 string const & type,
                 list<PipeInfo>::iterator pipe)
{
  if (type == TBDB_EVENTTYPE_UP)
  {
    changeParameter(pipe, Parameter(Parameter::LINK_UP, 1));
  }
  else if (type == TBDB_EVENTTYPE_DOWN)
  {
    changeParameter(pipe, Parameter(Parameter::LINK_UP, 0));
  }
  else if (type == TBDB_EVENTTYPE_MODIFY)
  {
    handleModify(handle, notification, pipe);
  }
  else
  {
    cerr << "EVENT: Uknown link type" << endl;
  }
}

void handleModify(event_handle_t handle,
                  event_notification_t notification,
                  list<PipeInfo>::iterator pipe)
{
  char args[EVENT_BUFFER_SIZE];
  if (event_notification_get_string(handle, notification,
                                    "ARGS", args, EVENT_BUFFER_SIZE) == 0)
  {
    cerr << "EVENT: Could not get event arguments" << endl;
    return;
  }

  istringstream line(args);
  string arg;
  line >> arg;
  while (line)
  {
    Parameter newParameter = parseArg(arg);
    if (newParameter.getType() != Parameter::NOTHING)
    {
      changeParameter(pipe, newParameter);
    }
    line >> arg;
  }
}

Parameter parseArg(string const & arg)
{
  Parameter result;
  size_t equalsPos = arg.find('=');
  if (equalsPos == string::npos)
  {
    cerr << "EVENT: Argument is not in the form of key=value: " << arg << endl;
    return result;
  }
  std::string key = arg.substr(0, equalsPos);
  std::string valueString = arg.substr(equalsPos + 1);
  if (key == "BANDWIDTH" || key == "bandwidth")
  {
    int value = stringToInt(valueString);
    result = Parameter(Parameter::BANDWIDTH, value);
  }
  else if (key == "DELAY" || key == "delay")
  {
    int value = stringToInt(valueString);
    result = Parameter(Parameter::DELAY, value);
  }
  else if (key == "PLR" || key == "plr")
  {
    // TODO: Implement PLR
    cerr << "EVENT: PLR is not yet implmented" << endl;
  }
  else
  {
    cerr << "EVENT: Unknown key: " << key << "=" << valueString << endl;
  }
  return result;
}

void changeParameter(list<PipeInfo>::iterator pipe, Parameter const & newParameter)
{
  Parameter & oldParameter = pipe->parameters[newParameter.getType()];
  if (newParameter.getType() == Parameter::LINK_UP
      && newParameter.getValue() == 1
      && oldParameter.getValue() == 0)
  {
    // Bring the link up
    oldParameter = newParameter;
    pipe->rawPipe.reset();
    map<Parameter::ParameterType, Parameter>::iterator pos = pipe->parameters.begin();
    map<Parameter::ParameterType, Parameter>::iterator limit = pipe->parameters.end();
    for (; pos != limit; ++pos)
    {
      if (pos->second.getType() != Parameter::LINK_UP)
      {
        pipe->rawPipe.resetParameter(pos->second);
      }
    }
  }
  else if (oldParameter.getValue() != newParameter.getValue())
  {
    oldParameter = newParameter;
    if (pipe->parameters[Parameter::LINK_UP].getValue() == 1)
    {
      pipe->rawPipe.resetParameter(newParameter);
    }
  }
}
