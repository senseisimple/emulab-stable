// main.cc

#include "lib.h"
#include "log.h"
#include "CommandInput.h"
#include "CommandOutput.h"
#include "Command.h"
#include "Time.h"
#include "Connection.h"
#include "Sensor.h"
#include "TrafficModel.h"

#include "KernelTcp.h"

#include "DirectInput.h"
#include "TrivialCommandOutput.h"

#include <iostream>

using namespace std;

namespace global
{
  int connectionModelArg = 0;
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

  map<Order, Connection> connections;
  // A connection is in this map only if it is currently connected.
  map<Order, Connection *> planetMap;

  fd_set readers;
  int maxReader = -1;

  std::auto_ptr<CommandInput> input;
  std::auto_ptr<CommandOutput> output;
}

void usageMessage(char *progname);
void processArgs(int argc, char * argv[]);
void init(void);
void mainLoop(void);
void replayLoop(void);
void writeToConnections(multimap<Time, Connection *> & schedule);
void addNewPeer(fd_set * readable);
void readFromPeers(fd_set * readable);
void packetCapture(fd_set * readable);
void exitHandler(int signal);

int main(int argc, char * argv[])
{
  umask(0);
  processArgs(argc, argv);
  init();
  if (global::doDaemonize)
  {
    // Run with no change in directory, and no closing of standard files.
    int error = daemon(1, 1);
    if (error == -1)
    {
      logWrite(ERROR, "Daemonization failed: %s", strerror(errno));
    }
  }
  if (global::replayArg == REPLAY_LOAD)
  {
    replayLoop();
  }
  else
  {
    mainLoop();
  }
  return 0;
}

void usageMessage(char *progname)
{
  cerr << "Usage: " << progname << " [options]" << endl;
  cerr << "  --connectionmodel=<null|kerneltcp> " << endl;
  cerr << "  --peerserverport=<int> " << endl;
  cerr << "  --monitorserverport=<int> " << endl;
  cerr << "  --interface=<iface> " << endl;
  cerr << "  --daemonize " << endl;
  cerr << "  --replay-save=<filename> " << endl;
  cerr << "  --replay-load=<filename> " << endl;
  cerr << "  --replay-sensor=<null|state|packet|delay|min-delay|max-delay|throughput|ewma-throughput>" << endl;
  logWrite(ERROR, "Bad command line argument", global::connectionModelArg);
}

