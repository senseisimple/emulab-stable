// RootPipe.cc

#include "lib.hh"
#include "RootPipe.hh"

using namespace std;

RootPipe::RootPipe()
  : refCount(1)
{
}

RootPipe::~RootPipe()
{
}

void RootPipe::acquire(void)
{
  ++refCount;
}

void RootPipe::release(void)
{
  --refCount;
  if (refCount <= 0)
  {
    delete this;
  }
}
