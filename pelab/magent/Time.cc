/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// Time.cc

#include "lib.h"
#include "Time.h"

using namespace std;

Time::Time()
{
  data.tv_sec = 0;
  data.tv_usec = 0;
}

Time::Time(struct timeval const & newData)
{
  data = newData;
}

long long Time::toMilliseconds(void) const
{
  long long result = data.tv_sec * 1000 + data.tv_usec / 1000;
  return result;
}

// Udp - CHANGES - Begin
unsigned long long Time::toMicroseconds(void) const
{
  unsigned long long result_sec = data.tv_sec; 
  unsigned long long result_usec = data.tv_usec;
  unsigned long long result = result_sec*1000000 + result_usec;
  return result;
}
// Udp - CHANGES - End

double Time::toDouble(void) const
{
  double result = data.tv_sec + data.tv_usec / 1000000.0;
  return result;
}

struct timeval * Time::getTimeval(void)
{
  return &data;
}

struct timeval const * Time::getTimeval(void) const
{
  return &data;
}

Time Time::operator+(int const & right) const
{
  Time result;
  result.data.tv_sec = data.tv_sec + right/1000;
  result.data.tv_usec = data.tv_usec + (right%1000)*1000;
  if (result.data.tv_usec < 0)
  {
    --(result.data.tv_sec);
    result.data.tv_usec += 1000000;
  }
  if (result.data.tv_usec >= 1000000)
  {
    ++(result.data.tv_sec);
    result.data.tv_usec -= 1000000;
  }
  return result;
}

Time Time::operator-(Time const & right) const
{
  Time result;
  result.data.tv_sec = data.tv_sec - right.data.tv_sec;
  long usec = data.tv_usec - right.data.tv_usec;
  if (usec < 0)
  {
    --(result.data.tv_sec);
    usec += 1000000;
  }
  result.data.tv_usec = usec;
  return result;
}

bool Time::operator<(Time const & right) const
{
  return make_pair(data.tv_sec, data.tv_usec)
    < make_pair(right.data.tv_sec, right.data.tv_usec);
}

bool Time::operator>(Time const & right) const
{
  return make_pair(data.tv_sec, data.tv_usec)
    > make_pair(right.data.tv_sec, right.data.tv_usec);
}

bool Time::operator==(Time const & right) const
{
  return make_pair(data.tv_sec, data.tv_usec)
    == make_pair(right.data.tv_sec, right.data.tv_usec);
}

bool Time::operator!=(Time const & right) const
{
  return !(*this == right);
}

Time getCurrentTime(void)
{
  Time now;
  gettimeofday(now.getTimeval(), NULL);
  return now;
}
