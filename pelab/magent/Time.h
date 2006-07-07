// Time.h

#ifndef TIME_H_STUB_2
#define TIME_H_STUB_2

class Time
{
public:
  Time();
  Time(struct timeval const & newData);
  long long toMilliseconds(void) const;
  Time operator-(Time const & right) const;
  bool operator<(Time const & right) const;
  bool operator==(Time const & right) const;
  bool operator!=(Time const & right) const;
private:
  struct timeval data;
};

#endif
