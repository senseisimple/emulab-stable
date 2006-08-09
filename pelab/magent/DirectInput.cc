// DirectInput.cc

#include "lib.h"
#include "log.h"
#include "DirectInput.h"

using namespace std;

DirectInput::DirectInput()
  : state(ACCEPTING)
  , monitorAccept(-1)
  , monitorSocket(-1)
  , index(0)
{
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
  {
    logWrite(ERROR, "Unable to generate a command accept socket. "
             "No incoming command connections will ever be accepted: %s",
             strerror(errno));
  }
  else
  {
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(global::monitorServerPort);
    address.sin_addr.s_addr = INADDR_ANY;
    int error = bind(sockfd, reinterpret_cast<struct sockaddr *>(&address),
                 sizeof(struct sockaddr));
    if (error == -1)
    {
      logWrite(ERROR, "Unable to bind a command accept socket. "
             "No incoming command connections will ever be accepted: %s",
             strerror(errno));
      close(sockfd);
    }
    else
    {
      setDescriptor(sockfd);
      monitorAccept = sockfd;
    }
  }
}

DirectInput::~DirectInput()
{
  if (monitorAccept != -1)
  {
    close(monitorAccept);
  }
  if (monitorSocket != -1)
  {
    close(monitorSocket);
  }
}

void DirectInput::nextCommand(fd_set * readable)
{
  if (state == ACCEPTING && monitorAccept != -1
      && FD_ISSET(monitorAccept, readable))
  {
  }
  if (monitorSocket == -1)
  {
    state = ACCEPTING;
  }
  if (state == HEADER && FD_ISSET(monitorSocket, readable))
  {
    int error = recv(monitorSocket, headerBuffer + index,
                     Header::headerSize - index, 0);
    if (error == Header::headerSize - index)
    {
      loadHeader(headerBuffer, &commandHeader);
      index = 0;
      state = BODY;
    }
    else if (error > 0)
    {
      index += error;
    }
    else if (error == 0)
    {
      disconnect();
    }
    else if (error == -1)
    {
      logWrite(EXCEPTION, "Failed read on monitorSocket state HEADER: %s",
               strerror(errno));
    }
  }
  if (state == BODY && FD_ISSET(monitorSocket, readable))
  {
    int error = recv(monitorSocket, bodyBuffer + index,
                     commandHeader.size - index, 0);
    if (error == commandHeader.size - index)
    {
      currentCommand = loadCommand(&commandHeader, bodyBuffer);
      index = 0;
      state = HEADER;
    }
    else if (error > 0)
    {
      index += error;
    }
    else if (error == 0)
    {
      disconnect();
    }
    else if (error == -1)
    {
      logWrite(EXCEPTION, "Failed read on monitorSocket state BODY: %s",
               strerror(errno));
    }
  }
}

int DirectInput::getMonitorSocket(void)
{
  return monitorSocket;
}

void DirectInput::disconnect(void)
{
  if (monitorSocket != -1)
  {
    clearDescriptor(monitorSocket);
    close(monitorSocket);
  }
  index = 0;
  state = ACCEPTING;
}

