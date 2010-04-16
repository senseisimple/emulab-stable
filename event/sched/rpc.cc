/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004, 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <sys/param.h>

#include <math.h>
#include <limits.h>
#include <sys/types.h>
#include <assert.h>
#include <pwd.h>

#include <string>
#include <set>
using namespace std;

#include "log.h"
#include "rpc.h"

/**
 * We cache the connection to the server until all of the RPCs have completed
 * so we do not have to reconnect.
 */
struct r_rpc_data rpc_data = {
	NULL,
	BOSSNODE,
	DEFAULT_RPC_PORT,
	-1,
	{ NULL, NULL },
	PTHREAD_MUTEX_INITIALIZER
};

const char *topography_name;

int RPC_init(const char *certpath, const char *host, unsigned short port)
{
	struct passwd *pwd;
	int retval;

	assert(host != NULL);
	assert(strlen(host) > 0);
	
	if ((pwd = getpwuid(getuid())) == NULL) {
		error("uid %d does not exist", getuid());
		retval = -1;
	}
	else if ((retval = pthread_mutex_init(&rpc_data.mutex, NULL)) != 0) {
		error("could not initialize mutex");
		retval = -1;
	}
	else {
		if (certpath == NULL) {
			char buf[BUFSIZ];
			
			snprintf(buf,
				 sizeof(buf),
				 "%s/.ssl/emulab.pem",
				 pwd->pw_dir);
			rpc_data.certpath = strdup(buf);
		}
		if (host != NULL)
			rpc_data.host = host;
		if (port > 0)
			rpc_data.port = port;
		rpc_data.refcount = 0;

		retval = 0;
	}
	
	return retval;
}

int RPC_grab(void)
{
	int retval = 0;

	if (pthread_mutex_lock(&rpc_data.mutex) != 0)
		assert(0);
	
	assert(rpc_data.refcount >= 0);
	
	rpc_data.refcount += 1;
	
	if (pthread_mutex_unlock(&rpc_data.mutex) != 0)
		assert(0);
	
	return retval;
}

void RPC_drop(void)
{
	if (pthread_mutex_lock(&rpc_data.mutex) != 0)
		assert(0);
	
	assert(rpc_data.refcount > 0);

	rpc_data.refcount -= 1;
	if (rpc_data.refcount == 0) {
		if (rpc_data.conn_proto.proto != NULL) {
			delete rpc_data.conn_proto.proto;
			rpc_data.conn_proto.proto = NULL;
			delete rpc_data.conn_proto.conn;
			rpc_data.conn_proto.conn = NULL;
		}
	}
	
	if (pthread_mutex_unlock(&rpc_data.mutex) != 0)
		assert(0);
}

static int
RPC_connect(struct rpc_conn_proto *rcp_out)
{
	ulxr::SSLConnection *sslconn;
	int retval = 0;
	
	assert(rcp_out != NULL);

	if (pthread_mutex_lock(&rpc_data.mutex) != 0)
		assert(0);
	
	if (rpc_data.conn_proto.conn != NULL) {
		*rcp_out = rpc_data.conn_proto;
		rpc_data.conn_proto.conn = NULL;
		rpc_data.conn_proto.proto = NULL;
	}
	else {
		rcp_out->conn = sslconn =
			new ulxr::SSLConnection(false,
						rpc_data.host,
						rpc_data.port);
		sslconn->setTimeout(30 * 60);
		rcp_out->proto =
			new ulxr::HttpProtocol(rcp_out->conn,
					       sslconn->getHostName());
		rcp_out->proto->setPersistent(true);
		sslconn->setCryptographyData("",
					     rpc_data.certpath,
					     rpc_data.certpath);
	}
	
	if (pthread_mutex_unlock(&rpc_data.mutex) != 0)
		assert(0);
}

static void
RPC_disconnect(struct rpc_conn_proto *rcp)
{
	assert(rcp != NULL);
	assert(rcp->proto != NULL);
	assert(rcp->conn != NULL);
	
	if (pthread_mutex_lock(&rpc_data.mutex) != 0)
		assert(0);

	if (rpc_data.conn_proto.conn == NULL) {
		rpc_data.conn_proto = *rcp;
	}
	else {
		delete rcp->proto;
		delete rcp->conn;
	}
	rcp->conn = NULL;
	rcp->proto = NULL;
	
	if (pthread_mutex_unlock(&rpc_data.mutex) != 0)
		assert(0);
}

