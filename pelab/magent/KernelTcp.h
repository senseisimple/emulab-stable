// KernelTcp.h

#ifndef KERNEL_TCP_H_PELAB_2
#define KERNEL_TCP_H_PELAB_2

#include "ConnectionModel.h"

enum ConnectionState
{
  DISCONNECTED,
  CONNECTED
};

class KernelTcp : public ConnectionModel
{
public:
  KernelTcp();
  virtual ~KernelTcp();
  virtual std::auto_ptr<ConnectionModel> clone(void);
  virtual void connect(Order & planet);
  virtual void addParam(ConnectionModelCommand const & param);
  virtual int writeMessage(int size, WriteResult & result);
  virtual bool isConnected(void);

  int getSock(void) const;
private:
  ConnectionState state;
  int peersock;
  int sendBufferSize;
  int receiveBufferSize;
  int maxSegmentSize;
  int useNagles;
public:
  static pcap_t * pcapDescriptor;
  static int pcapfd;
  static void init(void);
  static void addNewPeer(fd_set * readable);
  static void readFromPeers(fd_set * readable);
  static void packetCapture(fd_set * readable);
};

#endif
