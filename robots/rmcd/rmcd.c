/* Robot Master Control Daemon
 *
 * Dan Flickinger
 *
 *
 * RMCD will immediately try to connect to EMCD, get configuration data,
 * and then open connections to all robots as sent by EMCD. RMCD only heeds
 * one master; in other words, it will connect to only a single EMCD.
 *
 * Once connections are open, RMCD will wait for commands from EMCD.
 *
 * After a command is received, RMCD will wait for EMCD to send the
 * current position for the robot commanded to move by EMCD.  Once the
 * current position is received, the move command is relayed to the
 * appropriate robot after converting the coordinates into the local
 * robot's reference frame.
 *
 * Upon completion of a command, RMCD will send notice to EMCD, along
 * with a position update sent from the robot and converted into the
 * global coordinate frame.
 *
 *
 * 2004/12/01
 * 2004/12/07
 */

#include <stdio.h>
#include <assert.h>
#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <string.h>
#include <sys/socket.h>

#include "log.h"
#include "mtp.h"
#include "rmcd.h"

#define GOR_SERVERPORT 2531

/**
 * How close does the robot have to be before it is considered at the intended
 * position.  Measurement is in meters(?).
 */
#define METER_TOLERANCE 0.025

/**
 * How close does the angle have to be before it is considered at the intended
 * angle.
 */
#define RADIAN_TOLERANCE 0.01

/**
 * Maximum number of times to try and refine the position before giving up.
 */
#define MAX_REFINE_RETRIES 5

/**
 * Do a fuzzy comparison of two values.
 *
 * @param x1 The first value.
 * @param x2 The second value.
 * @param tol The amount of tolerance to take into account when doing the
 * comparison.
 */
#define cmp_fuzzy(x1, x2, tol) \
    ((((x1) - (tol)) < (x2)) && (x2 < ((x1) + (tol))))

static int debug = 0;

static int looping = 1;

static struct mtp_config_rmc *rmc_config = NULL;

enum {
    GCB_PENDING_POSITION,
    GCB_REFINING_POSITION,
};

enum {
    /**
     * Flag used to indicate when a new intended position has been submitted.
     */
    GCF_PENDING_POSITION = (1L << GCB_PENDING_POSITION),

    /**
     * Flag used to indicate when we're trying to refine the position.
     */
    GCF_REFINING_POSITION = (1L << GCB_REFINING_POSITION),
};

struct gorobot_conn {
    unsigned long gc_flags;
    struct robot_config *gc_robot;
    int gc_fd;
    int gc_tries_remaining;
    struct position gc_actual_pos;
    struct position gc_intended_pos;
};

static struct gorobot_conn gorobot_connections[128];

static int next_request_id = 0;

/**
 * Fill out a sockaddr_in by resolving the given host and port pair.
 *
 * @param host_addr The sockaddr_in to fill out with the IP address resolved
 * from host and the given port number.
 * @param host The host name or dotted quad.
 * @param port The port number.
 */
static int mygethostbyname(struct sockaddr_in *host_addr,
			   char *host,
			   int port)
{
    struct hostent *host_ent;
    int retval = 0;

    assert(host_addr != NULL);
    assert(host != NULL);
    assert(strlen(host) > 0);
  
    memset(host_addr, 0, sizeof(struct sockaddr_in));
    host_addr->sin_len = sizeof(struct sockaddr_in);
    host_addr->sin_family = AF_INET;
    host_addr->sin_port = htons(port);
    if( (host_ent = gethostbyname(host)) != NULL ) {
	memcpy((char *)&host_addr->sin_addr.s_addr,
	       host_ent->h_addr,
	       host_ent->h_length);
	retval = 1;
    }
    else {
	retval = inet_aton(host, &host_addr->sin_addr);
    }
    return( retval );
}

static void usage(void)
{
    fprintf(stderr,
	    "Usage: rmcd ...\n");
}

/**
 * Map the robot ID to a gorobot_conn object.
 *
 * @param robot_id The robot identifier to search for.
 * @return The gorobot_conn that matches the given ID or NULL if no match was
 * found.
 */
static struct gorobot_conn *find_gorobot(int robot_id)
{
    struct gorobot_conn *retval = NULL;
    int lpc;

