/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// CircularTraffic.h

#ifndef CIRCULAR_TRAFFIC_H_STUB_2
#define CIRCULAR_TRAFFIC_H_STUB_2

#include "TrafficModel.h"

class CircularTraffic : public TrafficModel
{
public:
  enum { EXPIRATION_TIME = 500 }; // in milliseconds
public:
  CircularTraffic();
  virtual ~CircularTraffic();
  virtual std::auto_ptr<TrafficModel> clone(void);
  virtual Time addWrite(TrafficWriteCommand const & newWrite,
                        Time const & deadline);
  virtual void writeToPeer(ConnectionModel * peer,
                           Time const & previousTime,
                           WriteResult & result);
private:
  // Quick function to treat the writes list as though it were
  // circular
  std::list<TrafficWriteCommand>::iterator advance(
    std::list<TrafficWriteCommand>::iterator old);
private:
  std::list<TrafficWriteCommand>::iterator current;
  std::list<TrafficWriteCommand> writes;
  unsigned int nextWriteSize;
};

#endif
