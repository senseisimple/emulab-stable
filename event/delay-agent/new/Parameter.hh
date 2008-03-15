// Parameter.hh

// A single discrete parameter which can be modified on a pipe.

#ifndef PARAMETER_HH_DELAY_AGENT_1
#define PARAMETER_HH_DELAY_AGENT_1

class Parameter
{
public:
  enum ParameterType
  {
    NOTHING,
    BANDWIDTH,
    DELAY,
    // This is the loss rate in thousandths. 1000 is 100% loss. 1 is 0.1% loss. Etc.
    LOSS,
    // 1 means reset to link up. 0 means reset to link down.
    LINK_UP
  };
public:
  Parameter(ParameterType newType=NOTHING, int newValue=0);

  ParameterType getType(void) const;
  int getValue(void) const;
private:
  ParameterType type;
  int value;
};

#endif
