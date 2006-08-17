// ConnectionModel.h

#ifndef CONNECTION_MODEL_H_PELAB_2
#define CONNECTION_MODEL_H_PELAB_2

class ConnectionModelCommand;

class ConnectionModel
{
public:
  virtual ~ConnectionModel() {}
  virtual std::auto_ptr<ConnectionModel> clone(void)=0;
  virtual void connect(Order & planet)=0;
  virtual void addParam(ConnectionModelCommand const & param)=0;
  // Returns the number of bytes actually written or -1 if there was
  // an error. Errno is not preserved.
  virtual int writeMessage(int size, WriteResult & result)=0;
  virtual bool isConnected(void)=0;

  // These are static members which are used. They are called from
  // switch statements in main.cc
  // static void init(void);
  // static void addNewPeer(fd_set * readable);
  // static void readFromPeers(fd_set * readable);
  // static void packetCapture(fd_set * readable);
};

class ConnectionModelNull : public ConnectionModel
{
public:
  virtual ~ConnectionModelNull() {}
  virtual std::auto_ptr<ConnectionModel> clone(void)
  {
    return std::auto_ptr<ConnectionModel>(new ConnectionModelNull());
  }
  virtual void connect(Order &) {}
  virtual void addParam(ConnectionModelCommand const &) {}

  static void init(void) {}
  static void addNewPeer(fd_set *) {}
  static void readFromPeers(fd_set *) {}
  static void packetCapture(fd_set *) {}
  virtual int writeMessage(int, WriteResult & result)
  {
    result.isConnected = false;
    result.bufferFull = false;
    return 0;
  }
  virtual bool isConnected(void)
  {
    return false;
  }
};

#endif
