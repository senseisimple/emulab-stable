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
  long usec = data.tv_sec - right.data.tv_usec;
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