    assert(robot_id >= 0);

    for (lpc = 0; (lpc < rmc_config->num_robots) && !retval; lpc++) {
	struct gorobot_conn *gc = &gorobot_connections[lpc];

	if (gc->gc_robot->id == robot_id) {
	    retval = gc;
	}
    }
    
    return retval;
}

/**
 * Convert an absolute position to a relative position fit for grboto.dgoto().
 *
 * @param rel The object to fill out with the relative position.
 * @param abs The current absolute position of the robot.
 */
static void conv_pos(struct position *rel, struct position *abs)
{
    assert(rel != NULL);
    assert(abs != NULL);
    
    *rel = *abs; // XXX DAN, fill this out.
}

/**
 * Compare two position objects to see if they are the "same", where "same" is
 * within some tolerance.
 *
 * @return True if they are the "same".
 */
static int cmp_pos(struct position *pos1, struct position *pos2)
{
    int retval = 0;

    assert(pos1 != NULL);
    assert(pos2 != NULL);

    if (cmp_fuzzy(pos1->x, pos2->x, METER_TOLERANCE) &&
	cmp_fuzzy(pos1->y, pos2->y, METER_TOLERANCE) &&
	cmp_fuzzy(pos1->theta, pos2->theta, RADIAN_TOLERANCE)) {
	retval = 1;
    }

    return retval;
}

/**
 * Handle an mtp packet from emcd.
 *
 * @param emc_fd The connection to emcd.
 */
