/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */
#include "rpc.h"

#define ULXR_INCLUDE_SSL_STUFF

#include <ulxmlrpcpp.h>  // always first header
#include <iostream>
#include <ulxr_tcpip_connection.h>  // first, don't move: msvc #include bug
#include <ulxr_ssl_connection.h> 
#include <ulxr_http_protocol.h> 
#include <ulxr_requester.h>
#include <ulxr_value.h>
#include <ulxr_except.h>
#include <emulab_proxy.h>
#ifdef SSHRPC
#include <ulxr_ssh_connection.h>
#include <ulxr_rfc822_protocol.h>
#endif

//
// It would be too much work to compile the event scheduler proper
// with the c++ compiler (cause main needs to be compiled with c++).
// This is just a stub that calls the realmain in event-sched.c
//
extern "C" int	realmain(int argc, char **argv);
	
int
main(int argc, char **argv)
{
	return realmain(argc, argv);
}

/*
 * Simply save the stuff we need for making the connections later.
 */
static char *CERTPATH;
static char *PATH;
static char *HOST = "localhost";
static int   PORT = 3069;

int RPC_init(char *certpath, char *host, int port)
{
  printf("%s %s %d\n", certpath, host, port);
  
#ifdef SSHRPC
  PATH     = "xmlrpc";
#else
  if (certpath)
    CERTPATH = strdup(certpath);
  else
    CERTPATH = "/usr/testbed/etc/client.pem";
#endif
  HOST     = strdup(host);
  PORT     = port;
  return 0;
}

/*
 * Contact the server and invoke the method. All of the methods we care
 * about using return a string, which we return to the caller after
 * making a copy. Might change this at some point to use real structures
 * once I have the SSL server running. The reason for strings was that
 * the first version used the external SSH XMLRPC client, and it was easier
 * to have it dump plain strings to stdout. 
 */
static int
RPC_invoke(char *pid, char *eid, char *method, emulab::EmulabResponse *er)
{
  try
    {
#ifdef SSHRPC
      const char *user = getenv("USER");
      
      ulxr::SSHConnection conn(user, HOST, PATH);
      ulxr::RFC822Protocol proto(&conn);
      emulab::ServerProxy proxy(&proto);
#else
      ulxr::SSLConnection conn(false, HOST, PORT); 
      ulxr::HttpProtocol proto(&conn, conn.getHostName());
      conn.setCryptographyData("", CERTPATH, CERTPATH);
      emulab::ServerProxy proxy(&proto, false, "/RPC2");
#endif

      *er = proxy.invoke(method,
			emulab::SPA_String, "proj", pid,
			emulab::SPA_String, "exp", eid,
			emulab::SPA_TAG_DONE);

      if (! er->isSuccess()){
	  ULXR_CERR << "SSL_waitforactive failed: "
		    << er->getOutput()
		    << std::endl;
	  return -1;
      }
      return 0;
    }
  catch(ulxr::Exception &ex)
    {
      ULXR_COUT << ULXR_PCHAR("Error occured: ") <<
	ULXR_GET_STRING(ex.why()) << std::endl;
      return -1;
    }
  catch(...)
    {
      ULXR_COUT << ULXR_PCHAR("unknown Error occured.\n");
      return -1;
    }
  return 0;
}

int
RPC_waitforactive(char *pid, char *eid)
{
  emulab::EmulabResponse er;

  return RPC_invoke(pid, eid, "experiment.waitforactive", &er);
}

int
RPC_agentlist(char *pid, char *eid)
{
  emulab::EmulabResponse er;
  int i, foo = RPC_invoke(pid, eid, "experiment.event_agentlist", &er);

  if (foo)
    return foo;

  ulxr::Array agents = (ulxr::Array)er.getValue();

  for (i = 0; i < agents.size(); i++) {
    char *vname, *vnode, *nodeid, *ipaddr, *type;
    ulxr::RpcString tmp;
    ulxr::Array agent = (ulxr::Array)agents.getItem(i);
    
    tmp = agent.getItem(0);
    vname = (char *) tmp.getString().c_str();
    tmp = agent.getItem(1);
    vnode = (char *) tmp.getString().c_str();
    tmp = agent.getItem(2);
    nodeid = (char *) tmp.getString().c_str();
    tmp = agent.getItem(3);
    ipaddr = (char *) tmp.getString().c_str();
    tmp = agent.getItem(4);
    type = (char *) tmp.getString().c_str();

    if (AddAgent(vname, vnode, nodeid, ipaddr, type) < 0) {
      return -1;
    }
  }

  return 0;
}

int
RPC_grouplist(char *pid, char *eid)
{
  emulab::EmulabResponse er;
  int i, foo = RPC_invoke(pid, eid, "experiment.event_grouplist", &er);
  
  if (foo)
    return foo;

  ulxr::Array groups = (ulxr::Array)er.getValue();

  for (i = 0; i < groups.size(); i++) {
    char *groupname, *agentname;
    ulxr::RpcString tmp;
    ulxr::Array group = (ulxr::Array)groups.getItem(i);
    
    tmp = group.getItem(0);
    groupname = (char *) tmp.getString().c_str();
    tmp = group.getItem(1);
    agentname = (char *) tmp.getString().c_str();

    if (AddGroup(groupname, agentname) < 0) {
      return -1;
    }
  }
  return 0;
}

int
RPC_eventlist(char *pid, char *eid,
	      event_handle_t handle, address_tuple_t tuple, long basetime)
{
  emulab::EmulabResponse er;
  int i, foo = RPC_invoke(pid, eid, "experiment.event_eventlist", &er);
  
  if (foo)
    return foo;

  ulxr::Array events = (ulxr::Array)er.getValue();

  for (i = 0; i < events.size(); i++) {
    char *exidx, *extime, *objname, *objtype, *evttype, *exargs;
    ulxr::RpcString tmp;
    ulxr::Array event = (ulxr::Array)events.getItem(i);
    
    tmp = event.getItem(0);
    exidx = (char *) tmp.getString().c_str();
    tmp = event.getItem(1);
    extime = (char *) tmp.getString().c_str();
    tmp = event.getItem(2);
    objname = (char *) tmp.getString().c_str();
    tmp = event.getItem(3);
    objtype = (char *) tmp.getString().c_str();
    tmp = event.getItem(4);
    evttype = (char *) tmp.getString().c_str();
    tmp = event.getItem(5);
    exargs = (char *) tmp.getString().c_str();

    if (AddEvent(handle, tuple, basetime, exidx,
		 extime, objname, exargs, objtype, evttype) < 0) {
      return -1;
    }
  }
  return 0;
}

