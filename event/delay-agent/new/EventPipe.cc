// EventPipe.cc

#include "lib.hh"
#include "EventPipe.hh"

using namespace std;

EventPipe::reset()
{
	string command;

	command = "/usr/testbed/bin/tevc -e " +
			projectName + '/' + experimentName +
			" now " + agentName +" MODIFY " +
			"BANDWIDTH=1000 " +
			"DELAY=0 " +
			"PLR=0";

	system(command);
}

EventPipe::resetParameter(Parameter const & newParameter)
{
	string parameterName;
	int parameterValue;
	stringstream ss;
	string valueString;
	string command;

	parameterValue = parameter.getValue();
	switch(newParameter.getType()) {
		case BANDWIDTH:
				parameterString = "BANDWITH";
				ss << parameterValue;
				break;
		case DELAY:	
				parameterString = "DELAY";
				ss << parameterValue;
				break;
		case LOSS:	
				parameterString = "LOSS";
				ss << parameterValue;
				break;
	}
	ss >> valueString;

	command = "/usr/testbed/bin/tevc -e " +
			projectName + '/' + experimentName +
			" now " + agentName + " MODIFY " +
			parameterString + '=' + valueString;

	system(command);
}
