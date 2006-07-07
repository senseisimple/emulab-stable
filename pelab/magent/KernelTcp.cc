// KernelTcp.cc

#include "lib.h"
#include "KernelTcp.h"

void kernelTcp_addNewPeer(fd_set * readable)
{
  if (global::peerAccept != -1
      && FD_ISSET(global::peerAccept, readable))
  {
    struct sockaddr_in remoteAddress;
    socklen_t addressSize = sizeof(remoteAddress);
    int fd = accept(global::peerAccept, &remoteAddress, &addressSize);
    if (fd != -1)
    {
      // Add the peer.
      int flags = fctl(fd, F_GETFL);
      if (flags != -1)
      {
        int error = fctl(fd, F_SETFL, flags | O_NONBLOCK);
        if (error != -1)
        {
          global::peers.push_back(
            make_pair(fd, ipToString(remoteAddress.sin_addr.s_addr)));
          addDescriptor(fd);
          logWrite(PEER_CYCLE,
                   "Peer connection %d from %s was accepted normally.",
                   global::peers.back().first, global::peers.back().second);
        }
        else
        {
          logWrite(EXCEPTION, "fctl(F_SETFL) failed: %s", strerror(errno));
          close(fd);
        }
      }
      else
      {
        logWrite(EXCEPTION, "fctl(F_GETFL) failed: %s", strerror(errno));
        close(fd);
      }
    }
    else
    {
      logWrite(EXCEPTION, "accept() called on a peer connection failed: %s",
               strerror(errno));
    }
  }
}

void kernelTcp_readFromPeers(fd_set * readable)
{
  list<int>::iterator pos = global::peers.begin();
  while (pos != peers.end())
  {
    if (FD_ISSET(pos->first, readable))
    {
      static const int bufferSize = 8096;
      static char buffer[bufferSize];
      int size = read(pos->first, buffer, bufferSize);
      if (size == 0)
      {
        logWrite(PEER_CYCLE,
                 "Peer connection %d from %s is closing normally.",
                 pos->first, pos->second);
        close(pos->first);
        list<int>::iterator temp = pos;
        ++pos;
        global::peers.erase(temp);
      }
      else if (size == -1 && errno != EAGAIN && errno != EINTR)
      {
        logWrite(EXCEPTION,
                 "Failed to read peer connection %d from %s so "
                 "I'm shutting it down: %s", pos->first, pos->second,
                 strerror(errno));
        close(pos->first);
        list<int>::iterator temp = pos;
        ++pos;
        global::peers.erase(temp);
      }
      else
      {
        ++pos;
      }
    }
    else
    {
      ++pos;
    }
  }
}
