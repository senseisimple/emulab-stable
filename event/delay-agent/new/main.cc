// main.cc

#include "lib.hh"

using namespace std;

// For getopt
#include <unistd.h>

void setupDefaultParameters(void);
void readArgs(int argc, char * argv[]);
void usage(char * name);
void initLogging(string const & logFile);
// Reads the map file, initializes the pipe and pipeVault data
// structure, and sets up the two subscription strings for events.
void readMapFile(string const & mapFile, string & linkSubscribe, string & resetSubscribe);
// Adds a name to a comma-delimited list.
void addEntry(string & commaList, string const & newName);
void writePidFile(string const & pidFile);
void initEvents(string const & server, string const & port, string const & keyFile,
                string const & linkSubscribe, string const & resetSubscribe);
void subscribeLink(event_handle_t handle, address_tuple_t eventTuple, string const & linkSubscription);
void subscribeReset(event_handle_t handle, address_tuple_t eventTuple, string const & resetSubscription);

int main(int argc, char * argv[])
{
  //TODO: Daemonize
  setupDefaultParameters();
  readArgs(argc, argv);
  return 0;
}

void setupDefaultParameters(void)
{
  g::defaultParameters[Parameter::BANDWIDTH] = Parameter(Parameter::BANDWIDTH, 100000);
  g::defaultParameters[Parameter::DELAY] = Parameter(Parameter::DELAY, 0);
  g::defaultParameters[Parameter::LOSS] = Parameter(Parameter::LOSS, 0);
  g::defaultParameters[Parameter::LINK_UP] = Parameter(Parameter::LINK_UP, 1);
}

void readArgs(int argc, char * argv[])
{
  string server;
  string port;
  string keyFile;
  string linkSubscribe;
  string resetSubscribe;

  string mapFile;
  string logFile;
  string pidFile;
  // Prevent getopt from printing an error message.
  opterr = 0;

  /* get params from the optstring */
  int option = getopt(argc, argv, "s:p:f:dE:l:i:k:j");
  while (option != -1)
  {
    switch (option)
    {
    case 'd':
      g::debug = true;
      break;
    case 's':
      server = optarg;
      break;
    case 'p':
      port = optarg;
      break;
    case 'f':
      mapFile = optarg;
      break;
    case 'l':
      logFile = optarg;
      break;
    case 'i':
      pidFile = optarg;
      break;
    case 'E':
      g::experimentName = optarg;
      break;
    case 'k':
      keyFile = optarg;
      break;
    case 'j':
      // TODO: Figure out what this does
      cerr << "-j not yet implemented" << endl;
//      myvnode = optarg;
      break;
    case '?':
    default:
      usage(argv[0]);
      break;
    }
  }

  /*Check if all params are specified, otherwise, print usage and exit*/
  if(server == "" || mapFile == "" || g::experimentName == "")
      usage(argv[0]);

  initLogging(logFile);
  readMapFile(mapFile, linkSubscribe, resetSubscribe);
  writePidFile(pidFile);
  initEvents(server, port, keyFile, linkSubscribe, resetSubscribe);
}

void usage(char * name)
{
  cerr << "Usage: " << name << " -E proj/exp -s server -f mapFile [-d] [-p port] [-l logFile] "
       << "[-i pidFile] [-k keyFile] [-j myvnode???]" << endl;
  exit(-1);
}

void initLogging(string const & /*logFile*/)
{
}

void readMapFile(string const & mapFile, string & linkSubscribe, string & resetSubscribe)
{
  ifstream in(mapFile.c_str(), ios::in);
  string lineString;
  getline(in, lineString);
  while (in)
  {
    istringstream line(lineString);
    string name;
    string type;

    string end;
    string interface;
    string pipeData;
    line >> name >> type;
    if (type == "duplex")
    {
      string secondEnd;
      string secondInterface;
      string secondPipeData;
      line >> end >> secondEnd;
      line >> interface >> secondInterface;
      line >> pipeData >> secondPipeData;

      PipeInfo first(name, end, end == secondEnd, interface, pipeData);
      g::pipeVault.push_front(first);
      g::pipes.insert(make_pair("new-" + first.name, g::pipeVault.begin()));
      g::pipes.insert(make_pair("new-" + first.linkName, g::pipeVault.begin()));
      addEntry(linkSubscribe, "new-" + first.endName);
      addEntry(linkSubscribe, "new-" + first.name);

      PipeInfo second(name, secondEnd, end == secondEnd, secondInterface,
                      secondPipeData);
      g::pipeVault.push_front(second);
      g::pipes.insert(make_pair("new-" + second.name, g::pipeVault.begin()));
      g::pipes.insert(make_pair("new-" + second.linkName, g::pipeVault.begin()));
      if (!second.isLan)
      {
        addEntry(linkSubscribe, "new-" + second.endName);
      }

      addEntry(resetSubscribe, "new-" + first.name);
    }
    else
    {
      line >> end >> interface >> pipeData;
      PipeInfo first(name, end, false, interface, pipeData);
      g::pipeVault.push_front(first);
      g::pipes.insert(make_pair("new-" + first.name, g::pipeVault.begin()));
      g::pipes.insert(make_pair("new-" + first.linkName, g::pipeVault.begin()));
      addEntry(linkSubscribe, "new-" + first.endName);
      addEntry(linkSubscribe, "new-" + first.name);

      addEntry(resetSubscribe, "new-" + first.name);
    }

    getline(in, lineString);
  }
}

