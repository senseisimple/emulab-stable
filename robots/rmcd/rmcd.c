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
#include "rclip.h"
#include "rmcd.h"
#include "pilotConnection.h"

#define OBSTACLE_BUFFER 0.25

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

int debug = 0;

static volatile int looping = 1;

static struct mtp_config_rmc *rmc_config = NULL;

static void usage(void)
{
    fprintf(stderr,
	    "Usage: rmcd [-hd] [-l logfile] [-i pidfile] [-e emchost] [-p emcport]\n");
}

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
}
#endif

/**
 * Handle an mtp packet from emcd.
 *
 * @param emc_handle The connection to emcd.
 */
static void handle_emc_packet(mtp_handle_t emc_handle)
{
    struct mtp_packet mp;
    
    assert(emc_handle != NULL);
    
    if (mtp_receive_packet(emc_handle, &mp) != MTP_PP_SUCCESS) {
	fatal("bad packet from emc!\n");
    }
    else {
	struct mtp_command_goto *mcg = &mp.data.mtp_payload_u.command_goto;
	struct mtp_update_position *mup =
	    &mp.data.mtp_payload_u.update_position;
	struct mtp_wiggle_request *mwr = &mp.data.mtp_payload_u.wiggle_request;
	struct pilot_connection *pc;
	
	if (debug > 1) {
	    fprintf(stderr, "emc packet: ");
	    mtp_print_packet(stderr, &mp);
	}
	
	switch (mp.data.opcode) {
	case MTP_COMMAND_GOTO:
	    if ((pc = pc_find_pilot(mcg->robot_id)) == NULL) {
		error("uknnown robot %d\n", mcg->robot_id);
	    }
	    else {
		pc_set_goal(pc, &mcg->position);
	    }
	    break;

	case MTP_WIGGLE_REQUEST:
	    if ((pc = pc_find_pilot(mwr->robot_id)) == NULL) {
		error("unknown robot %d\n", mwr->robot_id);
		// also need to send crap back to VMCD so that it doesn't
		// wait for a valid wiggle complete msg
		
		mtp_init_packet(&mp,
				MA_Opcode, MTP_WIGGLE_STATUS,
				MA_Role, MTP_ROLE_RMC,
				MA_RobotID, mwr->robot_id,
				MA_Status, MTP_POSITION_STATUS_ERROR,
				MA_TAG_DONE);
		if (mtp_send_packet(emc_handle, &mp) != MTP_PP_SUCCESS) {
		    error("cannot send reply packet\n");
		}
	    }
	    else {
		pc_wiggle(pc, mwr->wiggle_type);
	    }
	    break;
	case MTP_COMMAND_STOP:
	    assert(0);
	    break;
	case MTP_UPDATE_POSITION:
	    if ((pc = pc_find_pilot(mup->robot_id)) == NULL) {
		error("uknnown robot %d\n", mup->robot_id);
	    }
	    else {
		pc_set_actual(pc, &mup->position);
	    }
	    break;
	default:
	    warning("unhandled emc packet %d\n", mp.data.opcode);
	    break;
	}
    }
    
    mtp_free_packet(&mp);
}

