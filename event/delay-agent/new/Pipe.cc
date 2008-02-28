// Pipe.cc

#include "lib.hh"
#include "Pipe.hh"

using namespace std;

Pipe::Pipe(RootPipe * newSecret)
{
  assert(newSecret != NULL);
  secret = newSecret;
}

Pipe::~Pipe()
{
  release(secret);
}

Pipe::Pipe(Pipe const & right)
{
  right.secret->acquire();
  secret = right.secret;
}

Pipe & Pipe::operator=(Pipe const & right)
{
  right.secret->acquire();
  secret.release();
  secret = right.secret;
  return *this;
}

void Pipe::reset(void)
{
  secret->reset();
}

void Pipe::resetParameter(Parameter const & newParameter)
{
  secret->resetParameter(newParameter);
}