void processArgs(int argc, char * argv[])
{
  // Defaults, in case the user does not pass us explicit values
  global::connectionModelArg = CONNECTION_MODEL_KERNEL;
  global::peerServerPort = 3491;
  global::monitorServerPort = 4200;
  global::interface = "vnet";
  global::doDaemonize = false;
  global::replayArg = NO_REPLAY;
  global::replayfd = -1;

  static struct option longOptions[] = {
    // If you modify this, make sure to modify the shortOptions string below
    // too.
    {"connectionmodel",   required_argument, NULL, 'c'},
    {"peerserverport",    required_argument, NULL, 'p'},
    {"monitorserverport", required_argument, NULL, 'm'},
    {"interface",         required_argument, NULL, 'i'},
    {"daemonize",         no_argument      , NULL, 'd'},
    {"replay-save",       required_argument, NULL, 's'},
    {"replay-load",       required_argument, NULL, 'l'},
    {"replay-sensor",     required_argument, NULL, 'n'},
    // Required so that getopt_long can find the end of the list
    {NULL, 0, NULL, 0}
  };

  string shortOptions = "c:p:m:i:ds:l:";

  // Not used
  int longIndex;

  // Loop until all options have been handled
  while (true)
  {
    int ch =
      getopt_long(argc, argv, shortOptions.c_str(), longOptions, &longIndex);

    // Detect end of options
    if (ch == -1)
    {
      break;
    }

    int argIntVal;

    // Make a copy of optarg that's more c++-y.
    string optArg;
    if (optarg != NULL)
    {
        optArg = optarg;
    }

    switch ((char) ch)
    {
    case 'c':
      if (optArg == "null") {
        global::connectionModelArg = CONNECTION_MODEL_NULL;
      }
      else if (optArg == "kerneltcp")
      {
        global::connectionModelArg = CONNECTION_MODEL_KERNEL;
      }
      else
      {
        usageMessage(argv[0]);
        exit(1);
      }
      break;
    case 'p':
      if (sscanf(optarg,"%i",&argIntVal))
      {
        usageMessage(argv[0]);
        exit(1);
      }
      else
      {
        global::peerServerPort = argIntVal;
      }
      break;
    case 'm':
      if (sscanf(optarg,"%i",&argIntVal))
      {
        usageMessage(argv[0]);
        exit(1);
      }
      else
      {
        global::monitorServerPort = argIntVal;
      }
      break;
    case 'i':
      global::interface = optArg;
      break;
    case 'd':
      global::doDaemonize = true;
      break;
    case 's':
      if (global::replayArg == NO_REPLAY)
      {
        global::replayfd = open(optarg, O_WRONLY | O_CREAT | O_TRUNC,
                                S_IRWXU | S_IRWXG | S_IRWXO);
        if (global::replayfd != -1)
        {
          global::replayArg = REPLAY_SAVE;
        }
        else
        {
          fprintf(stderr,"Error opening replay-save file: %s: %s\n", optarg,
                   strerror(errno));
          exit(1);
        }
      }
      else
      {
        fprintf(stderr, "replay-save option was invoked when replay "
                 "was already set\n");
          exit(1);
      }
      break;
    case 'l':
      if (global::replayArg == NO_REPLAY)
      {
        global::replayfd = open(optarg, O_RDONLY);
        if (global::replayfd != -1)
        {
          global::replayArg = REPLAY_LOAD;
        }
        else
        {
          fprintf(stderr, "Error opening replay-load file: %s: %s\n", optarg,
                   strerror(errno));
          exit(1);
        }
      }
      else
      {
        fprintf(stderr, "replay-load option was invoked when replay "
                 "was already set\n");
          exit(1);
      }
      break;
    case 'n':
      if (optArg == "null")
      {
        global::replaySensors.push_back(NULL_SENSOR);
      }
      else if (optArg == "state")
      {
        global::replaySensors.push_back(STATE_SENSOR);
      }
      else if (optArg == "packet")
      {
        global::replaySensors.push_back(PACKET_SENSOR);
      }
      else if (optArg == "delay")
      {
        global::replaySensors.push_back(DELAY_SENSOR);
      }
      else if (optArg == "min-delay")
      {
        global::replaySensors.push_back(MIN_DELAY_SENSOR);
      }
      else if (optArg == "max-delay")
      {
        global::replaySensors.push_back(MAX_DELAY_SENSOR);
      }
      else if (optArg == "throughput")
      {
        global::replaySensors.push_back(THROUGHPUT_SENSOR);
      }
      else if (optArg == "ewma-throughput")
      {
        global::replaySensors.push_back(EWMA_THROUGHPUT_SENSOR);
      }
      else
      {
        usageMessage(argv[0]);
        exit(1);
      }
      break;
    case '?':
    default:
      usageMessage(argv[0]);
      exit(1);
    }
  }
}

