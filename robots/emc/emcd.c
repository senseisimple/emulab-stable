
/* emc sets up an interface for accepting commands over an internet socket;
 * listens on a well-known port for instances of vmc and rmc;
 * and maintains several key data structures.
 */

/* so emc reads config data, sets up data structures, listens on main port
 * every connecting client is configured or rejected, then is placed in
 * a working set to select() on.  Then commands are processed from either a 
 * file, or from an active user interface; requests/data are processed from
 * RMC and VMC, and appropriate responses are generated if necessary (whether
 * the response be to the requesting client, or to somebody else (i.e., if 
 * VMC pushes a position update, EMC does not need to respond to VMC -- but it
 * might need to push data to RMC!)
 */

// so we need to store a bunch of data for data from RMC, VMC, and initial
// all this data can reuse the same data structure, which includes a list of
//   robot id
//   position
//   orientation
//   status

// for the initial configuration of rmc, we need to send a list of id -> 
// hostname pairs (later we will also need to send a bounding box, but not
// until the vision system is a long ways along).

// for the initial configuration of vmc, we need to send a list of robot ids,
// nothing more (as the vision system starts picking up robots, it will query
// emc with known positions/orientations to get matching ids)

// so the emc config file is a bunch of lines with id number, hostname, initial
// position (x, y, and r floating point vals).  We read this file into the 
// initial position data structure, as well as a list of id to hostname
// mappings.

#include "config.h"

#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <paths.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>

#include "log.h"
#include "tbdefs.h"
#include "event.h"
#include <elvin/elvin.h>

#include "robot_list.h"
#include "mtp.h"

#include "emcd.h"

static event_handle_t handle;

static char *pideid;

static char *progname;

static int debug;

static int port = EMC_SERVER_PORT;
static int command_seq_no = 0;

/* initial data pulled from the config file */
/* it's somewhat non-intuitive to use lists here -- since we're doing maps,
 * we could just as well use hashtables -- but speed is not a big deal here,
 * and linked lists might be easier for everybody to understand
 */
/* the robot_id/hostname pairings as given in the config file */
struct robot_list *hostname_list = NULL;
/* the robot_id/initial positions as given in the config file */
struct robot_list *initial_position_list = NULL;
/* a list of positions read in from the movement file */
struct robot_list *position_queue = NULL;

static elvin_error_t elvin_error;

static struct rmc_client rmc_data = { -1 };
static struct vmc_client vmc_data = { -1 };
static int emulab_sock = -1;

static void ev_callback(event_handle_t handle,
			event_notification_t notification,
			void *data);

static int acceptor_callback(elvin_io_handler_t handler,
			     int fd,
			     void *rock,
			     elvin_error_t eerror);

static int unknown_client_callback(elvin_io_handler_t handler,
				   int fd,
				   void *rock,
				   elvin_error_t eerror);

static int rmc_callback(elvin_io_handler_t handler,
			int fd,
			void *rock,
			elvin_error_t eerror);

static int emulab_callback(elvin_io_handler_t handler,
			   int fd,
			   void *rock,
			   elvin_error_t eerror);

static int vmc_callback(elvin_io_handler_t handler,
			int fd,
			void *rock,
			elvin_error_t eerror);

static int update_callback(elvin_timeout_t timeout,
			   void *rock,
			   elvin_error_t eerror);

static void parse_config_file(char *filename);
static void parse_movement_file(char *filename);

#if defined(SIGINFO)
/* SIGINFO-related stuff */

/**
 * Variable used to tell the main loop that we received a SIGINFO.
 */
static int got_siginfo = 0;

/**
 * Handler for SIGINFO that sets the got_siginfo variable and breaks the main
 * loop so we can really handle the signal.
 *
 * @param sig The actual signal number received.
 */
static void siginfo(int sig)
{
  got_siginfo = 1;
#if 0
  if (handle->do_loop)
    event_stop_main(handle);
#endif
}

static void dump_info(void)
{
}
#endif

static void sigpanic(int sig)
{
  char buf[32];

  sprintf(buf, "sigpanic %d\n", sig);
  write(1, buf, strlen(buf));
  sleep(120);
  exit(1);
}

static void usage(char *progname)
{
  fprintf(stderr,
	  "Usage: emcd [-hd] [-c config] [-e pid/eid] [-k keyfile] [...]\n"
	  "Options:\n"
	  "  -h\t\tPrint this message.\n"
	  "  -d\t\tTurn on debugging messages and do not daemonize\n"
	  "  -e pid/eid\tConnect to the experiment event server\n"
	  "  -k keyfile\tKeyfile for the experiment\n"
	  "  -p port\tPort to listen on for mtp messages\n"
	  "  -c config\tThe config file to use\n"
	  "  -l logfile\tThe log file to use\n"
	  "  -i pidfile\tThe file to write the PID to\n");
}

