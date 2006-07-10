// KernelTcp.h

#ifndef KERNEL_TCP_H_PELAB_2
#define KERNEL_TCP_H_PELAB_2

#include "ConnectionModel.h"


enum ConnectionState
{
    DISCONNECTED,
    SOCKETED,
    CONNECTED
};

class KernelTcp : public ConnectionModel
{
    ConnectionState state;
    int peersock;
    int sendBufferSize;
    int receiveBufferSize;
    int maxSegmentSize;
    bool useNagles;
};

void kernelTcp_init(void);
void kernelTcp_addNewPeer(fd_set * readable);
void kernelTcp_readFromPeers(fd_set * readable);
void kernelTcp_packetCapture(fd_set * readable);

#endif
