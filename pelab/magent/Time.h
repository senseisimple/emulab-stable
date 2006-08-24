// Time.h

#ifndef TIME_H_STUB_2
#define TIME_H_STUB_2

class Time
{
public:
  Time();
  Time(struct timeval const & newData);
  long long toMilliseconds(void) const;
  double toDouble(void) const;
  struct timeval * getTimeval(void);
  struct timeval const * getTimeval(void) const;
  Time operator+(int const & right) const;
  Time operator-(Time const & right) const;
  bool operator<(Time const & right) const;
  bool operator==(Time const & right) const;
  bool operator!=(Time const & right) const;
private:
  struct timeval data;
};

Time getCurrentTime(void);

#endif