int main(int argc, char *argv[])
{
  int c, on_off = 1, emc_sock, retval = EXIT_SUCCESS;
  char pid[MAXHOSTNAMELEN], eid[MAXHOSTNAMELEN];
  char *server = "localhost";
  char *movement_file = NULL;
  char *config_file = NULL;
  struct sockaddr_in sin;
  elvin_io_handler_t eih;
  address_tuple_t tuple;
  char *keyfile = NULL;
  char *logfile = NULL;
  char *pidfile = NULL;
  char *eport = NULL;
  char buf[BUFSIZ], buf2[BUFSIZ];
  char *idx;
  FILE *fp;
  
  progname = argv[0];
  
  while ((c = getopt(argc, argv, "hde:k:p:c:f:s:P:l:i:")) != -1) {
    switch (c) {
    case 'h':
      usage(progname);
      exit(0);
      break;
    case 'd':
      debug += 1;
      break;
    case 'e':
      pideid = optarg;
      if ((idx = strchr(pideid, '/')) == NULL) {
	fprintf(stderr,
		"error: malformed pid/eid argument - %s\n",
		pideid);
	usage(progname);
      }
      else if ((idx - pideid) >= sizeof(pid)) {
	fprintf(stderr,
		"error: pid is too long - %s\n",
		pideid);
	usage(progname);
      }
      else if (strlen(idx + 1) >= sizeof(eid)) {
	fprintf(stderr,
		"error: eid is too long - %s\n",
		pideid);
	usage(progname);
      }
      else {
	strncpy(pid, pideid, (idx - pideid));
	pid[idx - pideid] = '\0';
	strcpy(eid, idx + 1);
      }
      break;
    case 'k':
      keyfile = optarg;
      break;
    case 'p':
      if (sscanf(optarg, "%d", &port) != 1) {
	fprintf(stderr, "error: illegible port: %s\n", optarg);
	exit(1);
      }
      break;
    case 'c':
      config_file = optarg;
      break;
    case 'f':
      movement_file = optarg;
      break;
    case 's':
      server = optarg;
      break;
    case 'P':
      eport = optarg;
      break;
    case 'l':
      logfile = optarg;
      break;
    case 'i':
      pidfile = optarg;
      break;
    default:
      break;
    }
  }
  
  argc -= optind;
  argv += optind;
  
  // initialize the global lists
  hostname_list = robot_list_create();
  initial_position_list = robot_list_create();
  position_queue = robot_list_create();
  
  /* read config file into the above lists*/
  parse_config_file(config_file);
  /* read movement file into the position queue */
  parse_movement_file(movement_file);

  if (debug) 
    loginit(0, logfile);
  else {
    /* Become a daemon */
    daemon(0, 0);
    
    if (logfile)
      loginit(0, logfile);
    else
      loginit(1, "emcd");
  }

  signal(SIGSEGV, sigpanic);
  signal(SIGBUS, sigpanic);
  
  if (pidfile)
    strcpy(buf, pidfile);
  else
    sprintf(buf, "%s/progagent.pid", _PATH_VARRUN);
  fp = fopen(buf, "w");
  if (fp != NULL) {
    fprintf(fp, "%d\n", getpid());
    (void) fclose(fp);
  }
  
  // first, EMC server sock:
  emc_sock = socket(AF_INET,SOCK_STREAM,0);
  if (emc_sock == -1) {
    fprintf(stdout,"Could not open socket for EMC: %s\n",strerror(errno));
    exit(1);
  }

  memset(&sin, 0, sizeof(sin));
  sin.sin_len = sizeof(sin);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = INADDR_ANY;

  setsockopt(emc_sock, SOL_SOCKET, SO_REUSEADDR, &on_off, sizeof(on_off));
  
  if (bind(emc_sock, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
    perror("bind");
    exit(1);
  }
  
  if (listen(emc_sock, 5) == -1) {
    perror("listen");
    exit(1);
  }
  
#if defined(SIGINFO)
  signal(SIGINFO, siginfo);
#endif
  
  /*
   * Convert server/port to elvin thing.
   *
   * XXX This elvin string stuff should be moved down a layer. 
   */
  if (server) {
    snprintf(buf, sizeof(buf), "elvin://%s%s%s",
	     server,
	     (eport ? ":"  : ""),
	     (eport ? eport : ""));
    server = buf;
  }

  if (!keyfile && pideid) {
    snprintf(buf2, sizeof(buf2), "/proj/%s/exp/%s/tbdata/eventkey", pid, eid);
    keyfile = buf2;
  }
  
  /*
   * Register with the event system. 
   */
  handle = event_register_withkeyfile(server, 0, keyfile);
  if (handle == NULL) {
    fatal("could not register with event system");
  }

  if (pideid != NULL) {
    /*
     * Construct an address tuple for subscribing to events for this node.
     */
    tuple = address_tuple_alloc();
    if (tuple == NULL) {
      fatal("could not allocate an address tuple");
    }
    
    /*
     * Ask for just the SETDEST event for NODEs we care about. 
     */
    tuple->expt      = pideid;
    tuple->objtype   = TBDB_OBJECTTYPE_NODE;
    tuple->eventtype = TBDB_EVENTTYPE_SETDEST;
    
    if (!event_subscribe(handle, ev_callback, tuple, NULL)) {
      fatal("could not subscribe to event");
    }
    
    if ((elvin_error = elvin_error_alloc()) == NULL) {
      fatal("could not allocate elvin error");
    }
  }
  
  if ((eih = elvin_sync_add_io_handler(NULL,
				       emc_sock,
				       ELVIN_READ_MASK,
				       acceptor_callback,
				       NULL,
				       elvin_error)) == NULL) {
    fatal("could not register I/O callback");
  }

  if (elvin_sync_add_timeout(NULL,
			     EMC_UPDATE_HZ,
			     update_callback,
			     NULL,
			     elvin_error) == NULL) {
    fatal("could not add timeout");
  }
  
  event_main(handle);
  
  return retval;
}

void parse_config_file(char *config_file) {
  char line[BUFSIZ];
  int line_no = 0;
  FILE *fp;

  if (config_file == NULL) {
    return;
  }

  fp = fopen(config_file,"r");
  if (fp == NULL) {
    fprintf(stderr,"Could not open config file!\n");
    exit(-1);
  }

  // read line by line
  while (fgets(line, sizeof(line), fp) != NULL) {
    int id;
    char *hostname, *vname;
    float init_x;
    float init_y;
    float init_theta;

    // parse the line
    // sorry, i ain't using regex.h to do this simple crap
    // lines look like '5 garcia1.flux.utah.edu 6.5 4.234582 0.38 garcia1'
    // the regex would be '\s*\S+\s+\S+\s+\S+\s+\S+\s+\S+'
    // so basically, 5 non-whitespace groups separated by whitespace (tabs or
    // spaces)

    // we use strtok_r because i'm complete
    char *token = NULL;
    char *delim = " \t\n";
    char *state = NULL;
    struct emc_robot_config *rc;
    struct position *p;
    struct hostent *he;
    
    ++line_no;

    token = strtok_r(line,delim,&state);
    if (token == NULL) {
      fprintf(stdout,"Syntax error in config file, line %d.\n",line_no);
      continue;
    }
    id = atoi(token);

    token = strtok_r(NULL,delim,&state);
    if (token == NULL) {
      fprintf(stdout,"Syntax error in config file, line %d.\n",line_no);
      continue;
    }
    hostname = strdup(token);

    token = strtok_r(NULL,delim,&state);
    if (token == NULL) {
      fprintf(stdout,"Syntax error in config file, line %d.\n",line_no);
      continue;
    }
    init_x = (float)atof(token);
	  
    token = strtok_r(NULL,delim,&state);
    if (token == NULL) {
      fprintf(stdout,"Syntax error in config file, line %d.\n",line_no);
      continue;
    }
    init_y = (float)atof(token);

    token = strtok_r(NULL,delim,&state);
    if (token == NULL) {
      fprintf(stdout,"Syntax error in config file, line %d.\n",line_no);
      continue;
    }
    init_theta = (float)atof(token);

    token = strtok_r(NULL,delim,&state);
    if (token == NULL) {
      fprintf(stdout,"Syntax error in config file, line %d.\n",line_no);
      continue;
    }
    vname = strdup(token);
	
    // now we save this data to the lists:
    rc = (struct emc_robot_config *)malloc(sizeof(struct emc_robot_config));
    rc->id = id;
    rc->hostname = hostname;
    if ((he = gethostbyname(hostname)) == NULL) {
      fprintf(stderr, "error: unknown robot: %s\n", hostname);
      free(hostname);
      free(rc);
      continue;
    }
    memcpy(&rc->ia, he->h_addr, he->h_length);
    rc->vname = vname;
    
    robot_list_append(hostname_list,id,(void*)rc);
    p = (struct position *)malloc(sizeof(struct position *));
    p->x = init_x;
    p->y = init_y;
    p->theta = init_theta;
    robot_list_append(initial_position_list,id,(void*)p);

    // next line!
  }

}

void parse_movement_file(char *movement_file) {
  char line[BUFSIZ];
  int line_no = 0;
  FILE *fp;

  if (movement_file == NULL) {
    return;
  }

  fp = fopen(movement_file,"r");
  if (fp == NULL) {
    fprintf(stderr,"Could not open movement file!\n");
    exit(-1);
  }

  // read line by line
  while (fgets(line, sizeof(line), fp) != NULL) {
    int id;
    float init_x;
    float init_y;
    float init_theta;

    // parse the line
    // sorry, i ain't using regex.h to do this simple crap
    // lines look like '5  6.5 4.234582 0.38'
    // the regex would be '\s*\S+\s+\S+\s+\S+\s+\S+'
    // so basically, 4 non-whitespace groups separated by whitespace (tabs or
    // spaces)

    // we use strtok_r because i'm complete
    char *token = NULL;
    char *delim = " \t";
    char *state = NULL;
    struct position *p;
    
    ++line_no;

    token = strtok_r(line,delim,&state);
    if (token == NULL) {
      fprintf(stdout,"Syntax error in movement file, line %d.\n",line_no);
      continue;
    }
    id = atoi(token);

    token = strtok_r(NULL,delim,&state);
    if (token == NULL) {
      fprintf(stdout,"Syntax error in movement file, line %d.\n",line_no);
      continue;
    }
    init_x = (float)atof(token);
	  
    token = strtok_r(NULL,delim,&state);
    if (token == NULL) {
      fprintf(stdout,"Syntax error in movement file, line %d.\n",line_no);
      continue;
    }
    init_y = (float)atof(token);

    token = strtok_r(NULL,delim,&state);
    if (token == NULL) {
      fprintf(stdout,"Syntax error in movement file, line %d.\n",line_no);
      continue;
    }
    init_theta = (float)atof(token);


    // now we save this data to the lists:
    p = (struct position *)malloc(sizeof(struct position *));
    p->x = init_x;
    p->y = init_y;
    p->theta = init_theta;
    robot_list_append(position_queue,id,(void*)p);

    // next line!
  }

}

void ev_callback(event_handle_t handle,
		 event_notification_t notification,
		 void *data)
{
  char host[TBDB_FLEN_IP];
  struct in_addr ia;
  
  event_notification_get_host(handle, notification, host, sizeof(host));
  
  if (!inet_aton(host, &ia)) {
    error("event's host value is not an IP address: %s\n", host);
  }
  else {
    struct emc_robot_config *match = NULL;
    robot_list_item_t *rli;
    
    for (rli = hostname_list->head; rli != NULL && !match; rli = rli->next) {
      struct emc_robot_config *erc = rli->data;
      
      if (erc->ia.s_addr == ia.s_addr) {
        match = erc;
        break;
      }
    }
    
    if (match == NULL) {
      error("no match for IP: %s\n", host);
    }
    else {
      char *value, args[BUFSIZ];
      float x, y, orientation;
      
      event_notification_get_arguments(handle,
                                       notification, args, sizeof(args));
      
      if (event_arg_get(args, "X", &value) > 0) {
        if (sscanf(value, "%f", &x) != 1) {
          error("X argument in event is not a float: %s\n", value);
        }
      }
      
      if (event_arg_get(args, "Y", &value) > 0) {
        if (sscanf(value, "%f", &y) != 1) {
          error("Y argument in event is not a float: %s\n", value);
        }
      }
      
      if (event_arg_get(args, "ORIENTATION", &value) > 0) {
        if (sscanf(value, "%f", &orientation) != 1) {
          error("ORIENTATION argument in event is not a float: %s\n", value);
        }
      }
      
      // XXX What to do with the data?

      // construct a COMMAND_GOTO packet and send to rmc.

    }
  }
}

int acceptor_callback(elvin_io_handler_t handler,
		      int fd,
		      void *rock,
		      elvin_error_t eerror)
{
  // we have a new client -- but who?
  int addrlen = sizeof(struct sockaddr_in);
  struct sockaddr_in client_sin;
  int client_sock;

  if ((client_sock = accept(fd,
			    (struct sockaddr *)(&client_sin),
			    &addrlen)) == -1) {
    perror("accept");
  }
  else if (elvin_sync_add_io_handler(NULL,
				     client_sock,
				     ELVIN_READ_MASK,
				     unknown_client_callback,
				     NULL,
				     eerror) == NULL) {
    error("could not add handler for %d\n", client_sock);
    
    close(client_sock);
    client_sock = -1;
  }
  
  return 1;
}

int unknown_client_callback(elvin_io_handler_t handler,
			    int fd,
			    void *rock,
			    elvin_error_t eerror)
{
  mtp_packet_t *mp = NULL;
  int rc, retval = 0;

  elvin_sync_remove_io_handler(handler, eerror);
  
  if (((rc = mtp_receive_packet(fd, &mp)) != MTP_PP_SUCCESS) ||
      (mp->version != MTP_VERSION) ||
      (mp->opcode != MTP_CONTROL_INIT)) {
    error("invalid client %p\n", mp);
  }
  else {
    switch (mp->role) {
    case MTP_ROLE_RMC:
      if (rmc_data.sock_fd != -1) {
	error("rmc client is already connected\n");
      }
      else {
	// write back an RMC_CONFIG packet
	struct mtp_config_rmc r;
	
	r.box.horizontal = 12*12*2.54/100.0;
	r.box.vertical = 8*12*2.54/100.0;
	
	r.num_robots = hostname_list->item_count;
	r.robots = (struct robot_config *)
	  malloc(sizeof(struct robot_config)*(r.num_robots));
	if (r.robots == NULL) {
	  struct mtp_packet *wb;
	  struct mtp_control c;
	  
	  c.id = -1;
	  c.code = -1;
	  c.msg = "internal server error";
	  wb = mtp_make_packet(MTP_CONTROL_ERROR, MTP_ROLE_EMC, &c);
	  mtp_send_packet(fd, wb);
	  
	  free(wb);
	  wb = NULL;
	}
	else {
	  struct robot_config *rc = NULL;
	  struct robot_list_enum *e;
	  struct mtp_packet *wb;
	  int i = 0;

	  e = robot_list_enum(hostname_list);
	  while ((rc = (struct robot_config *)
		  robot_list_enum_next_element(e)) != NULL) {
	    r.robots[i] = *rc;
	    i += 1;
	  }
	  robot_list_enum_destroy(e);
	  
	  // write back an rmc_config packet
	  if ((wb = mtp_make_packet(MTP_CONFIG_RMC,
				    MTP_ROLE_EMC,
				    &r)) == NULL) {
	    error("unable to allocate rmc_config packet");
	  }
	  else if ((retval= mtp_send_packet(fd, wb)) != MTP_PP_SUCCESS) {
	    error("unable to send rmc_config packet");
	  }
	  else if (elvin_sync_add_io_handler(NULL,
					     fd,
					     ELVIN_READ_MASK,
					     rmc_callback,
					     &rmc_data,
					     eerror) == NULL) {
	    error("unable to add rmc_callback handler");
	  }
	  else {
	    // add descriptor to list, etc:
	    rmc_data.sock_fd = fd;
	    if (rmc_data.position_list == NULL)
	      rmc_data.position_list = robot_list_create();

	    retval = 1;
	  }

	  free(wb);
	  wb = NULL;
	}
      }
      break;
    case MTP_ROLE_EMULAB:
      if (emulab_sock != -1) {
	error("emulab client is already connected\n");
      }
      else if (elvin_sync_add_io_handler(NULL,
					 fd,
					 ELVIN_READ_MASK,
					 emulab_callback,
					 NULL,
					 eerror) == NULL) {
	error("unable to add elvin io handler");
      }
      else {
	emulab_sock = fd;
	
	retval = 1;
      }
      break;
    case MTP_ROLE_VMC:
      if (vmc_data.sock_fd != -1) {
	error("vmc client is already connected\n");
      }
      else {
	// write back an RMC_CONFIG packet
	struct mtp_config_vmc r;
	
	r.num_robots = hostname_list->item_count;
	r.robots = (struct robot_config *)
	  malloc(sizeof(struct robot_config)*(r.num_robots));
	if (r.robots == NULL) {
	  struct mtp_packet *wb;
	  struct mtp_control c;
	  
	  c.id = -1;
	  c.code = -1;
	  c.msg = "internal server error";
	  wb = mtp_make_packet(MTP_CONTROL_ERROR, MTP_ROLE_EMC, &c);
	  mtp_send_packet(fd, wb);
	  
	  free(wb);
	  wb = NULL;
	}
	else {
	  struct robot_config *rc = NULL;
	  struct robot_list_enum *e;
	  struct mtp_packet *wb;
	  int i = 0;

	  e = robot_list_enum(hostname_list);
	  while ((rc = (struct robot_config *)
		  robot_list_enum_next_element(e)) != NULL) {
	    r.robots[i] = *rc;
	    i += 1;
	  }
	  robot_list_enum_destroy(e);
	  
	  // write back an rmc_config packet
	  if ((wb = mtp_make_packet(MTP_CONFIG_VMC,
				    MTP_ROLE_EMC,
				    &r)) == NULL) {
	    error("unable to allocate rmc_config packet");
	  }
	  else if ((retval= mtp_send_packet(fd, wb)) != MTP_PP_SUCCESS) {
	    error("unable to send rmc_config packet");
	  }
	  else if (elvin_sync_add_io_handler(NULL,
					     fd,
					     ELVIN_READ_MASK,
					     vmc_callback,
					     &vmc_data,
					     eerror) == NULL) {
	    error("unable to add vmc_callback handler");
	  }
	  else {
	    // add descriptor to list, etc:
	    vmc_data.sock_fd = fd;
	    vmc_data.position_list = robot_list_create();

	    retval = 1;
	  }

	  free(wb);
	  wb = NULL;
	}
      }
      break;
    default:
      error("unknown role %d\n", mp->role);
      break;
    }
  }

  mtp_free_packet(mp);
  mp = NULL;
  
  if (!retval) {
    info("dropping bad connection %d\n", fd);
    
    close(fd);
    fd = -1;
  }
  
  return retval;
}

int rmc_callback(elvin_io_handler_t handler,
		 int fd,
		 void *rock,
		 elvin_error_t eerror)
{
  struct rmc_client *rmc = rock;
  mtp_packet_t *mp = NULL;
  int rc, retval = 0;

  info("rmc packet\n");
  
  if (((rc = mtp_receive_packet(fd, &mp)) != MTP_PP_SUCCESS) ||
      (mp->version != MTP_VERSION)) {
    error("invalid client %p\n", mp);
  }
  else {
    switch (mp->opcode) {
    case MTP_REQUEST_POSITION:
      {
	// find the latest position data for the robot:
	//   supply init pos if no positions in rmc_data or vmc_data;
	//   otherwise, take the position with the most recent timestamp
	//     from rmc_data or vmc_data
	int my_id = mp->data.request_position->robot_id;
	struct mtp_update_position *vmc_up;
	struct mtp_update_position *rmc_up;
	struct mtp_update_position up_copy;
	struct mtp_packet *wb;
	
	info("request pos\n");
	
	vmc_up = (struct mtp_update_position *)
	  robot_list_search(vmc_data.position_list, my_id);
	rmc_up = (struct mtp_update_position *)
	  robot_list_search(rmc_data.position_list, my_id);

	if (vmc_up != NULL) {
	  rmc_up = vmc_up; // XXX prefer vmc?
	}
	
	if (rmc_up != NULL) {
	  // since VMC isn't hooked in, we simply write back the rmc posit
	  up_copy = *rmc_up;
	  // the status field has no meaning when this packet is being sent
	  up_copy.status = -1;
	  // construct the packet
	  wb = mtp_make_packet(MTP_UPDATE_POSITION, MTP_ROLE_EMC, &up_copy);
	  mtp_send_packet(fd, wb);
	}
	else  {
	  struct mtp_control mc;

	  info("no updates for %d\n", my_id);

	  mc.id = -1;
	  mc.code = -1;
	  mc.msg = "position not updated yet";
	  wb = mtp_make_packet(MTP_CONTROL_ERROR, MTP_ROLE_EMC, &mc);
	  mtp_send_packet(fd, wb);
	}
	
	free(wb);
	wb = NULL;
	
	retval = 1;
      }
      break;

    case MTP_UPDATE_POSITION:
      {
	// store the latest data in teh robot position/status list
	// in rmc_data
	int my_id = mp->data.update_position->robot_id;
	struct mtp_update_position *up = (struct mtp_update_position *)
	  robot_list_remove_by_id(rmc->position_list, my_id);
	struct mtp_update_position *up_copy;

	free(up);
	up = NULL;

	up_copy = (struct mtp_update_position *)
	  malloc(sizeof(struct mtp_update_position));
	*up_copy = *(mp->data.update_position);
	robot_list_append(rmc->position_list, my_id, up_copy);

	info("update pos\n");
	
	switch (mp->data.update_position->status) {
	case MTP_POSITION_STATUS_ERROR:
	case MTP_POSITION_STATUS_COMPLETE:
	  if (emulab_sock != -1) {
	    info("complete!\n");
	    mtp_send_packet(emulab_sock, mp);
	  }
	  break;
	}
	
	// also, if status is MTP_POSITION_STATUS_COMPLETE || 
	// MTP_POSITION_STATUS_ERROR, notify emulab, or issue the next
	// command to rmc from the movement list.

	// later ....

	retval = 1;
      }
      break;
      
    default:
      {
	struct mtp_packet *wb;
	struct mtp_control c;
	
	error("received bogus opcode from RMC: %d\n", mp->opcode);
	
	c.id = -1;
	c.code = -1;
	c.msg = "protocol error: bad opcode";
	wb = mtp_make_packet(MTP_CONTROL_ERROR, MTP_ROLE_EMC, &c);
	mtp_send_packet(rmc->sock_fd,wb);

	free(wb);
	wb = NULL;
      }
      break;
    }
  }

  mtp_free_packet(mp);
  mp = NULL;

  if (!retval) {
    info("dropping rmc connection %d\n", fd);

    elvin_sync_remove_io_handler(handler, eerror);

    rmc->sock_fd = -1;
    
    close(fd);
    fd = -1;
  }
  
  return retval;
}

int emulab_callback(elvin_io_handler_t handler,
		    int fd,
		    void *rock,
		    elvin_error_t eerror)
{
  mtp_packet_t *mp = NULL;
  int rc, retval = 0;
  
  if (((rc = mtp_receive_packet(fd, &mp)) != MTP_PP_SUCCESS) ||
      (mp->version != MTP_VERSION)) {
    error("invalid client %p\n", mp);
  }
  else {
    switch (mp->opcode) {
    case MTP_COMMAND_GOTO:
      if (rmc_data.sock_fd == -1) {
	error("no rmcd yet\n");
      }
      else if (mtp_send_packet(rmc_data.sock_fd, mp) != MTP_PP_SUCCESS) {
	error("could not forward packet to rmcd\n");
      }
      else {
	info("forwarded goto\n");
	
	retval = 1;
      }
      break;
    case MTP_REQUEST_POSITION:
      {
	// find the latest position data for the robot:
	//   supply init pos if no positions in rmc_data or vmc_data;
	//   otherwise, take the position with the most recent timestamp
	//     from rmc_data or vmc_data
	int my_id = mp->data.request_position->robot_id;
	struct mtp_update_position *vmc_up;
	struct mtp_update_position up_copy;
	struct mtp_packet *wb;
	
	info("request pos\n");
	
	vmc_up = (struct mtp_update_position *)
	  robot_list_search(vmc_data.position_list, my_id);

	if (vmc_up != NULL) {
	  // since VMC isn't hooked in, we simply write back the rmc posit
	  up_copy = *vmc_up;
	  // the status field has no meaning when this packet is being sent
	  up_copy.status = -1;
	  // construct the packet
	  wb = mtp_make_packet(MTP_UPDATE_POSITION, MTP_ROLE_EMC, &up_copy);
	  mtp_send_packet(fd, wb);
	}
	else  {
	  struct mtp_control mc;

	  info("no updates for %d\n", my_id);

	  mc.id = -1;
	  mc.code = -1;
	  mc.msg = "position not updated yet";
	  wb = mtp_make_packet(MTP_CONTROL_ERROR, MTP_ROLE_EMC, &mc);
	  mtp_send_packet(fd, wb);
	}
	
	free(wb);
	wb = NULL;
	
	retval = 1;
      }
      break;
    default:
      {
	struct mtp_packet *wb;
	struct mtp_control c;
	
	error("received bogus opcode from VMC: %d\n", mp->opcode);
	
	c.id = -1;
	c.code = -1;
	c.msg = "protocol error: bad opcode";
	wb = mtp_make_packet(MTP_CONTROL_ERROR, MTP_ROLE_EMC, &c);
	mtp_send_packet(fd, wb);

	free(wb);
	wb = NULL;
      }
      break;
    }
  }
  
  mtp_free_packet(mp);
  mp = NULL;

  if (!retval) {
    info("dropping emulab connection %d\n", fd);

    elvin_sync_remove_io_handler(handler, eerror);

    emulab_sock = -1;
    
    close(fd);
    fd = -1;
  }
  
  return retval;
}

/* we allow a track to be continued if there is a match within 5 cm */
#define DIST_THRESHOLD 0.05
/* and if pose difference is less than 45 degress */
#define POSE_THRESHOLD 0.785

int vmc_callback(elvin_io_handler_t handler,
		 int fd,
		 void *rock,
		 elvin_error_t eerror)
{
  struct vmc_client *vmc = rock;
  mtp_packet_t *mp = NULL;
  int rc, retval = 0;
  
  if (((rc = mtp_receive_packet(fd, &mp)) != MTP_PP_SUCCESS) ||
      (mp->version != MTP_VERSION)) {
    error("invalid client %p\n", mp);
  }
  else {
    switch (mp->opcode) {
    case MTP_UPDATE_POSITION:
      {
        // store the latest data in teh robot position/status list
        // in vmc_data
        int my_id = mp->data.update_position->robot_id;
        struct mtp_update_position *up = (struct mtp_update_position *)
          robot_list_remove_by_id(vmc->position_list, my_id);
        struct mtp_update_position *up_copy;
        
        free(up);
        up = NULL;
        
        up_copy = (struct mtp_update_position *)
          malloc(sizeof(struct mtp_update_position));
        *up_copy = *(mp->data.update_position);
        robot_list_append(vmc->position_list, my_id, up_copy);
        
        info("update for %d %p\n",
             my_id,
             robot_list_search(vmc_data.position_list, my_id));
        
        // also, if status is MTP_POSITION_STATUS_COMPLETE || 
        // MTP_POSITION_STATUS_ERROR, notify emulab, or issue the next
        // command to vmc from the movement list.
        
        // later ....
        
        retval = 1;
      }
      break;
    case MTP_REQUEST_ID:
      {
        struct mtp_request_id *rid = (struct mtp_request_id *)
          mp->data.request_id;
        // we need to search all the positions in the rmc_data.position_list
        // and find the closest match.  if this match is within the threshold,
        // we return it immediately.  if not, we search the initial_position
        // list and return the closest match, whatever it may be...
        // later we will want to change this to give an operator the chance 
        // to associate ids with unknown positions themselves, if we can't
        // do it automatically.

        double best_dist_delta = DIST_THRESHOLD;
        double best_pose_delta = POSE_THRESHOLD;
        int robot_id = -1;

        double dx,dy,dtheta,ddist;

        struct mtp_update_id uid;
	struct mtp_packet *ump;

	int my_retval;

        int id = -1;
        struct position *p;
	struct robot_list_item *i;

	mtp_print_packet(stdout, mp);
	fflush(stdout);
	
	if (rmc_data.position_list == NULL) {
	  info("no robot positions yet\n");
	}
	else {
	  i = rmc_data.position_list->head;
	  while (i != NULL) {
	    p = (struct position *)(i->data);
	    id = i->id;
	    
	    dx = rid->position.x - p->x;
	    dy = rid->position.y - p->y;
	    ddist = sqrt(dx*dx + dy*dy);
	    dtheta = rid->position.theta - p->theta;
	    
	    if (fabsf(ddist) > best_dist_delta) {
	      i = i->next;
	      continue;
	    }
	    
	    if (fabsf(dtheta) > best_pose_delta) {
	      i = i->next;
	      continue;
	    }
	    
	    best_dist_delta = ddist;
	    best_pose_delta = dtheta;
	    robot_id = id;
	    
	    i = i->next;
	  }
	}

        // we really ought to not search teh initial posit list AFTER vmc has
        // initialized completely (that is, associated IDs for all objects
        // it found).  I'll hack this in later if there's time.

        if (robot_id == -1) {
          // need to search the initial posit list -- only this time, we
          // take the closest match without regard for threshold and pose
          // -- just distance!
          id = -1;
          p = NULL;
          i = initial_position_list->head;
          best_dist_delta = 10000000000.0;
          best_pose_delta = 100000000000.0;

          while (i != NULL) {
            p = (struct position *)(i->data);
            id = i->id;

            dx = rid->position.x - p->x;
            dy = rid->position.y - p->y;
	    if (dx == 0 && dy == 0)
		ddist = 0;
	    else
		ddist = sqrt(dx*dx + dy*dy);
            dtheta = rid->position.theta - p->theta;

	    info("dx %f %f %f %f\n", dx, dy, ddist, dtheta);
            if (fabsf(ddist) > best_dist_delta) {
		info("pass ? %f %f\n", ddist, best_dist_delta);
	      i = i->next;
              continue;
            }

	    if (0) {
		if (fabsf(dtheta) > best_pose_delta) {
		    i = i->next;
		    continue;
		}
	    }

	    info("match %d\n", id);
            best_dist_delta = ddist;
            best_pose_delta = dtheta;
            robot_id = id;
	    
	    i = i->next;
          }

	  info("just using initial pos %f %f\n", dx, dy);
          // now send whatever we got to the caller as we fall through:
        }
        
	info("match %f %f -> %d\n",
	     rid->position.x, rid->position.y, robot_id);
        // we want to return whichever robot id immediately to the caller:
        
        uid.request_id = rid->request_id;
        uid.robot_id = robot_id;
        
        ump = mtp_make_packet(MTP_UPDATE_ID,MTP_ROLE_EMC,&uid);
        my_retval = mtp_send_packet(fd, ump);
        mtp_free_packet(ump);
        if (my_retval != MTP_PP_SUCCESS) {
          fprintf(stdout,"Could not send update_id packet!\n");
        }

	retval = 1;
      }
      break;
    default:
      {
        struct mtp_packet *wb;
        struct mtp_control c;
        
        error("received bogus opcode from VMC: %d\n", mp->opcode);
        
        c.id = -1;
        c.code = -1;
        c.msg = "protocol error: bad opcode";
        wb = mtp_make_packet(MTP_CONTROL_ERROR, MTP_ROLE_EMC, &c);
        mtp_send_packet(vmc->sock_fd,wb);
        
        free(wb);
        wb = NULL;
      }
      break;
    }
  }
  
  mtp_free_packet(mp);
  mp = NULL;
  
  if (!retval) {
    info("dropping vmc connection %d\n", fd);
    
    elvin_sync_remove_io_handler(handler, eerror);
    
    vmc->sock_fd = -1;
    
    close(fd);
    fd = -1;
  }
  
  return retval;
}

int update_callback(elvin_timeout_t timeout, void *rock, elvin_error_t eerror)
{
  struct mtp_update_position *mup;
  struct robot_list_enum *e;
  int retval = 1;

  if (elvin_sync_add_timeout(timeout,
			     EMC_UPDATE_HZ,
			     update_callback,
			     rock,
			     eerror) == NULL) {
    error("could not re-add update callback\n");
  }

  e = robot_list_enum(vmc_data.position_list);
  while ((mup = (struct mtp_update_position *)
	  robot_list_enum_next_element(e)) != NULL) {
    struct emc_robot_config *erc;

    erc = robot_list_search(hostname_list, mup->robot_id);
    printf("update %s %f %f\n", erc->vname, mup->position.x, mup->position.y);
    event_do(handle,
	     EA_Experiment, pideid,
	     EA_Type, TBDB_OBJECTTYPE_NODE,
	     EA_Event, TBDB_EVENTTYPE_MODIFY,
	     EA_Name, erc->vname,
	     EA_ArgFloat, "X", mup->position.x,
	     EA_ArgFloat, "Y", mup->position.y,
	     EA_ArgFloat, "ORIENTATION", mup->position.theta,
	     EA_TAG_DONE);
  }
  robot_list_enum_destroy(e);
  
  return retval;
}
