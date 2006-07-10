// TrafficModel.h

#ifndef TRAFFIC_MODEL_H_STUB_2
#define TRAFFIC_MODEL_H_STUB_2

class TrafficWriteCommand;
class ConnectionModel;

class TrafficModel
{
public:
  virtual ~TrafficModel() {}
  virtual std::auto_ptr<TrafficModel> clone(void)=0;
  virtual Time addWrite(TrafficWriteCommand const & newWrite,
                        Time const & deadline)=0;
  virtual WriteResult writeToPeer(ConnectionModel * peer,
                                  Time const & previousTime)=0;
};

#endif