// Adds a name to a comma-delimited list.
void addEntry(string & commaList, string const & newName)
{
  if (commaList == "")
  {
    commaList += newName;
  }
  else
  {
    commaList += "," + newName;
  }
}

void writePidFile(string const & pidFile)
{
  string filename = pidFile;
  if (pidFile == "")
  {
    // TODO: Find a platform-independent way of getting pid file path
    filename = "/var/run/delayagent.pid";
//    filename = _PATH_VARRUN + "/delayagent.pid";
  }
  ofstream file(filename.c_str(), ios::out);
  if (file)
  {
    file << getpid();
  }
}

void initEvents(string const & server, string const & port, string const & keyFile,
                string const & linkSubscription, string const & resetSubscription)
{
  string serverString = "elvin://" + server;
  event_handle_t handle;
  if (port != "")
  {
    serverString += ":" + port;
  }
  if (keyFile != "")
  {
    handle = event_register_withkeyfile(const_cast<char *>(server.c_str()), 0,
                                        const_cast<char *>(keyFile.c_str()));
  }
  else
  {
    handle = event_register_withkeyfile(const_cast<char *>(server.c_str()), 0, NULL);
  }
  if (handle == NULL)
  {
    cerr << "Could not register with event system" << endl;
    exit(1);
  }

  address_tuple_t eventTuple = address_tuple_alloc();

  subscribeLink(handle, eventTuple, linkSubscription);
  subscribeReset(handle, eventTuple, resetSubscription);

  address_tuple_free(eventTuple);

  event_main(handle);
}

void subscribeLink(event_handle_t handle, address_tuple_t eventTuple, string const & linkSubscription)
{
  eventTuple->objname = const_cast<char *>(linkSubscription.c_str());
  eventTuple->objtype = TBDB_OBJECTTYPE_LINK;
  eventTuple->eventtype = TBDB_EVENTTYPE_UP "," TBDB_EVENTTYPE_DOWN "," TBDB_EVENTTYPE_MODIFY;
  eventTuple->expt = const_cast<char *>(g::experimentName.c_str());
  eventTuple->host = ADDRESSTUPLE_ANY;
  eventTuple->site = ADDRESSTUPLE_ANY;
  eventTuple->group = ADDRESSTUPLE_ANY;
  eventTuple->scheduler = 0;
  if (event_subscribe(handle, linkCallback, eventTuple, NULL) == NULL)
  {
    cerr << "Could not subscribe to " << eventTuple->eventtype << " event" << endl;
    exit(1);
  }
}

void subscribeReset(event_handle_t handle, address_tuple_t eventTuple, string const & resetSubscription)
{
  eventTuple->objname = const_cast<char *>((resetSubscription + "," + ADDRESSTUPLE_ALL).c_str());
  eventTuple->objtype = TBDB_OBJECTTYPE_LINK;
  eventTuple->eventtype = TBDB_EVENTTYPE_RESET;
  eventTuple->expt = const_cast<char *>(g::experimentName.c_str());
  eventTuple->host = ADDRESSTUPLE_ANY;
  eventTuple->site = ADDRESSTUPLE_ANY;
  eventTuple->group = ADDRESSTUPLE_ANY;
  eventTuple->scheduler = 0;
  if (event_subscribe(handle, resetCallback, eventTuple, NULL) == NULL)
  {
    cerr << "Could not susbcribe to " << eventTuple->eventtype << " event" << endl;
    exit(1);
  }
}

