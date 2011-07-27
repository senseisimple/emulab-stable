// EventPipe.cc

#include "lib.hh"
#include "EventPipe.hh"

using namespace std;

EventPipe::EventPipe(std::string const & name)
{
  cerr << "Creating Event pipe" << endl;
  agentName = name;
}

EventPipe::~EventPipe()
{
}

void EventPipe::reset(void)
{
  string command;

  command = "/usr/testbed/bin/tevc -e "
    + g::experimentName + " now " + agentName +" UP ";
  cerr << "Sending event: " << command << endl;
  system(command.c_str());

  command = "/usr/testbed/bin/tevc -e "
    + g::experimentName + " now " + agentName +" MODIFY "
    + "BANDWIDTH=100000 "
    + "DELAY=0 "
    + "PLR=0";
  cerr << "Sending event: " << command << endl;
  system(command.c_str());
}

void EventPipe::resetParameter(Parameter const & newParameter)
{
  string parameterString;
  stringstream ss;
  string valueString;
  string command;

  int parameterValue = newParameter.getValue();
  if (newParameter.getType() == Parameter::LINK_UP)
  {
    string action;
    if (parameterValue == 1)
    {
      action = "UP";
    }
    else
    {
      action = "DOWN";
    }
    command = "/usr/testbed/bin/tevc -e "
      + g::experimentName + " now " + agentName + " " + action;
    cerr << "Sending event: " << command << endl;
    system(command.c_str());
  }
  else
  {
    switch(newParameter.getType())
    {
    case Parameter::BANDWIDTH:
      parameterString = "BANDWIDTH";
      ss << parameterValue;
      break;
    case Parameter::DELAY:
      parameterString = "DELAY";
      ss << parameterValue;
      break;
    case Parameter::LOSS:
      parameterString = "PLR";
      ss << parameterValue;
      break;
    default:
      break;
    }
    valueString = ss.str();

    command = "/usr/testbed/bin/tevc -e "
      + g::experimentName + " now "
      + agentName + " MODIFY "
      + parameterString + '=' + valueString;

    cerr << "Sending event: " << command << endl;
    system(command.c_str());
  }
}
