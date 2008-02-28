// Pipe.hh

// This is the container class for RootPipes. A RootPipe should never
// be created on its own. Rather, it should be created on the heap and
// immediately passed to a Pipe class which manages it.
//
// Example: Pipe current(new RemotePipe(some, args, here));
//
// All of the public functions are dispatches to their equivalents in
// the contained RootPipe.

#ifndef PIPE_HH_DELAY_AGENT_1
#define PIPE_HH_DELAY_AGENT_1

class RootPipe;

class Pipe
{
public:
  // Used to create a new Pipe based on a heap-allocated object. Never
  // pass NULL to this function.
  Pipe(RootPipe * newSecret);
  ~Pipe();
  Pipe(Pipe const & right);
  Pipe & operator=(Pipe const & right);
public:
  void reset(void);
  void resetParameter(Parameter const & newParameter);
private:
  Pipe();
private:
  RootPipe * secret;
};

#endif