int
RPC_invoke(char *method,
	   emulab::EmulabResponse *er_out,
	   emulab::spa_attr_t tag,
	   ...)
{
	struct rpc_conn_proto rcp;
	int retval = 0;
	va_list args;

	ULXR_COUT << "Beginning call to " << method << "\n";
	RPC_connect(&rcp);

	va_start(args, tag);
	try
	{
		emulab::ServerProxy proxy(rcp.proto, false, XMLRPC_ROOT);
		
		*er_out = proxy.invoke(method, tag, args);
		
		if (! er_out->isSuccess()){
			ULXR_CERR << "RPC_invoke failed: "
				  << method
				  << " "
				  << er_out->getOutput()
				  << std::endl;
			retval = -1;
		}
	}
	catch(ulxr::Exception &ex)
	{
		ULXR_COUT << ULXR_PCHAR("Error occured: ") <<
			ULXR_GET_STRING(ex.why()) << std::endl;
		retval = -1;
	}
	catch(...)
	{
		ULXR_COUT << ULXR_PCHAR("unknown Error occured.\n");
		retval = -1;
	}
	va_end(args);
	
	RPC_disconnect(&rcp);

	ULXR_COUT << "Finished with call to " << method << "\n";
	return retval;
}

int
RPC_invoke(char *pid, char *eid, char *method, emulab::EmulabResponse *er)
{
	struct rpc_conn_proto rcp;
	int retval = 0;

	assert(pid != NULL);
	assert(eid != NULL);
	assert(method != NULL);
	assert(er != NULL);
	
	ULXR_COUT << "Beginning call to " << method << "\n";
	RPC_connect(&rcp);
	
	try
	{
		emulab::ServerProxy proxy(rcp.proto, false, XMLRPC_ROOT);
		
		*er = proxy.invoke(method,
				   emulab::SPA_String, "proj", pid,
				   emulab::SPA_String, "exp", eid,
				   emulab::SPA_TAG_DONE);
		
		if (! er->isSuccess()){
			ULXR_CERR << "RPC_invoke failed: "
				  << method
				  << " "
				  << er->getOutput()
				  << std::endl;
			retval = -1;
		}
	}
	catch(ulxr::Exception &ex)
	{
		ULXR_COUT << ULXR_PCHAR("Error occured: ") <<
			ULXR_GET_STRING(ex.why()) << std::endl;
		retval = -1;
	}
	catch(...)
	{
		ULXR_COUT << ULXR_PCHAR("unknown Error occured.\n");
		retval = -1;
	}

	RPC_disconnect(&rcp);
	ULXR_COUT << "Finished with call to " << method << "\n";

	return retval;
}

int
RPC_metadata(char *pid, char *eid)
{
	emulab::EmulabResponse er;
	int retval;

	assert(pid != NULL);
	assert(eid != NULL);

	if ((retval = RPC_invoke(pid, eid, "experiment.metadata", &er)) == 0) {
		ulxr::RpcString path;
		ulxr::Array userenv;
		ulxr::Struct md;

		md = (ulxr::Struct)er.getValue();
		path = md.getMember(ULXR_PCHAR("path"));
		SetExpPath(path.getString().c_str());

		userenv = md.getMember(ULXR_PCHAR("user_environment"));
		if (userenv.size() > 0) {
			int lpc;

			for (lpc = 0; lpc < userenv.size(); lpc++) {
				ulxr::RpcString tmp;
				char *name, *value;
				ulxr::Struct ue;
				
				ue = (ulxr::Struct)userenv.getItem(lpc);
				tmp = ue.getMember("name");
				name = (char *)tmp.getString().c_str();
				ue = (ulxr::Struct)userenv.getItem(lpc);
				tmp = ue.getMember("value");
				value = (char *)tmp.getString().c_str();

				if ((retval = AddUserEnv(name, value)) != 0)
					return retval;
			}
		}
	}

	return retval;
}

expt_state_t
RPC_expt_state(char *pid, char *eid)
{
	expt_state_t retval = ES_UNKNOWN;
	emulab::EmulabResponse er;

	assert(pid != NULL);
	assert(strlen(pid) > 0);
	assert(eid != NULL);
	assert(strlen(eid) > 0);

	if (RPC_invoke(pid, eid, "experiment.state", &er) == 0) {
		ulxr::RpcString tmp;
		const char *state;

		tmp = er.getValue();
		state = tmp.getString().c_str();
		if (strcmp(state, "activating") == 0)
			retval = ES_ACTIVATING;
		else if (strcmp(state, "active") == 0)
			retval = ES_ACTIVE;
		else if (strcmp(state, "modify_reswap") == 0)
			retval = ES_MODIFY_RESWAP;
	}

	return retval;
}

