// RootPipe.hh

// This is the abstract base class for the pipe interface. This uses a
// reference-counted implementation, so 'RootPipe's should never refer
// to each other.

#ifndef ROOT_PIPE_HH_DELAY_AGENT_1
#define ROOT_PIPE_HH_DELAY_AGENT_1

class RootPipe
{
public:
  RootPipe();
  virtual ~RootPipe();

  // A new reference has been created which refers to this object.
  void acquire(void);
  // A current reference has been destroyed. Possibly destroys this
  // object if the count goes to 0.
  void release(void);
public:
  // Reset all parameters to their default values.
  virtual void reset(void)=0;
  // Set a particular parameter to a new value. This is used for
  // parameters which have a single discrete value which is discretely
  // set and reset.
  virtual void resetParameter(Parameter const & newParameter)=0;
private:
  mutable int refCount;
};

#endif
