#include "libtmcd.h"

#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include "decls.h"
#include "ssl.h"
#include "log.h"
#include "config.h"
#include "bootwhat.h"
#include "bootinfo.h"

#ifdef EVENTSYS
#include "event.h"
#endif

/*
 * XXX This needs to be localized!
 */
#define FSPROJDIR	FSNODE ":" FSDIR_PROJ
#define FSGROUPDIR	FSNODE ":" FSDIR_GROUPS
#define FSUSERDIR	FSNODE ":" FSDIR_USERS
#ifdef  FSDIR_SHARE
#define FSSHAREDIR	FSNODE ":" FSDIR_SHARE
#endif
#ifdef  FSDIR_SCRATCH
#define FSSCRATCHDIR	FSNODE ":" FSDIR_SCRATCH
#endif
#define PROJDIR		PROJROOT_DIR
#define GROUPDIR	GROUPSROOT_DIR
#define USERDIR		USERSROOT_DIR
#define SCRATCHDIR	SCRATCHROOT_DIR
#define SHAREDIR	SHAREROOT_DIR
#define NETBEDDIR	"/netbed"
#define PLISALIVELOGDIR "/usr/testbed/log/plabisalive"
#define RELOADPID	"emulab-ops"
#define RELOADEID	"reloading"
#define FSHOSTID	"/usr/testbed/etc/fshostid"
#define DOTSFS		".sfs"
#define NTPSERVER       "ntp1"
#define DEFAULTNETMASK	"255.255.255.0"

/* Defined in configure and passed in via the makefile */
#define DEFAULT_DBNAME	TBDBNAME
#define DBNAME_SIZE	64
#define HOSTID_SIZE	(32+64)