int RPC_notifystart(char *pid, char *eid, char *timeline, int set_or_clear)
{
	emulab::EmulabResponse er;
	double time_start = 0.0;
	
	assert(pid != NULL);
	assert(strlen(pid) > 0);
	assert(eid != NULL);
	assert(strlen(eid) > 0);
	assert(timeline != NULL);

	if (set_or_clear) {
		struct timeval tv;
		
		gettimeofday(&tv, NULL);
		time_start = tv.tv_sec + ((double)tv.tv_usec) / 1000000;
	}
	
	return RPC_invoke("experiment.event_time_start",
			  &er,
			  emulab::SPA_String, "pid", pid,
			  emulab::SPA_String, "eid", eid,
			  emulab::SPA_String, "timeline", timeline,
			  emulab::SPA_Double, "time", time_start,
			  emulab::SPA_TAG_DONE);
}

static int
RPC_cameralist(FILE *emcd_config, char *area)
{
	emulab::EmulabResponse er;
	int lpc, rc;
	
	assert(emcd_config != NULL);
	assert(area != NULL);
	
	rc = RPC_invoke("emulab.vision_config",
			&er,
			emulab::SPA_String, "area", area,
			emulab::SPA_TAG_DONE);
	
	if (rc < 0)
		return -1;
	
	ulxr::Array cameras;
	
	cameras = (ulxr::Array)er.getValue();
	for (lpc = 0; lpc < cameras.size(); lpc++) {
		ulxr::RpcString tmp;
		ulxr::Struct attr;
		int port;

		attr = cameras.getItem(lpc);
		tmp = attr.getMember("hostname");
		port = ((ulxr::Integer)attr.getMember("port")).getInteger();
		fprintf(emcd_config,
			"camera %s %s %d %f %f %f %f %f %f\n",
			area,
			tmp.getString().c_str(),
			port,
			((ulxr::Double)attr.getMember("loc_x")).getDouble(),
			((ulxr::Double)attr.getMember("loc_y")).getDouble(),
			((ulxr::Double)attr.getMember("width")).getDouble(),
			((ulxr::Double)attr.getMember("height")).getDouble(),
			((ulxr::Double)attr.getMember("fixed_x")).getDouble(),
			((ulxr::Double)attr.getMember("fixed_y")).getDouble());
	}
	
	return 0;
}

static int
RPC_obstaclelist(FILE *emcd_config, char *area)
{
	emulab::EmulabResponse er;
	int lpc, rc;
	
	assert(emcd_config != NULL);
	assert(area != NULL);
	
	rc = RPC_invoke("emulab.obstacle_config",
			&er,
			emulab::SPA_String, "area", area,
			emulab::SPA_String, "units", "meters",
			emulab::SPA_TAG_DONE);
	
	if (rc < 0)
		return -1;
	
	ulxr::Array obstacles;
	
	obstacles = (ulxr::Array)er.getValue();
	for (lpc = 0; lpc < obstacles.size(); lpc++) {
		ulxr::Struct attr;
		
		attr = obstacles.getItem(lpc);
		fprintf(emcd_config,
			"obstacle %s %d %f %f %f %f\n",
			area,
			((ulxr::Integer)attr.getMember("obstacle_id")).
			getInteger(),
			((ulxr::Double)attr.getMember("x1")).getDouble(),
			((ulxr::Double)attr.getMember("y1")).getDouble(),
			((ulxr::Double)attr.getMember("x2")).getDouble(),
			((ulxr::Double)attr.getMember("y2")).getDouble());
	}
	
	return 0;
}

int
RPC_waitforactive(char *pid, char *eid)
{
	emulab::EmulabResponse er;

	assert(pid != NULL);
	assert(strlen(pid) > 0);
	assert(eid != NULL);
	assert(strlen(eid) > 0);

	return RPC_invoke(pid, eid, "experiment.waitforactive", &er);
}