int main(int argc, char *argv[])
{
    char *emc_hostname = NULL, *emc_path = NULL;
    int c, emc_port = 0, retval = EXIT_SUCCESS;
    char *logfile = NULL, *pidfile = NULL;
    mtp_handle_t emc_handle = NULL;
    struct timeval last_time;
    struct mtp_packet rmp;
    fd_set readfds;

#if 0
    {
	struct robot_config rc = { 1, "fakegarcia" };
	struct pilot_connection *pc;
	struct mtp_config_rmc rmc;

	debug = 3;

	memset(&rmc, 0, sizeof(rmc));
	pc_data.pcd_config = &rmc;
	
	pc = pc_add_robot(&rc);

	pc->pc_obstacles[0].id = 1;
	pc->pc_obstacles[0].xmin = 2.0;
	pc->pc_obstacles[0].ymin = 2.0;
	pc->pc_obstacles[0].xmax = 3.0;
	pc->pc_obstacles[0].ymax = 3.0;

	pc->pc_obstacle_count += 1;

	pc->pc_actual_pos.x = 2.0;
	pc->pc_actual_pos.y = 2.7;
	pc->pc_goal_pos.x = 2.5;
	pc->pc_goal_pos.y = 3.3;

	pc_plot_waypoint(pc);

	pc->pc_flags &= ~PCF_WAYPOINT;
	pc->pc_actual_pos.x = 3.2;
	pc->pc_actual_pos.y = 2.5;
	pc->pc_goal_pos.x = 2.5;
	pc->pc_goal_pos.y = 3.3;

	pc_plot_waypoint(pc);

	pc->pc_flags &= ~PCF_WAYPOINT;
	pc->pc_actual_pos.x = 3.2;
	pc->pc_actual_pos.y = 2.5;
	pc->pc_goal_pos.x = 2.5;
	pc->pc_goal_pos.y = 1.3;

	pc_plot_waypoint(pc);

	pc->pc_flags &= ~PCF_WAYPOINT;
	pc->pc_actual_pos.x = 2.5;
	pc->pc_actual_pos.y = 1.9;
	pc->pc_goal_pos.x = 1.5;
	pc->pc_goal_pos.y = 2.7;

	pc_plot_waypoint(pc);

	pc->pc_flags &= ~PCF_WAYPOINT;
	pc->pc_actual_pos.x = 2.5;
	pc->pc_actual_pos.y = 1.3;
	pc->pc_goal_pos.x = 2.5;
	pc->pc_goal_pos.y = 3.7;

	pc_plot_waypoint(pc);

	exit(0);
    }
#else
    FD_ZERO(&readfds);

    while ((c = getopt(argc, argv, "hdp:l:i:e:c:U:")) != -1) {
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
	case 'U':
	    emc_path = optarg;
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

#if defined(SIGINFO)
    signal(SIGINFO, siginfo);
#endif

    if (emc_hostname != NULL || emc_path != NULL) {
	struct mtp_packet mp;
	
	mtp_init_packet(&mp,
			MA_Opcode, MTP_CONTROL_INIT,
			MA_Role, MTP_ROLE_RMC,
			MA_Message, "rmcd init",
			MA_TAG_DONE);

	/* Connect to emcd and get the configuration. */
	if ((emc_handle = mtp_create_handle2(emc_hostname,
					     emc_port,
					     emc_path)) == NULL) {
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

	    mtp_print_packet(stderr, &rmp);

	    FD_SET(emc_handle->mh_fd, &readfds);
	    rmc_config = &rmp.data.mtp_payload_u.config_rmc;

	    info("bounds %d\n", rmc_config->bounds.bounds_len);
	    
	    pc_data.pcd_emc_handle = emc_handle;
	    pc_data.pcd_config = rmc_config;
	    
	    /*
	     * Walk through the robot list and connect to the pilot daemons
	     * running on the robots.
	     */
	    for (lpc = 0; lpc < rmc_config->robots.robots_len; lpc++) {
		struct robot_config *rc = &rmc_config->robots.robots_val[lpc];
		struct pilot_connection *pc;
		
		info(" robot[%d] = %d %s\n", lpc, rc->id, rc->hostname);

		if ((pc = pc_add_robot(rc)) == NULL) {
		    fatal("eh?");
		}
		else {
		    FD_SET(pc->pc_handle->mh_fd, &readfds);
		}
	    }

	    for (lpc = 0; lpc < rmc_config->obstacles.obstacles_len; lpc++) {
		struct obstacle_config *oc;

		oc = &rmc_config->obstacles.obstacles_val[lpc];
		oc->xmin -= OBSTACLE_BUFFER;
		oc->ymin -= OBSTACLE_BUFFER;
		oc->xmax += OBSTACLE_BUFFER;
		oc->ymax += OBSTACLE_BUFFER;
	    }
	}
    }

    while (looping) {
	fd_set rreadyfds = readfds;
	struct timeval tv, diff;
	int rc;

#if defined(SIGINFO)
	if (got_siginfo) {
	    pc_dump_info();
	    got_siginfo = 0;
	}
#endif

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	rc = select(FD_SETSIZE, &rreadyfds, NULL, NULL, &tv);
	gettimeofday(&tv, NULL);
	
	if (rc > 0) {
	    if (FD_ISSET(emc_handle->mh_fd, &rreadyfds)) {
		do {
		    handle_emc_packet(emc_handle);
		} while (emc_handle->mh_remaining > 0);
	    }

	    pc_handle_signal(&rreadyfds, NULL);
	}
	else if (rc == -1 && errno != EINTR) {
	    errorc("select\n");
	}

	timersub(&tv, &last_time, &diff);
	if (diff.tv_sec > 1) {
	    pc_handle_timeout(&tv);
	    last_time = tv;
	}
    }
  
    return retval;
#endif
}
