// Parameter.hh

// A single discrete parameter which can be modified on a pipe.

#ifndef PARAMETER_HH_DELAY_AGENT_1
#define PARAMETER_HH_DELAY_AGENT_1

class Parameter
{
public:
  enum ParameterType
  {
    BANDWIDTH,
    DELAY,
    // This is the loss rate in thousandths. 1000 is 100% loss. 1 is 0.1% loss. Etc.
    LOSS
  };
public:
  Parameter(ParameterType newType=DELAY, int newValue=0);

  ParameterType getType(void) const;
  int getValue(void) const;
private:
  ParameterType type;
  int value;
};

#endif