int
RPC_waitforrobots(event_handle_t handle, char *pid, char *eid)
{
	emulab::EmulabResponse er;
	int lpc, rc, retval = 0;
	FILE *emcd_config;

	unlink("tbdata/emcd.config");
	
	rc = RPC_invoke("experiment.virtual_topology",
			&er,
			emulab::SPA_String, "proj", pid,
			emulab::SPA_String, "exp", eid,
			emulab::SPA_String, "tables", "node_startlocs",
			emulab::SPA_TAG_DONE);
	
	if (rc < 0)
		return rc;
	
	set<string> buildings;
	ulxr::Array top, locs;
	ulxr::Struct tables;
	
	top = (ulxr::Array)er.getValue();
	tables = (ulxr::Struct)top.getItem(0);
	tables = tables.getMember("experiment");
	locs = tables.getMember("node_startlocs");
	
	if (locs.size() == 0)
		return 0;

	info("info: waiting for robots\n");
	
	if ((emcd_config = fopen("tbdata/emcd.config", "w")) == NULL) {
		error("Cannot create emcd.config!\n");
		return -1;
	}

	if (topography_name != NULL) {
		fprintf(emcd_config, "topography %s\n", topography_name);
	}
	
	for (lpc = 0; lpc < locs.size(); lpc++) {
		ulxr::Struct loc = (ulxr::Struct)locs.getItem(lpc);
		emulab::EmulabResponse ner;
		ulxr::RpcString tmp, bldg;
		double x, y, orientation;
		struct agent *agent;
		char *vname;
		
		tmp = loc.getMember("vname");
		vname = (char *) tmp.getString().c_str();
		bldg = loc.getMember("building");
		x = ((ulxr::Double)loc.getMember("loc_x")).getDouble();
		y = ((ulxr::Double)loc.getMember("loc_y")).getDouble();
		orientation = ((ulxr::Double)loc.getMember("orientation")).
			getDouble();
		if ((agent = (struct agent *)
		     lnFindName(&agents, vname)) == NULL) {
			error("unknown robot %s\n", vname);
			
			return -1;
		}
		else if ((rc = RPC_invoke(
			"node.statewait",
			&ner,
			emulab::SPA_String, "node", agent->nodeid,
			emulab::SPA_Integer, "timeout", ROBOT_TIMEOUT,
			emulab::SPA_String, "state", "ISUP",
			emulab::SPA_TAG_DONE)) < 0) {
			error("robot %s did not come up!\n", agent->name);
			
			return -1;
		}
		else {
			fprintf(emcd_config,
				"robot %s %d %s %f %f %f %s\n",
				bldg.getString().c_str(),
				lpc + 1,
				agent->nodeid,
				x,
				y,
				orientation * M_PI / 180.0,
				agent->name);
			buildings.insert(bldg.getString());

			AddRobot(handle, agent, x, y, orientation);
		}
	}
	
	for (set<string>::iterator i = buildings.begin();
	     i != buildings.end();
	     i++) {
		if (RPC_cameralist(emcd_config, (char *)(*i).c_str()) < 0)
			return -1;
		
		if (RPC_obstaclelist(emcd_config, (char *)(*i).c_str()) < 0)
			return -1;
	}
	
	fclose(emcd_config);
	emcd_config = NULL;
	
	return retval;
}

int
RPC_agentlist(event_handle_t handle, char *pid, char *eid)
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
		info("D: adding agent %s\n", vname);
		if (AddAgent(handle, vname, vnode, nodeid, ipaddr, type) < 0) {
			return -1;
		}
	}
  
	return 0;
}

int
RPC_grouplist(event_handle_t handle, char *pid, char *eid)
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
		info("D: \tIn GroupList() parsed name %s\n", groupname);
		tmp = group.getItem(1);
		agentname = (char *) tmp.getString().c_str();
		info("D: \tIn GroupList() parsed agent %s\n", agentname);
		
		if (AddGroup(handle, groupname, agentname) != 0) {
			return -1;
		}
	}
	return 0;
}

int
RPC_eventlist(char *pid, char *eid,
	      event_handle_t handle, address_tuple_t tuple)
{ /* This function requests all the static events from boss using an RPC call, packing them into its
   * own data structures. */
	emulab::EmulabResponse er;
	int i, foo = RPC_invoke(pid, eid, "experiment.event_eventlist", &er);

// XXX
//	info("rpc.cc:RPC_eventlist(): Just called RPC_invoke()\n");
// XXX	
	if (foo)
		return foo;

// XXX
//	info("rpc.cc:RPC_eventlist(): calling getValue()\n");
// XXX	
	ulxr::Array events = (ulxr::Array)er.getValue();
	
// XXX
//	info("rpc.cc:RPC_eventlist(): Interating over invoke results\n");
// XXX	
	for (i = 0; i < events.size(); i++) {
		char *exidx, *extime, *objname, *objtype, *evttype, *exargs;
		char *parent, *triggertype;
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
		tmp = event.getItem(6);
		parent = (char *) tmp.getString().c_str();
		tmp = event.getItem(7);
		triggertype = (char *) tmp.getString().c_str();
// XXX
//	info("rpc.cc:RPC_eventlist(): Adding an event\n");
// XXX 		
		if (AddEvent(handle, tuple, exidx,
			     extime, objname, exargs, objtype, evttype,
			     parent, triggertype) < 0) {
			return -1;
		}
	}
	return 0;
}