static void handle_emc_packet(int emc_fd)
{
    struct mtp_packet *mp = NULL;

    assert(emc_fd >= 0);
    
    if (mtp_receive_packet(emc_fd, &mp) != MTP_PP_SUCCESS) {
	errorc("bad packet from emc!\n");
    }
    else {
	struct gorobot_conn *gc;
	
	switch (mp->opcode) {
	case MTP_COMMAND_GOTO:
	    if ((gc = find_gorobot(mp->data.command_goto->robot_id)) == NULL) {
		error("uknnown robot %d\n", mp->data.command_goto->robot_id);
	    }
	    else if (!(gc->gc_flags & (GCF_PENDING_POSITION|
				       GCF_REFINING_POSITION)) &&
		     cmp_pos(&gc->gc_actual_pos,
			     &mp->data.command_goto->position)) {
		struct mtp_update_position mup;
		struct mtp_packet *rmp;
		
		info("robot %d is already there\n",
		     mp->data.command_goto->robot_id);

		/* Nothing to do other than send back a complete. */
		mup.robot_id = gc->gc_robot->id;
		mup.position = gc->gc_actual_pos;
		mup.status = MTP_POSITION_STATUS_COMPLETE;
		if ((rmp = mtp_make_packet(MTP_UPDATE_POSITION,
					   MTP_ROLE_RMC,
					   &mup)) == NULL) {
		    error("cannot allocate reply packet\n");
		}
		else if (mtp_send_packet(emc_fd, rmp) != MTP_PP_SUCCESS) {
		    error("cannot send reply packet\n");
		}
		
		mtp_free_packet(rmp);
		rmp = NULL;
	    }
	    else {
		struct mtp_command_stop mcs;
		struct mtp_packet *smp;
		
		info("gonna move\n");

		/*
		 * There is a new position, send a stop to gorobot so that it
		 * will send back an IDLE update and we can do more processing.
		 */
		gc->gc_intended_pos =
		    mp->data.command_goto->position;
		gc->gc_flags |= GCF_PENDING_POSITION;
		gc->gc_flags &= ~GCF_REFINING_POSITION;
		gc->gc_tries_remaining = 0;
		
		mcs.command_id = 1;
		mcs.robot_id = gc->gc_robot->id;
		
		if ((smp = mtp_make_packet(MTP_COMMAND_STOP,
					   MTP_ROLE_RMC,
					   &mcs)) == NULL) {
		    error("cannot allocate stop packet");
		}
		else if (mtp_send_packet(gc->gc_fd, smp) != MTP_PP_SUCCESS) {
		    error("cannot send reply packet\n");
		}
		
		mtp_free_packet(smp);
		smp = NULL;
	    }
	    break;
	case MTP_COMMAND_STOP:
	    if ((gc = find_gorobot(mp->data.command_goto->robot_id)) == NULL) {
		error("uknnown robot %d\n", mp->data.command_goto->robot_id);
	    }
	    else {
		struct mtp_command_stop mcs;
		struct mtp_packet *smp;

		/* Just stop the robot in its tracks and clear out any state */
		
		gc->gc_flags &= ~GCF_PENDING_POSITION;
		gc->gc_flags &= ~GCF_REFINING_POSITION;
		gc->gc_tries_remaining = 0;
		
		mcs.command_id = 1;
		mcs.robot_id = gc->gc_robot->id;
		
		if ((smp = mtp_make_packet(MTP_COMMAND_STOP,
					   MTP_ROLE_RMC,
					   &mcs)) == NULL) {
		    error("cannot allocate stop packet");
		}
		else if (mtp_send_packet(gc->gc_fd, smp) != MTP_PP_SUCCESS) {
		    error("cannot send reply packet\n");
		}
		
		mtp_free_packet(smp);
		smp = NULL;
	    }
	    break;
	case MTP_UPDATE_POSITION:
	    if ((gc = find_gorobot(mp->data.update_position->
				   robot_id)) == NULL) {
		error("uknnown robot %d\n",mp->data.update_position->robot_id);
	    }
	    else {
		/* XXX Always update the position? */
		gc->gc_actual_pos =
		    mp->data.update_position->position;
		
		info("position update %f\n", gc->gc_actual_pos.x);
		
		if (gc->gc_flags & GCF_REFINING_POSITION) {

		    /*
		     * This update is a response to our request, try to refine
		     * the position more.
		     */
		    
		    gc->gc_tries_remaining -= 1;
		    
		    if (gc->gc_tries_remaining == 0) {
			struct mtp_update_position mup;
			struct mtp_packet *ump;
			
			info("didn't make it it\n");
			
			gc->gc_flags &= ~GCF_REFINING_POSITION;
			
			mup.robot_id = gc->gc_robot->id;
			mup.position = gc->gc_actual_pos;
			mup.status = MTP_POSITION_STATUS_ERROR;
			
			if ((ump = mtp_make_packet(MTP_UPDATE_POSITION,
						   MTP_ROLE_RMC,
						   &mup)) == NULL) {
			    fatal("cannot allocate packet");
			}
			else if (mtp_send_packet(emc_fd, ump) !=
				 MTP_PP_SUCCESS) {
			    fatal("request id failed");
			}
			
			mtp_free_packet(ump);
			ump = NULL;
		    }
		    else if (cmp_pos(&gc->gc_actual_pos,
				     &gc->gc_intended_pos)) {
			struct mtp_update_position mup;
			struct mtp_packet *ump;
			
			info("made it\n");

			/* We're on target, tell emcd. */
			
			gc->gc_flags &= ~GCF_REFINING_POSITION;
			
			mup.robot_id = gc->gc_robot->id;
			mup.position = gc->gc_actual_pos;
			mup.status = MTP_POSITION_STATUS_COMPLETE;
			
			if ((ump = mtp_make_packet(MTP_UPDATE_POSITION,
						   MTP_ROLE_RMC,
						   &mup)) == NULL) {
			    fatal("cannot allocate packet");
			}
			else if (mtp_send_packet(emc_fd, ump) !=
				 MTP_PP_SUCCESS) {
			    fatal("request id failed");
			}
			
			mtp_free_packet(ump);
			ump = NULL;
		    }
		    else {
			struct mtp_command_goto mcg;
			struct mtp_packet *gmp;
			
			info("moving again\n");

			/* Start the process all over again. */
			
			mcg.command_id = 1;
			mcg.robot_id = gc->gc_robot->id;
			conv_pos(&mcg.position, &gc->gc_intended_pos);
			if ((gmp = mtp_make_packet(MTP_COMMAND_GOTO,
						   MTP_ROLE_RMC,
						   &mcg)) == NULL) {
			    fatal("cannot allocate packet");
			}
			else if (mtp_send_packet(gc->gc_fd, gmp) !=
				 MTP_PP_SUCCESS) {
			    fatal("request id failed");
			}
			
			mtp_free_packet(gmp);
			gmp = NULL;
		    }
		}
	    }
	    break;
	default:
	    warning("unhandled emc packet %d\n", mp->opcode);
	    break;
	}
    }
    
    mtp_free_packet(mp);
    mp = NULL;
}

