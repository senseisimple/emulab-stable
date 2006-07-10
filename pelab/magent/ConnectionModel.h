// ConnectionModel.h

#ifndef CONNECTION_MODEL_H_PELAB_2
#define CONNECTION_MODEL_H_PELAB_2

class ConnectionModel
{
public:
  virtual ~ConnectionModel() {}
  virtual std::auto_ptr<ConnectionModel> clone(void)=0;
  virtual void connect(void)=0;
};

#endif
