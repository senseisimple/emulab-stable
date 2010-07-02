// Parameter.cc

#include "lib.hh"
#include "Parameter.hh"

using namespace std;

Parameter::Parameter(ParameterType newType, int newValue)
  : type(newType)
  , value(newValue)
{
}

Parameter::ParameterType Parameter::getType(void) const
{
  return type;
}

int Parameter::getValue(void) const
{
  return value;
}
