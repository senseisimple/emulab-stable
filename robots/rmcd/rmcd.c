/* Robot Master Control Daemon
 *
 * Dan Flickinger
 * ***
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
 * 2004/12/14
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

#include <math.h>

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
#define RADIAN_TOLERANCE 0.09

/**
 * Maximum number of times to try and refine the position before giving up.
 */
#define MAX_REFINE_RETRIES 3

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
    GCB_VISION_POSITION,
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

    /**
     * Flag used to indicate that the value in the gc_actual_pos field was
     * obtained from the vision system.
     */
    GCF_VISION_POSITION = (1L << GCB_VISION_POSITION),
};

struct gorobot_conn {
    unsigned long gc_flags;
    struct robot_config *gc_robot;
    mtp_handle_t gc_handle;
    int gc_tries_remaining;
    struct robot_position gc_actual_pos;
    struct robot_position gc_intended_pos;
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
#ifndef linux
    host_addr->sin_len = sizeof(struct sockaddr_in);
#endif
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
	    "Usage: rmcd [-hd] [-l logfile] [-i pidfile] [-e emchost] [-p emcport]\n");
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

    for (lpc = 0; (lpc < rmc_config->robots.robots_len) && !retval; lpc++) {
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
 * @param abs_start The current absolute position of the robot.
 * @param abs_finish The destination position for the robot.
 */
static void conv_a2r(struct robot_position *rel,
		     struct robot_position *abs_start,
		     struct robot_position *abs_finish)
{
   float ct, st;
  
  
    assert(rel != NULL);
    assert(abs_start != NULL);
    assert(abs_finish != NULL);
    
    ct = cos(abs_start->theta);
    st = sin(abs_start->theta);
    
#if 1
    rel->x = ct*(abs_finish->x - abs_start->x) +
             st*(-abs_finish->y - -abs_start->y);
    rel->y = ct*(-abs_finish->y - -abs_start->y) +
             st*(abs_start->x - abs_finish->x);
#else
    // Transpose x, y
    rel->x = ct*(abs_finish->y - abs_start->y) +
             st*(abs_finish->x - abs_start->x);
    rel->y = ct*(abs_finish->x - abs_start->x) +
             st*(abs_start->y - abs_finish->y);
#endif
    
    rel->theta = abs_finish->theta - abs_start->theta;
    rel->timestamp = abs_finish->timestamp;
    
    info("a2r %f %f %f\n", rel->x, rel->y, rel->theta);
}

/**
 * Convert an relative movement to an absolute position that we can send back
 * to emcd.
 *
 * @param abs_finish The object to fill out with the final absolute position.
 * @param abs_start The absolute position of the robot before the move.
 * @param rel The relative movement of the robot.
 */
static void conv_r2a(struct robot_position *abs_finish,
		     struct robot_position *abs_start,
		     struct robot_position *rel)
{
  
    float ct, st;
    
    
    assert(rel != NULL);
    assert(abs_start != NULL);
    assert(abs_finish != NULL);

    rel->x = floor(rel->x * 1000.0) / 1000.0;
    rel->y = floor(rel->y * 1000.0) / 1000.0;
    
    ct = cos(abs_start->theta);
    st = sin(abs_start->theta);
    
 //  abs_finish->x = ct*rel->x - st*rel->y + abs_start->x;
 //   abs_finish->y = ct*rel->y + st*rel->x + abs_start->y;
  
    // transpose x,y
    abs_finish->x = ct*rel->y - st*rel->x + -abs_start->y;
    abs_finish->y = ct*rel->x + st*rel->y + abs_start->x;
  
    abs_finish->theta = abs_start->theta + rel->theta;
    abs_finish->timestamp = rel->timestamp;

    info("r2a %f %f %f\n", abs_finish->x, abs_finish->y, abs_finish->theta);
}

/**
 * Compare two position objects to see if they are the "same", where "same" is
 * within some tolerance.
 *
 * @return True if they are the "same".
 */
static int cmp_pos(struct robot_position *pos1, struct robot_position *pos2)
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
static void handle_emc_packet(mtp_handle_t emc_handle)
{
    struct mtp_packet mp;
    
    assert(emc_handle != NULL);
    
    if (mtp_receive_packet(emc_handle, &mp) != MTP_PP_SUCCESS) {
	errorc("bad packet from emc!\n");
    }
    else {
	struct mtp_command_goto *mcg = &mp.data.mtp_payload_u.command_goto;
	struct mtp_command_stop *mcs = &mp.data.mtp_payload_u.command_stop;
	struct mtp_update_position *mup =
	    &mp.data.mtp_payload_u.update_position;
	struct gorobot_conn *gc;
	
	if (debug) {
	    fprintf(stderr, "emc packet: ");
	    mtp_print_packet(stderr, &mp);
	}
	
	switch (mp.data.opcode) {
	case MTP_COMMAND_GOTO:
	    if ((gc = find_gorobot(mcg->robot_id)) == NULL) {
		error("uknnown robot %d\n", mcg->robot_id);
	    }
	    else if (!(gc->gc_flags & (GCF_PENDING_POSITION|
				       GCF_REFINING_POSITION)) &&
		     cmp_pos(&gc->gc_actual_pos, &mcg->position)) {
		struct mtp_packet rmp;
		
		info("robot %d is already there\n", mcg->robot_id);

		/* Nothing to do other than send back a complete. */
		mtp_init_packet(&rmp,
				MA_Opcode, MTP_UPDATE_POSITION,
				MA_Role, MTP_ROLE_RMC,
				MA_Position, &gc->gc_actual_pos,
				MA_RobotID, gc->gc_robot->id,
				MA_Status, MTP_POSITION_STATUS_COMPLETE,
				MA_TAG_DONE);
		if (mtp_send_packet(emc_handle, &rmp) != MTP_PP_SUCCESS) {
		    error("cannot send reply packet\n");
		}
		
		mtp_free_packet(&rmp);
	    }
	    else {
		struct mtp_packet smp;
		
		info("gonna move\n");

		/*
		 * There is a new position, send a stop to gorobot so that it
		 * will send back an IDLE update and we can do more processing.
		 */
		gc->gc_intended_pos = mcg->position;
		gc->gc_flags |= GCF_PENDING_POSITION;
		gc->gc_flags &= ~GCF_REFINING_POSITION;
		gc->gc_tries_remaining = 0;
		
		mtp_init_packet(&smp,
				MA_Opcode, MTP_COMMAND_STOP,
				MA_Role, MTP_ROLE_RMC,
				MA_RobotID, gc->gc_robot->id,
				MA_CommandID, 1,
				MA_TAG_DONE);
		if (mtp_send_packet(gc->gc_handle, &smp) != MTP_PP_SUCCESS) {
		    error("cannot send reply packet\n");
		}
		
		mtp_free_packet(&smp);
	    }
	    break;
	case MTP_COMMAND_STOP:
	    if ((gc = find_gorobot(mcs->robot_id)) == NULL) {
		error("uknnown robot %d\n", mcs->robot_id);
	    }
	    else {
		struct mtp_packet smp;

		/* Just stop the robot in its tracks and clear out any state */
		
		gc->gc_flags &= ~GCF_PENDING_POSITION;
		gc->gc_flags &= ~GCF_REFINING_POSITION;
		gc->gc_tries_remaining = 0;
		
		mtp_init_packet(&smp,
				MA_Opcode, MTP_COMMAND_STOP,
				MA_Role, MTP_ROLE_RMC,
				MA_RobotID, gc->gc_robot->id,
				MA_CommandID, 1,
				MA_TAG_DONE);
		if (mtp_send_packet(gc->gc_handle, &smp) != MTP_PP_SUCCESS) {
		    error("cannot send reply packet\n");
		}
		
		mtp_free_packet(&smp);
	    }
	    break;
	case MTP_UPDATE_POSITION:
	    if ((gc = find_gorobot(mup->robot_id)) == NULL) {
		error("uknnown robot %d\n", mup->robot_id);
	    }
	    else {
		/*
		 * XXX Always update the position?  Should compare against the
		 * robot-derived position before overwriting it...
		 */
		gc->gc_actual_pos = mup->position;
		gc->gc_flags |= GCF_VISION_POSITION;
		
		info("position update %f\n", gc->gc_actual_pos.x);
		
		if (gc->gc_flags & GCF_REFINING_POSITION) {

		    /*
		     * This update is a response to our request, try to refine
		     * the position more.
		     */
		    
		    gc->gc_tries_remaining -= 1;
		    
		    if (gc->gc_tries_remaining == 0) {
			struct mtp_packet ump;
			
			info("didn't make it it\n");
			
			gc->gc_flags &= ~GCF_REFINING_POSITION;
			
			mtp_init_packet(&ump,
					MA_Opcode, MTP_UPDATE_POSITION,
					MA_Role, MTP_ROLE_RMC,
					MA_Position, &gc->gc_actual_pos,
					MA_RobotID, gc->gc_robot->id,
					MA_Status, MTP_POSITION_STATUS_ERROR,
					MA_TAG_DONE);
			if (mtp_send_packet(emc_handle, &ump) !=
			    MTP_PP_SUCCESS) {
			    fatal("request id failed");
			}
			
			mtp_free_packet(&ump);
		    }
		    else if (cmp_pos(&gc->gc_actual_pos,
				     &gc->gc_intended_pos)) {
			struct mtp_packet ump;
			
			info("made it\n");

			/* We're on target, tell emcd. */
			
			gc->gc_flags &= ~GCF_REFINING_POSITION;
			
			mtp_init_packet(
				&ump,
				MA_Opcode, MTP_UPDATE_POSITION,
				MA_Role, MTP_ROLE_RMC,
				MA_Position, &gc->gc_actual_pos,
				MA_RobotID, gc->gc_robot->id,
				MA_Status, MTP_POSITION_STATUS_COMPLETE,
				MA_TAG_DONE);
			if (mtp_send_packet(emc_handle, &ump) !=
			     MTP_PP_SUCCESS) {
			    fatal("request id failed");
			}
			
			mtp_free_packet(&ump);
		    }
		    else {
			struct robot_position new_pos;
			struct mtp_packet gmp;
			
			info("moving again\n");

			/* Not there yet, contiue refining. */
			
			conv_a2r(&new_pos,
				 &gc->gc_actual_pos,
				 &gc->gc_intended_pos);
			mtp_init_packet(&gmp,
					MA_Opcode, MTP_COMMAND_GOTO,
					MA_Role, MTP_ROLE_RMC,
					MA_Position, &new_pos,
					MA_RobotID, gc->gc_robot->id,
					MA_CommandID, 1,
					MA_TAG_DONE);
			if (mtp_send_packet(gc->gc_handle, &gmp) !=
				 MTP_PP_SUCCESS) {
			    fatal("request id failed");
			}
			
			mtp_free_packet(&gmp);
		    }
		}
	    }
	    break;
	default:
	    warning("unhandled emc packet %d\n", mp.data.opcode);
	    break;
	}
    }
    
    mtp_free_packet(&mp);
}

static void handle_gc_packet(struct gorobot_conn *gc, mtp_handle_t emc_handle)
{
    struct mtp_packet mp;

    assert(gc != NULL);
    assert(emc_handle != NULL);
    
    if (mtp_receive_packet(gc->gc_handle, &mp) != MTP_PP_SUCCESS) {
	fatal("bad packet from gorobot!\n");
    }
    else if (mp.data.opcode == MTP_UPDATE_POSITION) {
	struct mtp_update_position *mup =
	    &mp.data.mtp_payload_u.update_position;
	
	if (debug) {
	    fprintf(stderr, "gorobot packet: ");
	    mtp_print_packet(stderr, &mp);
	}

	if ((mup->position.x != 0.0f) ||
	    (mup->position.y != 0.0f) ||
	    (mup->position.theta != 0.0f)) {
	    struct robot_position new_pos;
	    struct mtp_packet ump;
	    
	    /* Update emcd and save the new estimated position. */
	    conv_r2a(&new_pos, &gc->gc_actual_pos, &mup->position);
	    gc->gc_actual_pos = new_pos;
	    gc->gc_flags &= ~GCF_VISION_POSITION;

	    mtp_init_packet(&ump,
			    MA_Opcode, MTP_UPDATE_POSITION,
			    MA_Role, MTP_ROLE_RMC,
			    MA_Position, &new_pos,
			    MA_RobotID, gc->gc_robot->id,
			    MA_Status, MTP_POSITION_STATUS_MOVING,
			    MA_TAG_DONE);
	    if (mtp_send_packet(emc_handle, &ump) != MTP_PP_SUCCESS) {
		fatal("request id failed");
	    }
	    
	    mtp_free_packet(&ump);
	}
	
	switch (mup->status) {
	case MTP_POSITION_STATUS_IDLE:
	    /* Response to a COMMAND_STOP. */
	    if (gc->gc_flags & GCF_PENDING_POSITION) {
		struct mtp_packet rmp;
		
		info("starting move\n");
		info("requesting position %d\n", gc->gc_robot->id);
		
		mtp_init_packet(&rmp,
				MA_Opcode, MTP_REQUEST_POSITION,
				MA_Role, MTP_ROLE_RMC,
				MA_RobotID, gc->gc_robot->id,
				MA_TAG_DONE);
		if (mtp_send_packet(emc_handle, &rmp) != MTP_PP_SUCCESS) {
		    fatal("request id failed");
		}
		
		mtp_free_packet(&rmp);

		/* Update state. */
		gc->gc_flags &= ~GCF_PENDING_POSITION;
		gc->gc_flags |= GCF_REFINING_POSITION;
		gc->gc_tries_remaining = MAX_REFINE_RETRIES;
	    }
	    break;
	case MTP_POSITION_STATUS_ERROR:
	    /* XXX */
	    {
		struct mtp_packet ump;
		
		info("error for %d\n", gc->gc_robot->id);
		
		mtp_init_packet(&ump,
				MA_Opcode, MTP_UPDATE_POSITION,
				MA_Role, MTP_ROLE_RMC,
				MA_Position, &gc->gc_actual_pos,
				MA_RobotID, gc->gc_robot->id,
				MA_Status, MTP_POSITION_STATUS_ERROR,
				MA_TAG_DONE);
		if (mtp_send_packet(emc_handle, &ump) != MTP_PP_SUCCESS) {
		    fatal("request id failed");
		}
		
		mtp_free_packet(&ump);
		
		gc->gc_flags &= ~GCF_PENDING_POSITION;
		gc->gc_flags &= ~GCF_REFINING_POSITION;
	    }
	    break;
	case MTP_POSITION_STATUS_COMPLETE:
	    info("gorobot finished\n");
	    
	    if (gc->gc_flags & GCF_REFINING_POSITION) {
		struct mtp_packet rmp;
		
		info("requesting position %d\n", gc->gc_robot->id);
		
		mtp_init_packet(&rmp,
				MA_Opcode, MTP_REQUEST_POSITION,
				MA_Role, MTP_ROLE_RMC,
				MA_RobotID, gc->gc_robot->id,
				MA_TAG_DONE);
		if (mtp_send_packet(emc_handle, &rmp) != MTP_PP_SUCCESS) {
		    fatal("request id failed");
		}
		
		mtp_free_packet(&rmp);
	    }
	    break;
	default:
	    break;
	}
    }
    else {
	warning("unhandled emc packet %d\n", mp.data.opcode);
    }
    
    mtp_free_packet(&mp);
}

int main(int argc, char *argv[])
{
    char *logfile = NULL, *pidfile = NULL, *emc_hostname = NULL;
    int c, emc_fd = -1, emc_port = 0, retval = EXIT_SUCCESS;
    mtp_handle_t emc_handle = NULL;
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
	struct mtp_packet mp, rmp;
	
	mtp_init_packet(&mp,
			MA_Opcode, MTP_CONTROL_INIT,
			MA_Role, MTP_ROLE_RMC,
			MA_Message, "rmcd init",
			MA_TAG_DONE);

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
	else if ((emc_handle = mtp_create_handle(emc_fd)) == NULL) {
	    pfatal("mtp_create_handle");
	}
	else if (mtp_send_packet(emc_handle, &mp) != MTP_PP_SUCCESS) {
	    pfatal("could not configure with emc");
	}
	else if (mtp_receive_packet(emc_handle, &rmp) != MTP_PP_SUCCESS) {
	    pfatal("could not get rmc config packet");
	}
	else {
	    int lpc;
	    
	    FD_SET(emc_fd, &readfds);
	    rmc_config = &rmp.data.mtp_payload_u.config_rmc;

	    /*
	     * Walk through the robot list and connect to the gorobots daemons
	     * running on the robots.
	     */
	    for (lpc = 0; lpc < rmc_config->robots.robots_len; lpc++) {
		struct gorobot_conn *gc = &gorobot_connections[lpc];
		struct mtp_packet qmp, qrmp;
		int fd;
		
		info(" robot[%d] = %d %s\n",
		     lpc,
		     rmc_config->robots.robots_val[lpc].id,
		     rmc_config->robots.robots_val[lpc].hostname);

		gc->gc_robot = &rmc_config->robots.robots_val[lpc];
		if (mygethostbyname(&saddr,
				    gc->gc_robot->hostname,
				    GOR_SERVERPORT) == 0) {
		    fatal("bad gorobot hostname: %s\n",
			  gc->gc_robot->hostname);
		}
		else if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		    fatal("socket");
		}
		else if (connect(fd,
				 (struct sockaddr *)&saddr,
				 sizeof(saddr)) == -1) {
		    fatal("robot connect");
		}
		else if ((gc->gc_handle = mtp_create_handle(fd)) == NULL) {
		    fatal("robot mtp_create_handle");
		}
		else {
		    FD_SET(fd, &readfds);
		}

		/* Get the robots initial position. */
		mtp_init_packet(&qmp,
				MA_Opcode, MTP_REQUEST_POSITION,
				MA_Role, MTP_ROLE_RMC,
				MA_RobotID, gc->gc_robot->id,
				MA_TAG_DONE);
		if (mtp_send_packet(emc_handle, &qmp) != MTP_PP_SUCCESS) {
		    fatal("request id failed");
		}
		else if (mtp_receive_packet(emc_handle, &qrmp) !=
			 MTP_PP_SUCCESS) {
		    fatal("request id response failed");
		}
		else if (qrmp.data.opcode != MTP_UPDATE_POSITION) {
		    fatal("request didn't evoke an update? %d\n",
			  qrmp.data.opcode);
		}
		else {
		    gc->gc_actual_pos =
			qrmp.data.mtp_payload_u.update_position.position;
		    gc->gc_flags |= GCF_VISION_POSITION;

		    info(" robot[%d].x = %f\n", lpc, gc->gc_actual_pos.x);
		    info(" robot[%d].y = %f\n", lpc, gc->gc_actual_pos.y);
		    info(" robot[%d].theta = %f\n",
			 lpc,
			 gc->gc_actual_pos.theta);
		}

		mtp_free_packet(&qrmp);
		mtp_free_packet(&qmp);
	    }
	}
    }

    while (looping) {
	fd_set rreadyfds = readfds;
	int rc;

	rc = select(FD_SETSIZE, &rreadyfds, NULL, NULL, NULL);

	if (rc > 0) {
	    int lpc;
	    
	    if (FD_ISSET(emc_fd, &rreadyfds)) {
		handle_emc_packet(emc_handle);
	    }

	    for (lpc = 0; lpc < rmc_config->robots.robots_len; lpc++) {
		struct gorobot_conn *gc = &gorobot_connections[lpc];
		
		if (FD_ISSET(gc->gc_handle->mh_fd, &rreadyfds)) {
		    handle_gc_packet(gc, emc_handle);
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