void init(void)
{
  FD_ZERO(&global::readers);
  global::maxReader = -1;

  logInit(stderr, LOG_EVERYTHING, true);

  /*
   * Install a signal handler so that we can catch control-C's, etc.
   */;
  struct sigaction action;
  action.sa_handler = exitHandler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  sigaction(SIGTERM,&action,NULL);
  sigaction(SIGINT,&action,NULL);

  /*
   * Make sure we leave a core dump if we crash
   */
  struct rlimit limit;
  limit.rlim_cur = RLIM_INFINITY;
  limit.rlim_max = RLIM_INFINITY;
  if (setrlimit(RLIMIT_CORE,&limit)) {
      logWrite(ERROR,"Unable to set core dump size!");
  }

  srandom(getpid());
  switch (global::connectionModelArg)
  {
  case CONNECTION_MODEL_NULL:
    ConnectionModelNull::init();
    break;
  case CONNECTION_MODEL_KERNEL:
    KernelTcp::init();
    break;
  default:
    logWrite(ERROR, "connectionModelArg is corrupt at initialization (%d)."
      "Terminating program.", global::connectionModelArg);
  }

  if (global::replayArg == REPLAY_LOAD)
  {
    global::input.reset(new NullCommandInput());
    global::output.reset(new NullCommandOutput());
  }
  else
  {
    global::input.reset(new DirectInput());
    global::output.reset(new TrivialCommandOutput());
  }
}