static void handle_gc_packet(struct gorobot_conn *gc, int emc_fd)
{
    struct mtp_packet *mp = NULL;

    assert(gc != NULL);
    assert(emc_fd >= 0);
    
    if (mtp_receive_packet(gc->gc_fd, &mp) != MTP_PP_SUCCESS) {
	fatal("bad packet from gorobot!\n");
    }
    else if (mp->opcode == MTP_UPDATE_POSITION) {
	switch (mp->data.update_position->status) {
	case MTP_POSITION_STATUS_IDLE:
	    /* Response to a COMMAND_STOP. */
	    if (gc->gc_flags & GCF_PENDING_POSITION) {
		struct mtp_request_position mrp;
		struct mtp_packet *rmp;
		
		info("starting move\n");
		
		mrp.robot_id = gc->gc_robot->id;
		
		info("requesting position %d\n", mrp.robot_id);
		
		if ((rmp = mtp_make_packet(MTP_REQUEST_POSITION,
					   MTP_ROLE_RMC,
					   &mrp)) == NULL) {
		    fatal("cannot allocate packet");
		}
		else if (mtp_send_packet(emc_fd, rmp) != MTP_PP_SUCCESS) {
		    fatal("request id failed");
		}
		
		mtp_free_packet(rmp);
		rmp = NULL;

		/* Update state. */
		gc->gc_flags &= ~GCF_PENDING_POSITION;
		gc->gc_flags |= GCF_REFINING_POSITION;
		gc->gc_tries_remaining = MAX_REFINE_RETRIES;
	    }
	    break;
	case MTP_POSITION_STATUS_ERROR:
	    /* XXX */
	    break;
	case MTP_POSITION_STATUS_COMPLETE:
	    info("gorobot finished\n");
	    
	    if (gc->gc_flags & GCF_REFINING_POSITION) {
		struct mtp_request_position mrp;
		struct mtp_packet *rmp;
		
		mrp.robot_id = gc->gc_robot->id;
		
		info("requesting position %d\n", mrp.robot_id);
		
		if ((rmp = mtp_make_packet(MTP_REQUEST_POSITION,
					   MTP_ROLE_RMC,
					   &mrp)) == NULL) {
		    fatal("cannot allocate packet");
		}
		else if (mtp_send_packet(emc_fd, rmp) != MTP_PP_SUCCESS) {
		    fatal("request id failed");
		}
		
		mtp_free_packet(rmp);
		rmp = NULL;
	    }
	    break;
	default:
	    break;
	}
    }
    else {
	warning("unhandled emc packet %d\n", mp->opcode);
    }
    
    mtp_free_packet(mp);
    mp = NULL;
}

