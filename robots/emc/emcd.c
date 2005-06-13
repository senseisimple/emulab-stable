/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
#include <assert.h>
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

#ifndef __XSTRING
#define __XSTRING(x) __STRING(x)
#endif

#define USE_POSTURE_REG

static event_handle_t handle;

static char *pideid;

static char *progname;

static int debug;

static int port = EMC_SERVER_PORT;

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

static struct mtp_packet config_rmc;
static struct mtp_packet config_vmc;

static char *topography_name;

/*
 * these are for some global bounds checking on user-requested
 * positions
 */
static struct camera_config *g_camera_config;
int g_camera_config_size = 0;
static struct obstacle_config *g_obstacle_config;
int g_obstacle_config_size = 0;


static struct rmc_client rmc_data;
static struct vmc_client vmc_data;
static mtp_handle_t emulab_handle = NULL;


/*
 * returns 1 if x,y are inside the union of the boxes defined
 * in the camera_config struct; 0 if not.
 */
static int position_in_camera(struct camera_config *cc,
			      int cc_size,
			      float x,
			      float y
			      );

/*
 * returns 0 if the position is not in a known obstacle; 1 if it is.
 */
static int position_in_obstacle(struct obstacle_config *oc,
				int oc_size,
				float x,
				float y
				);

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

static char *unix_path = NULL;

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
  if (handle->do_loop)
    event_stop_main(handle);
}

static void dump_info(void)
{
  if (vmc_data.position_list == NULL) {
    info("info: vision is not connected\n");
  }
  else {
    struct mtp_update_position *mup;
    struct robot_list_enum *e;

    info("info: vision position list\n");

    e = robot_list_enum(vmc_data.position_list);
    while ((mup = (struct mtp_update_position *)
	    robot_list_enum_next_element(e)) != NULL) {
      struct emc_robot_config *erc;

      erc = robot_list_search(hostname_list, mup->robot_id);
      info("  %s: %.2f %.2f %.2f\n",
	   erc->hostname,
	   mup->position.x,
	   mup->position.y,
	   mup->position.theta);
    }
    robot_list_enum_destroy(e);
  }
}
#endif

static void emcd_atexit(void)
{
  if (unix_path != NULL)
    unlink(unix_path);
}

static void sigquit(int sig)
{
  exit(0);
}

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
	  "  -U path\tUnix socket path\n"
	  "  -c config\tThe config file to use\n"
	  "  -l logfile\tThe log file to use\n"
	  "  -i pidfile\tThe file to write the PID to\n");
}