void mainLoop(void)
{
  multimap<Time, Connection *> schedule;
  fd_set readable;
  FD_ZERO(&readable);

  // Select on file descriptors
  while (true)
  {
//    struct timeval debugTimeout;
//    debugTimeout.tv_sec = 0;
//    debugTimeout.tv_usec = 100000;

    readable = global::readers;
    Time timeUntilWrite;
    struct timeval * waitPeriod;
    Time now = getCurrentTime();
    multimap<Time, Connection *>::iterator nextWrite = schedule.begin();
    if (nextWrite != schedule.end() && now < nextWrite->first)
    {
      timeUntilWrite = nextWrite->first - now;
      waitPeriod = timeUntilWrite.getTimeval();
    }
    else
    {
      // otherwise we want to wait forever.
      timeUntilWrite = Time();
      waitPeriod = NULL;
    }
    int error = select(global::maxReader + 1, &readable, NULL, NULL,
//                       &debugTimeout);
                       waitPeriod);
    if (error == -1)
    {
      switch (errno)
      {
      case EBADF:
      case EINVAL:
        logWrite(ERROR, "Select failed due to improper inputs. "
                 "Unable to continue: %s", strerror(errno));
        exit(1);
        break;
      case EINTR:
      case ENOMEM:
      default:
        logWrite(EXCEPTION, "Select failed. We may not be able to continue. "
                 "Retrying: %s", strerror(errno));
        continue;
        break;
      }
    }

    global::input->nextCommand(&readable);
    Command * current = global::input->getCommand();
    if (current != NULL)
    {
      current->run(schedule);
//      global::input->nextCommand(&readable);
//      current = global::input->getCommand();
    }
    writeToConnections(schedule);
    addNewPeer(&readable);
    readFromPeers(&readable);
    packetCapture(&readable);
  }
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

// Returns true on success
bool replayRead(char * dest, int size)
{
  bool result = true;
  int error = read(global::replayfd, dest, size);
  if (error <= 0)
  {
    if (error == 0)
    {
      logWrite(EXCEPTION, "End of replay input");
    }
    else if (error == -1)
    {
      logWrite(EXCEPTION, "Error reading replay input: %s", strerror(errno));
    }
    result = false;
  }
  return result;
}

void replayLoop(void)
{
  bool done = false;
  char headerBuffer[Header::headerSize];
  Header head;
  vector<char> packetBuffer;
  struct tcp_info kernel;
  struct ip ip;
  list<Option> ipOptions;
  struct tcphdr tcp;
  list<Option> tcpOptions;
  PacketInfo packet;
  map<Order, SensorList> streams;

  done = ! replayRead(headerBuffer, Header::headerSize);
  while (!done)
  {
    loadHeader(headerBuffer, &head);
    packetBuffer.resize(head.size);
    switch(head.type)
    {
    case NEW_CONNECTION_COMMAND:
    {
      map<Order, SensorList>::iterator current =
        streams.insert(make_pair(head.key, SensorList())).first;
      list<int>::iterator pos = global::replaySensors.begin();
      list<int>::iterator limit = global::replaySensors.end();
      SensorCommand tempSensor;
      for (; pos != limit; ++pos)
      {
        tempSensor.type = *pos;
        current->second.addSensor(tempSensor);
      }
    }
      break;
    case DELETE_CONNECTION_COMMAND:
      streams.erase(head.key);
      break;
    case SENSOR_COMMAND:
      done = ! replayRead(& packetBuffer[0], sizeof(int));
      if (!done)
      {
        if (global::replaySensors.empty())
        {
          unsigned int sensorType = 0;
          loadInt(& packetBuffer[0], & sensorType);
          SensorCommand sensor;
          sensor.type = sensorType;
          streams[head.key].addSensor(sensor);
        }
      }
      break;
    case PACKET_INFO_SEND_COMMAND:
    case PACKET_INFO_ACK_COMMAND:
      logWrite(EXCEPTION,"Got a packet: %d",head.type);
      done = ! replayRead(& packetBuffer[0], head.size);
      if (!done)
      {
        loadPacket(& packetBuffer[0], &packet, kernel, ip, tcp, ipOptions,
                   tcpOptions);
        Sensor * sensorHead = streams[head.key].getHead();
        if (sensorHead != NULL)
        {
          sensorHead->capturePacket(&packet);
        }
      }
      break;
    default:
      logWrite(ERROR, "Invalid command type in replay input: %d", head.type);
      done = true;
      break;
    }
    if (!done)
    {
      done = ! replayRead(headerBuffer, Header::headerSize);
    }
  }
}

void writeToConnections(multimap<Time, Connection *> & schedule)
{
  Time now = getCurrentTime();
//  bool done = false;
    // Notify any connection which is due, then erase that entry from
    // the schedule and insert another entry at the new time specified
    // by the connection.
//  while (! schedule.empty() && !done)
  if (! schedule.empty())
  {
    multimap<Time, Connection *>::iterator pos = schedule.begin();
    if (pos->first < now)
    {
      Connection * current = pos->second;
      Time nextTime = current->writeToConnection(pos->first);
      schedule.erase(pos);
      if (nextTime != Time())
      {
        schedule.insert(make_pair(nextTime, current));
      }
    }
//    else
//    {
//      done = true;
//    }
  }
}

void addNewPeer(fd_set * readable)
{
  switch (global::connectionModelArg)
  {
  case CONNECTION_MODEL_NULL:
    ConnectionModelNull::addNewPeer(readable);
    break;
  case CONNECTION_MODEL_KERNEL:
    KernelTcp::addNewPeer(readable);
    break;
  default:
    logWrite(ERROR, "connectionModelArg is corrupt (%d). "
             "No further peers will be accepted.", global::connectionModelArg);
    break;
  }
}

void readFromPeers(fd_set * readable)
{
  switch (global::connectionModelArg)
  {
  case CONNECTION_MODEL_NULL:
    ConnectionModelNull::readFromPeers(readable);
    break;
  case CONNECTION_MODEL_KERNEL:
    KernelTcp::readFromPeers(readable);
    break;
  default:
    logWrite(ERROR, "connectionModelArg is corrupt (%d). "
             "I can no longer read from current peers.",
             global::connectionModelArg);
    break;
  }
}

void packetCapture(fd_set * readable)
{
  switch (global::connectionModelArg)
  {
  case CONNECTION_MODEL_NULL:
    ConnectionModelNull::packetCapture(readable);
    break;
  case CONNECTION_MODEL_KERNEL:
    KernelTcp::packetCapture(readable);
    break;
  default:
    logWrite(ERROR, "connectionModelArg is corrupt (%d). "
             "I can no longer capture packets sent to peers.",
             global::connectionModelArg);
    break;
  }
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

void exitHandler(int signal) {
    logWrite(EXCEPTION,"Killed with signal %i, cleaning up",signal);
    exit(0);
}
