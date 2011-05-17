/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

// main.cc

#include <cassert>
#include <cmath>
#include <ctime>

#include <string>
#include <map>
#include <list>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <sys/time.h>

using namespace std;

// For getopt
#include <unistd.h>

#include "event.h"
#include "tbdefs.h"

enum { EVENT_BUFFER_SIZE = 50 };

namespace g
{
  std::string experimentName;
  bool debug = false;
}

void readArgs(int argc, char * argv[]);
void usage(char * name);
// Reads the map file, initializes the pipe and pipeVault data
// structure, and sets up the two subscription strings for events.
void writePidFile(string const & pidFile);
void initEvents(string const & server, string const & port,
                string const & keyFile, string const & subscription,
                string const & group);
void subscribe(event_handle_t handle, address_tuple_t eventTuple,
               string const & subscription, string const & group);
void callback(event_handle_t handle,
              event_notification_t notification, void *data);

int main(int argc, char * argv[])
{
  cerr << "Beginning program" << endl;
  readArgs(argc, argv);
  return 0;
}

void readArgs(int argc, char * argv[])
{
  cerr << "Processing arguments" << endl;
  string server;
  string port;
  string keyFile;
  string subscription;
  string group;

  // Prevent getopt from printing an error message.
  opterr = 0;

  /* get params from the optstring */
  char const * argstring = "s:p:dE:k:u:g:";
  int option = getopt(argc, argv, argstring);
  while (option != -1)
  {
    switch (option)
    {
    case 'd':
      g::debug = true;
      pubsub_debug = 1;
      break;
    case 's':
      server = optarg;
      break;
    case 'p':
      port = optarg;
      break;
    case 'E':
      g::experimentName = optarg;
      break;
    case 'k':
      keyFile = optarg;
      break;
    case 'u':
      subscription = optarg;
      break;
    case 'g':
      group = optarg;
      break;
    case '?':
    default:
      usage(argv[0]);
      break;
    }
    option = getopt(argc, argv, argstring);
  }

  /*Check if all params are specified, otherwise, print usage and exit*/
  if(server == "" || g::experimentName == "")
      usage(argv[0]);


  initEvents(server, port, keyFile, subscription, group);
}

void usage(char * name)
{
  cerr << "Usage: " << name << " -E proj/exp -s server [-d] [-p port] "
       << "[-i pidFile] [-k keyFile] [-u subscription] [-g group]" << endl;
  exit(-1);
}

void initEvents(string const & server, string const & port,
                string const & keyFile, string const & subscription,
                string const & group)
{
  cerr << "Initializing event system" << endl;
  string serverString = "elvin://" + server;
  event_handle_t handle;
  if (port != "")
  {
    serverString += ":" + port;
  }
  cerr << "Server string: " << serverString << endl;
  if (keyFile != "")
  {
    handle = event_register_withkeyfile(const_cast<char *>(serverString.c_str()), 0,
                                        const_cast<char *>(keyFile.c_str()));
  }
  else
  {
    handle = event_register_withkeyfile(const_cast<char *>(serverString.c_str()), 0, NULL);
  }
  if (handle == NULL)
  {
    cerr << "Could not register with event system" << endl;
    exit(1);
  }

  address_tuple_t eventTuple = address_tuple_alloc();

  subscribe(handle, eventTuple, subscription, group);

  address_tuple_free(eventTuple);

  event_main(handle);
}

void subscribe(event_handle_t handle, address_tuple_t eventTuple,
               string const & subscription, string const & group)
{
  string name = subscription;
  if (group != "")
  {
    name += "," + group;
  }
  cerr << "Link subscription names: " << name << endl;
  eventTuple->objname = const_cast<char *>(name.c_str());
//  eventTuple->objtype = TBDB_OBJECTTYPE_LINK;
  eventTuple->objtype = ADDRESSTUPLE_ANY;
//  eventTuple->eventtype = TBDB_EVENTTYPE_MODIFY;
  eventTuple->eventtype = ADDRESSTUPLE_ANY;
  eventTuple->expt = const_cast<char *>(g::experimentName.c_str());
  eventTuple->host = ADDRESSTUPLE_ANY;
  eventTuple->site = ADDRESSTUPLE_ANY;
  eventTuple->group = ADDRESSTUPLE_ANY;
  eventTuple->scheduler = 1;
  if (event_subscribe(handle, callback, eventTuple, NULL) == NULL)
  {
    cerr << "Could not subscribe to " << eventTuple->eventtype << " event" << endl;
    exit(1);
  }
}

void callback(event_handle_t handle,
              event_notification_t notification,
              void * data)
{
  char name[EVENT_BUFFER_SIZE];
  char type[EVENT_BUFFER_SIZE];
  char args[EVENT_BUFFER_SIZE];
  struct timeval basicTime;
  gettimeofday(&basicTime, NULL);
  double floatTime = basicTime.tv_sec + basicTime.tv_usec/1000000.0;
  ostringstream timeStream;
  timeStream << setprecision(3) << setiosflags(ios::fixed | ios::showpoint);
  timeStream << floatTime;
  string timestamp = timeStream.str();

  if (event_notification_get_string(handle, notification, "OBJNAME", name, EVENT_BUFFER_SIZE) == 0)
  {
    cerr << timestamp << ": ERROR: Could not get the object name" << endl;
    return;
  }

  if (event_notification_get_string(handle, notification, "EVENTTYPE", type, EVENT_BUFFER_SIZE) == 0)
  {
    cerr << timestamp << ": ERROR: Could not get the event type" << endl;
    return;
  }

  if (event_notification_get_string(handle, notification,
                                    "ARGS", args, EVENT_BUFFER_SIZE) == 0)
  {
    cerr << timestamp << ": ERROR: Could not get event arguments" << endl;
    return;
  }

  istringstream line(args);
  int sequence = 0;
  line >> sequence;

  cout << "RECEIVE: " << name << " " << sequence << " " << timestamp << endl;
}
