// CircularTraffic.h

#ifndef CIRCULAR_TRAFFIC_H_STUB_2
#define CIRCULAR_TRAFFIC_H_STUB_2

#include "TrafficModel.h"

class CircularTraffic : public TrafficModel
{
public:
  enum { DEFAULT_SIZE = 20 };
public:
  CircularTraffic();
  virtual ~CircularTraffic();
  virtual std::auto_ptr<TrafficModel> clone(void);
  virtual Time addWrite(TrafficWriteCommand const & newWrite,
                        Time const & deadline);
  virtual WriteResult writeToPeer(ConnectionModel * peer,
                                  Time const & previousTime);
private:
  int begin;
  int usedCount;
  int current;
  std::vector<TrafficWriteCommand> writes;
};

#endif