int main(int argc, char *argv[])
{
  char pid[MAXHOSTNAMELEN], eid[MAXHOSTNAMELEN];
  int c, emc_sock, retval = EXIT_SUCCESS;
  char *server = "localhost";
  char *movement_file = NULL;
  char *config_file = NULL;
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

  while ((c = getopt(argc, argv, "hde:k:p:c:f:s:P:U:l:i:")) != -1) {
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
    case 'U':
      unix_path = optarg;
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

  if (!config_file) {
    fprintf(stderr, "error: no config file specified\n");
    exit(1);
  }

  argc -= optind;
  argv += optind;

  // initialize the global lists
  hostname_list = robot_list_create();
  initial_position_list = robot_list_create();
  position_queue = robot_list_create();

  g_camera_config = NULL;
  g_obstacle_config = NULL;

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

  debug = 1;

  atexit(emcd_atexit);

  signal(SIGINT, sigquit);
  signal(SIGTERM, sigquit);
  signal(SIGQUIT, sigquit);

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
  emc_sock = mtp_bind("0.0.0.0", port, unix_path);
  if (emc_sock == -1) {
    fprintf(stdout,"Could not open socket for EMC: %s\n",strerror(errno));
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

  }

  if ((elvin_error = elvin_error_alloc()) == NULL) {
    fatal("could not allocate elvin error");
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

  while (1) {
    event_main(handle);

#if defined(SIGINFO)
    if (got_siginfo) {
      dump_info();
      got_siginfo = 0;
    }
#endif
  }

  return retval;
}

static int position_in_camera(struct camera_config *cc,
                              int cc_size,
                              float x,
                              float y
                              ) {
  int i;
  int retval = 0;

  for (i = 0; i < cc_size; ++i) {
    if (x >= cc[i].x && y >= cc[i].y &&
	x <= (cc[i].x + cc[i].width) && y <= (cc[i].y + cc[i].height)) {
      retval = 1;
      break;
    }
  }

  return retval;
}

static int position_in_obstacle(struct obstacle_config *oc,
                                int oc_size,
                                float x,
                                float y
                                ) {
  int i;
  int retval = 0;

  for (i = 0; i < oc_size; ++i) {
    if (x >= (oc[i].xmin - OBSTACLE_BUFFER) &&
	y >= (oc[i].ymin - OBSTACLE_BUFFER) &&
	x <= (oc[i].xmax + OBSTACLE_BUFFER) &&
	y <= (oc[i].ymax + OBSTACLE_BUFFER)) {
      retval = 1;
      break;
    }
  }

  return retval;
}

int have_camera_config(char *hostname, int port,
		       struct camera_config *cc, int cc_size)
{
  int lpc, retval = 0;

  for (lpc = 0; lpc < cc_size; lpc++) {
    if (strcmp(hostname, cc[lpc].hostname) == 0 && port == cc[lpc].port) {
      return 1;
    }
  }

  return retval;
}

void parse_config_file(char *config_file) {
  int oc_size = 0, cc_size = 0, line_no = 0;
  struct obstacle_config *oc = NULL;
  struct camera_config *cc = NULL;
  char line[BUFSIZ];
  struct stat st;
  FILE *fp;

  if (config_file == NULL) {
    fprintf(stderr, "error: no config file given\n");
    exit(1);
  }

  if (stat(config_file, &st) != 0) {
    fprintf(stderr, "error: cannot stat config file\n");
    exit(1);
  }
  else if (!S_ISREG(st.st_mode)) {
    fprintf(stderr, "error: config file is not a regular file\n");
    exit(1);
  }

  fp = fopen(config_file,"r");
  if (fp == NULL) {
    fprintf(stderr,"Could not open config file!\n");
    exit(-1);
  }

  // read line by line
  while (fgets(line, sizeof(line), fp) != NULL) {
    char directive[16] = "";

    // parse the line
    // sorry, i ain't using regex.h to do this simple crap
    // lines look like '5 garcia1.flux.utah.edu 6.5 4.234582 0.38 garcia1'
    // the regex would be '\s*\S+\s+\S+\s+\S+\s+\S+\s+\S+'
    // so basically, 5 non-whitespace groups separated by whitespace (tabs or
    // spaces)

    sscanf(line, "%16s", directive);

    if (directive[0] == '#' || directive[0] == '\0') {
      // Comment or empty line.
    }
    else if (strcmp(directive, "topography") == 0) {
      char area[32];

      if (sscanf(line, "%16s %32s", directive, area) != 2) {
	fprintf(stderr,
		"error:%d: syntax error in config file - %s\n",
		line_no,
		line);
      }
      else {
	topography_name = strdup(area);
      }
    }
    else if (strcmp(directive, "robot") == 0) {
      char area[32], hostname[MAXHOSTNAMELEN], vname[TBDB_FLEN_EVOBJNAME];
      int id;
      float init_x;
      float init_y;
      float init_theta;

      if (sscanf(line,
		 "%16s %32s %d %" __XSTRING(MAXHOSTNAMELEN) "s %f %f %f "
		 "%" __XSTRING(TBDB_FLEN_EVOBJNAME) "s",
		 directive,
		 area,
		 &id,
		 hostname,
		 &init_x,
		 &init_y,
		 &init_theta,
		 vname) != 8) {
	fprintf(stderr,
		"error:%d: syntax error in config file - %s\n",
		line_no,
		line);
      }
      else {
	struct emc_robot_config *rc;
	struct hostent *he;

	// now we save this data to the lists:
	rc = (struct emc_robot_config *)
	  malloc(sizeof(struct emc_robot_config));
	rc->id = id;
	rc->hostname = strdup(hostname);
	if ((he = gethostbyname(hostname)) == NULL) {
	  fprintf(stderr, "error: unknown robot: %s\n", hostname);
	  free(hostname);
	  free(rc);
	  continue;
	}
	memcpy(&rc->ia, he->h_addr, he->h_length);
	rc->vname = strdup(vname);
	rc->token = ~0;
	rc->init_x = init_x;
	rc->init_y = init_y;
	rc->init_theta = init_theta;

	robot_list_append(hostname_list, id, (void *)rc);
      }
    }
    else if (strcmp(directive, "camera") == 0) {
      char area[32], hostname[MAXHOSTNAMELEN];
      float x, y, width, height, fixed_x, fixed_y;
      int port;

      if (sscanf(line,
		 "%16s %32s %"
		 __XSTRING(MAXHOSTNAMELEN)
		 "s %d %f %f %f %f %f %f",
		 directive,
		 area,
		 hostname,
		 &port,
		 &x,
		 &y,
		 &width,
		 &height,
		 &fixed_x,
		 &fixed_y) != 10) {
	fprintf(stderr,
		"error:%d: syntax error in config file - %s\n",
		line_no,
		line);
      }
      else if (have_camera_config(hostname, port, cc, cc_size)) {
      }
      else if ((cc = realloc(cc,
			     (cc_size + 1) *
			     sizeof(struct camera_config))) == 0) {
	fatal("cannot allocate memory\n");
      }
      else {
	cc[cc_size].hostname = strdup(hostname);
	cc[cc_size].port = port;
	cc[cc_size].x = x;
	cc[cc_size].y = y;
	cc[cc_size].width = width;
	cc[cc_size].height = height;
	cc[cc_size].fixed_x = fixed_x;
	cc[cc_size].fixed_y = fixed_y;
	cc_size += 1;
      }
    }
    else if (strcmp(directive, "obstacle") == 0) {
      float x1, y1, x2, y2;
      char area[32];
      int id;

      if (sscanf(line,
		 "%16s %32s %d %f %f %f %f",
		 directive,
		 area,
		 &id,
		 &x1,
		 &y1,
		 &x2,
		 &y2) != 7) {
	fprintf(stderr,
		"error:%d: syntax error in config file - %s\n",
		line_no,
		line);
      }
      else if ((oc = realloc(oc,
			     (oc_size + 1) *
			     sizeof(struct camera_config))) == 0) {
	fatal("cannot allocate memory\n");
      }
      else {
	oc[oc_size].id = id;
	oc[oc_size].xmin = x1;
	oc[oc_size].ymin = y1;
	oc[oc_size].xmax = x2;
	oc[oc_size].ymax = y2;
	oc_size += 1;
      }
    }
    else {
      fprintf(stderr,
	      "error:%d: unknown directive - %s\n",
	      line_no,
	      directive);
    }
    // next line!
  }

  {
    struct robot_config *robot_val, *rc;
    struct robot_list_enum *e;
    struct box *boxes_val;
    int boxes_len;
    int lpc = 0;

    if (hostname_list->item_count == 0) {
      fatal("no robots have been specified!");
    }

    robot_val = malloc(sizeof(robot_config) * hostname_list->item_count);
    e = robot_list_enum(hostname_list);
    while ((rc = (struct robot_config *)
	    robot_list_enum_next_element(e)) != NULL) {
      robot_val[lpc] = *rc;
      lpc += 1;
    }
    robot_list_enum_destroy(e);

    if (cc_size == 0) {
      fatal("no cameras have been specified!");
    }

    boxes_len = cc_size;
    boxes_val = (struct box *)malloc(sizeof(struct box) * boxes_len);
    for (lpc = 0; lpc < boxes_len; ++lpc) {
      boxes_val[lpc].x = cc[lpc].x;
      boxes_val[lpc].y = cc[lpc].y;
      boxes_val[lpc].width = cc[lpc].width;
      boxes_val[lpc].height = cc[lpc].height;
    }

    mtp_init_packet(&config_rmc,
		    MA_Opcode, MTP_CONFIG_RMC,
		    MA_Role, MTP_ROLE_EMC,
		    MA_RobotLen, hostname_list->item_count,
		    MA_RobotVal, robot_val,
		    MA_BoundsLen, boxes_len,
		    MA_BoundsVal, boxes_val,
		    MA_ObstacleLen, oc_size,
		    MA_ObstacleVal, oc,
		    MA_TAG_DONE);
    mtp_init_packet(&config_vmc,
		    MA_Opcode, MTP_CONFIG_VMC,
		    MA_Role, MTP_ROLE_EMC,
		    MA_RobotLen, hostname_list->item_count,
		    MA_RobotVal, robot_val,
		    MA_CameraLen, cc_size,
		    MA_CameraVal, cc,
		    MA_TAG_DONE);
    /* don't free cc or oc! */
    g_camera_config = cc;
    g_camera_config_size = cc_size;
    g_obstacle_config = oc;
    g_obstacle_config_size = oc_size;
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
    struct robot_position *p;

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
    p = (struct robot_position *)malloc(sizeof(struct robot_position *));
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
  struct emc_robot_config *match = NULL;
  char objname[TBDB_FLEN_EVOBJNAME];
  robot_list_item_t *rli;
  int in_obstacle, in_camera;

  if (! event_notification_get_objname(handle, notification,
				       objname, sizeof(objname))) {
    error("Could not get objname from notification!\n");
    return;
  }

  for (rli = hostname_list->head; rli != NULL && !match; rli = rli->next) {
    struct emc_robot_config *erc = rli->data;

    if (strcmp(erc->vname, objname) == 0) {
      match = erc;
      break;
    }
  }

  if (match == NULL) {
    error("no match for host\n");
  }
  else {
    float x, y, orientation = 0.0, speed = 0.2;
    char *value, args[BUFSIZ];
    struct mtp_packet mp;

    event_notification_get_arguments(handle, notification, args, sizeof(args));

    /* XXX copy the current X, Y, and orientation! */
    if (event_arg_get(args, "X", &value) > 0) {
      if (sscanf(value, "%f", &x) != 1) {
	error("X argument in event is not a float: %s\n", value);
	return;
      }
    }

    if (event_arg_get(args, "Y", &value) > 0) {
      if (sscanf(value, "%f", &y) != 1) {
	error("Y argument in event is not a float: %s\n", value);
	return;
      }
    }

    if (event_arg_get(args, "ORIENTATION", &value) > 0) {
      if (sscanf(value, "%f", &orientation) != 1) {
	error("ORIENTATION argument in event is not a float: %s\n", value);
	return;
      }
    }

    if (event_arg_get(args, "SPEED", &value) > 0) {
      if (sscanf(value, "%f", &speed) != 1) {
	error("SPEED argument in event is not a float: %s\n", value);
	return;
      }
    }

    event_notification_get_int32(handle, notification,
				 "TOKEN", (int32_t *)&match->token);

    orientation = orientation * M_PI / 180.0;

    if ((in_camera = position_in_camera(g_camera_config,g_camera_config_size,
					x,y)) &&
	!(in_obstacle = position_in_obstacle(g_obstacle_config,
					     g_obstacle_config_size,
					     x,y))
	) {


      mtp_init_packet(&mp,
		      MA_Opcode, MTP_COMMAND_GOTO,
		      MA_Role, MTP_ROLE_EMC,
		      MA_CommandID, 1,
		      MA_RobotID, match->id,
		      MA_X, x,
		      MA_Y, y,
		      MA_Theta, orientation,
		      MA_Speed, speed,
		      MA_TAG_DONE);

      match->flags |= ERF_HAS_GOAL;
      match->last_goal_pos = mp.data.mtp_payload_u.command_goto.position;

      if (rmc_data.handle != NULL) {
	mtp_send_packet(rmc_data.handle, &mp);
      }
#if 0
      else {
	mtp_print_packet(stdout, &mp);

#if defined(TBDB_EVENTTYPE_COMPLETE)
	event_do(handle,
		 EA_Experiment, pideid,
		 EA_Type, TBDB_OBJECTTYPE_NODE,
		 EA_Name, match->vname,
		 EA_Event, TBDB_EVENTTYPE_COMPLETE,
		 EA_ArgInteger, "ERROR", 0,
		 EA_ArgInteger, "CTOKEN", match->token,
		 EA_TAG_DONE);
#endif
      }
#endif

      mtp_free_packet(&mp);
    }
    else {
      int which_err_num;

      which_err_num = (in_camera == 0)?1:2;

      info("requested position either outside camera bounds or inside"
	   " an obstacle!\n"
	   );
#if defined(TBDB_EVENTTYPE_COMPLETE)
      event_do(handle,
	       EA_Experiment, pideid,
	       EA_Type, TBDB_OBJECTTYPE_NODE,
	       EA_Name, match->vname,
	       EA_Event, TBDB_EVENTTYPE_COMPLETE,
	       EA_ArgInteger, "ERROR", which_err_num,
	       EA_ArgInteger, "CTOKEN", match->token,
	       EA_TAG_DONE
	       );
#endif
    }

    // XXX What to do with the data?

    // construct a COMMAND_GOTO packet and send to rmc.

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
  else {
    if (debug > 1) {
      info("debug: connect from %s:%d\n",
	   inet_ntoa(client_sin.sin_addr),
	   ntohs(client_sin.sin_port));
    }
  }

  return 1;
}

int unknown_client_callback(elvin_io_handler_t handler,
			    int fd,
			    void *rock,
			    elvin_error_t eerror)
{
  mtp_packet_t mp_data, *mp = &mp_data;
  int rc, retval = 0;
  mtp_handle_t mh;

  /*
   * NB: We remove the handler before adding the new one because elvin calls
   * FD_CLR(fd, ...) here and we do not want it clearing the bit after it was
   * set by the add.
   */
  elvin_sync_remove_io_handler(handler, eerror);

  if ((mh = mtp_create_handle(fd)) == NULL) {
    error("mtp_create_handle\n");
  }
  else if (((rc = mtp_receive_packet(mh, mp)) != MTP_PP_SUCCESS) ||
	   (mp->vers != MTP_VERSION) ||
	   (mp->data.opcode != MTP_CONTROL_INIT)) {
    error("invalid client %d %p\n", rc, mp);
  }
  else {
    switch (mp->role) {
    case MTP_ROLE_RMC:
      if (rmc_data.handle != NULL) {
	error("rmc client is already connected\n");
      }
      else if ((retval = mtp_send_packet(mh, &config_rmc)) != MTP_PP_SUCCESS) {
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
	robot_list_item_t *rli;

	if (debug) {
	  info("established rmc connection\n");
	}

	// add descriptor to list, etc:
	if (rmc_data.position_list == NULL)
	  rmc_data.position_list = robot_list_create();

	for (rli = hostname_list->head; rli != NULL; rli = rli->next) {
	  struct emc_robot_config *erc = rli->data;
	  struct mtp_packet gmp;

	  if (erc->flags & ERF_HAS_GOAL) {
	    mtp_init_packet(&gmp,
			    MA_Opcode, MTP_COMMAND_GOTO,
			    MA_Role, MTP_ROLE_EMC,
			    MA_CommandID, 1,
			    MA_RobotID, erc->id,
			    MA_Position, &erc->last_goal_pos,
			    MA_TAG_DONE);
	  }
	  else {
	    mtp_init_packet(&gmp,
			    MA_Opcode, MTP_COMMAND_GOTO,
			    MA_Role, MTP_ROLE_EMC,
			    MA_CommandID, 1,
			    MA_RobotID, erc->id,
			    MA_X, (double)erc->init_x,
			    MA_Y, (double)erc->init_y,
			    MA_Theta, (double)erc->init_theta,
			    MA_TAG_DONE);
	  }
	  mtp_send_packet(mh, &gmp);
	}

	rmc_data.handle = mh;
	mh = NULL;

	if (rmc_data.handle->mh_remaining > 0) {
	  // XXX run the callbacks here to catch any queued data.
	}

	retval = 1;
      }
      break;
    case MTP_ROLE_EMULAB:
      if (emulab_handle != NULL) {
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
	if (debug) {
	  info("established emulab connection\n");
	}

	emulab_handle = mh;
	mh = NULL;

	retval = 1;
      }
      break;
    case MTP_ROLE_VMC:
      if (vmc_data.handle != NULL) {
	error("vmc client is already connected\n");
      }
      else if ((retval = mtp_send_packet(mh, &config_vmc)) != MTP_PP_SUCCESS) {
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
	if (debug) {
	  info("established vmc connection\n");
	}

	vmc_data.handle = mh;
	mh = NULL;

	// add descriptor to list, etc:
	if (vmc_data.position_list == NULL)
	  vmc_data.position_list = robot_list_create();

	retval = 1;
      }
      break;
    default:
      error("unknown role %d\n", mp->role);
      break;
    }
  }

  mtp_free_packet(mp);
  mp = NULL;

  mtp_delete_handle(mh);
  mh = NULL;

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


  mtp_packet_t mp_data, *mp = &mp_data;
  struct rmc_client *rmc = rock;
  int rc, retval = 0;

  do {
    if (((rc = mtp_receive_packet(rmc->handle, mp)) != MTP_PP_SUCCESS) ||
	(mp->vers != MTP_VERSION)) {
      error("invalid rmc client %d %p\n", rc, mp);
    }
    else {
      if (0 || debug) {
	fprintf(stderr, "rmc_callback: ");
	mtp_print_packet(stderr, mp);
      }

      switch (mp->data.opcode) {
      case MTP_REQUEST_POSITION:
	{
	  // find the latest position data for the robot:
	  //   supply init pos if no positions in rmc_data or vmc_data;
	  //   otherwise, take the position with the most recent timestamp
	  //     from rmc_data or vmc_data
	  int my_id = mp->data.mtp_payload_u.request_position.robot_id;
	  struct mtp_update_position *up;
	  struct emc_robot_config *erc;


	  erc = robot_list_search(hostname_list, my_id);

	  up = (struct mtp_update_position *)
	    robot_list_search(vmc_data.position_list, my_id);

      if (0 || debug) {
          if (erc->flags & ERF_HAS_GOAL) {
              info("ERF_HAS_GOAL is set for this robot\n");
          }
          else {
              info("ERF_HAS_GOAL is not set for this robot\n");
          }
      }

	  if ((up != NULL && up->status != MTP_POSITION_STATUS_ERROR) &&
          !(erc->flags & ERF_HAS_GOAL)) {

          if (0 || debug) {
              info("Sending STATUS_UNKNOWN UPDATE_POSITION (ERF_HAS_GOAL not set)\n");
          }

          mtp_send_packet2(rmc->handle,
                           MA_Opcode, MTP_UPDATE_POSITION,
                           MA_Role, MTP_ROLE_EMC,
                           MA_RobotID, up->robot_id,
                           MA_Position, &up->position,
                           MA_Status, MTP_POSITION_STATUS_UNKNOWN,
                           MA_TAG_DONE);
	  }
	  else if (up == NULL) {
	    up = (struct mtp_update_position *)
	      calloc(1, sizeof(struct mtp_update_position));
	    up->robot_id = my_id;
	    up->status = MTP_POSITION_STATUS_ERROR;
	    robot_list_append(vmc_data.position_list, my_id, up);
	  }

	  retval = 1;
	}
	break;

      case MTP_UPDATE_POSITION:
	{
	  // store the latest data in the robot position/status list
	  // in rmc_data
	  int my_id = mp->data.mtp_payload_u.update_position.robot_id;
	  struct mtp_update_position *up = (struct mtp_update_position *)
	    robot_list_remove_by_id(rmc->position_list, my_id);
	  struct emc_robot_config *erc;

	  free(up);
	  up = NULL;

	  up = (struct mtp_update_position *)
	    malloc(sizeof(struct mtp_update_position));
	  *up = mp->data.mtp_payload_u.update_position;
	  robot_list_append(rmc->position_list, my_id, up);

	  erc = robot_list_search(hostname_list, my_id);

	  info("update %d\n", my_id);

	  switch (mp->data.mtp_payload_u.update_position.status) {
	  case MTP_POSITION_STATUS_ERROR:
	  case MTP_POSITION_STATUS_COMPLETE:
	    if (emulab_handle != NULL) {
	      mtp_send_packet(emulab_handle, mp);
	    }
	    if (erc->token != ~0) {
#if defined(TBDB_EVENTTYPE_COMPLETE)
	      event_do(handle,
		       EA_Experiment, pideid,
		       EA_Type, TBDB_OBJECTTYPE_NODE,
		       EA_Name, erc->vname,
		       EA_Event, TBDB_EVENTTYPE_COMPLETE,
		       EA_ArgInteger, "ERROR",
		       mp->data.mtp_payload_u.update_position.status ==
		       MTP_POSITION_STATUS_ERROR ? 1 : 0,
		       EA_ArgInteger, "CTOKEN", erc->token,
		       EA_TAG_DONE);
#endif
	      erc->token = ~0;
	    }

	    /* unset ERF_HAS_GOAL flag */
	    erc->flags &= ~ERF_HAS_GOAL;

	    info("Received STATUS COMPLETE from %d\n", my_id);


	    break;
	  default:
	    assert(0);
	    break;
	  }

	  // also, if status is MTP_POSITION_STATUS_COMPLETE ||
	  // MTP_POSITION_STATUS_ERROR, notify emulab, or issue the next
	  // command to rmc from the movement list.

	  // later ....

	  retval = 1;
	}
	break;

      case MTP_WIGGLE_STATUS:
	{
	  /* simply forward this to vmc */
	  /* actually, we'll also ack it back to rmc */
	  mp->role = MTP_ROLE_EMC;

	  if (mtp_send_packet(vmc_data.handle,mp) != MTP_PP_SUCCESS) {
	    error("vmc unavailable; cannot forward wiggle-status\n");
	  }

	  retval = 1;
      }
      break;

    case MTP_CREATE_OBSTACLE:
	{
	    struct dyn_obstacle_config *doc;
	    struct emc_robot_config *erc;

	    doc = &mp->data.mtp_payload_u.create_obstacle;
	    erc = robot_list_search(hostname_list, doc->robot_id);

	    doc->config.xmin += OBSTACLE_BUFFER;
	    doc->config.ymin += OBSTACLE_BUFFER;
	    doc->config.xmax -= OBSTACLE_BUFFER;
	    doc->config.ymax -= OBSTACLE_BUFFER;
	    if (erc != NULL) {
		event_do(handle,
			 EA_Experiment, pideid,
			 EA_Type, TBDB_OBJECTTYPE_TOPOGRAPHY,
			 EA_Event, TBDB_EVENTTYPE_CREATE,
			 EA_Name, topography_name,
			 EA_ArgInteger, "ID", doc->config.id,
			 EA_ArgFloat, "XMIN", doc->config.xmin,
			 EA_ArgFloat, "YMIN", doc->config.ymin,
			 EA_ArgFloat, "XMAX", doc->config.xmax,
			 EA_ArgFloat, "YMAX", doc->config.ymax,
			 EA_ArgString, "ROBOT", erc->vname,
			 EA_TAG_DONE);
	    }
	    else {
		event_do(handle,
			 EA_Experiment, pideid,
			 EA_Type, TBDB_OBJECTTYPE_TOPOGRAPHY,
			 EA_Event, TBDB_EVENTTYPE_CREATE,
			 EA_Name, topography_name,
			 EA_ArgInteger, "ID", doc->config.id,
			 EA_ArgFloat, "XMIN", doc->config.xmin,
			 EA_ArgFloat, "YMIN", doc->config.ymin,
			 EA_ArgFloat, "XMAX", doc->config.xmax,
			 EA_ArgFloat, "YMAX", doc->config.ymax,
			 EA_TAG_DONE);
	    }

	    retval = 1;
	}
	break;

      case MTP_UPDATE_OBSTACLE:
	{
	  struct obstacle_config *oc;

	  oc = &mp->data.mtp_payload_u.update_obstacle;
	  /* XXX Hack */
	  event_do(handle,
		   EA_Experiment, pideid,
		   EA_Type, TBDB_OBJECTTYPE_TOPOGRAPHY,
		   EA_Event, TBDB_EVENTTYPE_MODIFY,
		   EA_Name, topography_name,
		   EA_ArgInteger, "ID", oc->id,
		   EA_ArgFloat, "XMIN", oc->xmin + 0.25,
		   EA_ArgFloat, "YMIN", oc->ymin + 0.25,
		   EA_ArgFloat, "XMAX", oc->xmax - 0.25,
		   EA_ArgFloat, "YMAX", oc->ymax - 0.25,
		   EA_TAG_DONE);

	  retval = 1;
	}
	break;

      case MTP_REMOVE_OBSTACLE:
	event_do(handle,
		 EA_Experiment, pideid,
		 EA_Type, TBDB_OBJECTTYPE_TOPOGRAPHY,
		 EA_Event, TBDB_EVENTTYPE_CLEAR,
		 EA_Name, topography_name,
		 EA_ArgInteger, "ID", mp->data.mtp_payload_u.remove_obstacle,
		 EA_TAG_DONE);

	retval = 1;
	break;

      case MTP_TELEMETRY:
	retval = 1;
	break;

      default:
	{
	  struct mtp_packet mp_reply;

	  error("received bogus opcode from RMC: %d\n", mp->data.opcode);

	  mtp_init_packet(&mp_reply,
			  MA_Opcode, MTP_CONTROL_ERROR,
			  MA_Role, MTP_ROLE_EMC,
			  MA_Message, "protocol error: bad opcode",
			  MA_TAG_DONE);

	  mtp_send_packet(rmc->handle, &mp_reply);
	}
	break;
      }
    }

    mtp_free_packet(mp);
  } while (retval && (rmc->handle->mh_remaining > 0));

  if (!retval) {
    info("dropping rmc connection %d\n", fd);

    elvin_sync_remove_io_handler(handler, eerror);

    mtp_delete_handle(rmc->handle);
    rmc->handle = NULL;

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
  mtp_packet_t mp_data, *mp = &mp_data;
  int rc, retval = 0;

  struct emc_robot_config *match = NULL;
  robot_list_item_t *rli;


  do {
    if (((rc = mtp_receive_packet(emulab_handle, mp)) != MTP_PP_SUCCESS) ||
	(mp->vers != MTP_VERSION)) {
      error("invalid client %p\n", mp);
    }
    else {
      if (debug) {
	fprintf(stderr, "emulab_callback: ");
	mtp_print_packet(stderr, mp);
      }

      switch (mp->data.opcode) {
      case MTP_COMMAND_GOTO:
	if (position_in_camera(g_camera_config,g_camera_config_size,
			       mp->data.mtp_payload_u.command_goto.position.x,
			       mp->data.mtp_payload_u.command_goto.position.y
			       ) &&
	    !position_in_obstacle(g_obstacle_config,g_obstacle_config_size,
				  mp->data.mtp_payload_u.command_goto.position.x,
				  mp->data.mtp_payload_u.command_goto.position.y)
	    ) {

	  /* forward the packet on to rmc... */
	  if (rmc_data.handle == NULL) {
	    error("no rmcd yet\n");
	  }
	  else if (mtp_send_packet(rmc_data.handle, mp) != MTP_PP_SUCCESS) {
	    error("could not forward packet to rmcd\n");
	  }
	  else {
	    info("forwarded goto\n");


	    /* DAN */
	    /* set ERF_HAS_GOAL flag for this robot */

#ifdef USE_POSTURE_REG
	    /* go through robot list */
	    for (rli = hostname_list->head; rli != NULL && !match; rli = rli->next) {
	      struct emc_robot_config *erc = rli->data;

	      if (erc->id == mp->data.mtp_payload_u.request_position.robot_id) {
		match = erc;
		break;
	      }
	    }

	    match->flags |= ERF_HAS_GOAL;
        info("set ERF_HAS_GOAL flag for this robot\n");
#endif

	    retval = 1;
	  }
	}
	else {
	  error("invalid position request (outside camera bounds or in"
		" an obstacle\n"
		);
	}
	break;
      case MTP_REQUEST_POSITION:
	{
	  // find the latest position data for the robot:
	  //   supply init pos if no positions in rmc_data or vmc_data;
	  //   otherwise, take the position with the most recent timestamp
	  //     from rmc_data or vmc_data
	  int my_id = mp->data.mtp_payload_u.request_position.robot_id;
	  struct mtp_update_position *vmc_up, *rmc_up, *up;
	  struct mtp_packet mp_reply;

	  vmc_up = (struct mtp_update_position *)
	    robot_list_search(vmc_data.position_list, my_id);
	  rmc_up = (struct mtp_update_position *)
	    robot_list_search(rmc_data.position_list, my_id);

	  if (vmc_up != NULL)
	    up = vmc_up; // XXX prefer vmc?
	  else
	    up = rmc_up;

	  if (up != NULL) {
	    // since VMC isn't hooked in, we simply write back the rmc posit
	    mtp_init_packet(&mp_reply,
			    MA_Opcode, MTP_UPDATE_POSITION,
			    MA_Role, MTP_ROLE_EMC,
			    MA_RobotID, up->robot_id,
			    MA_Position, &up->position,
			    /*
			     * The status field has no meaning when this packet
			     * is being sent.
			     */
			    MA_Status, MTP_POSITION_STATUS_UNKNOWN,
			    MA_TAG_DONE);
	    mtp_send_packet(emulab_handle, &mp_reply);
	  }
	  else {
	    info("no updates for %d\n", my_id);

	    mtp_init_packet(&mp_reply,
			    MA_Role, MTP_ROLE_EMC,
			    MA_Opcode, MTP_CONTROL_ERROR,
			    MA_Message, "position not updated yet",
			    MA_TAG_DONE);

	    mtp_send_packet(emulab_handle, &mp_reply);
	  }

	  retval = 1;
	}
	break;

      default:
	{
	  struct mtp_packet mp_reply;

	  error("received bogus opcode from RMC: %d\n", mp->data.opcode);

	  mtp_init_packet(&mp_reply,
			  MA_Opcode, MTP_CONTROL_ERROR,
			  MA_Role, MTP_ROLE_EMC,
			  MA_Message, "protocol error: bad opcode",
			  MA_TAG_DONE);

	  mtp_send_packet(emulab_handle, &mp_reply);
	}
	break;
      }
    }

    mtp_free_packet(mp);
  } while (retval && (emulab_handle->mh_remaining > 0));

  if (!retval) {
    info("dropping emulab connection %d\n", fd);

    elvin_sync_remove_io_handler(handler, eerror);

    mtp_delete_handle(emulab_handle);
    emulab_handle = NULL;

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
  mtp_packet_t mp_data, *mp = &mp_data;
  struct vmc_client *vmc = rock;
  int rc, retval = 0;
  int sent_up_posture = 0;


  struct emc_robot_config *match = NULL;
  robot_list_item_t *rli;

  do {
    if (((rc = mtp_receive_packet(vmc->handle, mp)) != MTP_PP_SUCCESS) ||
	(mp->vers != MTP_VERSION)) {
      error("invalid client %p\n", mp);
    }
    else {
      if (debug > 1) {
	fprintf(stderr, "vmc_callback: ");
	mtp_print_packet(stderr, mp);
      }

      switch (mp->data.opcode) {
      case MTP_UPDATE_POSITION:
	{
	  // store the latest data in the robot position/status list
	  // in vmc_data
	  int my_id = mp->data.mtp_payload_u.update_position.robot_id;
	  struct mtp_update_position *up = (struct mtp_update_position *)
	    robot_list_remove_by_id(vmc->position_list, my_id);
	  struct mtp_update_position *up_copy, *up_in;

      up_in = &mp->data.mtp_payload_u.update_position;

	  /* DAN */
#ifdef USE_POSTURE_REG
	  for (rli = hostname_list->head; rli != NULL; rli = rli->next) {
            struct emc_robot_config *erc = rli->data;

            if (erc->id == my_id) {
	      match = erc;
	      break;
            }
	  }




	  if (match->flags & ERF_HAS_GOAL) {
            /* if ERF_HAS_GOAL is set for this robot
             * set status to 'MOVING'
             * If status is 'MOVING', the nonlinear controller will be used
             */
            mtp_send_packet2(rmc_data.handle,
			     MA_Opcode, MTP_UPDATE_POSITION,
			     MA_Role, MTP_ROLE_EMC,
			     MA_RobotID, up->robot_id,
			     MA_Position, &up_in->position,
			     MA_Status, MTP_POSITION_STATUS_MOVING,
			     MA_TAG_DONE);

            sent_up_posture = 1;

          }
#endif




	  if (up && (up->status == MTP_POSITION_STATUS_ERROR) &&
	      (up_in->status != MTP_POSITION_STATUS_ERROR)) {
	    struct mtp_packet mp_reply;




            /* unknown status */
            if (0 == sent_up_posture) {
                mtp_init_packet(&mp_reply,
                                MA_Opcode, MTP_UPDATE_POSITION,
                                MA_Role, MTP_ROLE_EMC,
                                MA_RobotID, up->robot_id,
                                MA_Position, &up_in->position,
                                MA_Status, MTP_POSITION_STATUS_UNKNOWN,
                                MA_TAG_DONE);

                mtp_send_packet(rmc_data.handle, &mp_reply);
            }


	  }

	  free(up);
	  up = NULL;

	  up_copy = (struct mtp_update_position *)
	    malloc(sizeof(struct mtp_update_position));
	  *up_copy = mp->data.mtp_payload_u.update_position;
	  robot_list_append(vmc->position_list, my_id, up_copy);

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
	    &mp->data.mtp_payload_u.request_id;
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

	  double dx = 0.0, dy = 0.0, dtheta, ddist;

	  struct mtp_packet mp_reply;

	  int my_retval;

	  int id = -1;
	  struct robot_position *p;
	  struct robot_list_item *i;

	  mtp_print_packet(stdout, mp);
	  fflush(stdout);

	  if (rmc_data.position_list == NULL) {
	    info("no robot positions yet\n");
	  }
	  else {
	    i = rmc_data.position_list->head;
	    while (i != NULL) {
	      p = (struct robot_position *)(i->data);
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
	      p = (struct robot_position *)(i->data);
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

	  mtp_init_packet(&mp_reply,
			  MA_Opcode, MTP_UPDATE_ID,
			  MA_Role, MTP_ROLE_EMC,
			  MA_RequestID, rid->request_id,
			  MA_RobotID, robot_id,
			  MA_TAG_DONE);

	  my_retval = mtp_send_packet(vmc->handle, &mp_reply);
	  if (my_retval != MTP_PP_SUCCESS) {
	    fprintf(stdout,"Could not send update_id packet!\n");
	  }

	  retval = 1;
	}
	break;

      case MTP_WIGGLE_REQUEST:
	{
	  int my_id = mp->data.mtp_payload_u.request_position.robot_id;
	  struct mtp_update_position *up;

	  up = (struct mtp_update_position *)
	    robot_list_search(vmc_data.position_list, my_id);
	  if (up != NULL)
	    up->status = MTP_POSITION_STATUS_ERROR;

	  /* simply forward this to rmc */
	  if (rmc_data.handle != NULL) {
	    mp->role = MTP_ROLE_EMC;
	    mtp_send_packet(rmc_data.handle,mp);
	  }
	  else {
	    error("rmc unavailable; cannot forward wiggle-request\n");
	    mtp_send_packet2(vmc_data.handle,
			     MA_Opcode, MTP_WIGGLE_STATUS,
			     MA_Role, MTP_ROLE_EMC,
			     MA_RobotID, my_id,
			     MA_Status, MTP_POSITION_STATUS_ERROR,
			     MA_TAG_DONE);
	  }

	  retval = 1;
	}
	break;

      default:
	{
	  struct mtp_packet mp_reply;

	  error("received bogus opcode from VMC: %d\n", mp->data.opcode);

	  mtp_init_packet(&mp_reply,
			  MA_Opcode, MTP_CONTROL_ERROR,
			  MA_Role, MTP_ROLE_EMC,
			  MA_Message, "protocol error: bad opcode",
			  MA_TAG_DONE);

	  mtp_send_packet(vmc->handle, &mp_reply);
	}
	break;
      }
    }

    mtp_free_packet(mp);
  } while (retval && (vmc->handle->mh_remaining > 0));

  if (!retval) {
    info("dropping vmc connection %d\n", fd);

    elvin_sync_remove_io_handler(handler, eerror);

    mtp_delete_handle(vmc->handle);
    vmc->handle = NULL;

    robot_list_destroy(vmc->position_list);
    vmc->position_list = NULL;

    close(fd);
    fd = -1;
  }

  return retval;
}

/**
 * Do a fuzzy comparison of two values.
 *
 * @param x1 The first value.
 * @param x2 The second value.
 * @param tol The amount of tolerance to take into account when doing the
 * comparison.
 */
#define cmp_fuzzy(x1, x2, tol)				\
  ((((x1) - (tol)) < (x2)) && (x2 < ((x1) + (tol))))

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

  if (vmc_data.position_list) {
    e = robot_list_enum(vmc_data.position_list);
    while ((mup = (struct mtp_update_position *)
	    robot_list_enum_next_element(e)) != NULL) {
      struct emc_robot_config *erc;
      float orientation;

      erc = robot_list_search(hostname_list, mup->robot_id);
      orientation = mtp_theta(mup->position.theta) * 180.0 / M_PI;
      if (!cmp_fuzzy(erc->last_update_pos.x, mup->position.x, 0.02) ||
	  !cmp_fuzzy(erc->last_update_pos.y, mup->position.y, 0.02) ||
	  !cmp_fuzzy(erc->last_update_pos.theta, mup->position.theta, 0.04)) {
	event_do(handle,
		 EA_Experiment, pideid,
		 EA_Type, TBDB_OBJECTTYPE_NODE,
		 EA_Event, TBDB_EVENTTYPE_MODIFY,
		 EA_Name, erc->vname,
		 EA_ArgFloat, "X", mup->position.x,
		 EA_ArgFloat, "Y", mup->position.y,
		 EA_ArgFloat, "ORIENTATION", orientation,
		 EA_TAG_DONE);

	erc->last_update_pos = mup->position;
      }
    }
    robot_list_enum_destroy(e);
  }

  return retval;
}