/* Output macro to check for string overflow */
#define OUTPUT(buf, size, format...) \
({ \
	int __count__ = snprintf((buf), (size), ##format); \
        \
        if (__count__ >= (size)) { \
		error("Not enough room in output buffer! line %d.\n", __LINE__);\
		return 1; \
	} \
	__count__; \
})

#ifdef EVENTSYS
int			myevent_send(tmcdreq_t* reqp, address_tuple_t address);
#endif

static MYSQL_RES *	mydb_query(tmcdreq_t* reqp, char *query, int ncols, ...);
static int		mydb_update(tmcdreq_t* reqp, char *query, ...);

static int	safesymlink(char *name1, char *name2);

xmlNode *new_response(xmlNode *root, const char *type)
{
	xmlNode *node = NULL;
	xmlAttr *attr = NULL;

	node = xmlNewChild(root, NULL, BAD_CAST type, NULL);
	if (node == NULL) {
		/* XXX Ryan: print error */
		return NULL;
	}

	return node;

	attr = xmlNewProp(node, BAD_CAST "type", BAD_CAST type);
	if (attr == NULL) {
		/* XXX Ryan: print error */
		/* XXX Ryan: should unlink and free the node */
		return NULL;
	}

	return node;
}

xmlNode *add_key(xmlNode *opt, const char *name, const char *value)
{
	xmlNode *node = NULL;
	xmlAttr *attr = NULL;

	node = xmlNewChild(opt, NULL, BAD_CAST "key",
			BAD_CAST value);
	if (node == NULL) {
		/* XXX Ryan: print error */
		return NULL;
	}

	attr = xmlNewProp(node, BAD_CAST "name", BAD_CAST name);
	if (attr == NULL) {
		/* XXX Ryan: print error */
		/* XXX Ryan: should unlink and free the node */
		return NULL;
	}

	return node;
}

xmlNode *add_format_key(xmlNode *opt, const char *name, const char *format, ...)
{
	xmlNode *node = NULL;
	xmlAttr *attr = NULL;
	char	value[MYBUFSIZE];
	va_list ap;

	va_start(ap, format);
	vsnprintf(value, sizeof(value), format, ap);
	va_end(ap);

	node = xmlNewChild(opt, NULL, BAD_CAST "key",
			BAD_CAST value);
	if (node == NULL) {
		/* XXX Ryan: print error */
		return NULL;
	}

	attr = xmlNewProp(node, BAD_CAST "name", BAD_CAST name);
	if (attr == NULL) {
		/* XXX Ryan: print error */
		/* XXX Ryan: should unlink and free the node */
		return NULL;
	}

	return node;
}

/* This can be tossed once all the changes are in place */
static char *
CHECKMASK(char *arg)
{
	if (arg && arg[0])
		return arg;

	error("No netmask defined!\n");
	return DEFAULTNETMASK;
}
/* #define CHECKMASK(arg)  ((arg) && (arg[0]) ? (arg) : DEFAULTNETMASK) */

#define DISKTYPE	"ad"
#define DISKNUM		0

/* Compiled in slothd parameters
 *
 * 1 - reg_interval  2 - agg_interval  3 - load_thresh  
 * 4 - expt_thresh   5 - ctl_thresh
 */
#define SDPARAMS        "reg=300 agg=5 load=1 expt=5 ctl=1000"

/*
 * Commands we support.
 */
#define XML_COMMAND_PROTOTYPE(x) \
	static int \
	x(xmlNode *root, tmcdreq_t *reqp, char *rdata)
#define RAW_COMMAND_PROTOTYPE(x) \
	static int \
	x(char **outbuf, int *length, int sock, tmcdreq_t *reqp, char *rdata)

XML_COMMAND_PROTOTYPE(doreboot);
XML_COMMAND_PROTOTYPE(donodeid);
XML_COMMAND_PROTOTYPE(dostatus);
XML_COMMAND_PROTOTYPE(doifconfig);
XML_COMMAND_PROTOTYPE(doaccounts);
XML_COMMAND_PROTOTYPE(dodelay);
XML_COMMAND_PROTOTYPE(dolinkdelay);
XML_COMMAND_PROTOTYPE(dohosts);
XML_COMMAND_PROTOTYPE(dorpms);
XML_COMMAND_PROTOTYPE(dodeltas);
XML_COMMAND_PROTOTYPE(dotarballs);
XML_COMMAND_PROTOTYPE(dostartcmd);
XML_COMMAND_PROTOTYPE(dostartstat);
XML_COMMAND_PROTOTYPE(doready);
XML_COMMAND_PROTOTYPE(doreadycount);
XML_COMMAND_PROTOTYPE(domounts);
XML_COMMAND_PROTOTYPE(dosfshostid);
XML_COMMAND_PROTOTYPE(doloadinfo);
XML_COMMAND_PROTOTYPE(doreset);
XML_COMMAND_PROTOTYPE(dorouting);
XML_COMMAND_PROTOTYPE(dotrafgens);
XML_COMMAND_PROTOTYPE(donseconfigs);
XML_COMMAND_PROTOTYPE(dostate);
XML_COMMAND_PROTOTYPE(docreator);
XML_COMMAND_PROTOTYPE(dotunnels);
XML_COMMAND_PROTOTYPE(dovnodelist);
XML_COMMAND_PROTOTYPE(dosubnodelist);
XML_COMMAND_PROTOTYPE(doisalive);
XML_COMMAND_PROTOTYPE(doipodinfo);
XML_COMMAND_PROTOTYPE(dontpinfo);
XML_COMMAND_PROTOTYPE(dontpdrift);
XML_COMMAND_PROTOTYPE(dojailconfig);
XML_COMMAND_PROTOTYPE(doplabconfig);
XML_COMMAND_PROTOTYPE(dosubconfig);
XML_COMMAND_PROTOTYPE(doixpconfig);
XML_COMMAND_PROTOTYPE(doslothdparams);
XML_COMMAND_PROTOTYPE(doprogagents);
XML_COMMAND_PROTOTYPE(dosyncserver);
XML_COMMAND_PROTOTYPE(dokeyhash);
XML_COMMAND_PROTOTYPE(doeventkey);
XML_COMMAND_PROTOTYPE(dofullconfig);
XML_COMMAND_PROTOTYPE(doroutelist);
XML_COMMAND_PROTOTYPE(dorole);
XML_COMMAND_PROTOTYPE(dorusage);
XML_COMMAND_PROTOTYPE(dodoginfo);
XML_COMMAND_PROTOTYPE(dohostkeys);
XML_COMMAND_PROTOTYPE(dofwinfo);
XML_COMMAND_PROTOTYPE(dohostinfo);
XML_COMMAND_PROTOTYPE(doemulabconfig);
XML_COMMAND_PROTOTYPE(doeplabconfig);
XML_COMMAND_PROTOTYPE(dolocalize);
XML_COMMAND_PROTOTYPE(dorootpswd);
XML_COMMAND_PROTOTYPE(dobooterrno);
XML_COMMAND_PROTOTYPE(dobattery);
XML_COMMAND_PROTOTYPE(douserenv);
XML_COMMAND_PROTOTYPE(dotiptunnels);
XML_COMMAND_PROTOTYPE(dorelayconfig);
XML_COMMAND_PROTOTYPE(dotraceconfig);
XML_COMMAND_PROTOTYPE(doelvindport);
XML_COMMAND_PROTOTYPE(doplabeventkeys);
XML_COMMAND_PROTOTYPE(dointfcmap);
XML_COMMAND_PROTOTYPE(domotelog);
XML_COMMAND_PROTOTYPE(doportregister);
XML_COMMAND_PROTOTYPE(dobootwhat);

RAW_COMMAND_PROTOTYPE(dotopomap);
RAW_COMMAND_PROTOTYPE(doltmap);
RAW_COMMAND_PROTOTYPE(doltpmap);
RAW_COMMAND_PROTOTYPE(dobootlog);

/*
 * The fullconfig slot determines what routines get called when pushing
 * out a full configuration. Physnodes get slightly different
 * than vnodes, and at some point we might want to distinguish different
 * types of vnodes (jailed, plab).
 */
#define FULLCONFIG_NONE		0x0
#define FULLCONFIG_PHYS		0x1
#define FULLCONFIG_VIRT		0x2
#define FULLCONFIG_ALL		(FULLCONFIG_PHYS|FULLCONFIG_VIRT)

/*
 * Flags encode a few other random properties of commands
 */
#define F_REMUDP	0x01	/* remote nodes can request using UDP */
#define F_MINLOG	0x02	/* record minimal logging info normally */
#define F_MAXLOG	0x04	/* record maximal logging info normally */
#define F_ALLOCATED	0x08	/* node must be allocated to make call */
#define F_REMNOSSL	0x10	/* remote nodes can request without SSL */
#define F_REMREQSSL	0x20	/* remote nodes must connect with SSL */

struct command {
	char	*cmdname;
	int	fullconfig;	
	int	flags;
	int    (*func)(xmlNode *, tmcdreq_t *, char *);
} xml_command_array[] = {
	{ "reboot",	  FULLCONFIG_NONE, 0, doreboot },
	{ "nodeid",	  FULLCONFIG_ALL,  0, donodeid },
	{ "status",	  FULLCONFIG_NONE, 0, dostatus },
	{ "ifconfig",	  FULLCONFIG_ALL,  F_ALLOCATED, doifconfig },
	{ "accounts",	  FULLCONFIG_ALL,  F_REMREQSSL, doaccounts },
	{ "delay",	  FULLCONFIG_ALL,  F_ALLOCATED, dodelay },
	{ "linkdelay",	  FULLCONFIG_ALL,  F_ALLOCATED, dolinkdelay },
	{ "hostnames",	  FULLCONFIG_NONE, F_ALLOCATED, dohosts },
	{ "rpms",	  FULLCONFIG_ALL,  F_ALLOCATED, dorpms },
	{ "deltas",	  FULLCONFIG_NONE, F_ALLOCATED, dodeltas },
	{ "tarballs",	  FULLCONFIG_ALL,  F_ALLOCATED, dotarballs },
	{ "startupcmd",	  FULLCONFIG_ALL,  F_ALLOCATED, dostartcmd },
	{ "startstatus",  FULLCONFIG_NONE, F_ALLOCATED, dostartstat }, /* Before startstat*/
	{ "startstat",	  FULLCONFIG_NONE, 0, dostartstat },
	{ "readycount",   FULLCONFIG_NONE, F_ALLOCATED, doreadycount },
	{ "ready",	  FULLCONFIG_NONE, F_ALLOCATED, doready },
	{ "mounts",	  FULLCONFIG_ALL,  F_ALLOCATED, domounts },
	{ "sfshostid",	  FULLCONFIG_NONE, F_ALLOCATED, dosfshostid },
	{ "loadinfo",	  FULLCONFIG_NONE, 0, doloadinfo},
	{ "reset",	  FULLCONFIG_NONE, 0, doreset},
	{ "routing",	  FULLCONFIG_ALL,  F_ALLOCATED, dorouting},
	{ "trafgens",	  FULLCONFIG_ALL,  F_ALLOCATED, dotrafgens},
	{ "nseconfigs",	  FULLCONFIG_ALL,  F_ALLOCATED, donseconfigs},
	{ "creator",	  FULLCONFIG_ALL,  F_ALLOCATED, docreator},
	{ "state",	  FULLCONFIG_NONE, 0, dostate},
	{ "tunnels",	  FULLCONFIG_ALL,  F_ALLOCATED, dotunnels},
	{ "vnodelist",	  FULLCONFIG_PHYS, 0, dovnodelist},
	{ "subnodelist",  FULLCONFIG_PHYS, 0, dosubnodelist},
	{ "isalive",	  FULLCONFIG_NONE, F_REMUDP|F_MINLOG, doisalive},
	{ "ipodinfo",	  FULLCONFIG_PHYS, 0, doipodinfo},
	{ "ntpinfo",	  FULLCONFIG_PHYS, 0, dontpinfo},
	{ "ntpdrift",	  FULLCONFIG_NONE, 0, dontpdrift},
	{ "jailconfig",	  FULLCONFIG_VIRT, F_ALLOCATED, dojailconfig},
	{ "plabconfig",	  FULLCONFIG_VIRT, F_ALLOCATED, doplabconfig},
	{ "subconfig",	  FULLCONFIG_NONE, 0, dosubconfig},
        { "sdparams",     FULLCONFIG_PHYS, 0, doslothdparams},
        { "programs",     FULLCONFIG_ALL,  F_ALLOCATED, doprogagents},
        { "syncserver",   FULLCONFIG_ALL,  F_ALLOCATED, dosyncserver},
        { "keyhash",      FULLCONFIG_ALL,  F_ALLOCATED|F_REMREQSSL, dokeyhash},
        { "eventkey",     FULLCONFIG_ALL,  F_ALLOCATED, doeventkey},
        { "fullconfig",   FULLCONFIG_NONE, F_ALLOCATED, dofullconfig},
        { "routelist",	  FULLCONFIG_PHYS, F_ALLOCATED, doroutelist},
        { "role",	  FULLCONFIG_PHYS, F_ALLOCATED, dorole},
        { "rusage",	  FULLCONFIG_NONE, F_REMUDP|F_MINLOG, dorusage},
        { "watchdoginfo", FULLCONFIG_ALL,  F_REMUDP|F_MINLOG, dodoginfo},
        { "hostkeys",     FULLCONFIG_NONE, 0, dohostkeys},
        { "firewallinfo", FULLCONFIG_ALL,  0, dofwinfo},
        { "hostinfo",     FULLCONFIG_NONE, 0, dohostinfo},
	{ "emulabconfig", FULLCONFIG_NONE, F_ALLOCATED, doemulabconfig},
	{ "eplabconfig",  FULLCONFIG_NONE, F_ALLOCATED, doeplabconfig},
	{ "localization", FULLCONFIG_PHYS, 0, dolocalize},
	{ "rootpswd",     FULLCONFIG_NONE, F_REMREQSSL, dorootpswd},
	{ "booterrno",    FULLCONFIG_NONE, 0, dobooterrno},
	{ "battery",      FULLCONFIG_NONE, F_REMUDP|F_MINLOG, dobattery},
	{ "userenv",      FULLCONFIG_ALL,  F_ALLOCATED, douserenv},
	{ "tiptunnels",	  FULLCONFIG_ALL,  F_ALLOCATED, dotiptunnels},
	{ "traceinfo",	  FULLCONFIG_ALL,  F_ALLOCATED, dotraceconfig },
	{ "elvindport",   FULLCONFIG_NONE, 0, doelvindport},
	{ "plabeventkeys",FULLCONFIG_NONE, F_REMREQSSL, doplabeventkeys},
	{ "intfcmap",     FULLCONFIG_NONE, 0, dointfcmap},
	{ "motelog",      FULLCONFIG_ALL,  F_ALLOCATED, domotelog},
	{ "portregister", FULLCONFIG_NONE, F_REMNOSSL, doportregister},
	{ "bootwhat",	  FULLCONFIG_NONE, 0, dobootwhat },
};
static int numcommands = sizeof(xml_command_array)/sizeof(struct command);

struct raw_command {
	char	*cmdname;
	int	fullconfig;	
	int	flags;
	int    (*func)(char **, int *, int, tmcdreq_t *, char *);
} raw_xml_command_array[] = {
	{ "bootlog",      FULLCONFIG_NONE, 0, dobootlog},
	{ "ltmap",        FULLCONFIG_NONE, F_MINLOG|F_ALLOCATED, doltmap},
	{ "ltpmap",       FULLCONFIG_NONE, F_MINLOG|F_ALLOCATED, doltpmap},
	{ "topomap",      FULLCONFIG_NONE, F_MINLOG|F_ALLOCATED, dotopomap},
};
static int numrawcommands = sizeof(raw_xml_command_array)/sizeof(struct raw_command);

#ifdef EVENTSYS
/*
 * Connect to the event system. It's not an error to call this function if
 * already connected. Returns 1 on failure, 0 on sucess.
 */
int
event_connect(tmcdreq_t *reqp)
{
	if (!reqp->event_handle) {
		reqp->event_handle =
		  event_register("elvin://localhost:" BOSSEVENTPORT, 0);
	}

	if (reqp->event_handle) {
		return 0;
	} else {
		error("event_connect: "
		      "Unable to register with event system!\n");
		return 1;
	}
}

/*
 * Send an event to the event system. Automatically connects (registers)
 * if not already done. Returns 0 on sucess, 1 on failure.
 */
int myevent_send(tmcdreq_t *reqp, address_tuple_t tuple) {
	event_notification_t notification;

	if (event_connect(reqp)) {
		return 1;
	}

	notification = event_notification_alloc(reqp->event_handle,tuple);
	if (notification == (event_notification_t) NULL) {
		error("myevent_send: Unable to allocate notification!");
		return 1;
	}

	if (! event_notify(reqp->event_handle, notification)) {
		event_notification_free(reqp->event_handle, notification);

		error("myevent_send: Unable to send notification!");
		/*
		 * Let's try to disconnect from the event system, so that
		 * we'll reconnect next time around.
		 */
		if (!event_unregister(reqp->event_handle)) {
			error("myevent_send: "
			      "Unable to unregister with event system!");
		}
		reqp->event_handle = NULL;
		return 1;
	} else {
		event_notification_free(reqp->event_handle,notification);
		return 0;
	}
}

/*
 * This is for bootinfo inclusion.
 */
int	bievent_init(void) { return 0; }

int
bievent_send(struct in_addr ipaddr, void *opaque, char *event)
{
	tmcdreq_t		*reqp = (tmcdreq_t *) opaque;
	static address_tuple_t	tuple;

	if (!tuple) {
		tuple = address_tuple_alloc();
		if (tuple == NULL) {
			error("bievent_send: Unable to allocate tuple!\n");
			return -1;
		}
	}
	tuple->host      = BOSSNODE;
	tuple->objtype   = "TBNODESTATE";
	tuple->objname	 = reqp->nodeid;
	tuple->eventtype = event;

	if (myevent_send(reqp, tuple)) {
		error("bievent_send: Error sending event\n");
		return -1;
	}
	return 0;
}
#endif /* EVENTSYS */
 
tmcdresp_t *tmcd_handle_request(tmcdreq_t *reqp, int sock, char *command, char *rdata)
{
	xmlDoc *doc = NULL;
	xmlChar *xmlbuf;
	xmlNode *root = NULL;
	tmcdresp_t *response = NULL;
	int i, err = 0;
	int buffersize;
	int is_xml_command = 1;
	int flags = 0;

	/*
	 * Figure out what command was given.
	 */
	for (i = 0; i < numcommands; i++) {
		if (strcmp(command, xml_command_array[i].cmdname) == 0) {
			flags = xml_command_array[i].flags;
			break;
		}
	}

	if (i == numcommands) {
		for (i = 0; i < numrawcommands; i++)  {
			if (strcmp(command, raw_xml_command_array[i].cmdname) == 0) {
				is_xml_command = 0;
				flags = raw_xml_command_array[i].flags;
			    	break;
			}
		}

		if (i == numrawcommands) {
			printf("%s: INVALID REQUEST: %.8s\n", reqp->nodeid, command);
			goto skipit;
		}

	}

	/* If this is a UDP request, make sure it is allowed */
	if (!reqp->istcp && (flags & F_UDP) == 0) {
		error("%s: %s: Invalid UDP request from node\n",
		      reqp->nodeid, command);
		goto skipit;
	}

	/*
	 * If this is a UDP request from a remote node,
	 * make sure it is allowed.
	 */
	if (!reqp->istcp && !reqp->islocal &&
	    (flags & F_REMUDP) == 0) {
		error("%s: %s: Invalid UDP request from remote node\n",
		      reqp->nodeid, command);
		goto skipit;
	}

	/*
	 * Ditto for remote node connection without SSL.
	 */
	if (reqp->istcp && !reqp->isssl && !reqp->islocal && 
	    (flags & F_REMREQSSL) != 0) {
		error("%s: %s: Invalid NO-SSL request from remote node\n",
		      reqp->nodeid, command);
		goto skipit;
	}

	if (!reqp->allocated && (flags & F_ALLOCATED) != 0) {
		if (reqp->verbose || (flags & F_MINLOG) == 0)
			error("%s: %s: Invalid request from free node\n",
			      reqp->nodeid, command);
		goto skipit;
	}

	/*
	 * Execute it.
	 */
	if ((flags & F_MAXLOG) != 0) {
		/* XXX Ryan? overbose = verbose; */
		reqp->verbose = 1;
	}
#if 0
	if (reqp->verbose || (xml_command_array[i].flags & F_MINLOG) == 0)
		info("%s: reqp->version%d %s %s\n", reqp->nodeid,
		     version, cp, xml_command_array[i].cmdname);
	setproctitle("%s: %s %s", reqp->nodeid, cp, xml_command_array[i].cmdname);
#endif

#if 0
	if ((flags & F_MAXLOG) != 0)
		reqp->verbose = overbose;
#endif
	response = malloc(sizeof(tmcdresp_t));
	if (response == NULL) {
		goto skipit;
	}

	if (is_xml_command) {
		/* XXX Ryan error checking */
		doc = xmlNewDoc(BAD_CAST "1.0");
		root = xmlNewNode(NULL, BAD_CAST "tmcd");
		xmlDocSetRootElement(doc, root);
		err = xml_command_array[i].func(root, reqp, rdata);

		if (err)
			info("%s: %s: returned %d\n",
			     reqp->nodeid, xml_command_array[i].cmdname, err);

		xmlDocDumpFormatMemory(doc, &xmlbuf, &buffersize, 1);
		xmlFreeDoc(doc);
		response->length = strlen((char *)xmlbuf);
		response->data = malloc(response->length);
		if (response->data == NULL) {
			error("unable to allocate memory for response\n");
			xmlFree(xmlbuf);
			goto skipit;
		}

		memcpy(response->data, (char *)xmlbuf, response->length);
		xmlFree(xmlbuf);
	} else {
		err = raw_xml_command_array[i].func(&response->data, &response->length, sock, reqp, rdata);

		if (err)
			info("%s: %s: returned %d\n",
			     reqp->nodeid, raw_xml_command_array[i].cmdname, err);
	}


skipit:

	return response;
}

void tmcd_free_response(tmcdresp_t *response)
{
	if (response) {
		if (response->data)
			free(response->data);
		free(response);
	}
}

/*
 * Accept notification of reboot. 
 */
XML_COMMAND_PROTOTYPE(doreboot)
{
	/*
	 * This is now a no-op. The things this used to do are now
	 * done by stated when we hit RELOAD/RELOADDONE state
	 */
	return 0;
}

/*
 * Return emulab nodeid (not the experimental name).
 */
XML_COMMAND_PROTOTYPE(donodeid)
{
	xmlNode *node;

	node = new_response(root, "nodeid");
	add_key(node, "id", reqp->nodeid);

	return 0;
}

/*
 * Return status of node. Is it allocated to an experiment, or free.
 */
XML_COMMAND_PROTOTYPE(dostatus)
{
	xmlNode *node;

	node = new_response(root, "status");
	/*
	 * Now check reserved table
	 */
	if (! reqp->allocated) {
		info("STATUS: %s: FREE\n", reqp->nodeid);
		add_key(node, "status", "free");
	}
	else {
		add_key(node, "status", "allocated");
		add_key(node, "pid", reqp->pid);
		add_key(node, "eid", reqp->eid);
		add_key(node, "nickname", reqp->nickname);
	}

	return 0;
}

/*
 * Return ifconfig information to client.
 */
XML_COMMAND_PROTOTYPE(doifconfig)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		clause[BUFSIZ];
	char		buf[MYBUFSIZE];
	xmlNode		*node, *ifconfigNode;
	int		nrows;
	int		num_interfaces=0;

	/* 
	 * For Virtual Nodes, we return interfaces that belong to it.
	 */
	if (reqp->isvnode && !reqp->issubnode)
		sprintf(clause, "i.vnode_id='%s'", reqp->vnodeid);
	else
		strcpy(clause, "i.vnode_id is NULL");

	/*
	 * Find all the interfaces.
	 */
	res = mydb_query(reqp, "select i.card,i.IP,i.MAC,i.current_speed,"
			 "       i.duplex,i.IPaliases,i.iface,i.role,i.mask,"
			 "       i.rtabid,i.interface_type,vl.vname "
			 "  from interfaces as i "
			 "left join virt_lans as vl on "
			 "  vl.pid='%s' and vl.eid='%s' and "
			 "  vl.vnode='%s' and vl.ip=i.IP "
			 "where i.node_id='%s' and %s",
			 12, reqp->pid, reqp->eid, reqp->nickname,
			 reqp->issubnode ? reqp->nodeid : reqp->pnodeid,
			 clause);

	/*
	 * We need pnodeid in the query. But error reporting is done
	 * by nodeid. For vnodes, nodeid is pcvmXX-XX and for the rest
	 * it is the same as pnodeid
	 */
	if (!res) {
		error("IFCONFIG: %s: DB Error getting interfaces!\n",
		      reqp->nodeid);
		return 1;
	}

	ifconfigNode = new_response(root, "ifconfig");

	nrows = (int)mysql_num_rows(res);
	while (nrows) {
		row = mysql_fetch_row(res);
		if (row[1] && row[1][0]) {
			char *iface  = row[6];
			char *role   = row[7];
			char *type   = row[10];
			char *lan    = row[11];
			char *speed  = "100";
			char *duplex = "full";
			char *mask;

			/* Never for the control net; sharks are dead */
			if (strcmp(role, TBDB_IFACEROLE_EXPERIMENT))
				goto skipit;

			/* Do this after above test to avoid error in log */
			mask = CHECKMASK(row[8]);

			/*
			 * Speed and duplex if not the default.
			 */
			if (row[3] && row[3][0])
				speed = row[3];
			if (row[4] && row[4][0])
				duplex = row[4];

			/*
			 * We now use the MAC to determine the interface, but
			 * older images still want that tag at the front.
			 */
			node = new_response(ifconfigNode, "interface");
			add_key(node, "type", type);
			add_key(node, "inet", row[1]);
			add_key(node, "mask", mask);
			add_key(node, "mac", row[2]);
			add_key(node, "speed", speed); /* in Mbps */
			add_key(node, "duplex", duplex);

#if 0
			/*
			 * For older clients, we tack on IPaliases.
			 * This used to be in the interfaces table as a
			 * comma separated list, now we have to extract
			 * it from the vinterfaces table.
			 */
			if (reqp->version>= 8 && reqp->version< 27) {
				MYSQL_RES *res2;
				MYSQL_ROW row2;
				int nrows2;
				
				res2 = mydb_query(reqp, "select IP "
						  "from vinterfaces "
						  "where type='alias' "
						  "and node_id='%s'",
						  1, reqp->nodeid);
				if (res2 == NULL)
					goto adone;

				nrows2 = (int)mysql_num_rows(res2);
				bufp += OUTPUT(bufp, ebufp - bufp,
					       " IPALIASES=\"");
				while (nrows2 > 0) {
					nrows2--;
					row2 = mysql_fetch_row(res2);
					if (!row2 || !row2[0])
						continue;
					bufp += OUTPUT(bufp, ebufp - bufp,
						       "%s", row2[0]);
					if (nrows2 > 0)
						bufp += OUTPUT(bufp,
							       ebufp - bufp,
							       ",");
				}
				bufp += OUTPUT(bufp, ebufp - bufp, "\"");
				mysql_free_result(res2);
			adone: ;
			}
#endif

			/*
			 * Tack on iface for IXPs. This should be a flag on
			 * the interface instead of a match against type.
			 */

			add_key(node, "iface", 
				       (strcmp(reqp->class, "ixp") ?
					"" : iface));
			add_key(node, "rtabid", row[9]);
			add_key(node, "lan", lan);

			num_interfaces++;
		}
	skipit:
		nrows--;
	}
	mysql_free_result(res);

	/*
	 * Interface settings.
	 */
	res = mydb_query(reqp, "select i.MAC,s.capkey,s.capval "
			 "from interface_settings as s "
			 "left join interfaces as i on "
			 "     s.node_id=i.node_id and s.iface=i.iface "
			 "where s.node_id='%s' and %s ",
			 3,
			 reqp->issubnode ? reqp->nodeid : reqp->pnodeid,
			 clause);

	if (!res) {
		error("IFCONFIG: %s: "
		      "DB Error getting interface_settings!\n",
		      reqp->nodeid);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);
	while (nrows) {
		row = mysql_fetch_row(res);
		node = new_response(ifconfigNode, "interface-settings");
		add_key(node, "mac", row[0]);
		add_key(node, "key", row[1]);
		add_key(node, "val", row[2]);

		nrows--;
	}
	mysql_free_result(res);

	/*
	 * First, return config info the physical interfaces underlying
	 * the virtual interfaces so that those can be set up.
	 *
	 * For virtual nodes, we do this just for the physical node;
	 * no need to send it back for every vnode!
	 */
	if (!reqp->isvnode) {

		/*
		 * First do phys interfaces underlying veth/vlan interfaces
		 */
		res = mydb_query(reqp, "select distinct "
				 "       i.interface_type,i.mac, "
				 "       i.current_speed,i.duplex "
				 "  from vinterfaces as v "
				 "left join interfaces as i on "
				 "  i.node_id=v.node_id and i.iface=v.iface "
				 "where v.iface is not null and "
				 "      v.type!='alias' and v.node_id='%s'",
				 4, reqp->pnodeid);
		if (!res) {
			error("IFCONFIG: %s: "
			     "DB Error getting interfaces underlying veths!\n",
			      reqp->nodeid);
			return 1;
		}

		nrows = (int)mysql_num_rows(res);
		while (nrows) {
			row = mysql_fetch_row(res);
			node = new_response(ifconfigNode, "interface");
			add_key(node, "type", row[0]);
			add_key(node, "mac", row[1]);
			add_key(node, "speed", row[2]); /* in Mbps */
			add_key(node, "duplex", row[3]);

			nrows--;
		}
		mysql_free_result(res);

		/*
		 * Now do phys interfaces underlying delay interfaces.
		 */
		res = mydb_query(reqp, "select i.interface_type,i.MAC,"
				 "       i.current_speed,i.duplex, "
				 "       j.interface_type,j.MAC,"
				 "       j.current_speed,j.duplex "
				 " from delays as d "
				 "left join interfaces as i on "
				 "    i.node_id=d.node_id and i.iface=iface0 "
				 "left join interfaces as j on "
				 "    j.node_id=d.node_id and j.iface=iface1 "
				 "where d.node_id='%s'",
				 8, reqp->pnodeid);
		if (!res) {
			error("IFCONFIG: %s: "
			    "DB Error getting interfaces underlying delays!\n",
			      reqp->nodeid);
			return 1;
		}
		nrows = (int)mysql_num_rows(res);
		while (nrows) {
			row = mysql_fetch_row(res);

			node = new_response(ifconfigNode, "interface");
			add_key(node, "type", row[0]);
			add_key(node, "mac", row[1]);
			add_key(node, "speed", row[2]); /* in Mbps */
			add_key(node, "duplex", row[3]);

			node = new_response(ifconfigNode, "interface");
			add_key(node, "type", row[4]);
			add_key(node, "mac", row[5]);
			add_key(node, "speed", row[6]); /* in Mbps */
			add_key(node, "duplex", row[7]);

			nrows--;
		}
		mysql_free_result(res);
	}

	/*
	 * Outside a vnode, return only those virtual devices that have
	 * vnode=NULL, which indicates its an emulated interface on a
	 * physical node. When inside a vnode, only return virtual devices
	 * for which vnode=curvnode, which are the interfaces that correspond
	 * to a jail node.
	 */
	if (reqp->isvnode)
		sprintf(buf, "v.vnode_id='%s'", reqp->vnodeid);
	else
		strcpy(buf, "v.vnode_id is NULL");

	/*
	 * Find all the virtual interfaces.
	 */
	res = mydb_query(reqp, "select v.unit,v.IP,v.mac,i.mac,v.mask,v.rtabid, "
			 "       v.type,vll.vname,v.virtlanidx,la.attrvalue "
			 "  from vinterfaces as v "
			 "left join interfaces as i on "
			 "  i.node_id=v.node_id and i.iface=v.iface "
			 "left join virt_lan_lans as vll on "
			 "  vll.idx=v.virtlanidx and vll.exptidx='%d' "
			 "left join lan_attributes as la on "
			 "  la.lanid=v.vlanid and la.attrkey='vlantag' "
			 "left join lan_attributes as la2 on "
			 "  la2.lanid=v.vlanid and la2.attrkey='stack' "
			 "where v.node_id='%s' and "
			 "      (la2.attrvalue='Experimental' or "
			 "       la2.attrvalue is null) "
			 "      and %s",
			 10, reqp->exptidx, reqp->pnodeid, buf);
	if (!res) {
		error("IFCONFIG: %s: DB Error getting veth interfaces!\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		/* XXX not sure why this is ever an error? */
		if (!reqp->isplabdslice && num_interfaces == 0) {
			error("IFCONFIG: %s: No interfaces!\n", reqp->nodeid);
			return 1;
		}
		return 0;
	}
	while (nrows) {
		int isveth, doencap;

		row = mysql_fetch_row(res);
		nrows--;

		if (strcmp(row[6], "veth") == 0) {
			isveth = 1;
			doencap = 1;
		} else if (strcmp(row[6], "veth-ne") == 0) {
			isveth = 1;
			doencap = 0;
		} else {
			isveth = 0;
			doencap = 0;
		}

		/*
		 * Note that PMAC might be NULL, which happens if there is
		 * no underlying phys interface (say, colocated nodes in a
		 * link).
		 */

		node = new_response(ifconfigNode, "interface");
		add_key(node, "type", isveth ? "veth" : row[6]);
		add_key(node, "inet", row[1]);
		add_key(node, "mask", CHECKMASK(row[4]));
		add_key(node, "id", row[0]);
		add_key(node, "vmac", row[2]);
		add_key(node, "pmac", row[3] ? row[3] : "none");
		add_key(node, "rtabid", row[5]);
		add_key(node, "encapsulate", doencap ? "1" : "0" );
		add_key(node, "lan", row[7]);

		/*
		 * Return a VLAN tag.
		 *
		 * XXX for veth devices it comes out of the virt_lan_lans
		 * table, for vlan devices it comes out of the vlans table,
		 * for anything else it is zero.
		 */
		{
			char *tag = "0";
			if (isveth)
				tag = row[8];
			else if (strcmp(row[6], "vlan") == 0)
				tag = row[9];

			/* sanity check the tag */
			if (!isdigit(tag[0])) {
				error("IFCONFIG: bogus encap tag '%s'\n", tag);
				tag = "0";
			}

			add_key(node, "vtag", tag);
		}

		if (reqp->verbose)
			info("IFCONFIG: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return account stuff.
 */
XML_COMMAND_PROTOTYPE(doaccounts)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	xmlNode *node, *accounts_node;
	int		nrows, gidint;
	int		tbadmin, didwidearea = 0, nodetypeprojects = 0;
	int		didnonlocal = 0;

	/*
	 * Now check reserved table
	 */
	if ((reqp->islocal || reqp->isvnode) && !reqp->allocated) {
		error("%s: accounts: Invalid request from free node\n",
		      reqp->nodeid);
		return 1;
	}

	/*
	 * See if a per-node-type set of projects is specified for accounts.
	 */
	res = mydb_query(reqp, "select na.attrvalue from nodes as n "
			 "left join node_type_attributes as na on "
			 "  n.type=na.type "
			 "where n.node_id='%s' and "
			 "na.attrkey='project_accounts'", 1, reqp->nodeid);
	if (res) {
		if ((int)mysql_num_rows(res) != 0) {
			nodetypeprojects = 1;
		}
		mysql_free_result(res);
	}

        /*
	 * We need the unix GID and unix name for each group in the project.
	 */
	if (reqp->iscontrol) {
		/*
		 * All groups! 
		 */
		res = mydb_query(reqp, "select unix_name,unix_gid from groups", 2);
	}
	else if (nodetypeprojects) {
		/*
		 * The projects/groups are specified as a comma separated
		 * list in the node_type_attributes table. Return this
		 * set of groups, plus those in emulab-ops since we want
		 * to include admin people too.
		 */
		res = mydb_query(reqp, "select g.unix_name,g.unix_gid "
				 " from projects as p "
				 "left join groups as g on "
				 "     p.pid_idx=g.pid_idx "
				 "where p.approved!=0 and "
				 "  (FIND_IN_SET(g.gid_idx, "
				 "   (select na.attrvalue from nodes as n "
				 "    left join node_type_attributes as na on "
				 "         n.type=na.type "
				 "    where n.node_id='%s' and "
				 "    na.attrkey='project_accounts')) > 0 or "
				 "   p.pid='%s')",
				 2, reqp->nodeid, RELOADPID);
	}
	else if (reqp->islocal || reqp->isvnode) {
		res = mydb_query(reqp, "select unix_name,unix_gid from groups "
				 "where pid='%s'",
				 2, reqp->pid);
	}
	else if (reqp->jailflag) {
		/*
		 * A remote node, doing jails. We still want to return
		 * a group for the admin people who get accounts outside
		 * the jails. Lets use the same query as above for now,
		 * but switch over to emulab-ops. 
		 */
		res = mydb_query(reqp, "select unix_name,unix_gid from groups "
				 "where pid='%s'",
				 2, RELOADPID);
	}
	else {
		/*
		 * XXX - Old style node, not doing jails.
		 *
		 * Added this for Dave. I love subqueries!
		 */
		res = mydb_query(reqp, "select g.unix_name,g.unix_gid "
				 " from projects as p "
				 "left join groups as g on p.pid=g.pid "
				 "where p.approved!=0 and "
				 "  FIND_IN_SET(g.gid_idx, "
				 "   (select attrvalue from node_attributes "
				 "      where node_id='%s' and "
				 "            attrkey='dp_projects')) > 0",
				 2, reqp->pnodeid);

		if (!res || (int)mysql_num_rows(res) == 0) {
		 /*
		  * Temporary hack until we figure out the right model for
		  * remote nodes. For now, we use the pcremote-ok slot in
		  * in the project table to determine what remote nodes are
		  * okay'ed for the project. If connecting node type is in
		  * that list, then return all of the project groups, for
		  * each project that is allowed to get accounts on the type.
		  */
		  res = mydb_query(reqp, "select g.unix_name,g.unix_gid "
				   "  from projects as p "
				   "join groups as g on p.pid=g.pid "
				   "where p.approved!=0 and "
				   "      FIND_IN_SET('%s',pcremote_ok)>0",
				   2, reqp->type);
		}
	}
	if (!res) {
		error("ACCOUNTS: %s: DB Error getting gids!\n", reqp->pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		error("ACCOUNTS: %s: No Project!\n", reqp->pid);
		mysql_free_result(res);
		return 1;
	}

	accounts_node = new_response(root, "accounts");

	while (nrows) {
		row = mysql_fetch_row(res);
		if (!row[1] || !row[1][1]) {
			error("ACCOUNTS: %s: No Project GID!\n", reqp->pid);
			mysql_free_result(res);
			return 1;
		}

		node = new_response(accounts_node, "addgroup");
		add_key(node, "name", row[0]);
		add_key(node, "gid", row[1]);

		nrows--;
	}
	mysql_free_result(res);

	/*
	 * Each time a node picks up accounts, decrement the update
	 * counter. This ensures that if someone kicks off another
	 * update after this point, the node will end up getting to
	 * do it again in case it missed something.
	 */
	if (mydb_update(reqp, "update nodes set update_accounts=update_accounts-1 "
			"where node_id='%s' and update_accounts!=0",
			reqp->nodeid)) {
		error("ACCOUNTS: %s: DB Error setting exit update_accounts!\n",
		      reqp->nodeid);
	}
			 
	/*
	 * Now onto the users in the project.
	 */
	if (reqp->iscontrol) {
		/*
		 * All users! This is not currently used. The problem
		 * is that returning a list of hundreds of users whenever
		 * any single change is required is bad. Works fine for
		 * experimental nodes where the number of accounts is small,
		 * but is not scalable. 
		 */
		res = mydb_query(reqp, "select distinct "
				 "  u.uid,u.usr_pswd,u.unix_uid,u.usr_name, "
				 "  p.trust,g.pid,g.gid,g.unix_gid,u.admin, "
				 "  u.emulab_pubkey,u.home_pubkey, "
				 "  UNIX_TIMESTAMP(u.usr_modified), "
				 "  u.usr_email,u.usr_shell,u.uid_idx "
				 "from group_membership as p "
				 "join users as u on p.uid_idx=u.uid_idx "
				 "join groups as g on p.pid=g.pid "
				 "where p.trust!='none' "
				 "      and u.webonly=0 "
                                 "      and g.unix_id is not NULL "
				 "      and u.status='active' order by u.uid",
				 15, reqp->pid, reqp->gid);
	}
	else if (nodetypeprojects) {
		/*
		 * The projects/groups are specified as a comma separated
		 * list in the node_type_attributes table. Return this
		 * set of users.
		 */
		res = mydb_query(reqp, "select distinct  "
				 "u.uid,'*',u.unix_uid,u.usr_name, "
				 "m.trust,g.pid,g.gid,g.unix_gid,u.admin, "
				 "u.emulab_pubkey,u.home_pubkey, "
				 "UNIX_TIMESTAMP(u.usr_modified), "
				 "u.usr_email,u.usr_shell, "
				 "u.widearearoot,u.wideareajailroot, "
				 "u.usr_w_pswd,u.uid_idx "
				 "from projects as p "
				 "join group_membership as m on "
				 "     m.pid_idx=p.pid_idx "
				 "join groups as g on "
				 "     g.gid_idx=m.gid_idx "
				 "join users as u on u.uid_idx=m.uid_idx "
				 "where p.approved!=0 "
				 "      and m.trust!='none' "
				 "      and u.webonly=0 "
                                 "      and g.unix_gid is not NULL "
				 "      and u.status='active' "
				 "      and u.admin=0 and "
				 "  (FIND_IN_SET(g.gid_idx, "
				 "   (select na.attrvalue from nodes as n "
				 "    left join node_type_attributes as na on "
				 "         n.type=na.type "
				 "    where n.node_id='%s' and "
				 "    na.attrkey='project_accounts')) > 0) "
				 "order by u.uid",
				 18, reqp->nodeid);
	}
	else if (reqp->islocal || reqp->isvnode) {
		/*
		 * This crazy join is going to give us multiple lines for
		 * each user that is allowed on the node, where each line
		 * (for each user) differs by the project PID and it unix
		 * GID. The intent is to build up a list of GIDs for each
		 * user to return. Well, a primary group and a list of aux
		 * groups for that user.
		 */
	  	char adminclause[MYBUFSIZE];
		strcpy(adminclause, "");
#ifdef ISOLATEADMINS
		sprintf(adminclause, "and u.admin=%d", reqp->swapper_isadmin);
#endif
		res = mydb_query(reqp, "select distinct "
				 "  u.uid,u.usr_pswd,u.unix_uid,u.usr_name, "
				 "  p.trust,g.pid,g.gid,g.unix_gid,u.admin, "
				 "  u.emulab_pubkey,u.home_pubkey, "
				 "  UNIX_TIMESTAMP(u.usr_modified), "
				 "  u.usr_email,u.usr_shell, "
				 "  u.widearearoot,u.wideareajailroot, "
				 "  u.usr_w_pswd,u.uid_idx "
				 "from group_membership as p "
				 "join users as u on p.uid_idx=u.uid_idx "
				 "join groups as g on "
				 "     p.pid=g.pid and p.gid=g.gid "
				 "where ((p.pid='%s')) and p.trust!='none' "
				 "      and u.status='active' "
				 "      and u.webonly=0 "
				 "      %s "
                                 "      and g.unix_gid is not NULL "
				 "order by u.uid",
				 18, reqp->pid, adminclause);
	}
	else if (reqp->jailflag) {
		/*
		 * A remote node, doing jails. We still want to return
		 * accounts for the admin people outside the jails.
		 */
		res = mydb_query(reqp, "select distinct "
			     "  u.uid,'*',u.unix_uid,u.usr_name, "
			     "  p.trust,g.pid,g.gid,g.unix_gid,u.admin, "
			     "  u.emulab_pubkey,u.home_pubkey, "
			     "  UNIX_TIMESTAMP(u.usr_modified), "
			     "  u.usr_email,u.usr_shell, "
			     "  u.widearearoot,u.wideareajailroot, "
			     "  u.usr_w_pswd,u.uid_idx "
			     "from group_membership as p "
			     "join users as u on p.uid_idx=u.uid_idx "
			     "join groups as g on "
			     "     p.pid=g.pid and p.gid=g.gid "
			     "where (p.pid='%s') and p.trust!='none' "
			     "      and u.status='active' and u.admin=1 "
			     "      order by u.uid",
			     18, RELOADPID);
	}
	else if (!reqp->islocal && reqp->isdedicatedwa) {
		/*
		 * Wonder why this code is a copy of the islocal || vnode
		 * case above?  It's because I don't want to prevent us from
		 * from the above case where !islocal && jailflag
		 * for dedicated widearea nodes.
		 */

		/*
		 * This crazy join is going to give us multiple lines for
		 * each user that is allowed on the node, where each line
		 * (for each user) differs by the project PID and it unix
		 * GID. The intent is to build up a list of GIDs for each
		 * user to return. Well, a primary group and a list of aux
		 * groups for that user.
		 */
	  	char adminclause[MYBUFSIZE];
		strcpy(adminclause, "");
#ifdef ISOLATEADMINS
		sprintf(adminclause, "and u.admin=%d", reqp->swapper_isadmin);
#endif
		res = mydb_query("select distinct "
				 "  u.uid,'*',u.unix_uid,u.usr_name, "
				 "  p.trust,g.pid,g.gid,g.unix_gid,u.admin, "
				 "  u.emulab_pubkey,u.home_pubkey, "
				 "  UNIX_TIMESTAMP(u.usr_modified), "
				 "  u.usr_email,u.usr_shell, "
				 "  u.widearearoot,u.wideareajailroot, "
				 "  u.usr_w_pswd,u.uid_idx "
				 "from group_membership as p "
				 "join users as u on p.uid_idx=u.uid_idx "
				 "join groups as g on "
				 "     p.pid=g.pid and p.gid=g.gid "
				 "where ((p.pid='%s')) and p.trust!='none' "
				 "      and u.status='active' "
				 "      and u.webonly=0 "
				 "      %s "
                                 "      and g.unix_gid is not NULL "
				 "order by u.uid",
				 18, reqp->pid, adminclause);
	}
	else if (!reqp->islocal && reqp->isdedicatedwa) {
	        /*
		 * Catch widearea nodes that are dedicated to a single pid/eid.
		 * Same as local case.
		 */
		res = mydb_query("select unix_name,unix_gid from groups "
				 "where pid='%s'",
				 2, reqp->pid);
	}
	else {
		/*
		 * XXX - Old style node, not doing jails.
		 *
		 * Temporary hack until we figure out the right model for
		 * remote nodes. For now, we use the pcremote-ok slot in
		 * in the project table to determine what remote nodes are
		 * okay'ed for the project. If connecting node type is in
		 * that list, then return user info for all of the users
		 * in those projects (crossed with group in the project). 
		 */
		char subclause[MYBUFSIZE];
		int  count = 0;
		
		res = mydb_query(reqp, "select attrvalue from node_attributes "
				 " where node_id='%s' and "
				 "       attrkey='dp_projects'",
				 1, reqp->pnodeid);

		if (res) {
			if ((int)mysql_num_rows(res) > 0) {
				row = mysql_fetch_row(res);

				count = snprintf(subclause,
					     sizeof(subclause) - 1,
					     "FIND_IN_SET(g.gid_idx,'%s')>0",
					     row[0]);
			}
			else {
				count = snprintf(subclause,
					     sizeof(subclause) - 1,
					     "FIND_IN_SET('%s',pcremote_ok)>0",
					     reqp->type);
			}
			mysql_free_result(res);
			
			if (count >= sizeof(subclause)) {
				error("ACCOUNTS: %s: Subclause too long!\n",
				      reqp->nodeid);
				return 1;
			}
		}
			
		res = mydb_query(reqp, "select distinct  "
				 "u.uid,'*',u.unix_uid,u.usr_name, "
				 "m.trust,g.pid,g.gid,g.unix_gid,u.admin, "
				 "u.emulab_pubkey,u.home_pubkey, "
				 "UNIX_TIMESTAMP(u.usr_modified), "
				 "u.usr_email,u.usr_shell, "
				 "u.widearearoot,u.wideareajailroot, "
				 "u.usr_w_pswd,u.uid_idx "
				 "from projects as p "
				 "join group_membership as m "
				 "  on m.pid=p.pid "
				 "join groups as g on "
				 "  g.pid=m.pid and g.gid=m.gid "
				 "join users as u on u.uid_idx=m.uid_idx "
				 "where p.approved!=0 "
				 "      and %s "
				 "      and m.trust!='none' "
				 "      and u.webonly=0 "
				 "      and u.admin=0 "
                                 "      and g.unix_gid is not NULL "
				 "      and u.status='active' "
				 "order by u.uid",
				 18, subclause);
	}

	if (!res) {
		error("ACCOUNTS: %s: DB Error getting users!\n", reqp->pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		error("ACCOUNTS: %s: No Users!\n", reqp->pid);
		mysql_free_result(res);
		return 0;
	}

 again:
	row = mysql_fetch_row(res);
	while (nrows) {
		MYSQL_ROW	nextrow = 0;
		MYSQL_RES	*pubkeys_res;
		MYSQL_RES	*sfskeys_res;
		int		pubkeys_nrows, sfskeys_nrows, i;
		int 		is_root;
		int		auxgids[128], gcount = 0;
		char		glist[BUFSIZ];
		char		home_dir[BUFSIZ];
		char		*pswd, *wpswd, wpswd_buf[9];

		gidint     = -1;
		tbadmin    = is_root = atoi(row[8]);
		gcount     = 0;
		
		while (1) {
			
			/*
			 * The whole point of this mess. Figure out the
			 * main GID and the aux GIDs. Perhaps trying to make
			 * distinction between main and aux is unecessary, as
			 * long as the entire set is represented.
			 */
			if (strcmp(row[5], reqp->pid) == 0 &&
			    strcmp(row[6], reqp->gid) == 0) {
				gidint = atoi(row[7]);

				/*
				 * Only people in the main pid can get root
				 * at this time, so do this test here.
				 */
				if ((strcmp(row[4], "local_root") == 0) ||
				    (strcmp(row[4], "group_root") == 0) ||
				    (strcmp(row[4], "project_root") == 0))
					is_root = 1;
			}
			else {
				int k, newgid = atoi(row[7]);
				
				/*
				 * Avoid dups, which can happen because of
				 * different trust levels in the join.
				 */
				for (k = 0; k < gcount; k++) {
				    if (auxgids[k] == newgid)
					goto skipit;
				}
				auxgids[gcount++] = newgid;
			skipit:
				;
			}
			nrows--;

			if (!nrows)
				break;

			/*
			 * See if the next row is the same UID. If so,
			 * we go around the inner loop again.
			 */
			nextrow = mysql_fetch_row(res);
			if (strcmp(row[0], nextrow[0]))
				break;
			row = nextrow;
		}

		/*
		 * widearearoot and wideareajailroot override trust values
		 * from the project (above) (IF the node is not isdedicatedwa,
		 * since these must behave like local). Of course, tbadmin
		 * overrides everthing!
		 */
		if (!reqp->islocal && !reqp->isdedicatedwa && !reqp->isplabdslice) {
			if (!reqp->isvnode)
				is_root = atoi(row[14]);
			else
				is_root = atoi(row[15]);

			if (tbadmin)
				is_root = 1;
		}

		/* There is an optional Windows password column. */
		pswd = row[1];
		wpswd = row[16];
		if (strncmp(rdata, "windows", 7) == 0) {
			if (wpswd != NULL && strlen(wpswd) > 0) {
				row[1] = wpswd;
			}
			else {

				/* The initial random default for the Windows Password
				 * is based on the Unix encrypted password hash, in
				 * particular the random salt when it's an MD5 crypt.
				 * THis is the 8 characters after an initial "$1$" and
				 * followed by a "$".  Just use the first 8 chars if
				 * the hash is not an MD5 crypt.
				 */
				strncpy(wpswd_buf, 
					(strncmp(pswd,"$1$",3)==0) ? pswd + 3 : pswd,
					8);
				wpswd_buf[8]='\0';
				row[1] = wpswd_buf;
			}
		}
		 
		/*
		 * Okay, process the UID. If there is no primary gid,
		 * then use one from the list. Then convert the rest of
		 * the list for the GLIST argument below.
		 */
		if (gidint == -1) {
			gidint = auxgids[--gcount];
		}
		glist[0] = '\0';
		for (i = 0; i < gcount; i++) {
			sprintf(&glist[strlen(glist)], "%d", auxgids[i]);

			if (i < gcount-1)
				strcat(glist, ",");
		}

		if (!reqp->islocal) {
			row[1] = "*";
		}

		strcpy(home_dir, USERDIR);
		strcat(home_dir, row[0]);

		node = new_response(accounts_node, "adduser");
		add_key(node, "login", row[0]);
		add_key(node, "pswd", row[1]);
		add_key(node, "uid", row[2]);
		add_key(node, "gid", row[7]);
		add_key(node, "root", (is_root ? "1" : "0"));
		add_key(node, "name", row[3]);
		add_key(node, "homedir", home_dir);
		add_key(node, "glist", glist);
		add_key(node, "serial", row[11]);
		add_key(node, "email", row[12]);
		add_key(node, "shell", row[13]);

		if (reqp->verbose)
			info("ACCOUNTS: "
			     "ADDUSER LOGIN=%s "
			     "UID=%s GID=%d ROOT=%d GLIST=%s\n",
			     row[0], row[2], gidint, is_root, glist);

		/*
		 * Locally, everything is NFS mounted so no point in
		 * sending back pubkey stuff; it's never used except on CygWin.
		 * Add an argument of "pubkeys" to get the PUBKEY data.
		 * An "windows" argument also returns a user's Windows Password.
		 */
		if (reqp->islocal && !reqp->genisliver_idx &&
		    ! (strncmp(rdata, "pubkeys", 7) == 0
		       || strncmp(rdata, "windows", 7) == 0))
			goto skipsshkeys;

		/*
		 * Need a list of keys for this user.
		 */
		pubkeys_res = mydb_query(reqp, "select idx,pubkey "
					 " from user_pubkeys "
					 "where uid_idx='%s'",
					 2, row[17]);
	
		if (!pubkeys_res) {
			error("ACCOUNTS: %s: DB Error getting keys\n", row[0]);
			goto skipkeys;
		}
		if ((pubkeys_nrows = (int)mysql_num_rows(pubkeys_res))) {
			while (pubkeys_nrows) {
				MYSQL_ROW	pubkey_row;

				pubkey_row = mysql_fetch_row(pubkeys_res);

				/*
				 * Pubkeys can be large, so we may have to
				 * malloc a special buffer.
				 */
				node = new_response(accounts_node, "pubkey");
				add_key(node, "login", row[0]);
				add_key(node, "key", pubkey_row[1]);

				pubkeys_nrows--;
			}
			
		}
		mysql_free_result(pubkeys_res);

	skipsshkeys:
		/*
		 * Do not bother to send back SFS keys if the node is not
		 * running SFS.
		 */
		if (!strlen(reqp->sfshostid))
			goto skipkeys;

		/*
		 * Need a list of SFS keys for this user.
		 */
		sfskeys_res = mydb_query(reqp, "select comment,pubkey "
					 " from user_sfskeys "
					 "where uid_idx='%s'",
					 2, row[17]);

		if (!sfskeys_res) {
			error("ACCOUNTS: %s: DB Error getting SFS keys\n", row[0]);
			goto skipkeys;
		}
		if ((sfskeys_nrows = (int)mysql_num_rows(sfskeys_res))) {
			while (sfskeys_nrows) {
				MYSQL_ROW	sfskey_row;

				sfskey_row = mysql_fetch_row(sfskeys_res);

				node = new_response(accounts_node, "sfskey");
				add_key(node, "key", sfskey_row[1]);

				sfskeys_nrows--;

				if (reqp->verbose)
					info("ACCOUNTS: SFSKEY LOGIN=%s "
					     "COMMENT=%s\n",
					     row[0], sfskey_row[0]);
			}
		}
		mysql_free_result(sfskeys_res);
		
	skipkeys:
		row = nextrow;
	}
	mysql_free_result(res);

	if (!(reqp->islocal || reqp->isvnode) && !didwidearea) {
		didwidearea = 1;

		/*
		 * Sleazy. The only real downside though is that
		 * we could get some duplicate entries, which won't
		 * really harm anything on the client.
		 */
		res = mydb_query(reqp, "select distinct "
				 "u.uid,'*',u.unix_uid,u.usr_name, "
				 "w.trust,'guest','guest',31,u.admin, "
				 "u.emulab_pubkey,u.home_pubkey, "
				 "UNIX_TIMESTAMP(u.usr_modified), "
				 "u.usr_email,u.usr_shell, "
				 "u.widearearoot,u.wideareajailroot "
				 "from widearea_accounts as w "
				 "join users as u on u.uid_idx=w.uid_idx "
				 "where w.trust!='none' and "
				 "      u.status='active' and "
				 "      node_id='%s' "
				 "order by u.uid",
				 16, reqp->nodeid);

		if (res) {
			if ((nrows = mysql_num_rows(res)))
				goto again;
			else
				mysql_free_result(res);
		}
	}
	if (reqp->genisliver_idx && !didnonlocal) {
	        didnonlocal = 1;

		res = mydb_query("select distinct "
				 "  u.uid,'*',u.uid_idx,u.name, "
				 "  'local_root',g.pid,g.gid,g.unix_gid,0, "
				 "  NULL,NULL,0, "
				 "  u.email,'csh', "
				 "  0,0, "
				 "  NULL,u.uid_idx "
				 "from nonlocal_user_bindings as b "
				 "join nonlocal_users as u on "
				 "     b.uid_idx=u.uid_idx "
				 "join groups as g on "
				 "     g.pid='%s' and g.pid=g.gid "
				 "where (b.exptidx='%d') "
				 "order by u.uid",
				 18, reqp->pid, reqp->exptidx);
		
		if (res) {
			if ((nrows = mysql_num_rows(res)))
				goto again;
			else
				mysql_free_result(res);
		}
	}
	return 0;
}

/*
 * Return delay config stuff.
 */
XML_COMMAND_PROTOTYPE(dodelay)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode *node;

	/*
	 * Get delay parameters for the machine. The point of this silly
	 * join is to get the type out so that we can pass it back. Of
	 * course, this assumes that the type is the BSD name, not linux.
	 */
	res = mydb_query(reqp, "select i.MAC,j.MAC, "
		 "pipe0,delay0,bandwidth0,lossrate0,q0_red, "
		 "pipe1,delay1,bandwidth1,lossrate1,q1_red, "
		 "vname, "
		 "q0_limit,q0_maxthresh,q0_minthresh,q0_weight,q0_linterm, " 
		 "q0_qinbytes,q0_bytes,q0_meanpsize,q0_wait,q0_setbit, " 
		 "q0_droptail,q0_gentle, "
		 "q1_limit,q1_maxthresh,q1_minthresh,q1_weight,q1_linterm, "
		 "q1_qinbytes,q1_bytes,q1_meanpsize,q1_wait,q1_setbit, "
		 "q1_droptail,q1_gentle,vnode0,vnode1,noshaping, "
		 "backfill0,backfill1"
                 " from delays as d "
		 "left join interfaces as i on "
		 " i.node_id=d.node_id and i.iface=iface0 "
		 "left join interfaces as j on "
		 " j.node_id=d.node_id and j.iface=iface1 "
		 " where d.node_id='%s'",	 
		 42, reqp->nodeid);
	if (!res) {
		error("DELAY: %s: DB Error getting delays!\n", reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	while (nrows) {
		row = mysql_fetch_row(res);

		/*
		 * Yikes, this is ugly! Sanity check though, since I saw
		 * some bogus values in the DB.
		 */
		if (!row[0] || !row[1] || !row[2] || !row[3]) {
			error("DELAY: %s: DB values are bogus!\n",
			      reqp->nodeid);
			mysql_free_result(res);
			return 1;
		}


		node = new_response(root, "delay");
		add_key(node, "int0", row[0]);
		add_key(node, "int1", row[1]);
		add_key(node, "pipe0", row[2]);
		add_key(node, "delay0", row[3]);
		add_key(node, "bw0", row[4]);
		add_key(node, "plr0", row[5]);
		add_key(node, "pipe1", row[7]);
		add_key(node, "delay1", row[8]);
		add_key(node, "bw1", row[9]);
		add_key(node, "plr1", row[10]);
		add_key(node, "linkname", row[12]);
		add_key(node, "red0", row[6]);
		add_key(node, "red1", row[11]);
		add_key(node, "limit0", row[13]);
		add_key(node, "maxthresh0", row[14]);
		add_key(node, "minthresh0", row[15]);
		add_key(node, "weight0", row[16]);
		add_key(node, "linterm0", row[17]);
		add_key(node, "qinbytes0", row[18]);
		add_key(node, "bytes0", row[19]);
		add_key(node, "meanpsize0", row[20]);
		add_key(node, "wait0", row[21]);
		add_key(node, "setbit0", row[22]);
		add_key(node, "droptail0", row[23]);
		add_key(node, "gentle0", row[24]);
		add_key(node, "limit1", row[25]);
		add_key(node, "maxthresh1", row[26]);
		add_key(node, "minthresh1", row[27]);
		add_key(node, "weight1", row[28]);
		add_key(node, "linterm1", row[29]);
		add_key(node, "qinbytes1", row[30]);
		add_key(node, "bytes1", row[31]);
		add_key(node, "meanpsize1", row[32]);
		add_key(node, "wait1", row[33]);
		add_key(node, "setbit1", row[34]);
		add_key(node, "droptail1", row[35]);
		add_key(node, "gentle1", row[36]);
		add_key(node, "vnode0", (row[37] ? row[37] : "foo"));
		add_key(node, "vnode1", (row[38] ? row[38] : "bar"));
		add_key(node, "noshaping", row[39]);
		add_key(node, "backfill0", row[40]);
		add_key(node, "backfill1", row[41]);

		nrows--;
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return link delay config stuff.
 */
XML_COMMAND_PROTOTYPE(dolinkdelay)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		buf[2*MYBUFSIZE];
	int		nrows;
	xmlNode		*node;

	/*
	 * Get link delay parameters for the machine. We store veth
	 * interfaces in another dynamic table, so must join with both
	 * interfaces and vinterfaces to see which iface this link
	 * delay corresponds to. If there is a veth entry use that, else
	 * use the normal interfaces entry. I do not like this much.
	 * Maybe we should use the regular interfaces table, with type veth,
	 * entries added/deleted on the fly. I avoided that cause I view
	 * the interfaces table as static and pertaining to physical
	 * interfaces.
	 *
	 * Outside a vnode, return only those linkdelays for veths that have
	 * vnode=NULL, which indicates its an emulated interface on a
	 * physical node. When inside a vnode, only return veths for which
	 * vnode=curvnode, which are the interfaces that correspond to a
	 * jail node.
	 */
	if (reqp->isvnode)
		sprintf(buf, "and v.vnode_id='%s'", reqp->vnodeid);
	else
		strcpy(buf, "and v.vnode_id is NULL");

	res = mydb_query(reqp, "select i.MAC,d.type,vlan,vnode,d.ip,netmask, "
		 "pipe,delay,bandwidth,lossrate, "
		 "rpipe,rdelay,rbandwidth,rlossrate, "
		 "q_red,q_limit,q_maxthresh,q_minthresh,q_weight,q_linterm, " 
		 "q_qinbytes,q_bytes,q_meanpsize,q_wait,q_setbit, " 
		 "q_droptail,q_gentle,v.mac "
                 " from linkdelays as d "
		 "left join interfaces as i on "
		 " i.node_id=d.node_id and i.iface=d.iface "
		 "left join vinterfaces as v on "
		 " v.node_id=d.node_id and v.IP=d.ip "
		 "where d.node_id='%s' %s",
		 28, reqp->pnodeid, buf);
	if (!res) {
		error("LINKDELAY: %s: DB Error getting link delays!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	while (nrows) {
		row = mysql_fetch_row(res);
		node = new_response(root, "linkdelay");

		add_key(node, "iface", (row[27] ? row[27] : row[0]));
		add_key(node, "type", row[1]);
		add_key(node, "linkname", row[2]);
		add_key(node, "vnode", row[3]);
		add_key(node, "inet", row[4]);
		add_key(node, "mask", CHECKMASK(row[5]));
		add_key(node, "pipe", row[6]);
		add_key(node, "delay", row[7]);
		add_key(node, "bw", row[8]);
		add_key(node, "plr", row[9]);
		add_key(node, "rpipe", row[10]);
		add_key(node, "rdelay", row[11]);
		add_key(node, "rbw", row[12]);
		add_key(node, "rplr", row[14]);
		add_key(node, "red", row[14]);
		add_key(node, "limit", row[15]);
		add_key(node, "maxthresh", row[16]);
		add_key(node, "minthresh", row[17]);
		add_key(node, "weight", row[18]);
		add_key(node, "linterm", row[19]);
		add_key(node, "qinbytes", row[20]);
		add_key(node, "bytes", row[21]);
		add_key(node, "meanpsize", row[22]);
		add_key(node, "wait", row[23]);
		add_key(node, "setbit", row[24]);
		add_key(node, "droptail", row[25]);
		add_key(node, "gentle", row[26]);
			
		nrows--;
		if (reqp->verbose)
			info("LINKDELAY: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

XML_COMMAND_PROTOTYPE(dohosts)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		hostcount, nrows;
	int		rv = 0;
	char		*thisvnode = (char *) NULL;
	xmlNode		*node;

	/*
	 * We build up a canonical host table using this data structure.
	 * There is one item per node/iface. We need a shared structure
	 * though for each node, to allow us to compute the aliases.
	 */
	struct shareditem {
	    	int	hasalias;
		char	*firstvlan;	/* The first vlan to another node */
		int	is_me;          /* 1 if this node is the tmcc client */
	};
	struct shareditem *shareditem = (struct shareditem *) NULL;
	
	struct hostentry {
		char	nodeid[TBDB_FLEN_NODEID];
		char	vname[TBDB_FLEN_VNAME];
		char	vlan[TBDB_FLEN_VNAME];
		int	virtiface;
		struct in_addr	  ipaddr;
		struct shareditem *shared;
		struct hostentry  *next;
	} *hosts = 0, *host;

	/*
	 * Now use the virt_nodes table to get a list of all of the
	 * nodes and all of the IPs on those nodes. This is the basis
	 * for the canonical host table. Join it with the reserved
	 * table to get the node_id at the same time (saves a step).
	 *
	 * XXX NSE hack: Using the v2pmap table instead of reserved because
	 * of multiple simulated to one physical node mapping. Currently,
	 * reserved table contains a vname which is generated in the case of
	 * nse
	 *
	 * XXX PELAB hack: If virt_lans.protocol is 'ipv4' then this is a
	 * "LAN" of real internet nodes and we should return the control
	 * network IP address.  The intent is to give plab nodes some
	 * convenient aliases for refering to each other.
	 */
	res = mydb_query(reqp, "select v.vname,v.vnode,v.ip,v.vport,v2p.node_id,"
			 "v.protocol,i.IP "
			 "    from virt_lans as v "
			 "left join v2pmap as v2p on "
			 "     v.vnode=v2p.vname and v.pid=v2p.pid and "
			 "     v.eid=v2p.eid "
			 "left join nodes as n on "
			 "     v2p.node_id=n.node_id "
			 "left join interfaces as i on "
			 "     n.phys_nodeid=i.node_id and i.role='ctrl' "
			 "where v.pid='%s' and v.eid='%s' and "
			 "      v2p.node_id is not null "
			 "      order by v.vnode,v.vname",
			 7, reqp->pid, reqp->eid);

	if (!res) {
		error("HOSTNAMES: %s: DB Error getting virt_lans!\n",
		      reqp->nodeid);
		return 1;
	}
	if (! (nrows = mysql_num_rows(res))) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Parse the list, creating an entry for each node/IP pair.
	 */
	while (nrows--) {
		row = mysql_fetch_row(res);
		if (!row[0] || !row[0][0] ||
		    !row[1] || !row[1][0])
			continue;

		if (!thisvnode || strcmp(thisvnode, row[1])) {
			if (! (shareditem = (struct shareditem *)
			       calloc(1, sizeof(*shareditem)))) {
				error("HOSTNAMES: "
				      "Out of memory for shareditem!\n");
				exit(1);
			}
			thisvnode = row[1];
		}

		/*
		 * Check to see if this is the node we're talking to
		 */
		if (!strcmp(row[1], reqp->nickname)) {
		    shareditem->is_me = 1;
		}

		/*
		 * Alloc the per-link struct and fill in.
		 */
		if (! (host = (struct hostentry *) calloc(1, sizeof(*host)))) {
			error("HOSTNAMES: Out of memory!\n");
			exit(1);
		}

		strcpy(host->vlan, row[0]);
		strcpy(host->vname, row[1]);
		strcpy(host->nodeid, row[4]);
		host->virtiface = atoi(row[3]);
		host->shared = shareditem;

		/*
		 * As mentioned above, links with protocol 'ipv4'
		 * use the control net addresses of connected nodes.
		 */
		if (row[5] && strcmp("ipv4", row[5]) == 0 && row[6])
			inet_aton(row[6], &host->ipaddr);
		else
			inet_aton(row[2], &host->ipaddr);

		host->next = hosts;
		hosts = host;
	}
	mysql_free_result(res);

	/*
	 * The last part of the puzzle is to determine who is directly
	 * connected to this node so that we can add an alias for the
	 * first link to each connected node (could be more than one link
	 * to another node). Since we have the vlan names for all the nodes,
	 * its a simple matter of looking in the table for all of the nodes
	 * that are in the same vlan as the node that we are making the
	 * host table for.
	 */
	host = hosts;
	while (host) {
		/*
		 * Only care about this nodes vlans.
		 */
		if (strcmp(host->nodeid, reqp->nodeid) == 0 && host->vlan) {
			struct hostentry *tmphost = hosts;

			while (tmphost) {
				if (strlen(tmphost->vlan) &&
				    strcmp(host->vlan, tmphost->vlan) == 0 &&
				    strcmp(host->nodeid, tmphost->nodeid) &&
				    (!tmphost->shared->firstvlan ||
				     !strcmp(tmphost->vlan,
					     tmphost->shared->firstvlan))) {
					
					/*
					 * Use as flag to ensure only first
					 * link flagged as connected (gets
					 * an alias), but since a vlan could
					 * be a real lan with more than one
					 * other member, must tag all the
					 * members.
					 */
					tmphost->shared->firstvlan =
						tmphost->vlan;
				}
				tmphost = tmphost->next;
			}
		}
		host = host->next;
	}
#if 0
	host = hosts;
	while (host) {
		printf("%s %s %s %d %s %d\n", host->vname, host->nodeid,
		       host->vlan, host->virtiface, inet_ntoa(host->ipaddr),
		       host->connected);
		host = host->next;
	}
#endif

	/*
	 * Okay, spit the entries out!
	 */
	hostcount = 0;
	host = hosts;
	while (host) {
		char	*alias = " ";

		if ((host->shared->firstvlan &&
		     !strcmp(host->shared->firstvlan, host->vlan)) ||
		    /* Directly connected, first interface on this node gets an
		       alias */
		    (!strcmp(host->nodeid, reqp->nodeid) && !host->virtiface)){
			alias = host->vname;
		} else if (!host->shared->firstvlan &&
			   !host->shared->hasalias &&
			   !host->shared->is_me) {
		    /* Not directly connected, but we'll give it an alias
		       anyway */
		    alias = host->vname;
		    host->shared->hasalias = 1;
		}

		node = new_response(root, "host");
		add_key(node, "name", host->vname);
		add_key(node, "vlan", host->vlan);
		add_key(node, "ip", inet_ntoa(host->ipaddr));
		add_format_key(node, "vertiface", "%d", host->virtiface);
		add_key(node, "aliases", alias);

#if 0
		OUTPUT(buf, sizeof(buf),
		       "NAME=%s-%s IP=%s ALIASES='%s-%i %s'\n",
		       host->vname, host->vlan,
		       inet_ntoa(host->ipaddr),
		       host->vname, host->virtiface, alias);
#endif

		host = host->next;
		hostcount++;
	}

	/*
	 * For plab slices, lets include boss and ops IPs as well
	 * in case of flaky name service on the nodes.
	 *
	 * XXX we only do this if we were going to return something
	 * otherwise.  In the event there are no other hosts, we would
	 * not overwrite the existing hosts file which already has boss/ops.
	 */
	if (reqp->isplabdslice && hostcount > 0) {
		node = new_response(root, "host");
		add_key(node, "name", BOSSNODE);
		add_key(node, "ip", EXTERNAL_BOSSNODE_IP);
		add_key(node, "name", USERNODE);
		add_key(node, "ip", EXTERNAL_USERNODE_IP);
#if 0
		OUTPUT(buf, sizeof(buf),
		       "NAME=%s IP=%s ALIASES=''\nNAME=%s IP=%s ALIASES=''\n",
		       BOSSNODE, EXTERNAL_BOSSNODE_IP,
		       USERNODE, EXTERNAL_USERNODE_IP);
#endif
		hostcount += 2;
	}

#if 0
	/*
	 * List of control net addresses for jailed nodes.
	 * Temporary.
	 */
	res = mydb_query(reqp, "select r.node_id,r.vname,n.jailip "
			 " from reserved as r "
			 "left join nodes as n on n.node_id=r.node_id "
			 "where r.pid='%s' and r.eid='%s' "
			 "      and jailflag=1 and jailip is not null",
			 3, reqp->pid, reqp->eid);
	if (res) {
	    if ((nrows = mysql_num_rows(res))) {
		while (nrows--) {
		    row = mysql_fetch_row(res);

		    OUTPUT(buf, sizeof(buf),
			   "NAME=%s IP=%s ALIASES='%s.%s.%s.%s'\n",
			   row[0], row[2], row[1], reqp->eid, reqp->pid,
			   OURDOMAIN);
		    client_writeback(sock, buf, strlen(buf), tcp);
		    hostcount++;
		}
	    }
	    mysql_free_result(res);
	}
#endif
	info("HOSTNAMES: %d hosts in list\n", hostcount);
	host = hosts;
	while (host) {
		struct hostentry *tmphost = host->next;
		free(host);
		host = tmphost;
	}
	return rv;
}

/*
 * Return RPM stuff.
 */
XML_COMMAND_PROTOTYPE(dorpms)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		*bp, *sp;
	xmlNode		*node;

	/*
	 * Get RPM list for the node.
	 */
	res = mydb_query(reqp, "select rpms from nodes where node_id='%s' ",
			 1, reqp->nodeid);

	if (!res) {
		error("RPMS: %s: DB Error getting RPMS!\n", reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Text string is a colon separated list.
	 */
	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}
	
	bp  = row[0];
	sp  = bp;
	do {
		bp = strsep(&sp, ":");

		node = new_response(root, "rpm");
		add_key(node, "package", bp);
	} while ((bp = sp));
	
	mysql_free_result(res);
	return 0;
}

/*
 * Return Tarball stuff.
 */
XML_COMMAND_PROTOTYPE(dotarballs)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		*bp, *sp, *tp;
	xmlNode		*node, *tarballs_node;

	/*
	 * Get Tarball list for the node.
	 */
	res = mydb_query(reqp, "select tarballs from nodes where node_id='%s' ",
			 1, reqp->nodeid);

	if (!res) {
		error("TARBALLS: %s: DB Error getting tarballs!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Text string is a colon separated list of "dir filename". 
	 */
	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}
	
	tarballs_node = new_response(root, "tarballs");
	
	bp  = row[0];
	sp  = bp;
	do {
		bp = strsep(&sp, ":");
		if ((tp = strchr(bp, ' ')) == NULL)
			continue;
		*tp++ = '\0';

		node = new_response(tarballs_node, "tarball");
		add_key(node, "dir", bp);
		add_key(node, "filename", tp);
	} while ((bp = sp));
	
	mysql_free_result(res);
	return 0;
}

/*
 * This is deprecated, but left in case old images reference it.
 */
XML_COMMAND_PROTOTYPE(dodeltas)
{
	return 0;
}

/*
 * Return node run command. We return the command to run, plus the UID
 * of the experiment creator to run it as!
 */
XML_COMMAND_PROTOTYPE(dostartcmd)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	xmlNode		*node;

	/*
	 * Get run command for the node.
	 */
	res = mydb_query(reqp, "select startupcmd from nodes where node_id='%s'",
			 1, reqp->nodeid);

	if (!res) {
		error("STARTUPCMD: %s: DB Error getting startup command!\n",
		       reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Simple text string.
	 */
	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}

	node = new_response(root, "startupcmd");
	add_key(node, "cmd", row[0]);
	add_key(node, "uid", reqp->swapper);

	mysql_free_result(res);
	
	return 0;
}

/*
 * Accept notification of start command exit status. 
 */
XML_COMMAND_PROTOTYPE(dostartstat)
{
	int		exitstatus;

	/*
	 * Dig out the exit status
	 */
	if (! sscanf(rdata, "%d", &exitstatus)) {
		error("STARTSTAT: %s: Invalid status: %s\n",
		      reqp->nodeid, rdata);
		return 1;
	}

	if (reqp->verbose)
		info("STARTSTAT: "
		     "%s is reporting startup command exit status: %d\n",
		     reqp->nodeid, exitstatus);

	/*
	 * Update the node table record with the exit status. Setting the
	 * field to a non-null string value is enough to tell whoever is
	 * watching it that the node is done.
	 */
	if (mydb_update(reqp, "update nodes set startstatus='%d' "
			"where node_id='%s'", exitstatus, reqp->nodeid)) {
		error("STARTSTAT: %s: DB Error setting exit status!\n",
		      reqp->nodeid);
		return 1;
	}
	return 0;
}

/*
 * Accept notification of ready for action
 */
XML_COMMAND_PROTOTYPE(doready)
{
	/*
	 * Vnodes not allowed!
	 */
	if (reqp->isvnode)
		return 0;

	/*
	 * Update the ready_bits table.
	 */
	if (mydb_update(reqp, "update nodes set ready=1 "
			"where node_id='%s'", reqp->nodeid)) {
		error("READY: %s: DB Error setting ready bit!\n",
		      reqp->nodeid);
		return 1;
	}

	if (reqp->verbose)
		info("READY: %s: Node is reporting ready\n", reqp->nodeid);

	/*
	 * Nothing is written back
	 */
	return 0;
}

/*
 * Return ready bits count (NofM)
 */
XML_COMMAND_PROTOTYPE(doreadycount)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		total, ready, i;
	xmlNode 	*node;

	/*
	 * Vnodes not allowed!
	 */
	if (reqp->isvnode)
		return 0;

	/*
	 * See how many are ready. This is a non sync protocol. Clients
	 * keep asking until N and M are equal. Can only be used once
	 * of course, after experiment creation.
	 */
	res = mydb_query(reqp, "select ready from reserved "
			 "left join nodes on nodes.node_id=reserved.node_id "
			 "where reserved.eid='%s' and reserved.pid='%s'",
			 1, reqp->eid, reqp->pid);

	if (!res) {
		error("READYCOUNT: %s: DB Error getting ready bits.\n",
		      reqp->nodeid);
		return 1;
	}

	ready = 0;
	total = (int) mysql_num_rows(res);
	if (total) {
		for (i = 0; i < total; i++) {
			row = mysql_fetch_row(res);

			if (atoi(row[0]))
				ready++;
		}
	}
	mysql_free_result(res);

	node = new_response(root, "readycount");
	add_format_key(node, "ready", "%d", ready);
	add_format_key(node, "total", "%d", total);

	return 0;
}

/*
 * Return mount stuff.
 */
XML_COMMAND_PROTOTYPE(domounts)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		nrows, usesfs;
#ifdef  ISOLATEADMINS
	int		isadmin;
#endif
	xmlNode		*node, *mount_node;

	/*
	 * Should SFS mounts be served?
	 */
	usesfs = 0;
	if (strlen(reqp->fshostid)) {
		if (strlen(reqp->sfshostid))
			usesfs = 1;
		else {
			while (*rdata && isspace(*rdata))
				rdata++;

			if (!strncmp(rdata, "USESFS=1", strlen("USESFS=1")))
				usesfs = 1;
		}

		if (reqp->verbose) {
			if (usesfs) {
				info("Using SFS\n");
			}
			else {
				info("Not using SFS\n");
			}
		}
	}

	/*
	 * Remote nodes must use SFS.
	 */
	if (!reqp->islocal && !usesfs)
		return 0;

	/*
	 * Jailed nodes (the phys or virt node) do not get mounts.
	 * Locally, though, the jailflag is not set on nodes (hmm,
	 * maybe fix that) so that the phys node still looks like it
	 * belongs to the experiment (people can log into it). 
	 */
	if (reqp->jailflag)
		return 0;

	/*
	 * XXX
	 */
	if (reqp->genisliver_idx)
		return 0;

	mount_node = new_response(root, "mounts");
	
	/*
	 * If SFS is in use, the project mount is done via SFS.
	 */
	if (!usesfs) {
		/*
		 * Return project mount first. 
		 */
		node = new_response(mount_node, "mount");
		add_format_key(node, "remote", "%s/%s", FSPROJDIR, reqp->pid);
		add_format_key(node, "local", "%s/%s", PROJDIR, reqp->pid);

		/* Leave this logging on all the time for now. */
		/* XXX Ryan */
#if 0
		info("MOUNTS: %s", buf);
#endif
		
#ifdef FSSCRATCHDIR
		/*
		 * Return scratch mount if its defined.
		 */
		node = new_response(mount_node, "mount");
		add_format_key(node, "remote", "%s/%s", FSSCRATCHDIR,
				reqp->pid);
		add_format_key(node, "local", "%s/%s", SCRATCHDIR, reqp->pid);

		/* Leave this logging on all the time for now. */
#if 0
		info("MOUNTS: %s", buf);
#endif
#endif
#ifdef FSSHAREDIR
		/*
		 * Return share mount if its defined.
		 */
		node = new_response(mount_node, "mount");
		add_key(node, "remote", FSSHAREDIR);
		add_key(node, "local", SHAREDIR);

		/* Leave this logging on all the time for now. */
#if 0
		info("MOUNTS: %s", buf);
#endif
#endif
		/*
		 * If pid!=gid, then this is group experiment, and we return
		 * a mount for the group directory too.
		 */
		if (strcmp(reqp->pid, reqp->gid)) {
			node = new_response(mount_node, "mount");
			snprintf(buf, sizeof(buf), "%s/%s/%s", FSGROUPDIR, reqp->pid,
					reqp->gid);
			add_key(node, "remote", buf);
			snprintf(buf, sizeof(buf), "%s/%s/%s", GROUPDIR, reqp->pid,
					reqp->gid);
			add_key(node, "local", buf);

			/* Leave this logging on all the time for now. */
#if 0
			info("MOUNTS: %s", buf);
#endif
		}
	}
	else {
		/*
		 * Return SFS-based mounts. Locally, we send back per
		 * project/group mounts (really symlinks) cause thats the
		 * local convention. For remote nodes, no point. Just send
		 * back mounts for the top level directories. 
		 */
		if (reqp->islocal) {
			node = new_response(mount_node, "sfs");
			snprintf(buf, sizeof(buf), "%s%s/%s", reqp->fshostid,
					FSDIR_PROJ, reqp->pid);
			add_key(node, "remote", buf);
			add_format_key(node, "local", "%s/%s", PROJDIR,
					reqp->pid);

#if 0
			if (reqp->verbose)
				info("MOUNTS: %s", buf);
#endif

			/*
			 * Return SFS-based group mount.
			 */
			if (strcmp(reqp->pid, reqp->gid)) {
				node = new_response(mount_node, "sfs");
				snprintf(buf, sizeof(buf), "%s%s/%s/%s", reqp->fshostid,
						FSDIR_GROUPS, reqp->pid, reqp->gid);
				add_key(node, "remote", buf);
				snprintf(buf, sizeof(buf), "%s/%s/%s", GROUPDIR,
							reqp->pid, reqp->gid);
				add_key(node, "local", buf);

#if 0
				info("MOUNTS: %s", buf);
#endif
			}
#ifdef FSSCRATCHDIR
			/*
			 * Pointer to per-project scratch directory.
			 */

			node = new_response(mount_node, "sfs");
			snprintf(buf, sizeof(buf), "%s%s/%s", reqp->fshostid,
					FSDIR_SCRATCH, reqp->pid);
			add_key(node, "remote", buf);
			add_format_key(node, "local", "%s/%s", SCRATCHDIR, reqp->pid);

/*
			if (reqp->verbose)
				info("MOUNTS: %s", buf);
*/
#endif
#ifdef FSSHAREDIR
			/*
			 * Pointer to /share.
			 */
			node = new_response(mount_node, "sfs");
			snprintf(buf, sizeof(buf), "%s%s", reqp->fshostid,
					FSDIR_SHARE);
			add_key(node, "remote", buf);
			add_format_key(node, "local", "%s", SHAREDIR);

/*
			if (reqp->verbose)
				info("MOUNTS: %s", buf);
*/
#endif
			/*
			 * Return a mount for "certprog dirsearch"
			 * that matches the local convention. This
			 * allows the same paths to work on remote
			 * nodes.
			 */
			node = new_response(mount_node, "sfs");
			snprintf(buf, sizeof(buf), "%s%s/%s", reqp->fshostid,
					FSDIR_PROJ, DOTSFS);
			add_key(node, "remote", buf);
			add_format_key(node, "local", "%s%s", PROJDIR, DOTSFS);
		}
		else {
			/*
			 * Remote nodes get slightly different mounts.
			 * in /netbed.
			 *
			 * Pointer to /proj.
			 */
			node = new_response(mount_node, "sfs");
			snprintf(buf, sizeof(buf), "%s%s", reqp->fshostid,
					FSDIR_PROJ);
			add_key(node, "remote", buf);
			add_format_key(node, "local", "%s/%s", NETBEDDIR, PROJDIR);

#if 0
			if (reqp->verbose)
				info("MOUNTS: %s", buf);
#endif

			/*
			 * Pointer to /groups
			 */
			node = new_response(mount_node, "sfs");
			snprintf(buf, sizeof(buf), "%s%s", reqp->fshostid,
					FSDIR_GROUPS);
			add_key(node, "remote", buf);
			add_format_key(node, "local", "%s%s", NETBEDDIR, PROJDIR);

#if 0
			if (reqp->verbose)
				info("MOUNTS: %s", buf);
#endif

			/*
			 * Pointer to /users
			 */
			node = new_response(mount_node, "sfs");
			snprintf(buf, sizeof(buf), "%s%s", reqp->fshostid,
					FSDIR_USERS);
			add_key(node, "remote", buf);
			add_format_key(node, "local", "%s%s", NETBEDDIR, USERDIR);

#if 0
			if (reqp->verbose)
				info("MOUNTS: %s", buf);
#endif
#ifdef FSSCRATCHDIR
			/*
			 * Pointer to per-project scratch directory.
			 */
			node = new_response(mount_node, "sfs");
			snprintf(buf, sizeof(buf), "%s%s", reqp->fshostid,
					FSDIR_SCRATCH);
			add_key(node, "remote", buf);
			add_format_key(node, "local", "%s/%s", NETBEDDIR, SCRATCHDIR);

#if 0
			if (reqp->verbose)
				info("MOUNTS: %s", buf);
#endif
#endif
#ifdef FSSHAREDIR
			/*
			 * Pointer to /share.
			 */
			node = new_response(mount_node, "sfs");
			snprintf(buf, sizeof(buf), "%s%s", reqp->fshostid,
					FSDIR_SHARE);
			add_key(node, "remote", buf);
			add_format_key(node, "local", "%s%s", NETBEDDIR, SHAREDIR);

#if 0
			if (reqp->verbose)
				info("MOUNTS: %s", buf);
#endif
#endif
		}
	}

	/*
	 * Remote nodes do not get per-user mounts. See above.
	 */
	if (!reqp->islocal || reqp->genisliver_idx)
		return 0;
	
	/*
	 * Now check for aux project access. Return a list of mounts for
	 * those projects.
	 */
	res = mydb_query(reqp, "select pid from exppid_access "
			 "where exp_pid='%s' and exp_eid='%s'",
			 1, reqp->pid, reqp->eid);
	if (!res) {
		error("MOUNTS: %s: DB Error getting users!\n", reqp->pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res))) {
		while (nrows) {
			row = mysql_fetch_row(res);
			node = new_response(mount_node, "mount");
			snprintf(buf, sizeof(buf), "%s/%s", FSPROJDIR,
					row[0]);
			add_key(node, "remote", buf);
			add_format_key(node, "local", "%s/%s", PROJDIR, row[0]);
			
			nrows--;
		}
	}
	mysql_free_result(res);

	/*
	 * Now a list of user directories. These include the members of the
	 * experiments projects, plus all the members of all of the projects
	 * that have been granted access to share the nodes in that expt.
	 */
	res = mydb_query("select u.uid,u.admin from users as u "
			 "left join group_membership as p on "
			 "     p.uid_idx=u.uid_idx "
			 "where p.pid='%s' and p.gid='%s' and "
			 "      u.status='active' and "
			 "      u.webonly=0 and "
			 "      p.trust!='none'",
			 2, reqp->pid, reqp->gid);
	if (!res) {
		error("MOUNTS: %s: DB Error getting users!\n", reqp->pid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		error("MOUNTS: %s: No Users!\n", reqp->pid);
		mysql_free_result(res);
		return 0;
	}

	while (nrows--) {
		row = mysql_fetch_row(res);

#ifdef ISOLATEADMINS
		isadmin = atoi(row[1]);
		if (isadmin != reqp->swapper_isadmin) {
			continue;
		}
#endif
		node = new_response(mount_node, "mount");
		snprintf(buf, sizeof(buf), "%s/%s", FSUSERDIR,
				row[0]);
		add_key(node, "remote", buf);
		add_format_key(node, "local", "%s/%s", USERDIR, row[0]);
				
		if (reqp->verbose)
		    info("MOUNTS: %s", buf);
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Used by dosfshostid to make sure NFS doesn't give us problems.
 * (This code really unnerves me)
 */
int sfshostiddeadfl;
jmp_buf sfshostiddeadline;
static void
dosfshostiddead()
{
	sfshostiddeadfl = 1;
	longjmp(sfshostiddeadline, 1);
}

static int
safesymlink(char *name1, char *name2)
{
	/*
	 * Really, there should be a cleaner way of doing this, but
	 * this works, at least for now.  Perhaps using the DB and a
	 * symlinking deamon alone would be better.
	 */
	if (setjmp(sfshostiddeadline) == 0) {
		sfshostiddeadfl = 0;
		signal(SIGALRM, dosfshostiddead);
		alarm(1);

		unlink(name2);
		if (symlink(name1, name2) < 0) {
			sfshostiddeadfl = 1;
		}
	}
	alarm(0);
	if (sfshostiddeadfl) {
		errorc("symlinking %s to %s", name2, name1);
		return -1;
	}
	return 0;
}

/*
 * Create dirsearch entry for node.
 */
XML_COMMAND_PROTOTYPE(dosfshostid)
{
	char	nodehostid[HOSTID_SIZE], buf[BUFSIZ];
	char	sfspath[BUFSIZ], dspath[BUFSIZ];
	xmlNode	*node;

	if (!strlen(reqp->fshostid)) {
		/* SFS not being used */
		info("dosfshostid: Called while SFS is not in use\n");
		return 0;
	}
	
	/*
	 * Dig out the hostid. Need to be careful about not overflowing
	 * the buffer.
	 */
	sprintf(buf, "%%%ds", sizeof(nodehostid));
	if (sscanf(rdata, buf, nodehostid) != 1) {
		error("dosfshostid: No hostid reported!\n");
		return 1;
	}

	/*
	 * No slashes allowed! This path is going into a symlink below. 
	 */
	if (index(nodehostid, '/')) {
		error("dosfshostid: %s Invalid hostid: %s!\n",
		      reqp->nodeid, nodehostid);
		return 1;
	}

	/*
	 * Create symlink names
	 */
	node = new_response(root, "sfshostid");
	snprintf(sfspath, sizeof(sfspath), "/sfs/%s", nodehostid);
	snprintf(dspath, sizeof(dspath), "%s/%s/%s.%s.%s", PROJDIR, DOTSFS,
	       reqp->nickname, reqp->eid, reqp->pid);
	add_key(node, "sfspath", sfspath);
	add_key(node, "dspath", dspath);

	/*
	 * Create the symlink. The directory in which this is done has to be
	 * either owned by the same uid used to run tmcd, or in the same group.
	 */ 
	if (safesymlink(sfspath, dspath) < 0) {
		return 1;
	}

	/*
	 * Stash into the DB too.
	 */
	mysql_escape_string(buf, nodehostid, strlen(nodehostid));
	
	if (mydb_update(reqp, "update node_hostkeys set sfshostid='%s' "
			"where node_id='%s'", buf, reqp->nodeid)) {
		error("SFSHOSTID: %s: DB Error setting sfshostid!\n",
		      reqp->nodeid);
		return 1;
	}
	if (reqp->verbose)
		info("SFSHOSTID: %s: %s\n", reqp->nodeid, nodehostid);
	return 0;
}

/*
 * Return routing stuff.
 */
XML_COMMAND_PROTOTYPE(dorouting)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		n, nrows, isstatic = 0;
	xmlNode		*node, *routing_node;

	/*
	 * Get the routing type from the nodes table.
	 */
	res = mydb_query(reqp, "select routertype from nodes where node_id='%s'",
			 1, reqp->nodeid);

	if (!res) {
		error("ROUTES: %s: DB Error getting router type!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	/*
	 * Return type. At some point we might have to return a list of
	 * routes too, if we support static routes specified by the user
	 * in the NS file.
	 */
	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}
	if (!strcmp(row[0], "static")) {
		isstatic = 1;
	}
	routing_node = new_response(root, "routing");
	add_key(routing_node, "routertype", row[0]);
	mysql_free_result(res);

	/*
	 * New images treat "static" as "static-ddijk", so even if there
	 * are routes in the DB, we do not want to return them to the node
	 * since that would be a waste of bandwidth. 
	 */
	if (isstatic) {
		return 0;
	}

	/*
	 * Get the routing type from the nodes table.
	 */
	res = mydb_query(reqp, "select dst,dst_type,dst_mask,nexthop,cost,src "
			 "from virt_routes as vi "
			 "where vi.vname='%s' and "
			 " vi.pid='%s' and vi.eid='%s'",
			 6, reqp->nickname, reqp->pid, reqp->eid);
	
	if (!res) {
		error("ROUTES: %s: DB Error getting manual routes!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	n = nrows;

	while (n) {
		char dstip[32];

		row = mysql_fetch_row(res);
				
		/*
		 * OMG, the Linux route command is too stupid to accept a
		 * host-on-a-subnet as the subnet address, so we gotta mask
		 * off the bits manually for network routes.
		 *
		 * Eventually we'll perform this operation in the NS parser
		 * so it appears in the DB correctly.
		 */
		if (strcmp(row[1], "net") == 0) {
			struct in_addr tip, tmask;

			inet_aton(row[0], &tip);
			inet_aton(row[2], &tmask);
			tip.s_addr &= tmask.s_addr;
			strncpy(dstip, inet_ntoa(tip), sizeof(dstip));
		} else
			strncpy(dstip, row[0], sizeof(dstip));

		node = new_response(routing_node, "route");
		add_key(node, "ipaddr", dstip);
		add_key(node, "type", row[1]);
		add_key(node, "ipmask", row[2]);
		add_key(node, "gateway", row[3]);
		add_key(node, "cost", row[4]);
		add_key(node, "srcipaddr", row[5]);

		n--;
	}
	mysql_free_result(res);
	if (reqp->verbose)
	    info("ROUTES: %d routes in list\n", nrows);

	return 0;
}

/*
 * Return address from which to load an image, along with the partition that
 * it should be written to and the OS type in that partition.
 */
XML_COMMAND_PROTOTYPE(doloadinfo)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		*disktype, *useacpi, *useasf, address[MYBUFSIZE];
	char		mbrvers[51];
	int		disknum, zfill;
	xmlNode		*node;

	/*
	 * Get the address the node should contact to load its image
	 */
	res = mydb_query(reqp, "select load_address,loadpart,OS,frisbee_pid,"
			 "   mustwipe,mbr_version,access_key,imageid"
			 "  from current_reloads as r "
			 "left join images as i on i.imageid = r.image_id "
			 "left join os_info as o on i.default_osid = o.osid "
			 "where node_id='%s'",
			 8, reqp->nodeid);

	if (!res) {
		error("doloadinfo: %s: DB Error getting loading address!\n",
		       reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}
	row = mysql_fetch_row(res);

	/*
	 * Remote nodes get a URL for the address. 
	 */
	node = new_response(root, "loadinfo");
	if (!reqp->islocal) {
		if (!row[6] || !row[6][0]) {
			error("doloadinfo: %s: "
			      "No access key associated with imageid %s\n",
			      reqp->nodeid, row[7]);
			mysql_free_result(res);
			return 1;
		}
		OUTPUT(address, sizeof(address),
		       "%s/spewimage.php?imageid=%s&access_key=%s",
		       TBBASE, row[7], row[6]);
	}
	else {
		/*
		 * Simple text string.
		 */
		if (! row[0] || !row[0][0]) {
			mysql_free_result(res);
			return 0;
		}
		strcpy(address, row[0]);

		/*
		 * Sanity check
		 */
		if (!row[3] || !row[3][0]) {
			error("doloadinfo: %s: "
			      "No pid associated with address %s\n",
			      reqp->nodeid, row[0]);
			mysql_free_result(res);
			return 1;
		}
	}

	add_key(node, "address", address);
	add_key(node, "part", row[1]);
	add_key(node, "partos", row[2]);

	/*
	 * Remember zero-fill free space, mbr version fields, and access_key
	 */
	zfill = 0;
	if (row[4] && row[4][0])
		zfill = atoi(row[4]);
	strcpy(mbrvers,"1");
	if (row[5] && row[5][0])
		strcpy(mbrvers, row[5]);

	/*
	 * Get disk type and number
	 */
	disktype = DISKTYPE;
	disknum = DISKNUM;
	useacpi = "unknown";
	useasf = "unknown";

	res = mydb_query(reqp, "select attrkey,attrvalue from nodes as n "
			 "left join node_type_attributes as a on "
			 "     n.type=a.type "
			 "where (a.attrkey='bootdisk_unit' or "
			 "       a.attrkey='disktype' or "
			 "       a.attrkey='use_acpi' or "
			 "       a.attrkey='use_asf') and "
			 "      n.node_id='%s'", 2, reqp->nodeid);
	
	if (!res) {
		error("doloadinfo: %s: DB Error getting disktype!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) > 0) {
		int nrows = (int)mysql_num_rows(res);

		while (nrows) {
			row = mysql_fetch_row(res);

			if (row[1] && row[1][0]) {
				if (strcmp(row[0], "bootdisk_unit") == 0) {
					disknum = atoi(row[1]);
				}
				else if (strcmp(row[0], "disktype") == 0) {
					disktype = row[1];
				}
				else if (strcmp(row[0], "use_acpi") == 0) {
					useacpi = row[1];
				}
				else if (strcmp(row[0], "use_asf") == 0) {
					useasf = row[1];
				}
			}
			nrows--;
		}
	}

	add_format_key(node, "disk", "%s%d", disktype, disknum);
	add_format_key(node, "zfill", "%d", zfill);
	add_key(node, "acpi", useacpi);
	add_format_key(node, "mbrvers", "%s", mbrvers);
	add_key(node, "asf", useasf);

	mysql_free_result(res);

	return 0;
}

/*
 * Have stated reset any next_pxe_boot_* and next_boot_* fields.
 * Produces no output to the client.
 */
XML_COMMAND_PROTOTYPE(doreset)
{
#ifdef EVENTSYS
	address_tuple_t tuple;
	/*
	 * Send the state out via an event
	 */
	/* XXX: Maybe we don't need to alloc a new tuple every time through */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		error("doreset: Unable to allocate address tuple!\n");
		return 1;
	}

	tuple->host      = BOSSNODE;
	tuple->objtype   = TBDB_OBJECTTYPE_TESTBED; /* "TBCONTROL" */
	tuple->objname	 = reqp->nodeid;
	tuple->eventtype = TBDB_EVENTTYPE_RESET;

	if (myevent_send(reqp, tuple)) {
		error("doreset: Error sending event\n");
		return 1;
	} else {
	        info("Reset event sent for %s\n", reqp->nodeid);
	} 
	
	address_tuple_free(tuple);
#else
	info("No event system - no reset performed.\n");
#endif
	return 0;
}

/*
 * Return trafgens info
 */
XML_COMMAND_PROTOTYPE(dotrafgens)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node, *trafgensNode;

	res = mydb_query(reqp, "select vi.vname,role,proto,"
			 "  vnode,port,ip,target_vnode,target_port,target_ip, "
			 "  generator "
			 " from virt_trafgens as vi "
			 "where vi.vnode='%s' and "
			 " vi.pid='%s' and vi.eid='%s'",
			 10, reqp->nickname, reqp->pid, reqp->eid);

	if (!res) {
		error("TRAFGENS: %s: DB Error getting virt_trafgens\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	trafgensNode = new_response(root, "trafgens");

	while (nrows) {
		char myname[TBDB_FLEN_VNAME+2];
		char peername[TBDB_FLEN_VNAME+2];

		node = new_response(trafgensNode, "entry");
		
		row = mysql_fetch_row(res);

		if (row[5] && row[5][0]) {
			strcpy(myname, row[5]);
			strcpy(peername, row[8]);
		}
		else {
			/* This can go away once the table is purged */
			strcpy(myname, row[3]);
			strcat(myname, "-0");
			strcpy(peername, row[6]);
			strcat(peername, "-0");
		}

		add_key(node, "trafgen", row[0]);
		add_key(node, "myname", myname);
		add_key(node, "myport", row[4]);
		add_key(node, "peername", peername);
		add_key(node, "peerport", row[7]);
		add_key(node, "proto", row[2]);
		add_key(node, "role", row[1]);
		add_key(node, "generator", row[9]);

		nrows--;
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return nseconfigs info
 */
XML_COMMAND_PROTOTYPE(donseconfigs)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node;

	res = mydb_query(reqp, "select nseconfig from nseconfigs as nse "
			 "where nse.pid='%s' and nse.eid='%s' "
			 "and nse.vname='%s'",
			 1, reqp->pid, reqp->eid, reqp->nickname);

	if (!res) {
		error("NSECONFIGS: %s: DB Error getting nseconfigs\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	row = mysql_fetch_row(res);

	/*
	 * Just shove the whole thing out.
	 */

	node = new_response(root, "nseconfigs");
	node = add_key(node, "config", row[0]);
	mysql_free_result(res);
	return 0;
}

/*
 * Report that the node has entered a new state
 */
XML_COMMAND_PROTOTYPE(dostate)
{
	char 		newstate[128];	/* More then we will ever need */
#ifdef EVENTSYS
	address_tuple_t tuple;
#endif

#ifdef LBS
	return 0;
#endif

	/*
	 * Dig out state that the node is reporting
	 */
	if (sscanf(rdata, "%128s", newstate) != 1 ||
	    strlen(newstate) == sizeof(newstate)) {
		error("DOSTATE: %s: Bad arguments\n", reqp->nodeid);
		return 1;
	}

#ifdef EVENTSYS
	/*
	 * Send the state out via an event
	 */
	/* XXX: Maybe we don't need to alloc a new tuple every time through */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		error("dostate: Unable to allocate address tuple!\n");
		return 1;
	}

	tuple->host      = BOSSNODE;
	tuple->objtype   = "TBNODESTATE";
	tuple->objname	 = reqp->nodeid;
	tuple->eventtype = newstate;

	if (myevent_send(reqp, tuple)) {
		error("dostate: Error sending event\n");
		return 1;
	}

	address_tuple_free(tuple);
#endif /* EVENTSYS */
	
	/* Leave this logging on all the time for now. */
	info("STATE: %s\n", newstate);
	return 0;

}

/*
 * Return creator of experiment. Total hack. Must kill this.
 */
XML_COMMAND_PROTOTYPE(docreator)
{
	xmlNode		*node;

	node = new_response(root, "creator");
	add_key(node, "creator", reqp->creator);
	add_key(node, "swapper", reqp->swapper);

	return 0;
}

/*
 * Return tunnels info.
 */
XML_COMMAND_PROTOTYPE(dotunnels)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node, *tunnels_node;

	res = mydb_query(reqp, "select lma.lanid,lma.memberid,"
			 "   lma.attrkey,lma.attrvalue from lans as l "
			 "left join lan_members as lm on lm.lanid=l.lanid "
			 "left join lan_member_attributes as lma on "
			 "     lma.lanid=lm.lanid and "
			 "     lma.memberid=lm.memberid "
			 "where l.exptidx='%d' and l.type='tunnel' and "
			 "      lm.node_id='%s' and "
			 "      lma.attrkey like 'tunnel_%%'",
			 4, reqp->exptidx, reqp->nodeid);

	if (!res) {
		error("TUNNELS: %s: DB Error getting tunnels\n", reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	tunnels_node = new_response(root, "tunnels");
	while (nrows) {
		row = mysql_fetch_row(res);
		node = new_response(tunnels_node, "tunnel");

		add_key(node, "tunnel", row[0]);
		add_key(node, "member", row[1]);
		add_key(node, "key", row[2]);
		add_key(node, "value", row[3]);

		nrows--;
		if (reqp->verbose)
			info("TUNNEL=%s MEMBER=%s KEY='%s' VALUE='%s'\n",
			     row[0], row[1], row[2], row[3]);
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return vnode list for a widearea node.
 */
XML_COMMAND_PROTOTYPE(dovnodelist)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node, *vnodelist_node;

	res = mydb_query(reqp, "select r.node_id,n.jailflag from reserved as r "
			 "left join nodes as n on r.node_id=n.node_id "
                         "left join node_types as nt on nt.type=n.type "
                         "where nt.isvirtnode=1 and n.phys_nodeid='%s'",
                         2, reqp->nodeid);

	if (!res) {
		error("VNODELIST: %s: DB Error getting vnode list\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	vnodelist_node = new_response(root, "vnodelist");
	while (nrows) {
		row = mysql_fetch_row(res);
		node = new_response(vnodelist_node, "vnode");

		/* XXX Plab? */
		add_key(node, "vnodeid", row[0]);
		add_key(node, "jailed", row[1]);
		
		nrows--;
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return subnode list, and their types.
 */
XML_COMMAND_PROTOTYPE(dosubnodelist)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*subnodelist_node, *node;

	res = mydb_query(reqp, "select n.node_id,nt.class from nodes as n "
                         "left join node_types as nt on nt.type=n.type "
                         "where nt.issubnode=1 and n.phys_nodeid='%s'",
                         2, reqp->nodeid);

	if (!res) {
		error("SUBNODELIST: %s: DB Error getting vnode list\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	subnodelist_node = new_response(root, "subnodelist");
	while (nrows) {
		row = mysql_fetch_row(res);
		node = new_response(subnodelist_node, "subnode");
		add_key(node, "nodeid", row[0]);
		add_key(node, "type", row[1]);

		nrows--;
	}
	mysql_free_result(res);
	return 0;
}

/*
 * IsAlive(). Mark nodes as being alive. 
 */
XML_COMMAND_PROTOTYPE(doisalive)
{
	int		doaccounts = 0;
	xmlNode		*node;

	/*
	 * See db/node_status script, which uses this info (timestamps)
	 * to determine when nodes are down.
	 */
	mydb_update(reqp, "replace into node_status "
		    " (node_id, status, status_timestamp) "
		    " values ('%s', 'up', now())",
		    reqp->nodeid);

	/*
	 * Return info about what needs to be updated. 
	 */
	if (reqp->update_accounts)
		doaccounts = 1;
	
	/*
	 * At some point, maybe what we will do is have the client
	 * make a request asking what needs to be updated. Right now,
	 * just return yes/no and let the client assume it knows what
	 * to do (update accounts).
	 */
	node = new_response(root, "isalive");
	add_key(node, "update", (doaccounts ? "1" : "0"));

	return 0;
}
  
/*
 * Return ipod info for a node
 */
XML_COMMAND_PROTOTYPE(doipodinfo)
{
	unsigned char	randdata[16], hashbuf[16*2+1];
	char		*bp;
	int		fd, cc, i;
	xmlNode		*node;

	if ((fd = open("/dev/urandom", O_RDONLY)) < 0) {
		errorc("opening /dev/urandom");
		return 1;
	}
	if ((cc = read(fd, randdata, sizeof(randdata))) < 0) {
		errorc("reading /dev/urandom");
		close(fd);
		return 1;
	}
	if (cc != sizeof(randdata)) {
		error("Short read from /dev/urandom: %d", cc);
		close(fd);
		return 1;
	}
	close(fd);

	bp = hashbuf;
	for (i = 0; i < sizeof(randdata); i++) {
		bp += sprintf(bp, "%02x", randdata[i]);
	}
	*bp = '\0';

	mydb_update(reqp, "update nodes set ipodhash='%s' "
		    "where node_id='%s'",
		    hashbuf, reqp->nodeid);
	
	/*
	 * XXX host/mask hardwired to us
	 */
	node = new_response(root, "ipodinfo");
	add_key(node, "host", inet_ntoa(reqp->myipaddr));
	add_key(node, "mask", "255.255.255.255");
	add_key(node, "hash", hashbuf);

	return 0;
}
  
/*
 * Return ntp config for a node. 
 */
XML_COMMAND_PROTOTYPE(dontpinfo)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node;

	/*
	 * First get the servers and peers.
	 */
	res = mydb_query(reqp, "select type,IP from ntpinfo where node_id='%s'",
			 2, reqp->nodeid);

	if (!res) {
		error("NTPINFO: %s: DB Error getting ntpinfo!\n",
		      reqp->nodeid);
		return 1;
	}

	node = new_response(root, "ntpinfo");
	
	if ((nrows = (int)mysql_num_rows(res))) {
		while (nrows) {
			row = mysql_fetch_row(res);
			if (row[0] && row[0][0] &&
			    row[1] && row[1][0]) {
				if (!strcmp(row[0], "peer")) {
					add_key(node, "peer", row[1]);
				}
				else {
					add_key(node, "server", row[1]);
				}
			}
			nrows--;
		}
	}
	else if (reqp->islocal) {
		/*
		 * All local nodes default to a our local ntp server,
		 * which is typically a CNAME to ops.
		 */
		add_format_key(node, "server", "%s.%s", NTPSERVER, OURDOMAIN);
	}
	mysql_free_result(res);

	/*
	 * Now get the drift.
	 */
	res = mydb_query(reqp, "select ntpdrift from nodes "
			 "where node_id='%s' and ntpdrift is not null",
			 1, reqp->nodeid);

	if (!res) {
		error("NTPINFO: %s: DB Error getting ntpdrift!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res)) {
		row = mysql_fetch_row(res);
		if (row[0] && row[0][0]) {
			add_key(node, "drift", row[0]);
		}
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Upload the current ntp drift for a node.
 */
XML_COMMAND_PROTOTYPE(dontpdrift)
{
	float		drift;

	if (!reqp->islocal) {
		error("NTPDRIFT: %s: remote nodes not allowed!\n",
		      reqp->nodeid);
		return 1;
	}

	/*
	 * Node can be free?
	 */

	if (sscanf(rdata, "%f", &drift) != 1) {
		error("NTPDRIFT: %s: Bad argument\n", reqp->nodeid);
		return 1;
	}
	mydb_update(reqp, "update nodes set ntpdrift='%f' where node_id='%s'",
		    drift, reqp->nodeid);

	if (reqp->verbose)
		info("NTPDRIFT: %f", drift);
	return 0;
}

/*
 * Return the config for a virtual (jailed) node.
 */
XML_COMMAND_PROTOTYPE(dojailconfig)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		low, high;
	xmlNode		*node;

	/*
	 * Only vnodes get a jailconfig of course, and only allocated ones.
	 */
	if (!reqp->isvnode) {
		/* Silent error is fine */
		return 0;
	}
	/*
	 * geni nodes get something completely different. 
	 */
	if (reqp->genisliver_idx) {
		node = new_response(root, "jailconfig");
		add_format_key(node, "eventserver", "event-server.%s", OURDOMAIN);
		return 0;
	}
	if (!reqp->jailflag)
		return 0;

	/*
	 * Get the portrange for the experiment. Cons up the other params I
	 * can think of right now. 
	 */
	res = mydb_query(reqp, "select low,high from ipport_ranges "
			 "where pid='%s' and eid='%s'",
			 2, reqp->pid, reqp->eid);
	
	if (!res) {
		error("JAILCONFIG: %s: DB Error getting config!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}
	row  = mysql_fetch_row(res);
	low  = atoi(row[0]);
	high = atoi(row[1]);
	mysql_free_result(res);

	/*
	 * Now need the sshdport and jailip for this node.
	 */
	res = mydb_query(reqp, "select sshdport,jailip from nodes "
			 "where node_id='%s'",
			 2, reqp->nodeid);
	
	if (!res) {
		error("JAILCONFIG: %s: DB Error getting config!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}
	row   = mysql_fetch_row(res);
	node = new_response(root, "jailconfig");

	if (row[1]) {
		add_format_key(node, "jailip", "%s,%s", row[1], JAILIPMASK);
	}

	add_format_key(node, "portrange", "%d,%d", low, high);
	add_key(node, "sshdport", row[0]);
	add_key(node, "sysvipc", "1");
	add_key(node, "inetraw", "1");
	add_key(node, "bpfro", "1");
	add_key(node, "inaddrany", "1");
	add_key(node, "ipfw", "1");
	add_key(node, "ipdivert", "1");
	add_key(node, "routing",  reqp->islocal ? "1" : "0");
	add_key(node, "devmem",  reqp->islocal ? "1" : "0");
	add_format_key(node, "eventserver", "event-server.%s", OURDOMAIN);

	mysql_free_result(res);

	/*
	 * See if a per-node-type vnode disk size is specified
	 */
	res = mydb_query(reqp, "select na.attrvalue from nodes as n "
			 "left join node_type_attributes as na on "
			 "  n.type=na.type "
			 "where n.node_id='%s' and "
			 "na.attrkey='virtnode_disksize'", 1, reqp->pnodeid);
	if (res) {
		if ((int)mysql_num_rows(res) != 0) {
			row = mysql_fetch_row(res);
			if (row[0]) {
				add_key(node, "vdsize", row[0]);
			}
		}
		mysql_free_result(res);
	}

	/*
	 * Now return the IP interface list that this jail has access to.
	 * These are tunnels or ip aliases on the real interfaces, but
	 * its easier just to consult the virt_lans table for all the
	 * ip addresses for this vnode.
	 */

	res = mydb_query(reqp, "select ip from virt_lans "
			 "where vnode='%s' and pid='%s' and eid='%s'",
			 1, reqp->nickname, reqp->pid, reqp->eid);

	if (!res) {
		error("JAILCONFIG: %s: DB Error getting virt_lans table\n",
		      reqp->nodeid);
		return 1;
	}
	if (mysql_num_rows(res)) {
		int nrows = mysql_num_rows(res);

		while (nrows) {
			nrows--;
			row = mysql_fetch_row(res);

			/* XXX Ryan: is this too confusing? Should I choose a different key name?*/
			/* Or should I use a comma-separated list? */
			if (row[0] && row[0][0]) {
				add_key(node, "ipaddr", row[0]);
				
			}
		}
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return the config for a virtual Plab node.
 */
XML_COMMAND_PROTOTYPE(doplabconfig)
{
	MYSQL_RES	*res, *res2;	
	MYSQL_ROW	row;
	xmlNode		*node;

	if (!reqp->isplabdslice) {
		/* Silent error is fine */
		return 0;
	}
	/* XXX Check for Plab-ness */

	/*
	 * Now need the sshdport for this node.
	 */
	res = mydb_query(reqp, "select n.sshdport, ps.admin, i.IP "
                         " from reserved as r "
                         " left join nodes as n " 
                         "  on n.node_id=r.node_id "
                         " left join interfaces as i "
                         "  on n.phys_nodeid=i.node_id "
                         " left join plab_slices as ps "
                         "  on r.pid=ps.pid and r.eid=ps.eid "
                         " where i.role='ctrl' and r.node_id='%s'",
			 3, reqp->nodeid);

        /*
         * .. And the elvind port.
         */
        res2 = mydb_query(reqp, "select attrvalue from node_attributes "
                          " where node_id='%s' and attrkey='elvind_port'",
                          1, reqp->pnodeid);

	if (!res || !res2) {
		error("PLABCONFIG: %s: DB Error getting config!\n",
		      reqp->nodeid);
                if (res) {
                    mysql_free_result(res);
                }
                if (res2) {
                    mysql_free_result(res2);
                }
		return 1;
	}
	
	node = new_response(root, "plabconfig");
	
        /* Add the sshd port (if any) to the output */
        if ((int)mysql_num_rows(res) > 0) {
            row = mysql_fetch_row(res);
	    add_key(node, "sshdport", row[0]);
	    add_key(node, "svcslice", row[1]);
	    add_key(node, "ipaddr", row[2]);
        }
        mysql_free_result(res);

        /* Add the elvind port to the output */
        if ((int)mysql_num_rows(res2) > 0) {
            row = mysql_fetch_row(res2);
	    add_key(node, "elvind_port", row[0]);
        }
        else {
            /*
             * XXX: should not hardwire port number here, but what should
             *      I reference for it?
             */
	    add_key(node, "elvind_port", "2917");
        }
        mysql_free_result(res2);

	/* XXX Anything else? */
	
	return 0;
}

/*
 * Return the config for a subnode (this is returned to the physnode).
 */
XML_COMMAND_PROTOTYPE(dosubconfig)
{
	if (!reqp->issubnode) {
		error("SUBCONFIG: %s: Not a subnode\n", reqp->nodeid);
		return 1;
	}

	if (! strcmp(reqp->type, "ixp-bv")) 
		return(doixpconfig(root, reqp, rdata));
	
	if (! strcmp(reqp->type, "mica2")) 
		return(dorelayconfig(root, reqp, rdata));
	
	error("SUBCONFIG: %s: Invalid subnode class %s\n",
	      reqp->nodeid, reqp->class);
	return 1;
}

XML_COMMAND_PROTOTYPE(doixpconfig)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	struct in_addr  mask_addr, bcast_addr;
	char		bcast_ip[16];
	xmlNode		*node;

	/*
	 * Get the "control" net address for the IXP from the interfaces
	 * table. This is really a virtual pci/eth interface.
	 */
	res = mydb_query(reqp, "select i1.IP,i1.iface,i2.iface,i2.mask,i2.IP "
			 " from nodes as n "
			 "left join interfaces as i1 on i1.node_id=n.node_id "
			 "     and i1.role='ctrl' "
			 "left join interfaces as i2 on i2.node_id='%s' "
			 "     and i2.card=i1.card "
			 "where n.node_id='%s'",
			 5, reqp->pnodeid, reqp->nodeid);
	
	if (!res) {
		error("IXPCONFIG: %s: DB Error getting config!\n",
		      reqp->nodeid);
		return 1;
	}
	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}
	row   = mysql_fetch_row(res);
	if (!row[1]) {
		error("IXPCONFIG: %s: No IXP interface!\n", reqp->nodeid);
		return 1;
	}
	if (!row[2]) {
		error("IXPCONFIG: %s: No host interface!\n", reqp->nodeid);
		return 1;
	}
	if (!row[3]) {
		error("IXPCONFIG: %s: No mask!\n", reqp->nodeid);
		return 1;
	}
	inet_aton(CHECKMASK(row[3]), &mask_addr);	
	inet_aton(row[0], &bcast_addr);

	bcast_addr.s_addr = (bcast_addr.s_addr & mask_addr.s_addr) |
		(~mask_addr.s_addr);
	strcpy(bcast_ip, inet_ntoa(bcast_addr));

	node = new_response(root, "ixpconfig");
	add_key(node, "ip", row[0]);
	add_key(node, "iface", row[1]);
	add_key(node, "bcast", bcast_ip);
	add_key(node, "hostname", reqp->nickname);
	add_key(node, "host_ip", row[4]);
	add_key(node, "host_iface", row[2]);
	add_key(node, "netmask", row[3]);

	mysql_free_result(res);
	return 0;
}

/*
 * return slothd params - just compiled in for now.
 */
XML_COMMAND_PROTOTYPE(doslothdparams) 
{
	xmlNode *node;

	node = new_response(root, "slothdparams");
	add_key(node, "params", SDPARAMS);
	
	return 0;
}

/*
 * Return program agent info.
 */
XML_COMMAND_PROTOTYPE(doprogagents)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node;

	res = mydb_query(reqp, "select vname,command,dir,timeout,expected_exit_code "
			 "from virt_programs "
			 "where vnode='%s' and pid='%s' and eid='%s'",
			 5, reqp->nickname, reqp->pid, reqp->eid);

	if (!res) {
		error("PROGRAM: %s: DB Error getting virt_agents\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	
	while (nrows) {
		node = new_response(root, "progagent");
		row = mysql_fetch_row(res);

		/*
		 * XXX Ryan
		 * Old behavior: 
		 * First spit out the UID, then the agents one to a line.
		 *
		 * New behavior:
		 * the uid is output for each agent.  Wasteful, but how
		 * else to do this without special structure for agent tags?
		 */
		add_key(node, "uid", reqp->swapper);
		add_key(node, "agent", row[0]);
		if (row[2] && strlen(row[2]) > 0)
			add_key(node, "dir", row[2]);
		if (row[3] && strlen(row[3]) > 0)
			add_key(node, "timeout", row[3]);
		if (row[4] && strlen(row[4]) > 0)
			add_key(node, "expected_exit_code", row[4]);

		add_key(node, "command", row[1]);
		
		nrows--;
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return sync server info.
 */
XML_COMMAND_PROTOTYPE(dosyncserver)
{
	xmlNode		*node;

	if (!strlen(reqp->syncserver))
		return 0;

	node = new_response(root, "syncserver");
	add_format_key(node, "server", "%s.%s.%s.%s",
	       reqp->syncserver,
	       reqp->eid, reqp->pid, OURDOMAIN);
	add_key(node, "isserver", 
	       (strcmp(reqp->syncserver, reqp->nickname) ? "0": "1"));
	
	return 0;
}

/*
 * Return keyhash info
 */
XML_COMMAND_PROTOTYPE(dokeyhash)
{
	xmlNode		*node;

	if (!strlen(reqp->keyhash))
		return 0;

	node = new_response(root, "keyhash");
	add_key(node, "hash", reqp->keyhash);

	return 0;
}

/*
 * Return eventkey info
 */
XML_COMMAND_PROTOTYPE(doeventkey)
{
	xmlNode		*node;

	if (!strlen(reqp->eventkey))
		return 0;

	node = new_response(root, "eventkey");
	add_key(node, "key", reqp->eventkey);

	return 0;
}

/*
 * Return routing stuff for all vnodes mapped to the requesting physnode
 */
XML_COMMAND_PROTOTYPE(doroutelist)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	int		n, nrows;
	xmlNode		*node, *routelist_node;

	/*
	 * Get the routing type from the nodes table.
	 */
	res = mydb_query(reqp, "select routertype from nodes where node_id='%s'",
			 1, reqp->nodeid);

	if (!res) {
		error("ROUTES: %s: DB Error getting router type!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	routelist_node = new_response(root, "routelist");

	/*
	 * Return type. At some point we might have to return a list of
	 * routes too, if we support static routes specified by the user
	 * in the NS file.
	 */
	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}
	add_key(routelist_node, "routertype", row[0]);
	mysql_free_result(res);

	/*
	 * Get the routing type from the nodes table.
	 */
	res = mydb_query(reqp, "select vr.vname,src,dst,dst_type,dst_mask,nexthop,cost "
			 "from virt_routes as vr "
			 "left join v2pmap as v2p "
			 "using (pid,eid,vname) "
			 "where vr.pid='%s' and "
			 " vr.eid='%s' and v2p.node_id='%s'",
			 7, reqp->pid, reqp->eid, reqp->nodeid);
	
	if (!res) {
		error("ROUTELIST: %s: DB Error getting manual routes!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	n = nrows;

	while (n) {
		char dstip[32];

		row = mysql_fetch_row(res);
				
		/*
		 * OMG, the Linux route command is too stupid to accept a
		 * host-on-a-subnet as the subnet address, so we gotta mask
		 * off the bits manually for network routes.
		 *
		 * Eventually we'll perform this operation in the NS parser
		 * so it appears in the DB correctly.
		 */
		if (strcmp(row[3], "net") == 0) {
			struct in_addr tip, tmask;

			inet_aton(row[2], &tip);
			inet_aton(row[4], &tmask);
			tip.s_addr &= tmask.s_addr;
			strncpy(dstip, inet_ntoa(tip), sizeof(dstip));
		} else
			strncpy(dstip, row[2], sizeof(dstip));
			node = new_response(routelist_node, "route");

			add_key(node, "node", row[0]);
			add_key(node, "srcipaddr", row[1]);
			add_key(node, "ipaddr", dstip);
			add_key(node, "type", row[3]);
			add_key(node, "ipmask", row[4]);
			add_key(node, "gateway", row[5]);
			add_key(node, "cost", row[6]);

		n--;
	}
	mysql_free_result(res);
	if (reqp->verbose)
	    info("ROUTES: %d routes in list\n", nrows);

	return 0;
}

/*
 * Return routing stuff for all vnodes mapped to the requesting physnode
 */
XML_COMMAND_PROTOTYPE(dorole)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	xmlNode		*node;

	/*
	 * Get the erole from the reserved table
	 */
	res = mydb_query(reqp, "select erole from reserved where node_id='%s'",
			 1, reqp->nodeid);

	if (!res) {
		error("ROLE: %s: DB Error getting the role of this node!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		return 0;
	}

	row = mysql_fetch_row(res);
	if (! row[0] || !row[0][0]) {
		mysql_free_result(res);
		return 0;
	}

	node = new_response(root, "role");
	add_key(node, "role", row[0]);
	mysql_free_result(res);

	return 0;
}

/*
 * Return entire config.
 */
XML_COMMAND_PROTOTYPE(dofullconfig)
{
	int		i;
	int		mask;

	if (reqp->isvnode)
		mask = FULLCONFIG_VIRT;
	else
		mask = FULLCONFIG_PHYS;

	for (i = 0; i < numcommands; i++) {
		if (xml_command_array[i].fullconfig & mask) {
			if (tcp && !isssl && !reqp->islocal && 
			    (xml_command_array[i].flags & F_REMREQSSL) != 0) {
				/*
				 * Silently drop commands that are not
				 * allowed for remote non-ssl connections.
				 */
				continue;
			}
			xml_command_array[i].func(root, reqp, rdata);
		}
	}
	return 0;
}

/*
 * Report node resource usage. This also serves as an isalive(),
 * so we send back update info. The format for upload is:
 *
 *  LA1=x.y LA5=x.y LA15=x.y DUSED=x ...
 */
XML_COMMAND_PROTOTYPE(dorusage)
{
	char		buf[MYBUFSIZE];
	float		la1, la5, la15, dused;
        int             plfd;
        struct timeval  now;
        struct tm       *tmnow;
        char            pllogfname[MAXPATHLEN];
        char            timebuf[10];
	xmlNode		*node;

	if (sscanf(rdata, "LA1=%f LA5=%f LA15=%f DUSED=%f",
		   &la1, &la5, &la15, &dused) != 4) {
		strncpy(buf, rdata, 64);
		error("RUSAGE: %s: Bad arguments: %s...\n", reqp->nodeid, buf);
		return 1;
	}

	/*
	 * See db/node_status script, which uses this info (timestamps)
	 * to determine when nodes are down.
	 *
	 * XXX: Plab physnode status is reported from the management slice.
         *
	 */
	mydb_update(reqp, "replace into node_rusage "
		    " (node_id, status_timestamp, "
		    "  load_1min, load_5min, load_15min, disk_used) "
		    " values ('%s', now(), %f, %f, %f, %f)",
		    reqp->pnodeid, la1, la5, la15, dused);

	if (reqp->isplabdslice) {
		mydb_update(reqp, "replace into node_status "
			    " (node_id, status, status_timestamp) "
			    " values ('%s', 'up', now())",
			    reqp->pnodeid);

		mydb_update(reqp, "replace into node_status "
			    " (node_id, status, status_timestamp) "
			    " values ('%s', 'up', now())",
			    reqp->vnodeid);
        }

	/*
	 * At some point, maybe what we will do is have the client
	 * make a request asking what needs to be updated. Right now,
	 * just return yes/no and let the client assume it knows what
	 * to do (update accounts).
	 */
	node = new_response(root, "rusage");
	add_format_key(node, "update", "%d", reqp->update_accounts);

        /* We're going to store plab up/down data in a file for a while. */
        if (reqp->isplabsvc) {
            gettimeofday(&now, NULL);
            tmnow = localtime((time_t *)&now.tv_sec);
            strftime(timebuf, sizeof(timebuf), "%Y%m%d", tmnow);
            snprintf(pllogfname, sizeof(pllogfname), 
                     "%s/%s-isalive-%s", 
                     PLISALIVELOGDIR, 
                     reqp->pnodeid,
                     timebuf);

            snprintf(buf, sizeof(buf), "%ld %ld\n", 
                     now.tv_sec, now.tv_usec);
            plfd = open(pllogfname, O_WRONLY|O_APPEND|O_CREAT, 
                        S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
            if (plfd < 0) {
                errorc("Can't open log: %s", pllogfname);
            } else {
                write(plfd, buf, strlen(buf));
                close(plfd);
            }
        }

	return 0;
}

/*
 * Report time intervals to use in the watchdog process.  All times in
 * seconds (0 means never, -1 means no value is available in the DB):
 *
 *	INTERVAL=	how often to check for new intervals
 *	ISALIVE=	how often to report isalive info
 *			(note that also controls how often new accounts
 *			 are noticed)
 *	NTPDRIFT=	how often to report NTP drift values
 *	CVSUP=		how often to check for software updates
 *	RUSAGE=		how often to collect/report resource usage info
 */
XML_COMMAND_PROTOTYPE(dodoginfo)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows, *iv;
	int		iv_interval, iv_isalive, iv_ntpdrift, iv_cvsup;
	int		iv_rusage, iv_hkeys;
	int		rootpswdinterval = 0;
	xmlNode		*node;

	/*
	 * XXX sitevar fetching should be a library function
	 */
	res = mydb_query(reqp, "select name,value,defaultvalue from "
			 "sitevariables where name like 'watchdog/%%'", 3);
	if (!res || (nrows = (int)mysql_num_rows(res)) == 0) {
		error("WATCHDOGINFO: no watchdog sitevars\n");
		if (res)
			mysql_free_result(res);
		return 1;
	}

	iv_interval = iv_isalive = iv_ntpdrift = iv_cvsup =
		iv_rusage = iv_hkeys = -1;
	while (nrows) {
		iv = 0;
		row = mysql_fetch_row(res);
		if (strcmp(row[0], "watchdog/interval") == 0) {
			iv = &iv_interval;
		} else if (strcmp(row[0], "watchdog/ntpdrift") == 0) {
			iv = &iv_ntpdrift;
		} else if (strcmp(row[0], "watchdog/cvsup") == 0) {
			iv = &iv_cvsup;
		} else if (strcmp(row[0], "watchdog/rusage") == 0) {
			iv = &iv_rusage;
		} else if (strcmp(row[0], "watchdog/hostkeys") == 0) {
			iv = &iv_hkeys;
		} else if (strcmp(row[0], "watchdog/isalive/local") == 0) {
			if (reqp->islocal && !reqp->isvnode)
				iv = &iv_isalive;
		} else if (strcmp(row[0], "watchdog/isalive/vnode") == 0) {
			if (reqp->islocal && reqp->isvnode)
				iv = &iv_isalive;
		} else if (strcmp(row[0], "watchdog/isalive/plab") == 0) {
			if (!reqp->islocal && reqp->isplabdslice)
				iv = &iv_isalive;
		} else if (strcmp(row[0], "watchdog/isalive/wa") == 0) {
			if (!reqp->islocal && !reqp->isplabdslice)
				iv = &iv_isalive;
		}

		if (iv) {
			/* use the current value if set */
			if (row[1] && row[1][0])
				*iv = atoi(row[1]) * 60;
			/* else check for default value */
			else if (row[2] && row[2][0])
				*iv = atoi(row[2]) * 60;
			else
				error("WATCHDOGINFO: sitevar %s not set\n",
				      row[0]);
		}
		nrows--;
	}
	mysql_free_result(res);

	/*
	 * XXX adjust for local policy
	 * - vnodes and plab nodes do not send NTP drift or cvsup
	 * - vnodes do not report host keys
	 * - widearea nodes do not record drift
	 * - local nodes do not cvsup
	 * - only a plab node service slice reports rusage
	 *   (which it uses in place of isalive)
	 */
	if ((reqp->islocal && reqp->isvnode) || reqp->isplabdslice) {
		iv_ntpdrift = iv_cvsup = 0;
		if (!reqp->isplabdslice)
			iv_hkeys = 0;
	}
	if (!reqp->islocal)
		iv_ntpdrift = 0;
	else
		iv_cvsup = 0;
	if (!reqp->isplabsvc)
		iv_rusage = 0;
	else
		iv_isalive = 0;

#ifdef DYNAMICROOTPASSWORDS
	        rootpswdinterval = 3600;
#endif

	node = new_response(root, "doginfo");
	add_format_key(node, "interval", "%d", iv_interval);
	add_format_key(node, "isalive", "%d", iv_isalive);
	add_format_key(node, "ntpdrift", "%d", iv_ntpdrift);
	add_format_key(node, "cvsup", "%d", iv_cvsup);
	add_format_key(node, "rusage", "%d", iv_rusage);
	add_format_key(node, "hostkeys", "%d", iv_hkeys);
	add_format_key(node, "setrootpswd", "%d", rootpswdinterval);

	return 0;
}

/*
 * Stash info returned by the host into the DB
 * Right now we only recognize CDVERSION=<str> for CD booted systems.
 */
XML_COMMAND_PROTOTYPE(dohostinfo)
{
	char		*bp, buf[MYBUFSIZE];

	bp = rdata;
	if (sscanf(bp, "CDVERSION=%31[a-zA-Z0-9-]", buf) == 1) {
		if (reqp->verbose)
			info("HOSTINFO CDVERSION=%s\n", buf);
		if (mydb_update(reqp, "update nodes set cd_version='%s' "
				"where node_id='%s'",
				buf, reqp->nodeid)) {
			error("HOSTINFO: %s: DB update failed\n", reqp->nodeid);
			return 1;
		}
	}
	return 0;
}

/*
 * XXX Stash ssh host keys into the DB. 
 */
XML_COMMAND_PROTOTYPE(dohostkeys)
{
#define MAXKEY		1024
#define RSAV1_STR	"SSH_HOST_KEY='"
#define RSAV2_STR	"SSH_HOST_RSA_KEY='"
#define DSAV2_STR	"SSH_HOST_DSA_KEY='"
	
	char	*bp, rsav1[2*MAXKEY], rsav2[2*MAXKEY], dsav2[2*MAXKEY];
	char	buf[MAXKEY];

#if 0
	printf("%s\n", rdata);
#endif

	/*
	 * The maximum key length we accept is 1024 bytes, but after we
	 * run it through mysql_escape_string() it could potentially double
	 * in size (although that is very unlikely).
	 */
	rsav1[0] = rsav2[0] = dsav2[0] = (char) NULL;

	/*
	 * Sheesh, perl string matching would be so much easier!
	 */
	bp = rdata;
	while (*bp) {
		char	*ep, *kp, *thiskey = (char *) NULL;
		
		while (*bp == ' ')
			bp++;
		if (! *bp)
			break;

		if (! strncasecmp(bp, RSAV1_STR, strlen(RSAV1_STR))) {
			thiskey = rsav1;
			bp += strlen(RSAV1_STR);
		}
		else if (! strncasecmp(bp, RSAV2_STR, strlen(RSAV2_STR))) {
			thiskey = rsav2;
			bp += strlen(RSAV2_STR);
		}
		else if (! strncasecmp(bp, DSAV2_STR, strlen(DSAV2_STR))) {
			thiskey = dsav2;
			bp += strlen(DSAV2_STR);
		}
		else {
			error("HOSTKEYS: %s: "
			      "Unrecognized key type '%.8s ...'\n",
			      reqp->nodeid, bp);
			if (reqp->verbose)
				error("HOSTKEYS: %s\n", rdata);
			return 1;
		}
		kp = buf;
		ep = &buf[sizeof(buf) - 1];

		/* Copy the part between the single quotes to the holding buf */
		while (*bp && *bp != '\'' && kp < ep)
			*kp++ = *bp++;
		if (*bp != '\'') {
			error("HOSTKEYS: %s: %s key data too long!\n",
			      reqp->nodeid,
			      thiskey == rsav1 ? "RSA v1" :
			      (thiskey == rsav2 ? "RSA v2" : "DSA v2"));
			if (reqp->verbose)
				error("HOSTKEYS: %s\n", rdata);
			return 1;
		}
		bp++;
		*kp = '\0';

		/* Okay, turn into something for mysql statement. */
		thiskey[0] = '\'';
		mysql_escape_string(&thiskey[1], buf, strlen(buf));
		strcat(thiskey, "'");
	}
	if (mydb_update(reqp, "update node_hostkeys set "
			"       sshrsa_v1=%s,sshrsa_v2=%s,sshdsa_v2=%s "
			"where node_id='%s'",
			(rsav1[0] ? rsav1 : "NULL"),
			(rsav2[0] ? rsav2 : "NULL"),
			(dsav2[0] ? dsav2 : "NULL"),
			reqp->nodeid)) {
		error("HOSTKEYS: %s: setting hostkeys!\n", reqp->nodeid);
		return 1;
	}
	if (reqp->verbose) {
		info("sshrsa_v1=%s,sshrsa_v2=%s,sshdsa_v2=%s\n",
		     (rsav1[0] ? rsav1 : "NULL"),
		     (rsav2[0] ? rsav2 : "NULL"),
		     (dsav2[0] ? dsav2 : "NULL"));
	}
	return 0;
}

XML_COMMAND_PROTOTYPE(dofwinfo)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		fwname[TBDB_FLEN_VNAME+2];
	char		fwtype[TBDB_FLEN_VNAME+2];
	char		fwstyle[TBDB_FLEN_VNAME+2];
	int		n, nrows;
	char		*vlan;
	xmlNode		*node, *fwinfo_node, *fwvars_node;
	xmlNode		*fwhosts_node, *host_node;

	/*
	 * See if this node's experiment has an associated firewall
	 *
	 * XXX will only work if there is one firewall per experiment.
	 */
	res = mydb_query(reqp, "select r.node_id,v.type,v.style,v.log,f.fwname,"
			 "  i.IP,i.mac,f.vlan "
			 "from firewalls as f "
			 "left join reserved as r on"
			 "  f.pid=r.pid and f.eid=r.eid and f.fwname=r.vname "
			 "left join virt_firewalls as v on "
			 "  v.pid=f.pid and v.eid=f.eid and v.fwname=f.fwname "
			 "left join interfaces as i on r.node_id=i.node_id "
			 "where f.pid='%s' and f.eid='%s' "
			 "and i.role='ctrl'",	/* XXX */
			 8, reqp->pid, reqp->eid);
	
	if (!res) {
		error("FWINFO: %s: DB Error getting firewall info!\n",
		      reqp->nodeid);
		return 1;
	}


	fwinfo_node = new_response(root, "firewallinfo");

	/*
	 * Common case, no firewall
	 */
	if ((int)mysql_num_rows(res) == 0) {
		mysql_free_result(res);
		add_key(fwinfo_node, "type", "none");
		return 0;
	}

	/*
	 * DB data is bogus
	 */
	row = mysql_fetch_row(res);
	if (!row[0] || !row[0][0]) {
		mysql_free_result(res);
		error("FWINFO: %s: DB Error in firewall info, no firewall!\n",
		      reqp->nodeid);
		add_key(fwinfo_node, "type", "none");
		return 0;
	}

	/*
	 * There is a firewall, but it isn't us.
	 * Put out base info with TYPE=remote.
	 */
	if (strcmp(row[0], reqp->nodeid) != 0) {
		char *fwip;

		/*
		 * XXX sorta hack: if we are a HW enforced firewall,
		 * then the client doesn't need to do anything.
		 * Set the FWIP to 0 to indicate this.
		 */
		if (strcmp(row[1], "ipfw2-vlan") == 0)
			fwip = "0.0.0.0";
		else
			fwip = row[5];

		add_key(fwinfo_node, "type", "remote");
		add_key(fwinfo_node, "fwip", fwip);
		mysql_free_result(res);

		return 0;
	}

	/*
	 * Grab vlan info if available
	 */
	if (row[7] && row[7][0])
		vlan = row[7];
	else
		vlan = "0";

	/*
	 * We are the firewall.
	 * Put out base information and all the rules
	 *
	 * XXX for now we use the control interface for in/out
	 */
	add_key(fwinfo_node, "type", row[1]);
	add_key(fwinfo_node, "style", row[2]);
	add_key(fwinfo_node, "in_if", row[6]);
	add_key(fwinfo_node, "out_if", row[6]);
	add_key(fwinfo_node, "in_vlan", vlan);
	add_key(fwinfo_node, "out_vlan", vlan);

	/*
	 * Put out info about firewall rule logging
	 */
	if (row[3] && row[3][0]) {
		add_key(fwinfo_node, "log", row[3]);
	}

	strncpy(fwtype, row[1], sizeof(fwtype));
	strncpy(fwstyle, row[2], sizeof(fwstyle));
	strncpy(fwname, row[4], sizeof(fwname));
	mysql_free_result(res);

	/*
	 * Return firewall variables
	 */
	/* XXX Ryan: how to state that these are variables? breaks simple key/value pair idea */
	/*
	 * Grab the node gateway MAC which is not currently part
	 * of the firewall variables table.
	 */
	res = mydb_query(reqp, "select value from sitevariables "
			 "where name='node/gw_mac'", 1);
	fwvars_node = new_response(fwinfo_node, "fwvars");
	if (res && mysql_num_rows(res) > 0) {
		row = mysql_fetch_row(res);
		if (row[0]) { /* XXX Ryan */
			node = new_response(fwinfo_node, "fwvar");
			add_key(node, "var", "EMULAB_GWIP");
			add_key(node, "value", CONTROL_ROUTER_IP);
			node = new_response(fwinfo_node, "fwvar");
			add_key(node, "var", "EMULAB_GWMAC");
			add_key(node, "value", row[0]);
		}
	}
	if (res)
		mysql_free_result(res);

	res = mydb_query(reqp, "select name,value from default_firewall_vars",
			 2);
	if (!res) {
		error("FWINFO: %s: DB Error getting firewall vars!\n",
		      reqp->nodeid);
		nrows = 0;
	} else
		nrows = (int)mysql_num_rows(res);
	for (n = nrows; n > 0; n--) {
		row = mysql_fetch_row(res);
		if (!row[0] || !row[1])
			continue;
		node = new_response(fwinfo_node, "fwvar");
		add_key(node, "var", row[0]);
		add_key(node, "value", row[1]);
	}
	if (res)
		mysql_free_result(res);
	if (reqp->verbose)
		info("FWINFO: %d variables\n", nrows);

	/*
	 * Get the user firewall rules from the DB and return them.
	 */
	res = mydb_query(reqp, "select ruleno,rule from firewall_rules "
			 "where pid='%s' and eid='%s' and fwname='%s' "
			 "order by ruleno",
			 2, reqp->pid, reqp->eid, fwname);
	if (!res) {
		error("FWINFO: %s: DB Error getting firewall rules!\n",
		      reqp->nodeid);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);

	for (n = nrows; n > 0; n--) {
		row = mysql_fetch_row(res);
		node = new_response(fwinfo_node, "fwrule");
		add_key(node, "ruleno", row[0]);
		add_key(node, "rule", row[1]);
	}

	mysql_free_result(res);
	if (reqp->verbose)
	    info("FWINFO: %d user rules\n", nrows);

	/*
	 * Get the default firewall rules from the DB and return them.
	 */
	res = mydb_query(reqp, "select ruleno,rule from default_firewall_rules "
			 "where type='%s' and style='%s' and enabled!=0 "
			 "order by ruleno",
			 2, fwtype, fwstyle);
	if (!res) {
		error("FWINFO: %s: DB Error getting default firewall rules!\n",
		      reqp->nodeid);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);

	for (n = nrows; n > 0; n--) {
		row = mysql_fetch_row(res);
		node = new_response(fwinfo_node, "fwrule");
		add_key(node, "ruleno", row[0]);
		add_key(node, "rule", row[1]);
	}

	mysql_free_result(res);
	if (reqp->verbose)
	    info("FWINFO: %d default rules\n", nrows);

	/*
	 * Ohhh...I gotta bad case of the butt-uglies!
	 *
	 * Return the list of the unqualified names of the firewalled hosts
	 * along with their IP addresses.  The client code uses this to
	 * construct a local hosts file so that symbolic host names can
	 * be used in firewall rules.
	 *
	 * We also return the control net MAC address for each node so
	 * that we can provide proxy ARP.
	 */
	 /* XXX Ryan fix this */
	res = mydb_query(reqp, "select r.vname,i.IP,i.mac "
		"from reserved as r "
		"left join interfaces as i on r.node_id=i.node_id "
		"where r.pid='%s' and r.eid='%s' and i.role='ctrl'",
		 3, reqp->pid, reqp->eid);
	if (!res) {
		error("FWINFO: %s: DB Error getting host info!\n",
		      reqp->nodeid);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);

	for (n = nrows; n > 0; n--) {
		row = mysql_fetch_row(res);
		node = new_response(fwinfo_node, "fwhost");
		add_key(node, "host", row[0]);
		add_key(node, "cnetip", row[1]);
		add_key(node, "cnetmac", row[2]);
	}

	mysql_free_result(res);
	if (reqp->verbose)
		info("FWINFO: %d firewalled hosts\n", nrows);

	return 0;
}

/*
 * Return the config for an inner emulab
 */
XML_COMMAND_PROTOTYPE(doemulabconfig)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node;

	/*
	 * Must be an elab_in_elab experiment.
	 */
	if (!reqp->elab_in_elab) {
		/* Silent error is fine */
		return 0;
	}

	/*
	 * Get the control network IPs. At the moment, it is assumed that
	 * that all the nodes are on a single lan. If we get fancier about
	 * that, we will have to figure out how to label the specific lans
	 * se we know which is which.
	 *
	 * Note the single vs dual control network differences!
	 */
	if (reqp->singlenet) {
		res = mydb_query(reqp, "select r.node_id,r.inner_elab_role,"
				 "   i.IP,r.vname from reserved as r "
				 "left join interfaces as i on "
				 "     i.node_id=r.node_id and i.role='ctrl' "
				 "where r.pid='%s' and r.eid='%s' and "
				 "      r.inner_elab_role is not null",
				 4, reqp->pid, reqp->eid);
	}
	else {
		res = mydb_query(reqp, "select r.node_id,r.inner_elab_role,"
				 "   vl.ip,r.vname from reserved as r "
				 "left join virt_lans as vl on "
				 "     vl.vnode=r.vname and "
				 "     vl.pid=r.pid and vl.eid=r.eid "
				 "where r.pid='%s' and r.eid='%s' and "
				 "      r.inner_elab_role is not null",
				 4, reqp->pid, reqp->eid);
	}

	if (!res) {
		error("EMULABCONFIG: %s: DB Error getting elab_in_elab\n",
		      reqp->nodeid);
		return 1;
	}
	if (! mysql_num_rows(res)) {
		mysql_free_result(res);
		return 0;
	}
	nrows = (int)mysql_num_rows(res);
	
	/*
	 * Lets find the role for the current node and spit that out first.
	 */
	node = new_response(root, "elabconfig");
	while (nrows--) {
		row = mysql_fetch_row(res);

		if (!strcmp(row[0], reqp->nodeid)) {
			add_key(node, "role", row[1]);
			break;
		}
	}

	/*
	 * Spit the names of the boss, ops and fs nodes for everyones benefit.
	 */
	mysql_data_seek(res, 0);
	nrows = (int)mysql_num_rows(res);

	while (nrows--) {
		row = mysql_fetch_row(res);

		if (!strcmp(row[1], "boss") || !strcmp(row[1], "boss+router")) {
			add_key(node, "bossnode", row[3]);
			add_key(node, "bossip", row[2]);
		}
		else if (!strcmp(row[1], "ops") || !strcmp(row[1], "ops+fs")) {
			add_key(node, "opsnode", row[3]);
			add_key(node, "opsip", row[2]);
		}
		else if (!strcmp(row[1], "fs")) {
			add_key(node, "fsnode", row[3]);
			add_key(node, "fsip", row[2]);
		}
	}
	mysql_free_result(res);

	/*
	 * Some package info and other stuff from sitevars.
	 */
	res = mydb_query(reqp, "select name,value from sitevariables "
			 "where name like 'elabinelab/%%'", 2);
	if (!res) {
		error("EMULABCONFIG: %s: DB Error getting elab_in_elab\n",
		      reqp->nodeid);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);
	while (nrows--) {
		row = mysql_fetch_row(res);
		if (strcmp(row[0], "elabinelab/boss_pkg_dir") == 0) {
			add_key(node, "boss_pkg_dir", row[1]);
		} else if (strcmp(row[0], "elabinelab/boss_pkg") == 0) {
			add_key(node, "boss_pkg", row[1]);
		} else if (strcmp(row[0], "elabinelab/ops_pkg_dir") == 0) {
			add_key(node, "ops_pkg_dir", row[1]);
		} else if (strcmp(row[0], "elabinelab/ops_pkg") == 0) {
			add_key(node, "ops_pkg", row[1]);
		} else if (strcmp(row[0], "elabinelab/fs_pkg_dir") == 0) {
			add_key(node, "fs_pkg_dir", row[1]);
		} else if (strcmp(row[0], "elabinelab/fs_pkg") == 0) {
			add_key(node, "fs_pkg", row[1]);
		} else if (strcmp(row[0], "elabinelab/windows") == 0) {
			add_key(node, "winsupport", row[1]);
		}
	}
	mysql_free_result(res);

	/*
	 * Stuff from the experiments table.
	 */
	res = mydb_query(reqp, "select elabinelab_cvstag,elabinelab_nosetup "
			 "   from experiments "
			 "where pid='%s' and eid='%s'",
			 2, reqp->pid, reqp->eid);
	if (!res) {
		error("EMULABCONFIG: %s: DB Error getting experiments info\n",
		      reqp->nodeid);
		return 1;
	}
	if ((int)mysql_num_rows(res)) {
	    row = mysql_fetch_row(res);

	    if (row[0] && row[0][0]) {
		add_key(node, "cvsrctag", row[0]);
	    }
	    if (row[1] && strcmp(row[1], "0")) {
		add_key(node, "nosetup", "1");
	    }
	}
	mysql_free_result(res);

	/*
	 * Tell the inner elab if its a single control network setup.
	 */
	add_key(node, "single_controlnet", (reqp->singlenet ? "1" : "0"));

	return 0;
}

/*
 * Return the config for an emulated ("inner") planetlab
 */
 /* XXX Ryan finish this */
XML_COMMAND_PROTOTYPE(doeplabconfig)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE];
	char		*bufp = buf;
	int		nrows;
	xmlNode		*eplabconfig_node, *node;;

	/*
	 * We only respond if we are a PLC node
	 */
	res = mydb_query(reqp, "select node_id from reserved "
			 "where pid='%s' and eid='%s' and plab_role='plc'",
			 1, reqp->pid, reqp->eid);
	if (!res) {
		error("EPLABCONFIG: %s: DB Error getting plab_role\n",
		      reqp->nodeid);
		return 1;
	}
	if (!mysql_num_rows(res)) {
		mysql_free_result(res);
		return 0;
	}
	row = mysql_fetch_row(res);
	if (!row[0] || strcmp(row[0], reqp->nodeid) != 0) {
		mysql_free_result(res);
		return 0;
	}
	mysql_free_result(res);

	/*
	 * VNAME=<vname> PNAME=<FQpname> ROLE=<role> CNETIP=<IP> CNETMAC=<MAC> 
	 */
	res = mydb_query(reqp, "select r.node_id,r.vname,r.plab_role,i.IP,i.mac "
			 "  from reserved as r join interfaces as i "
			 "  where r.node_id=i.node_id and "
			 "    i.role='ctrl' and r.pid='%s' and r.eid='%s'",
			 5, reqp->pid, reqp->eid);

	if (!res || mysql_num_rows(res) == 0) {
		error("EMULABCONFIG: %s: DB Error getting plab_in_elab info\n",
		      reqp->nodeid);
		if (res)
			mysql_free_result(res);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);
	
	eplabconfig_node = new_response(root, "epelabconfig");
	/*
	 * Spit out the PLC node first just cuz
	 */
	bzero(buf, sizeof(buf));
	while (nrows--) {
		row = mysql_fetch_row(res);

		if (!strcmp(row[2], "plc")) {
			node = new_response(eplabconfig_node, "node");
			add_key(node, "vname", row[1]);
			add_format_key(node, "pname", "%s.%s", row[0], OURDOMAIN);
			add_key(node, "role", row[2]);
			add_key(node, "cnetip", row[3]);
			add_key(node, "cnetmac", row[4]);
			break;
		}
	}

	/*
	 * Then all the nodes
	 */
	mysql_data_seek(res, 0);
	nrows = (int)mysql_num_rows(res);

	while (nrows--) {
		row = mysql_fetch_row(res);

		if (!strcmp(row[1], "plc"))
			continue;

		node = new_response(eplabconfig_node, "node");
		add_key(node, "vname", row[1]);
		add_format_key(node, "pname", "%s.%s", row[0], OURDOMAIN);
		add_key(node, "role", row[2]);
		add_key(node, "cnetip", row[3]);
		add_key(node, "cnetmac", row[4]);
	}
	mysql_free_result(res);

	/*
	 * Now report all the configured experimental interfaces:
	 *
	 * VNAME=<vname> IP=<IP> NETMASK=<mask> MAC=<MAC>
	 */
	res = mydb_query(reqp, "select vl.vnode,i.IP,i.mask,i.mac from reserved as r"
			 "  left join virt_lans as vl"
			 "    on r.pid=vl.pid and r.eid=vl.eid"
			 "  left join interfaces as i"
			 "    on vl.ip=i.IP and r.node_id=i.node_id"
			 "  where r.pid='%s' and r.eid='%s' and"
			 "    r.plab_role!='none'"
			 "    and i.IP!='' and i.role='expt'",
			 4, reqp->pid, reqp->eid);

	if (!res) {
		error("EMULABCONFIG: %s: DB Error getting plab_in_elab info\n",
		      reqp->nodeid);
		return 1;
	}
	nrows = (int)mysql_num_rows(res);
	while (nrows--) {
		row = mysql_fetch_row(res);
		bufp = buf;
		node = new_response(eplabconfig_node, "interface");
		add_key(node, "vname", row[0]);
		add_key(node, "ip", row[1]);
		add_key(node, "netmask", row[2]);
		add_key(node, "mac", row[3]);
	}
	mysql_free_result(res);

	/*
	 * Grab lanlink on which the node should be/contact plc.
	 */
	/* 
	 * For now, just assume that plab_plcnet is a valid lan name and 
	 * join it with virtlans and ifaces.
	 */
	res = mydb_query(reqp, "select vl.vnode,r.node_id,vn.plab_plcnet,"
			 "       vn.plab_role,i.IP,i.mask,i.mac"
			 "  from reserved as r left join virt_lans as vl"
			 "    on r.exptidx=vl.exptidx"
			 "  left join interfaces as i"
			 "    on vl.ip=i.IP and r.node_id=i.node_id"
			 "  left join virt_nodes as vn"
			 "    on vl.vname=vn.plab_plcnet and r.vname=vn.vname"
			 "      and vn.exptidx=r.exptidx"
			 "  where r.pid='%s' and r.eid='%s' and"
                         "    r.plab_role != 'none' and i.IP != ''"
			 "      and vn.plab_plcnet != 'none'"
			 "      and vn.plab_plcnet != 'control'",
			 7,reqp->pid,reqp->eid);
	if (!res) {
	    error("EPLABCONFIG: %s: DB Error getting plab_in_elab info\n",
		  reqp->nodeid);
	    return 1;
	}
	nrows = (int)mysql_num_rows(res);
	while (nrows--) {
	    row = mysql_fetch_row(res);
	    bufp = buf;
	    node = new_response(eplabconfig_node, "lanlink");
	    add_key(node, "vname", row[0]);
	    add_format_key(node, "pname", "%s.%s", row[1], OURDOMAIN);
	    add_key(node, "plcnetwork", row[2]);
	    add_key(node, "role", row[3]);
	    add_key(node, "ip", row[4]);
	    add_key(node, "netmask", row[5]);
	    add_key(node, "mac", row[6]);
	}
	mysql_free_result(res);

	return 0;
}
/*
 * Return node localization. For example, boss root pub key, root password,
 * stuff like that.
 */
XML_COMMAND_PROTOTYPE(dolocalize)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node;

	/*
	 * XXX sitevar fetching should be a library function.
	 * WARNING: This sitevar (node/ssh_pubkey) is referenced in
	 *          install/boss-install during initial setup.
	 */
	res = mydb_query(reqp, "select name,value "
			 "from sitevariables where name='node/ssh_pubkey'",
			 2);
	if (!res || (nrows = (int)mysql_num_rows(res)) == 0) {
		error("DOLOCALIZE: sitevar node/ssh_pubkey does not exist\n");
		if (res)
			mysql_free_result(res);
		return 1;
	}

	node = new_response(root, "localization");
	row = mysql_fetch_row(res);
	if (row[1]) {
		add_key(node, "rootpubkey", row[1]);
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return root password
 */
XML_COMMAND_PROTOTYPE(dorootpswd)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE], hashbuf[MYBUFSIZE], *bp;
	xmlNode		*node;

	res = mydb_query("select attrvalue from node_attributes "
			 " where node_id='%s' and "
			 "       attrkey='root_password'",
			 1, reqp->pnodeid);

	if (!res || (int)mysql_num_rows(res) == 0) {
		unsigned char	randdata[5];
		int		fd, cc, i;
	
		if ((fd = open("/dev/urandom", O_RDONLY)) < 0) {
			errorc("opening /dev/urandom");
			return 1;
		}
		if ((cc = read(fd, randdata, sizeof(randdata))) < 0) {
			errorc("reading /dev/urandom");
			close(fd);
			return 1;
		}
		if (cc != sizeof(randdata)) {
			error("Short read from /dev/urandom: %d", cc);
			close(fd);
			return 1;
		}
		close(fd);

		bp = hashbuf;
		for (i = 0; i < sizeof(randdata); i++) {
			bp += sprintf(bp, "%02x", randdata[i]);
		}
		*bp = '\0';

		mydb_update("replace into node_attributes set "
			    "  node_id='%s', "
			    "  attrkey='root_password',attrvalue='%s'",
			    reqp->nodeid, hashbuf);
	}
	else {
		row = mysql_fetch_row(res);
		strcpy(hashbuf, row[0]);
	}
	if (res)
		mysql_free_result(res);
	
	/*
	 * Need to crypt() this for the node since we obviously do not want
	 * to return the plain text.
	 */
	sprintf(buf, "$1$%s", hashbuf);
	bp = crypt(hashbuf, buf);

	node = new_response(root, "rootpswd");
	add_key(node, "hash", bp);

	return 0;
}

/*
 * Upload boot log to DB for the node. 
 */
RAW_COMMAND_PROTOTYPE(dobootlog)
{
	char		*cp = (char *) NULL, *bp;
	int		len;

	/*
	 * Dig out the log message text.
	 */
	while (*rdata && isspace(*rdata))
		rdata++;

	/*
	 * Stash optional text. Must escape it of course. 
	 */
	if ((len = strlen(rdata))) {
		char	buf[MAXTMCDPACKET];

		memcpy(buf, rdata, len);
		
		/*
		 * Ick, Ick, Ick. For non-ssl mode I should have required
		 * that the client side close its output side so that we
		 * could read till EOF. Or, included a record len. As it
		 * stands, fragmentation will cause a large message (like
		 * console output) to not appear all at once. Getting this in
		 * a backwards compatable manner is a pain in the ass. So, I
		 * just bumped the version number. Newer tmcc will close the 
		 * output side.
		 *
		 * Note that tmcc version 22 now closes its write side.
		 */
		 if (reqp->istcp && !reqp->isssl) {
			bp = &buf[len];
			
			while (len < sizeof(buf)) {
				int cc = READ(sock, bp, sizeof(buf) - len);

				if (cc <= 0)
					break;

				len += cc;
				bp  += cc;
			}
		}
		
		if ((cp = (char *) malloc((2*len)+1)) == NULL) {
			error("DOBOOTLOG: %s: Out of Memory\n", reqp->nodeid);
			return 1;
		}
		mysql_escape_string(cp, buf, len);

		if (mydb_update(reqp, "replace into node_bootlogs "
				" (node_id, bootlog, bootlog_timestamp) values "
				" ('%s', '%s', now())", 
				reqp->nodeid, cp)) {
			error("DOBOOTLOG: %s: DB Error setting bootlog\n",
			      reqp->nodeid);
			free(cp);
			return 1;
			
		}
		if (reqp->verbose)
		    printf("DOBOOTLOG: %d '%s'\n", len, cp);
		
		free(cp);
	}

	*length = 0;
	*outbuf = NULL;
	
	return 0;
}

/*
 * Tell us about boot problems with a specific error code that we can use
 * in os_setup to figure out what went wrong, and if we should retry.
 */
XML_COMMAND_PROTOTYPE(dobooterrno)
{
	int		myerrno;

	/*
	 * Dig out errno that the node is reporting
	 */
	if (sscanf(rdata, "%d", &myerrno) != 1) {
		error("DOBOOTERRNO: %s: Bad arguments\n", reqp->nodeid);
		return 1;
	}

	/*
	 * and update DB.
	 */
	if (mydb_update(reqp, "update nodes set boot_errno='%d' "
			"where node_id='%s'",
			myerrno, reqp->nodeid)) {
		error("DOBOOTERRNO: %s: setting boot errno!\n", reqp->nodeid);
		return 1;
	}
	if (reqp->verbose)
		info("DOBOOTERRNO: errno=%d\n", myerrno);
		
	return 0;
}

/*
 * Tell us about battery statistics.
 */
XML_COMMAND_PROTOTYPE(dobattery)
{
	float		capacity = 0.0, voltage = 0.0;
	xmlNode		*node;
	
	/*
	 * Dig out the capacity and voltage, then
	 */
	if ((sscanf(rdata,
		    "CAPACITY=%f VOLTAGE=%f",
		    &capacity,
		    &voltage) != 2) ||
	    (capacity < 0.0f) || (capacity > 100.0f) ||
	    (voltage < 5.0f) || (voltage > 15.0f)) {
		error("DOBATTERY: %s: Bad arguments\n", reqp->nodeid);
		return 1;
	}

	/*
	 * ... update DB.
	 */
	if (mydb_update(reqp, "UPDATE nodes SET battery_percentage=%f,"
			"battery_voltage=%f,"
			"battery_timestamp=UNIX_TIMESTAMP(now()) "
			"WHERE node_id='%s'",
			capacity, voltage, reqp->nodeid)) {
		error("DOBATTERY: %s: setting boot errno!\n", reqp->nodeid);
		return 1;
	}
	if (reqp->verbose) {
		info("DOBATTERY: capacity=%.2f voltage=%.2f\n",
		     capacity,
		     voltage);
	}
	
	node = new_response(root, "battery");
	add_key(node, "response", "OK");
	
	return 0;
}

int slurp_file(char *filename, char **outbuf, int *length, char *function)
{
	FILE 		*fp;
	int 		cc;
	struct stat 	st;

	*outbuf = NULL;
	*length = 0;

	if (stat(filename, &st) < 0) {
		error("%s: Could not stat %s:", function,
			filename);
		return 1;
	}

	if ((fp = fopen(filename, "r")) == NULL) {
		errorc("%s: Could not open %s:", function,
		       filename);
		return 1;
	}

	*outbuf = malloc(st.st_size);
	if (*outbuf == NULL) {
		fclose(fp);
		return 1;
	}

	*length = 0;
	while (1) {
		cc = fread(*outbuf + *length, sizeof(char), st.st_size - *length, fp);
		*length += cc;
		if (cc == 0) {
			if (ferror(fp)) {
				fclose(fp);
				return 1;
			}
			break;
		}
	}
	fclose(fp);
	return 0;
}

/*
 * Spit back the topomap. This is a backup for when NFS fails.
 * We send back the gzipped version.
 */
RAW_COMMAND_PROTOTYPE(dotopomap)
{
	char		filename[MAXPATHLEN];
	/*
	 * Open up the file on boss and spit it back.
	 */
	sprintf(filename, "%s/expwork/%s/%s/topomap.gz", TBROOT,
		reqp->pid, reqp->eid);

	return slurp_file(filename, outbuf, length, "dotopomap");

}

/*
 * Spit back the ltmap. This is a backup for when NFS fails.
 * We send back the gzipped version.
 */
RAW_COMMAND_PROTOTYPE(doltmap)
{
	char		filename[MAXPATHLEN];

	/*
	 * Open up the file on boss and spit it back.
	 */
	sprintf(filename, "%s/expwork/%s/%s/ltmap.gz", TBROOT,
		reqp->pid, reqp->eid);

	return slurp_file(filename, outbuf, length, "doltmap");
}

/*
 * Spit back the ltpmap. This is a backup for when NFS fails.
 * We send back the gzipped version.
 */
RAW_COMMAND_PROTOTYPE(doltpmap)
{
	char		filename[MAXPATHLEN];

	/*
	 * Open up the file on boss and spit it back.
	 */
	sprintf(filename, "%s/expwork/%s/%s/ltpmap.gz", TBROOT,
		reqp->pid, reqp->eid);

	return slurp_file(filename, outbuf, length, "doltpmap");
}

/*
 * Return user environment.
 */
XML_COMMAND_PROTOTYPE(douserenv)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node;

	res = mydb_query(reqp, "select name,value from virt_user_environment "
			 "where pid='%s' and eid='%s' order by idx",
			 2, reqp->pid, reqp->eid);

	if (!res) {
		error("USERENV: %s: DB Error getting virt_user_environment\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	
	/* XXX Ryan: Is there a better way to do this? */
	node = new_response(root, "userenv");
	while (nrows) {
		row = mysql_fetch_row(res);

		add_key(node, row[0], row[1]);
		
		nrows--;
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return tip tunnels for the node.
 */
XML_COMMAND_PROTOTYPE(dotiptunnels)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node, *tunnel_node;

	res = mydb_query(reqp, "select vtt.vnode,tl.server,tl.portnum,tl.keylen,"
			 "tl.keydata "
			 "from virt_tiptunnels as vtt "
			 "left join reserved as r on r.vname=vtt.vnode and "
			 "  r.pid=vtt.pid and r.eid=vtt.eid "
			 "left join tiplines as tl on tl.node_id=r.node_id "
			 "where vtt.pid='%s' and vtt.eid='%s' and "
			 "vtt.host='%s'",
			 5, reqp->pid, reqp->eid, reqp->nickname);

	if (!res) {
		error("TIPTUNNELS: %s: DB Error getting virt_tiptunnels\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	node = new_response(root, "tiptunnels");
	
	while (nrows) {
		tunnel_node = new_response(node, "tiptunnel");
		row = mysql_fetch_row(res);

		if (row[1]) {
			add_key(tunnel_node, "vnode", row[0]);
			add_key(tunnel_node, "server", row[1]);
			add_key(tunnel_node, "port", row[2]);
			add_key(tunnel_node, "keylen", row[3]);
			add_key(tunnel_node, "key", row[4]);
		}
		
		nrows--;
	}
	mysql_free_result(res);
	return 0;
}

XML_COMMAND_PROTOTYPE(dorelayconfig)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node;

	res = mydb_query(reqp, "select tl.server,tl.portnum from tiplines as tl "
			 "where tl.node_id='%s'",
			 2, reqp->nodeid);

	if (!res) {
		error("RELAYCONFIG: %s: DB Error getting relay config\n",
		      reqp->nodeid);
		return 1;
	}
	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}
	
	while (nrows) {
		row = mysql_fetch_row(res);
		node = new_response(root, "relayconfig");

		add_key(node, "type", reqp->type);
		add_key(node, "capserver", row[0]);
		add_key(node, "capport", row[1]);
		
		nrows--;
	}
	mysql_free_result(res);
	return 0;
}

/*
 * Return trace config
 */
XML_COMMAND_PROTOTYPE(dotraceconfig)
{
	MYSQL_RES	*res;	
	MYSQL_ROW	row;
	int		nrows;
	xmlNode		*node, *traceconfig_node;

	/*
	 * Get delay parameters for the machine. The point of this silly
	 * join is to get the type out so that we can pass it back. Of
	 * course, this assumes that the type is the BSD name, not linux.
	 */
	if (! reqp->isvnode) {
	  res = mydb_query(reqp, "select linkvname,i0.MAC,i1.MAC,t.vnode,i2.MAC, "
			   "       t.trace_type,t.trace_expr,t.trace_snaplen, "
			   "       t.idx "
			   "from traces as t "
			   "left join interfaces as i0 on "
			   " i0.node_id=t.node_id and i0.iface=t.iface0 "
			   "left join interfaces as i1 on "
			   " i1.node_id=t.node_id and i1.iface=t.iface1 "
			   "left join reserved as r on r.vname=t.vnode and "
			   "     r.pid='%s' and r.eid='%s' "
			   "left join virt_lans as v on v.vname=t.linkvname  "
			   " and v.pid=r.pid and v.eid=r.eid and "
			   "     v.vnode=t.vnode "
			   "left join interfaces as i2 on "
			   "     i2.node_id=r.node_id and i2.IP=v.ip "
			   " where t.node_id='%s'",	 
			   9, reqp->pid, reqp->eid, reqp->nodeid);
	}
	else {
	  res = mydb_query(reqp, "select linkvname,i0.mac,i1.mac,t.vnode,'', "
			   "       t.trace_type,t.trace_expr,t.trace_snaplen, "
			   "       t.idx "
			   "from traces as t "
			   "left join vinterfaces as i0 on "
			   " i0.vnode_id=t.node_id and "
			   " i0.unit=SUBSTRING(t.iface0, 5) "
			   "left join vinterfaces as i1 on "
			   " i1.vnode_id=t.node_id and "
			   " i1.unit=SUBSTRING(t.iface1, 5) "
			   "left join reserved as r on r.vname=t.vnode and "
			   "     r.pid='%s' and r.eid='%s' "
			   "left join virt_lans as v on v.vname=t.linkvname  "
			   " and v.pid=r.pid and v.eid=r.eid and "
			   "     v.vnode=t.vnode "
			   "left join interfaces as i2 on "
			   "     i2.node_id=r.node_id and i2.IP=v.ip "
			   " where t.node_id='%s'",	 
			   9, reqp->pid, reqp->eid, reqp->nodeid);
	}
			 
	if (!res) {
		error("TRACEINFO: %s: DB Error getting trace table!\n",
		      reqp->nodeid);
		return 1;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	traceconfig_node = new_response(root, "traceconfig");
	while (nrows) {
		int     idx;
		
		row = mysql_fetch_row(res);

		/*
		 * XXX plab hack: add the vnode number to the idx to
		 * prevent vnodes on the same pnode from using the same
		 * port number!
		 */
		idx = atoi(row[8]);
		if (reqp->isplabdslice &&
		    strncmp(reqp->nodeid, "plabvm", 6) == 0) {
		    char *cp = index(reqp->nodeid, '-');
		    if (cp && *(cp+1))
			idx += (atoi(cp+1) * 10);
		}

		node = new_response(traceconfig_node, "entry");
		add_key(node, "linkname", row[0]);
		add_format_key(node, "index", "%d", idx);
		add_key(node, "mac0", (row[1] ? row[1] : ""));
		add_key(node, "mac1", (row[2] ? row[2] : ""));
		add_key(node, "vnode", row[3]);
		add_key(node, "vnode_mac", row[4]);
		add_key(node, "type", row[5]);
		add_key(node, "expr", row[6]);
		add_key(node, "snaplen", row[7]);

		nrows--;
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Acquire plab node elvind port update.
 *
 * Since there is (currently) no way to hard reserve a port on a plab node
 * we might bind to a non-default port, and so must track this so that
 * client vservers on the plab node know where to connect.
 *
 * XXX: should make sure it's the service slice reporting this.
 */
XML_COMMAND_PROTOTYPE(doelvindport)
{
	char		buf[MYBUFSIZE];
	unsigned int	elvport = 0;

	if (sscanf(rdata, "%u",
		   &elvport) != 1) {
		strncpy(buf, rdata, 64);
		error("ELVIND_PORT: %s: Bad arguments: %s...\n", reqp->nodeid,
                      buf);
		return 1;
	}

	/*
         * Now shove the elvin port # we got into the db.
	 */
	mydb_update(reqp, "replace into node_attributes "
                    " values ('%s', '%s', %u)",
		    reqp->pnodeid, "elvind_port", elvport);

	return 0;
}

/*
 * Return all event keys on plab node to service slice.
 */
XML_COMMAND_PROTOTYPE(doplabeventkeys)
{
        int             nrows = 0;
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	xmlNode		*node, *eventkey_node;

        if (!reqp->isplabsvc) {
                error("PLABEVENTKEYS: Unauthorized request from node: %s\n", 
                      reqp->vnodeid);
                return 1;
        }

        res = mydb_query(reqp, "select e.pid, e.eid, e.eventkey from reserved as r "
                         " left join nodes as n on r.node_id = n.node_id "
                         " left join experiments as e on r.pid = e.pid "
                         "  and r.eid = e.eid "
                         " where n.phys_nodeid = '%s' ",
                         3, reqp->pnodeid);

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 0;
	}

	node = new_response(root, "plabeventkeys");

	while (nrows) {
		row = mysql_fetch_row(res);
		eventkey_node = new_response(node, "plabeventkey");

		add_key(eventkey_node, "pid", row[0]);
		add_key(eventkey_node, "eid", row[1]);
		add_key(eventkey_node, "key", row[2]);

		nrows--;
	}
	mysql_free_result(res);

	return 0;
}

/*
 * Return a map of pc node id's with their interface MAC addresses.
 */
XML_COMMAND_PROTOTYPE(dointfcmap)
{
	char		pc[8] = {0};
        int             nrows = 0, npcs = 0;
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	xmlNode		*node = NULL;

        res = mydb_query(reqp, "select node_id, mac from interfaces "
			 "where node_id like 'pc%%' order by node_id",
                         2);

	nrows = (int)mysql_num_rows(res);
	if (reqp->verbose)
		info("intfcmap: nrows %d\n", nrows);
	if (nrows == 0) {
		mysql_free_result(res);
		return 0;
	}

	/* XXX Ryan: might this benefit from more structure? */
	while (nrows) {
		row = mysql_fetch_row(res);
		/* if (reqp->verbose) info("intfcmap: %s %s\n", row[0], row[1]); */

		/* Consolidate interfaces on the same pc into a single line. */
		if (pc[0] == '\0') {
			/* First pc. */
			node = new_response(root, "intfcmap");
			add_key(node, "nodeid", row[0]);
			add_key(node, "mac", row[1]);
		} else if (strncmp(pc, row[0], 8) == 0 ) {
			/* Same pc, append. */
			add_key(node, "mac", row[1]);
		} else {   
			/* Different pc, dump this one and start the next. */
			npcs++;

			node = new_response(root, "intfcmap");
			add_key(node, "nodeid", row[0]);
			add_key(node, "mac", row[1]);
		}
		nrows--;
	}

	npcs++;
	if (reqp->verbose)
		info("intfcmap: npcs %d\n", npcs);

	mysql_free_result(res);

	return 0;
}


/*
 * Return motelog info for this node.
 */
XML_COMMAND_PROTOTYPE(domotelog)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    int nrows;
    xmlNode *node, *motelog_node;

    res = mydb_query(reqp, "select vnm.logfileid,ml.classfilepath,ml.specfilepath "
		     "from virt_node_motelog as vnm "
		     "left join motelogfiles as ml on vnm.pid=ml.pid "
		     "  and vnm.logfileid=ml.logfileid "
		     "left join reserved as r on r.vname=vnm.vname "
		     "  and vnm.eid=r.eid and vnm.pid=r.pid "
		     "where vnm.pid='%s' and vnm.eid='%s' "
		     "  and vnm.vname='%s'",
		     3,reqp->pid,reqp->eid,reqp->nickname);

    if (!res) {
	error("MOTELOG: %s: DB Error getting virt_node_motelog\n",
	      reqp->nodeid);
    }

    /* no motelog stuff for this node */
    if ((nrows = (int)mysql_num_rows(res)) == 0) {
	mysql_free_result(res);
	return 0;
    }

    node = new_response(root, "motelogs");

    while (nrows) {
	row = mysql_fetch_row(res);
	
	/* only specfilepath can possibly be null */
	motelog_node = new_response(root, "motelog");
	add_key(motelog_node, "motelogid", row[0]);
	add_key(motelog_node, "classfile", row[1]);
	add_key(motelog_node, "specfile", row[2]);
    
	--nrows;
    }

    mysql_free_result(res);
    return 0;
}

/*
 * Return motelog info for this node.
 */
XML_COMMAND_PROTOTYPE(doportregister)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	char		buf[MYBUFSIZE], service[128];
	int		rc, port;
	xmlNode		*node;

	/*
	 * Dig out the service and the port number.
	 * Need to be careful about not overflowing the buffer.
	 */
	sprintf(buf, "%%%ds %%d", sizeof(service));
	rc = sscanf(rdata, buf, service, &port);

	if (rc == 0) {
		error("doportregister: No service specified!\n");
		return 1;
	}
	if (rc != 1 && rc != 2) {
		error("doportregister: Wrong number of arguments!\n");
		return 1;
	}

	/* No special characters means it will fit */
	mysql_escape_string(buf, service, strlen(service));
	if (strlen(buf) >= sizeof(service)) {
		error("doportregister: Illegal chars in service!\n");
		return 1;
	}
	strcpy(service, buf);

	/*
	 * Single argument, lookup up service.
	 */
	if (rc == 1) {
		res = mydb_query(reqp, "select port,node_id "
				 "   from port_registration "
				 "where pid='%s' and eid='%s' and "
				 "      service='%s'",
				 2, reqp->pid, reqp->eid, service);
		
		if (!res) {
			error("doportregister: %s: "
			      "DB Error getting registration for %s\n",
			      reqp->nodeid, service);
			return 1;
		}
		if ((int)mysql_num_rows(res) > 0) {
			row = mysql_fetch_row(res);
			node = new_response(root, "portregister");
			add_key(node, "port", row[0]);
			add_format_key(node, "nodeid", "%s.%s", row[1],
					OURDOMAIN);

			if (reqp->verbose)
				info("PORTREG: %s: %s", reqp->nodeid, buf);
		}
		mysql_free_result(res);
		return 0;
	}
	
	/*
	 * If port is zero, clear it from the DB
	 */
	if (port == 0) {
		mydb_update(reqp, "delete from port_registration  "
			    "where pid='%s' and eid='%s' and "
			    "      service='%s'",
			    reqp->pid, reqp->eid, service);
	}
	else {
		/*
		 * Register port for the service.
		 */
		if (mydb_update(reqp, "replace into port_registration set "
				"     pid='%s', eid='%s', exptidx=%d, "
				"     service='%s', node_id='%s', port='%d'",
				reqp->pid, reqp->eid, reqp->exptidx,
				service, reqp->nodeid, port)) {
			error("doportregister: %s: DB Error setting %s=%d!\n",
			      reqp->nodeid, service, port);
			return 1;
		}
	}
	if (reqp->verbose)
		info("PORTREG: %s: %s=%d\n", reqp->nodeid, service, port);
	
	return 0;
}

/*
 * Return bootwhat info using the bootinfo routine from the pxe directory.
 * This is used by the dongle boot on widearea nodes. Better to use tmcd
 * then open up another daemon to the widearea.
 */
XML_COMMAND_PROTOTYPE(dobootwhat)
{
	boot_info_t	boot_info;
	boot_what_t	*boot_whatp = (boot_what_t *) &boot_info.data;
	xmlNode		*node;

	boot_info.opcode  = BIOPCODE_BOOTWHAT_REQUEST;
	boot_info.version = BIVERSION_CURRENT;

	node = new_response(root, "bootwhat");

	if(strlen(reqp->privkey) > 1) { /* We have a private key, so prepare bootinfo for it. */
		boot_info.opcode = BIOPCODE_BOOTWHAT_KEYED_REQUEST;
		strncpy(boot_info.data, reqp->privkey, TBDB_FLEN_PRIVKEY);
	}

	if (bootinfo(reqp->client, &boot_info, (void *) reqp)) {
		add_key(node, "status", "failed");
	}
	else {
		add_key(node, "status", "success");
		add_format_key(node, "type", "%d", boot_whatp->type);

		switch (boot_whatp->type) {
			case BIBOOTWHAT_TYPE_PART:
				add_format_key(node, "what", "%d", boot_whatp->what.partition);
				break;
			case BIBOOTWHAT_TYPE_SYSID:
				add_format_key(node, "what", "%d", boot_whatp->what.sysid);
				break;
			case BIBOOTWHAT_TYPE_MFS:
				add_key(node, "what", boot_whatp->what.mfs);
				break;
		}
		if (strlen(boot_whatp->cmdline)) {
			add_key(node, "cmdline", boot_whatp->cmdline);
		}

	}

	return 0;
}

/*
 * DB stuff
 */
static void	mydb_disconnect(tmcdreq_t* reqp);

static int
mydb_connect(tmcdreq_t* reqp)
{
	/*
	 * Each time we talk to the DB, we check to see if the name
	 * matches the last connection. If so, nothing more needs to
	 * be done. If we switched DBs (checkdbredirect()), then drop
	 * the current connection and form a new one.
	 */
	if (reqp->db_connected) {
		if (strcmp(reqp->prev_dbname, reqp->dbname) == 0)
			return 1;
		mydb_disconnect(reqp);
	}
	
	mysql_init(&reqp->db);
	if (mysql_real_connect(&reqp->db, 0, "tmcd", 0,
			       reqp->dbname, 0, 0, CLIENT_INTERACTIVE) == 0) {
		error("%s: connect failed: %s\n", reqp->dbname, mysql_error(&reqp->db));
		return 0;
	}
	strcpy(reqp->prev_dbname, reqp->dbname);
	reqp->db_connected = 1;
	return 1;
}

static void
mydb_disconnect(tmcdreq_t* reqp)
{
	mysql_close(&reqp->db);
	reqp->db_connected = 0;
}

/*
 * Just so we can include bootinfo from the pxe directory.
 */
#if 0
extern int	dbinit(void) { return 0;}
extern void	dbclose(void) {}
#endif

MYSQL_RES *
mydb_query(tmcdreq_t *reqp, char *query, int ncols, ...)
{
	MYSQL_RES	*res;
	char		querybuf[2*MYBUFSIZE];
	va_list		ap;
	int		n;

	va_start(ap, ncols);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	if (n > sizeof(querybuf)) {
		error("query too long for buffer\n");
		return (MYSQL_RES *) 0;
	}

	if (! mydb_connect(reqp))
		return (MYSQL_RES *) 0;

	if (mysql_real_query(&reqp->db, querybuf, n) != 0) {
		error("%s: query failed: %s, retrying\n",
		      reqp->dbname, mysql_error(&reqp->db));
		mydb_disconnect(reqp);
		/*
		 * Try once to reconnect.  In theory, the caller (client)
		 * will retry the tmcc call and we will reconnect and
		 * everything will be fine.  The problem is that the
		 * client may get a different tmcd process each time,
		 * and every one of those will fail once before
		 * reconnecting.  Hence, the client could wind up failing
		 * even if it retried.
		 */
		if (!mydb_connect(reqp) ||
		    mysql_real_query(&reqp->db, querybuf, n) != 0) {
			error("%s: query failed: %s\n",
			      reqp->dbname, mysql_error(&reqp->db));
			return (MYSQL_RES *) 0;
		}
	}

	res = mysql_store_result(&reqp->db);
	if (res == 0) {
		error("%s: store_result failed: %s\n",
		      reqp->dbname, mysql_error(&reqp->db));
		mydb_disconnect(reqp);
		return (MYSQL_RES *) 0;
	}

	if (ncols && ncols != (int)mysql_num_fields(res)) {
		error("%s: Wrong number of fields returned "
		      "Wanted %d, Got %d\n",
		      reqp->dbname, ncols, (int)mysql_num_fields(res));
		mysql_free_result(res);
		return (MYSQL_RES *) 0;
	}
	return res;
}

int
mydb_update(tmcdreq_t *reqp, char *query, ...)
{
	char		querybuf[8 * 1024];
	va_list		ap;
	int		n;

	va_start(ap, query);
	n = vsnprintf(querybuf, sizeof(querybuf), query, ap);
	if (n > sizeof(querybuf)) {
		error("query too long for buffer\n");
		return 1;
	}

	if (! mydb_connect(reqp))
		return 1;

	if (mysql_real_query(&reqp->db, querybuf, n) != 0) {
		error("%s: query failed: %s\n", reqp->dbname, mysql_error(&reqp->db));
		mydb_disconnect(reqp);
		return 1;
	}
	return 0;
}

/*
 * Map IP to node ID (plus other info).
 */
int
iptonodeid(tmcdreq_t *reqp, struct in_addr ipaddr, char* nodekey)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	/*
	 * I love a good query!
	 *
	 * The join on node_types using control_iface is to prevent the
	 * (unlikely) possibility that we get an experimental interface
	 * trying to contact us! I doubt that can happen though. 
	 *
	 * XXX Locally, the jail flag is not set on the phys node, only
	 * on the virtnodes. This is okay since all the routines that
	 * check jailflag also check to see if its a vnode or physnode.
	 */
	/*
	 * Widearea nodes have wanodekeys that should be used to get
	 * the nodeid.
	 */ 
	if ((nodekey != NULL) && (strlen(nodekey) > 1)) {
		res = mydb_query("SELECT t.class,t.type,n.node_id,n.jailflag,"
				 " r.pid,r.eid,r.vname,e.gid,e.testdb, "
				 " n.update_accounts,n.role, "
				 " e.expt_head_uid,e.expt_swap_uid, "
				 " e.sync_server,t.class,t.type, "
				 " t.isremotenode,t.issubnode,e.keyhash, "
				 " nk.sfshostid,e.eventkey,0, "
				 " 0,e.elab_in_elab,e.elabinelab_singlenet, "
				 " e.idx,e.creator_idx,e.swapper_idx, "
				 " u.admin,dedicated_wa_types.attrvalue "
				 "   AS isdedicated_wa, "
				 " r.genisliver_idx,r.tmcd_redirect "
				 "FROM nodes AS n "
				 "LEFT JOIN reserved AS r ON "
				 "  r.node_id=n.node_id "
				 "LEFT JOIN experiments AS e ON "
				 " e.pid=r.pid and e.eid=r.eid "
				 "LEFT JOIN node_types AS t ON "
				 " t.type=n.type "
				 "LEFT JOIN node_hostkeys AS nk ON "
				 " nk.node_id=n.node_id "
				 "LEFT JOIN users AS u ON "
				 " u.uid_idx=e.swapper_idx "
				 "LEFT OUTER JOIN "
				 "  (SELECT type,attrvalue "
				 "    FROM node_type_attributes "
				 "    WHERE attrkey='nobootinfo' "
				 "      AND attrvalue='1' "
				 "     GROUP BY type) AS nobootinfo_types "
				 "  ON n.type=nobootinfo_types.type "
				 "LEFT OUTER JOIN "
				 "  (SELECT type,attrvalue "
				 "   FROM node_type_attributes "
				 "   WHERE attrkey='dedicated_widearea' "
				 "   GROUP BY type) AS dedicated_wa_types "
				 "  ON n.type=dedicated_wa_types.type "
				"WHERE n.node_id IN "
                                        "(SELECT node_id FROM widearea_nodeinfo WHERE privkey='%s') "
				 "  AND nobootinfo_types.attrvalue IS NULL",
				 32, nodekey);

	}

	else if (reqp->isvnode) {
		res = mydb_query(reqp, "select vt.class,vt.type,np.node_id,"
				 " nv.jailflag,r.pid,r.eid,r.vname, "
				 " e.gid,e.testdb,nv.update_accounts, "
				 " np.role,e.expt_head_uid,e.expt_swap_uid, "
				 " e.sync_server,pt.class,pt.type, "
				 " pt.isremotenode,vt.issubnode,e.keyhash, "
				 " nk.sfshostid,e.eventkey,vt.isplabdslice, "
				 " ps.admin, "
				 " e.elab_in_elab,e.elabinelab_singlenet, "
				 " e.idx,e.creator_idx,e.swapper_idx, "
				 " u.admin,null, "
				 " r.genisliver_idx,r.tmcd_redirect "
				 "from nodes as nv "
				 "left join nodes as np on "
				 " np.node_id=nv.phys_nodeid "
				 "left join interfaces as i on "
				 " i.node_id=np.node_id "
				 "left join reserved as r on "
				 " r.node_id=nv.node_id "
				 "left join experiments as e on "
				 "  e.pid=r.pid and e.eid=r.eid "
				 "left join node_types as pt on "
				 " pt.type=np.type "
				 "left join node_types as vt on "
				 " vt.type=nv.type "
				 "left join plab_slices as ps on "
				 " ps.pid=e.pid and ps.eid=e.eid "
				 "left join node_hostkeys as nk on "
				 " nk.node_id=nv.node_id "
				 "left join users as u on "
				 " u.uid_idx=e.swapper_idx "
				 "where nv.node_id='%s' and "
				 " ((i.IP='%s' and i.role='ctrl') or "
				 "  nv.jailip='%s')",
				 32, reqp->vnodeid,
				 inet_ntoa(ipaddr), inet_ntoa(ipaddr));
	}
	else {
		res = mydb_query(reqp, "select t.class,t.type,n.node_id,n.jailflag,"
				 " r.pid,r.eid,r.vname,e.gid,e.testdb, "
				 " n.update_accounts,n.role, "
				 " e.expt_head_uid,e.expt_swap_uid, "
				 " e.sync_server,t.class,t.type, "
				 " t.isremotenode,t.issubnode,e.keyhash, "
				 " nk.sfshostid,e.eventkey,0, "
				 " 0,e.elab_in_elab,e.elabinelab_singlenet, "
				 " e.idx,e.creator_idx,e.swapper_idx, "
				 " u.admin,dedicated_wa_types.attrvalue "
				 "   as isdedicated_wa, "
				 " r.genisliver_idx,r.tmcd_redirect "
				 "from interfaces as i "
				 "left join nodes as n on n.node_id=i.node_id "
				 "left join reserved as r on "
				 "  r.node_id=i.node_id "
				 "left join experiments as e on "
				 " e.pid=r.pid and e.eid=r.eid "
				 "left join node_types as t on "
				 " t.type=n.type "
				 "left join node_hostkeys as nk on "
				 " nk.node_id=n.node_id "
				 "left join users as u on "
				 " u.uid_idx=e.swapper_idx "
				 "left outer join "
				 "  (select type,attrvalue "
				 "    from node_type_attributes "
				 "    where attrkey='nobootinfo' "
				 "      and attrvalue='1' "
				 "     group by type) as nobootinfo_types "
				 "  on n.type=nobootinfo_types.type "
				 "left outer join "
				 "  (select type,attrvalue "
				 "   from node_type_attributes "
				 "   where attrkey='dedicated_widearea' "
				 "   group by type) as dedicated_wa_types "
				 "  on n.type=dedicated_wa_types.type "
				 "where i.IP='%s' and i.role='ctrl' "
				 "  and nobootinfo_types.attrvalue is NULL",
				 32, inet_ntoa(ipaddr));
	}

	if (!res) {
		error("iptonodeid: %s: DB Error getting interfaces!\n",
		      inet_ntoa(ipaddr));
		return 1;
	}

	if (! (int)mysql_num_rows(res)) {
		mysql_free_result(res);
		return 1;
	}
	row = mysql_fetch_row(res);
	if (!row[0] || !row[1] || !row[2]) {
		error("iptonodeid: %s: Malformed DB response!\n",
		      inet_ntoa(ipaddr)); 
		mysql_free_result(res);
		return 1;
	}
	reqp->client = ipaddr;
	strncpy(reqp->class,  row[0],  sizeof(reqp->class));
	strncpy(reqp->type,   row[1],  sizeof(reqp->type));
	strncpy(reqp->pclass, row[14], sizeof(reqp->pclass));
	strncpy(reqp->ptype,  row[15], sizeof(reqp->ptype));
	strncpy(reqp->nodeid, row[2],  sizeof(reqp->nodeid));
	if(nodekey != NULL) {
		strncpy(reqp->privkey, nodekey, TBDB_FLEN_PRIVKEY); 
	}
	else {
		strcpy(reqp->privkey, ""); 
	}
	reqp->islocal      = (! strcasecmp(row[16], "0") ? 1 : 0);
	reqp->jailflag     = (! strcasecmp(row[3],  "0") ? 0 : 1);
	reqp->issubnode    = (! strcasecmp(row[17], "0") ? 0 : 1);
	reqp->isplabdslice = (! strcasecmp(row[21], "0") ? 0 : 1);
	reqp->isplabsvc    = (row[22] && strcasecmp(row[22], "0")) ? 1 : 0;
	reqp->elab_in_elab = (row[23] && strcasecmp(row[23], "0")) ? 1 : 0;
	reqp->singlenet    = (row[24] && strcasecmp(row[24], "0")) ? 1 : 0;
	reqp->isdedicatedwa = (row[29] && !strncmp(row[29], "1", 1)) ? 1 : 0;

	if (row[8])
		strncpy(reqp->testdb, row[8], sizeof(reqp->testdb));
	if (row[4] && row[5]) {
		strncpy(reqp->pid, row[4], sizeof(reqp->pid));
		strncpy(reqp->eid, row[5], sizeof(reqp->eid));
		reqp->exptidx = atoi(row[25]);
		reqp->allocated = 1;

		if (row[6])
			strncpy(reqp->nickname, row[6],sizeof(reqp->nickname));
		else
			strcpy(reqp->nickname, reqp->nodeid);

		strcpy(reqp->creator, row[11]);
		reqp->creator_idx = atoi(row[26]);
		if (row[12]) {
			strcpy(reqp->swapper, row[12]);
			reqp->swapper_idx = atoi(row[27]);
		}
		else {
			strcpy(reqp->swapper, reqp->creator);
			reqp->swapper_idx = reqp->creator_idx;
		}
		if (row[28])
			reqp->swapper_isadmin = atoi(row[28]);
		else
			reqp->swapper_isadmin = 0;

		/*
		 * If there is no gid (yes, thats bad and a mistake), then 
		 * copy the pid in. Also warn.
		 */
		if (row[7])
			strncpy(reqp->gid, row[7], sizeof(reqp->gid));
		else {
			strcpy(reqp->gid, reqp->pid);
			error("iptonodeid: %s: No GID for %s/%s (pid/eid)!\n",
			      reqp->nodeid, reqp->pid, reqp->eid);
		}
		/* Sync server for the experiment */
		if (row[13]) 
			strcpy(reqp->syncserver, row[13]);
		/* keyhash for the experiment */
		if (row[18]) 
			strcpy(reqp->keyhash, row[18]);
		/* event key for the experiment */
		if (row[20]) 
			strcpy(reqp->eventkey, row[20]);
		/* geni sliver idx */
		if (row[30]) 
			reqp->genisliver_idx = atoi(row[30]);
		else
			reqp->genisliver_idx = 0;
		if (row[31]) 
			strcpy(reqp->tmcd_redirect, row[31]);
	}
	
	if (row[9])
		reqp->update_accounts = atoi(row[9]);
	else
		reqp->update_accounts = 0;

	/* SFS hostid for the node */
	if (row[19]) 
		strcpy(reqp->sfshostid, row[19]);
	
	reqp->iscontrol = (! strcasecmp(row[10], "ctrlnode") ? 1 : 0);

	/* If a vnode, copy into the nodeid. Eventually split this properly */
	strcpy(reqp->pnodeid, reqp->nodeid);
	if (reqp->isvnode) {
		strcpy(reqp->nodeid,  reqp->vnodeid);
	}
	
	mysql_free_result(res);
	return 0;
}
 
/*
 * Map nodeid to PID/EID pair.
 */
static int
nodeidtoexp(tmcdreq_t *reqp)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	res = mydb_query(reqp, "select r.pid,r.eid,e.gid from reserved as r "
			 "left join experiments as e on "
			 "     e.pid=r.pid and e.eid=r.eid "
			 "where node_id='%s'",
			 3, reqp->nodeid);
	if (!res) {
		error("nodeidtoexp: %s: DB Error getting reserved!\n", reqp->nodeid);
		return 1;
	}

	if (! (int)mysql_num_rows(res)) {
		mysql_free_result(res);
		return 1;
	}
	row = mysql_fetch_row(res);
	mysql_free_result(res);
	strncpy(reqp->pid, row[0], TBDB_FLEN_PID);
	strncpy(reqp->eid, row[1], TBDB_FLEN_EID);

	/*
	 * If there is no gid (yes, thats bad and a mistake), then copy
	 * the pid in. Also warn.
	 */
	if (row[2]) {
		strncpy(reqp->gid, row[2], TBDB_FLEN_GID);
	}
	else {
		strcpy(reqp->gid, reqp->pid);
		error("nodeidtoexp: %s: No GID for %s/%s (pid/eid)!\n",
		      reqp->nodeid, reqp->pid, reqp->eid);
	}

	return 0;
}
 
/*
 * Check private key. 
 */
int
checkprivkey(tmcdreq_t *reqp, struct in_addr ipaddr, char *privkey)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;

	res = mydb_query(reqp, "select privkey from widearea_privkeys "
			 "where IP='%s'",
			 1, inet_ntoa(ipaddr));
	
	if (!res) {
		error("checkprivkey: %s: DB Error getting privkey!\n",
		      inet_ntoa(ipaddr));
		return 1;
	}

	if (! (int)mysql_num_rows(res)) {
		mysql_free_result(res);
		return 1;
	}
	row = mysql_fetch_row(res);
	mysql_free_result(res);
	if (! row[0] || !row[0][0])
		return 1;

	return strcmp(privkey, row[0]);
}
 
/*
 * Check for DBname redirection.
 */
int
checkdbredirect(tmcdreq_t *reqp)
{
	if (! reqp->allocated || !strlen(reqp->testdb))
		return 0; /* XXX Ryan shouldn't this be 1, not 0? */

	/* This changes the DB we talk to. */
	strcpy(reqp->dbname, reqp->testdb);

	/*
	 * Okay, lets test to make sure that DB exists. If not, fall back
	 * on the main DB. 
	 */
	if (nodeidtoexp(reqp)) {
		error("CHECKDBREDIRECT: %s: %s DB does not exist\n",
		      reqp->nodeid, reqp->dbname);
		strcpy(reqp->dbname, DEFAULT_DBNAME);
	}
	return 0;
}


int tmcd_init(tmcdreq_t *reqp, struct in_addr *addr, char *db)
{
	FILE *fp;

	memcpy(&reqp->myipaddr, addr, sizeof(&reqp->myipaddr));

	/* Start with default DB */
	strcpy(reqp->dbname, DEFAULT_DBNAME);

	if (db) {
		strncpy(reqp->dbname, db, sizeof(reqp->dbname));
	}

#ifdef EVENTSYS
	reqp->event_handle = NULL;
#endif

	/*
	 * Get FS's SFS hostid
	 * XXX This approach is somewhat kludgy
	 */
	strcpy(reqp->fshostid, "");
	if (access(FSHOSTID,R_OK) == 0) {
		fp = fopen(FSHOSTID, "r");
		if (!fp) {
			error("Failed to get FS's hostid");
		}
		else {
			fgets(reqp->fshostid, HOSTID_SIZE, fp);
			if (rindex(reqp->fshostid, '\n')) {
				*rindex(reqp->fshostid, '\n') = 0;
				if (reqp->debug) {
				    info("fshostid: %s\n", reqp->fshostid);
				}
			}
			else {
				error("fshostid from %s may be corrupt: %s",
				      FSHOSTID, reqp->fshostid);
			}
			fclose(fp);
		}
	}

	return 0;
	
}
