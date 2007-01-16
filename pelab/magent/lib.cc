// lib.cc

#include "lib.h"
#include "log.h"
#include "saveload.h"
#include "ConnectionModel.h"
#include "Connection.h"
#include "CommandInput.h"
#include "CommandOutput.h"
#include "TrafficModel.h"

using namespace std;


namespace global
{
  int connectionModelArg = 0;
  unsigned short peerUdpServerPort = 0;
  unsigned short peerServerPort = 0;
  unsigned short monitorServerPort = 0;
  bool doDaemonize = false;
  int replayArg = NO_REPLAY;
  int replayfd = -1;
  list<int> replaySensors;

  int peerAccept = -1;
  string interface;
  auto_ptr<ConnectionModel> connectionModelExemplar;
  list< pair<int, string> > peers;

  map<ElabOrder, Connection> connections;
  // A connection is in this map only if it is currently connected.
  map<PlanetOrder, Connection *> planetMap;

  fd_set readers;
  int maxReader = -1;

  std::auto_ptr<CommandInput> input;
  std::auto_ptr<CommandOutput> output;

  int logFlags =  LOG_EVERYTHING /*& ~SENSOR_COMPLETE*/;

  // The versionless pre-history was '0'. We can be backwards
  // compatible by taking advantage of the fact that the old headers
  // has a char 'transport' that was always set to 0 for
  // TCP_CONNECTION.
  const unsigned char CONTROL_VERSION = 1;
}

size_t PacketInfo::census(void) const
{
  savePacket(NULL, *this);
  return static_cast<size_t>(getLastSaveloadSize());
}


void setDescriptor(int fd)
{
  if (fd > -1 && fd < FD_SETSIZE)
  {
    FD_SET(fd, &(global::readers));
    if (fd > global::maxReader)
    {
      global::maxReader = fd;
    }
  }
  else
  {
    logWrite(ERROR, "Invalid descriptor sent to setDescriptor: "
             "%d (FDSET_SIZE=%d)", fd, FD_SETSIZE);
  }
}

void clearDescriptor(int fd)
{
  if (fd > -1 && fd < FD_SETSIZE)
  {
    FD_CLR(fd, &(global::readers));
    if (fd > global::maxReader)
    {
      global::maxReader = fd;
    }
  }
  else
  {
    logWrite(ERROR, "Invalid descriptor sent to clearDescriptor: "
             "%d (FDSET_SIZE=%d)", fd, FD_SETSIZE);
  }
}

string ipToString(unsigned int ip)
{
  struct in_addr address;
  address.s_addr = ip;
  return string(inet_ntoa(address));
}

int createServer(int port, string const & debugString)
{
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
  {
    logWrite(ERROR, "socket(): %s: %s", debugString.c_str(), strerror(errno));
    return -1;
  }

  int doesReuse = 1;
  int error = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &doesReuse,
                         sizeof(doesReuse));
  if (error == -1)
  {
    logWrite(ERROR, "setsockopt(SO_REUSEADDR): %s: %s", debugString.c_str(),
             strerror(errno));
    close(fd);
    return -1;
  }

  int forcedSize = 262144;
  error = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &forcedSize,
                     sizeof(forcedSize));
  if (error == -1)
  {
    logWrite(ERROR, "Failed to set receive buffer size: %s",
             strerror(errno));
    close(fd);
    return -1;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  error = bind(fd, reinterpret_cast<struct sockaddr *>(&address),
               sizeof(address));
  if (error == -1)
  {
    logWrite(ERROR, "bind(): %s: %s", debugString.c_str(), strerror(errno));
    close(fd);
    return -1;
  }

  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
  {
    logWrite(ERROR, "fcntl(F_GETFL): %s: %s", debugString.c_str(),
             strerror(errno));
    close(fd);
    return -1;
  }

  error = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  if (error == -1)
  {
    logWrite(ERROR, "fcntl(F_SETFL): %s: %s", debugString.c_str(),
             strerror(errno));
    close(fd);
    return -1;
  }

  error = listen(fd, 4);
  if (error == -1)
  {
    logWrite(ERROR, "listen(): %s: %s", debugString.c_str(), strerror(errno));
    close(fd);
    return -1;
  }

  setDescriptor(fd);
  return fd;
}

int acceptServer(int acceptfd, struct sockaddr_in * remoteAddress,
                 string const & debugString)
{
  struct sockaddr_in stackAddress;
  struct sockaddr_in * address;
  socklen_t addressSize = sizeof(struct sockaddr_in);
  if (remoteAddress == NULL)
  {
    address = &stackAddress;
  }
  else
  {
    address = remoteAddress;
  }

  int resultfd = accept(acceptfd,
                        reinterpret_cast<struct sockaddr *>(address),
                        &addressSize);
  if (resultfd == -1)
  {
    if (errno != EINTR && errno != EWOULDBLOCK && errno != ECONNABORTED
        && errno != EPROTO)
    {
      logWrite(EXCEPTION, "accept(): %s: %s", debugString.c_str(),
               strerror(errno));
    }
    return -1;
  }

  int flags = fcntl(resultfd, F_GETFL, 0);
  if (flags == -1)
  {
    logWrite(EXCEPTION, "fcntl(F_GETFL): %s: %s", debugString.c_str(),
             strerror(errno));
    close(resultfd);
    return -1;
  }

  int error = fcntl(resultfd, F_SETFL, flags | O_NONBLOCK);
  if (error == -1)
  {
    logWrite(EXCEPTION, "fcntl(F_SETFL): %s: %s", debugString.c_str(),
             strerror(errno));
    close(resultfd);
    return -1;
  }

  setDescriptor(resultfd);
  return resultfd;
}


// Returns true on success
bool replayWrite(char * source, int size)
{
  bool result = true;
  if (size == 0) {
      return true;
  }
  int error = write(global::replayfd, source, size);
  if (error <= 0)
  {
    if (error == 0)
    {
      logWrite(EXCEPTION, "replayfd was closed unexpectedly");
    }
    else if (error == -1)
    {
      logWrite(EXCEPTION, "Error writing replay output: %s", strerror(errno));
    }
    result = false;
  }
  return result;
}

void replayWriteCommand(char * head, char * body, unsigned short bodySize)
{
  bool success = true;
  success = replayWrite(head, Header::headerSize);
  if (success)
  {
    replayWrite(body, bodySize);
  }
}

void replayWritePacket(PacketInfo * packet)
{
  Header head;
  head.type = packet->packetType;
  head.size = packet->census();
  head.key = packet->elab;
  char headBuffer[Header::headerSize];
  saveHeader(headBuffer, head);

  bool success = replayWrite(headBuffer, Header::headerSize);
  if (success)
  {
    char *packetBuffer;
    packetBuffer = static_cast<char*>(malloc(head.size));
    //logWrite(REPLAY,"Making a packet buffer of size %d",head.size);
    char* endptr = savePacket(& packetBuffer[0], *packet);
    // find out how many bytes were written
    int writtensize = (endptr - packetBuffer);
    if (writtensize != head.size) {
        logWrite(ERROR,"replayWritePacket(): Made packet save buffer of size "
                       "%d, but wrote %d", head.size, writtensize);
    }
    replayWrite(& packetBuffer[0], head.size);
    free(packetBuffer);
  }
}
