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
  logWrite(COMMAND_INPUT, "Creating the command accept socket");
  monitorAccept = createServer(global::monitorServerPort,
                               "Command accept socket (No incoming command "
                               "connections will be accepted.");
}

DirectInput::~DirectInput()
{
  logWrite(COMMAND_INPUT, "Destroying DirectInput()");
  if (monitorAccept != -1)
  {
    clearDescriptor(monitorAccept);
    close(monitorAccept);
  }
  if (monitorSocket != -1)
  {
    clearDescriptor(monitorSocket);
    close(monitorSocket);
  }
}

void DirectInput::nextCommand(fd_set * readable)
{
  currentCommand.reset(NULL);
//  logWrite(COMMAND_INPUT, "Before ACCEPTING check");
  if (state == ACCEPTING && monitorAccept != -1
      && FD_ISSET(monitorAccept, readable))
  {
    logWrite(COMMAND_INPUT, "Creating the command socket");
    monitorSocket = acceptServer(monitorAccept, NULL,
                                 "Command socket (Incoming command "
                                 "connection was not accepted)");
    if (monitorSocket != -1)
    {
      state = HEADER;
    }
  }
//  logWrite(COMMAND_INPUT, "Before HEADER check");
  if (state == HEADER && monitorSocket != -1
      && FD_ISSET(monitorSocket, readable))
  {
    int error = recv(monitorSocket, headerBuffer + index,
                     Header::headerSize - index, 0);
    if (error == Header::headerSize - index)
    {
//      logWrite(COMMAND_INPUT, "Finished reading a command header");
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
      logWrite(EXCEPTION, "Read count of 0 returned (state BODY): %s",
               strerror(errno));
      disconnect();
    }
    else if (error == -1)
    {
      logWrite(EXCEPTION, "Failed read on monitorSocket "
               "(state HEADER) index=%d: %s", index,
               strerror(errno));
    }
  }
//  logWrite(COMMAND_INPUT, "Before BODY check");
  if (state == BODY && monitorSocket != -1
      && FD_ISSET(monitorSocket, readable))
  {
    int error = 0;
    if (index != commandHeader.size)
    {
      error = recv(monitorSocket, bodyBuffer + index,
                   commandHeader.size - index, 0);
    }
    if (error == commandHeader.size - index)
    {
//      logWrite(COMMAND_INPUT, "Finished reading a command: CHECKSUM=%d",
//               checksum());
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
      logWrite(EXCEPTION, "Read count of 0 returned (state BODY): %s",
               strerror(errno));
      disconnect();
    }
    else if (error == -1)
    {
      logWrite(EXCEPTION, "Failed read on monitorSocket "
               "(state BODY) index=%d: %s", index,
               strerror(errno));
    }
  }
//  logWrite(EXCEPTION, "monitorSocket: %d, FD_ISSET: %d", monitorSocket,
//           FD_ISSET(monitorSocket, & global::readers));
}

int DirectInput::getMonitorSocket(void)
{
  return monitorSocket;
}

void DirectInput::disconnect(void)
{
  logWrite(COMMAND_INPUT, "Disconnecting from command socket");
  if (monitorSocket != -1)
  {
    clearDescriptor(monitorSocket);
    close(monitorSocket);
    monitorSocket = -1;
  }
  index = 0;
  state = ACCEPTING;
  currentCommand.reset(NULL);
}

int DirectInput::checksum(void)
{
  int flip = 1;
  int total = 0;
  int i = 0;
  for (i = 0; i < Header::headerSize; ++i)
  {
    total += (headerBuffer[i] & 0xff) * flip;
//    flip *= -1;
  }
  for (i = 0; i < commandHeader.size; ++i)
  {
    total += (bodyBuffer[i] & 0xff) * flip;
//    flip *= -1;
  }
  return total;
}