int main(int argc, char *argv[])
{
    char *logfile = NULL, *pidfile = NULL, *emc_hostname = NULL;
    int c, emc_fd = -1, emc_port = 0, retval = EXIT_SUCCESS;
    struct sockaddr_in saddr;
    fd_set readfds;

    FD_ZERO(&readfds);

    while ((c = getopt(argc, argv, "hdp:l:i:e:c:P:")) != -1) {
	switch (c) {
	case 'h':
	    usage();
	    exit(0);
	    break;
	case 'd':
	    debug += 1;
	    break;
	case 'l':
	    logfile = optarg;
	    break;
	case 'i':
	    pidfile = optarg;
	    break;
	case 'e':
	    emc_hostname = optarg;
	    break;
	case 'p':
	    if (sscanf(optarg, "%d", &emc_port) != 1) {
		error("-p option is not a number: %s\n", optarg);
		usage();
		exit(1);
	    }
	    break;
	}
    }
  
    if (debug) {
	loginit(0, logfile);
    }
    else {
	/* Become a daemon */
	daemon(0, 0);
    
	if (logfile)
	    loginit(0, logfile);
	else
	    loginit(1, "vmcclient");
    }
  
    if (pidfile) {
	FILE *fp;
    
	if ((fp = fopen(pidfile, "w")) != NULL) {
	    fprintf(fp, "%d\n", getpid());
	    (void) fclose(fp);
	}
    }
  
    if (emc_hostname != NULL) {
	struct mtp_packet *mp = NULL, *rmp = NULL;
	struct mtp_control mc;
    
	mc.id = -1;
	mc.code = -1;
	mc.msg = "rmcd init";

	/* Connect to emcd and get the configuration. */
	if (mygethostbyname(&saddr, emc_hostname, emc_port) == 0) {
	    pfatal("bad emc hostname: %s\n", emc_hostname);
	}
	else if ((emc_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	    pfatal("socket");
	}
	else if (connect(emc_fd,
			 (struct sockaddr *)&saddr,
			 sizeof(saddr)) == -1) {
	    pfatal("connect");
	}
	else if ((mp = mtp_make_packet(MTP_CONTROL_INIT,
				       MTP_ROLE_RMC,
				       &mc)) == NULL) {
	    pfatal("mtp_make_packet");
	}
	else if (mtp_send_packet(emc_fd, mp) != MTP_PP_SUCCESS) {
	    pfatal("could not configure with emc");
	}
	else if (mtp_receive_packet(emc_fd, &rmp) != MTP_PP_SUCCESS) {
	    pfatal("could not get rmc config packet");
	}
	else {
	    int lpc;
	    
	    FD_SET(emc_fd, &readfds);
	    rmc_config = rmp->data.config_rmc;

	    /*
	     * Walk through the robot list and connect to the gorobots daemons
	     * running on the robots.
	     */
	    for (lpc = 0; lpc < rmc_config->num_robots; lpc++) {
		struct gorobot_conn *gc = &gorobot_connections[lpc];
		struct mtp_request_position mrp;
		struct mtp_packet *qmp, *qrmp;
		
		info(" robot[%d] = %d %s\n",
		     lpc,
		     rmc_config->robots[lpc].id,
		     rmc_config->robots[lpc].hostname);

		gc->gc_robot = &rmc_config->robots[lpc];
		if (mygethostbyname(&saddr,
				    rmc_config->robots[lpc].hostname,
				    GOR_SERVERPORT) == 0) {
		    fatal("bad gorobot hostname: %s\n",
			  rmc_config->robots[lpc].hostname);
		}
		else if ((gc->gc_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		    fatal("socket");
		}
		else if (connect(gc->gc_fd,
				 (struct sockaddr *)&saddr,
				 sizeof(saddr)) == -1) {
		    fatal("connect");
		}
		else {
		    FD_SET(gc->gc_fd, &readfds);
		}

		/* Get the robots initial position. */
		mrp.robot_id = gc->gc_robot->id;
		if ((qmp = mtp_make_packet(MTP_REQUEST_POSITION,
					   MTP_ROLE_RMC,
					   &mrp)) == NULL) {
		    fatal("cannot allocate packet");
		}
		else if (mtp_send_packet(emc_fd, qmp) != MTP_PP_SUCCESS) {
		    fatal("request id failed");
		}
		else if (mtp_receive_packet(emc_fd, &qrmp) != MTP_PP_SUCCESS) {
		    fatal("request id response failed");
		}
		else if (qrmp->opcode != MTP_UPDATE_POSITION) {
		    fatal("request didn't evoke an update? %d %s\n",
			  qrmp->opcode,
			  qrmp->data.control->msg);
		}
		else {
		    gc->gc_actual_pos = qrmp->data.update_position->position;

		    info(" robot[%d].x = %f\n", lpc, gc->gc_actual_pos.x);
		    info(" robot[%d].y = %f\n", lpc, gc->gc_actual_pos.y);
		    info(" robot[%d].theta = %f\n",
			 lpc,
			 gc->gc_actual_pos.theta);
		}

		mtp_free_packet(qrmp);
		qrmp = NULL;
		
		mtp_free_packet(qmp);
		qmp = NULL;
	    }
	}
    
	free(mp);
	mp = NULL;
    }

    while (looping) {
	fd_set rreadyfds = readfds;
	int rc;

	rc = select(FD_SETSIZE, &rreadyfds, NULL, NULL, NULL);

	if (rc > 0) {
	    int lpc;
	    
	    if (FD_ISSET(emc_fd, &rreadyfds)) {
		handle_emc_packet(emc_fd);
	    }

	    for (lpc = 0; lpc < rmc_config->num_robots; lpc++) {
		struct gorobot_conn *gc = &gorobot_connections[lpc];
		
		if (FD_ISSET(gc->gc_fd, &rreadyfds)) {
		    handle_gc_packet(gc, emc_fd);
		}
	    }
	}
	else {
	    errorc("accept\n");
	}
    }
  
    return retval;
}

#if 0
int main(int argc, char **argv) {
  int retval = EXIT_SUCCESS;

  
  /* You are watching RMCD */

  // miscellaneous administration junk
  int retval = 0;       // return value
  int cpacket_rcvd = 0; // configuration packet received

  
  // file descriptors
  int FD_emc; // connection to EMC server
  int max_fd = -1;
  

  // file descriptor set and time value for select() loop
  fd_set defaultset, readset;
  struct timeval default_tv, tv;


  // set up file descriptors and select variables
  FD_ZERO(&defaultset);
  default_tv.tv_sec = 5; // 5 seconds
  default_tv.tv_usec = 0; // wah?
  

  struct sockaddr_in EMC_serv;
  struct hostent *EMC_host;
  
  struct mtp_packet *read_packet;
  
  
  
  
 
  // parse options
  // FIXME
         
  int EMC_port = EMC_SERVERPORT; // EMC server port (SET DEFAULT VALUE)
  char EMC_hostname[128]; // FIXME: what is the default hostname?

  
  
  
  // daemon mode -- go to hell
  // FIXME
  

  
         
         
  
  
  
  /*************************************/
  /*** Connect to EMCD and configure ***/
  /*************************************/
  
  // create socket for connection to EMC server
  FD_emc = socket(AF_INET, SOCK_STREAM, 0);
  if (FD_emc == -1) {
    // fuckup
    fprintf(stdout, "FATAL: Could not open socket to EMC: %s\n", strerror(errno));
    exit(1); 
  }
  
  
  // add this new socket to the default FD set
  FD_SET(FD_emc, &defaultset);
  max_fd = (FD_emc > max_fd)?FD_emc:max_fd;

  
  // client socket to connect to EMCD
  EMC_serv.sin_family = AF_INET; // TCP/IP
  EMC_serv.sin_port = htons(port); // port
  EMC_host = gethostbyname(&EMC_hostname);
  bcopy(EMC_host->h_addr, &EMC_serv, EMC_host->h_length);
  
    
  // connect socket
  if (-1 == connect(FD_emc, (struct sockaddr *)(&EMC_serv), sizeof(struct sockaddr))) {
    // fuckup
    fprintf(stdout, "FATAL: Could not connect to EMC: %s\n", strerror(errno));
    exit(1);
  }
  // now connected to EMCD
  
  
  
  
  // send configuration request packet to EMCD
  // FIXME: need to look at structure in the protocol
  struct mtp_config cfg;
  cfg.id = -1; // ID: FIXME
  cfg.code = -1; // code: FIXME
  cfg.msg = ""; // message: FIXME
  
  struct mtp_packet *cfg_packet = mtp_make_packet(MTP_CONFIG, MTP_ROLE_RMC, &cfg); // FIXME: is it MTP_CONFIG??
  mtp_send_packet(FD_emc, cfg_packet);
  
  
  
  
  while (cpacket_rcvd == 0) {
    // wait for configuration data from EMC
    
  
    // select??
    
    
    // read a packet in
    read_packet = receive_packet(FD_emc);
    if (read_packet == NULL) {
      // fuckup
      fprintf(stdout, "FATAL: Could not read config data from EMC\n");
      exit(1);
    }
  
  
    if (read_packet->opcode == MTP_CONFIG_RMC) {
      // this is a configuration packet
      
      // store robot list data
      // FIXME: need data structure here
      
      // FIXME: global coordinate bound box
      //cfg.box.horizontal = read_packet.box.horizontal;
      //cfg.box.vertical = read_packet.box.vertical;
      
      
      cpacket_rcvd = 1;
    } else {
      printf("Still missing configuration packet from EMC\n");
    
      // send notification to EMC
      // Hello? Where is CONFIG PACKET??
     
      // send the configuration request packet AGAIN
      mtp_send_packet(FD_emc, cfg);
    
    }
  }

  /********************************************/
  /*** Now connected to EMCD and configured ***/
  /********************************************/

  
  
  
  /************************/
  /*** Configure robots ***/
  /************************/
  
  
  // open a connection to each robot listener
  // FIXME: monkey
         
  // {loop}
  
  // create packet for this robot
  // send packet to robot
         
  // configure
         
  // add connection to default set

  // add robot to list as configured
  
  // notify that robot is configured
  
  // {endloop}
  
         
         
         
  /*********************************/
  /*** Robots are now configured ***/
  /*********************************/

         
         


  /**********************************************************/
  /*** Notify EMCD that RMCD is ready to receive commands ***/
  /**********************************************************/   
  
  // FIXME: is this right??
  // send packet to EMC updating ready status
  struct mtp_control rc_packet;
  rc_packet.id = -1; // ID: ?? FIXME
  rc_packet.code = -1; // Code: ?? FIXME
  rc_packet.msg = "RMCD ready";
  
  struct mtp_packet *rc = mtp_make_packet(MTP_CONFIG, MTP_ROLE_RMC, &rc_packet); // FIXME: not MTP_CONFIG
  
  mtp_send_packet(FD_emc, rc);
  


  /*** EMCD has been notified of status ***/  

  // set up select bitmap


  

  /*****************************************/  
  /*** ALL CONFIGURATION IS NOW COMPLETE ***/
  /*** Let's get down to business        ***/
  /*****************************************/

  
  // malloc packet structure for use in the main loop
  struct mtp_packet *packet = (struct mtp_packet *)malloc(sizeof(struct mtp_packet));
  if (packet == NULL) {
    fprintf(stdout, "FATAL: Could not malloc packet structure.\n");
    exit(1);
  }

  
  
  
  // main loop, whoo!
  while (quitmain == 0) {
   
    
    readset = defaultset; // select tramples defaultset otherwise
    tv = default_tv; // same here (shame!)
    
    // select() on all EMC and sub-instance file descriptors
    retval = select(max_fd + 1, &readset, NULL, NULL, &tv);
    if (retval == -1) {
      // fuckup
      fprintf(stdout, "RMCD: Error in select loop: %s\n", strerror(errno));
    } else if (retval == 0) {
      // timeout was reached
      // do nothing!
    } else {
      // check all file descriptors for activity
      
      if (FD_ISSET(FD_emc, &readset)) {
        // Uh oh, something came back from EMCD
        
        
    
    
    // receive packet(s) in
    
    // if packet is from EMCD:
    
    if (packet->role == MTP_ROLE_EMC) {
      
      
      // if goto command from EMC:
      // if this robot already moving, drop the packet
      // otherwise -- put requested position in "robot state array"
      // issue request to EMC for latest position
      // then implicitly wait to hear a position update for this robot id
    
    
    
    
      // if stop command from EMC:
      // if "stop all robots", issue commands to all subinstances saying
      // terminate all motion
      // if stop single robot:
      //   update state array saying we're trying to stop a robot.
      //   forward the stop to subinstance
    
    
    
    
      // if position update from EMC:
      // store the position in position array
      // compare intended position to current position (issue goto commands
      //    IF necessary)
    
      // transform position to robot local coordinate frame and send
      // to robot
      // ELSE: tell EMCD that this robot is at it's intended position

    
    } else if (packet->role == MTP_GOROBOT) {
      // FIXME: what is the correct role here??
      // it's from a robot
      
      // send get position update request
                 
             
      // IF robot says that it thinks goal position has been reached
      
             
      // IF robot says that it cannot get to commanded position
    

      
    }
    
     
  } // end main loop


  // have a nice day, and good bye.
  printf("RMCD exiting without complaint.\n");
  return 0; // hope to see you soon
}
#endif
