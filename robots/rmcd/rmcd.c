/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* Robot Master Control Daemon
 */

#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/time.h>

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
#include "obstacles.h"
#include "pilotConnection.h"

#define DEFAULT_MAX_REFINE_RETRIES 4
#define DEFAULT_METER_TOLERANCE 0.0125f
#define DEFAULT_RADIAN_TOLERANCE 0.04f
#define DEFAULT_MAX_DISTANCE 1.5f

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

int debug = 0;

/**
 *
 */
static volatile int looping = 1;

static struct mtp_config_rmc *rmc_config = NULL;

extern char *statsfile;

FILE *plogfilep = NULL;

/**
 * Print the usage message for rmcd.
 */
static void usage(void)
{
    fprintf(stderr,
	    "Usage: rmcd [OPTIONS]\n"
	    "Options:\n"
	    "  -h\t\tPrint this message\n"
	    "  -d\t\tIncrease debugging level and do not daemonize\n"
	    "  -n enable nonlinear controller for posture regulation\n"
	    "  -l logfile\tLog file name\n"
	    "  -k packet logfile\tPacket Log file name\n"
	    "  -i pidfile\tPid file name\n"
	    "  -s statsfile\tStatistics file name\n"
	    "  -e host\tThe hostname where emcd is running\n"
	    "  -p port\tThe port where emcd is listening\n"
	    "  -U path\tPath to unix domain socket where emcd is listening\n"
	    "  -t tries\tMax number of times to retry reaching a point "
	    "(Default: %d)\n"
	    "  -m tolerance\tDistance tolerance in meters (Default: %.2f)\n"
	    "  -r tolerance\tRotation tolerance in radians (Default: %.2f)\n"
	    "  -a distance\tMax distance for a single line segment "
	    "(Default: %.2f)\n"
	    "\n"
	    "Version: %s\n",
	    DEFAULT_MAX_REFINE_RETRIES,
	    DEFAULT_METER_TOLERANCE,
	    DEFAULT_RADIAN_TOLERANCE,
	    DEFAULT_MAX_DISTANCE,
	    build_info);
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

struct timeval tv_last = {0,0};
struct timeval tv_cur = {0,0};


/**
 * Handle an mtp packet from emcd.
 *
 * @param emc_handle The connection to emcd.
 */
static void handle_emc_packet(mtp_handle_t emc_handle)
{
    struct mtp_packet mp;
    double tf_last, tf_cur, tf_diff, tf_lat;



    assert(emc_handle != NULL);

    if (mtp_receive_packet(emc_handle, &mp) != MTP_PP_SUCCESS) {
	fatal("bad packet from emc!\n");
    }
    else {
	struct mtp_command_goto *mcg = &mp.data.mtp_payload_u.command_goto;
	struct mtp_update_position *mup =
	    &mp.data.mtp_payload_u.update_position;
	struct mtp_wiggle_request *mwr = &mp.data.mtp_payload_u.wiggle_request;
	struct pilot_connection *pc = NULL;

	if (debug > 1) {
	    fprintf(stderr, "emc packet: ");
	    mtp_print_packet(stderr, &mp);
	}






	switch (mp.data.opcode) {
	case MTP_COMMAND_GOTO:
	    pc = pc_find_pilot(mcg->robot_id);
	    break;
	case MTP_WIGGLE_REQUEST:
	    pc = pc_find_pilot(mwr->robot_id);
	    break;
	case MTP_UPDATE_POSITION:
	    pc = pc_find_pilot(mup->robot_id);

	    /* timestamp */
	    tv_last = tv_cur;
	    gettimeofday(&tv_cur, NULL);

	    tf_last = (double)(tv_last.tv_sec) + (double)(tv_last.tv_usec) / 1000000.0;
	    tf_cur = (double)(tv_cur.tv_sec) + (double)(tv_cur.tv_usec) / 1000000.0;

	    tf_diff = tf_cur - tf_last;
	    tf_lat = tf_cur - mup->position.timestamp;

	    if (plogfilep != NULL) {
		fprintf(plogfilep, "%f %f\n", tf_diff, tf_lat);
	    }


	    break;

	case MTP_COMMAND_STOP:
	    assert(0);
	    break;
	default:
	    warning("unhandled emc packet %d\n", mp.data.opcode);
	    break;
	}




	if (pc != NULL) {
	    pc_handle_emc_packet(pc, &mp);
	}
    }

    mtp_free_packet(&mp);
}

int main(int argc, char *argv[])
{
    char *emc_hostname = NULL, *emc_path = NULL;
    int c, emc_port = 0, retval = EXIT_SUCCESS;
    char *logfile = NULL, *plogfile = NULL, *pidfile = NULL;
    mtp_handle_t emc_handle = NULL;
    struct timeval tv, next_time;
    struct mtp_packet rmp;
    time_t start_time;

    ob_init();

    /* set default tolerances */
    mc_data.mcd_max_refine_retries = DEFAULT_MAX_REFINE_RETRIES;
    mc_data.mcd_meter_tolerance = DEFAULT_METER_TOLERANCE;
    mc_data.mcd_radian_tolerance = DEFAULT_RADIAN_TOLERANCE;
    pp_data.ppd_max_distance = DEFAULT_MAX_DISTANCE;

    while ((c = getopt(argc, argv, "hdnp:l:k:i:e:c:U:t:m:r:a:s:")) != -1) {
	switch (c) {
	case 'h':
	    usage();
	    exit(0);
	    break;
	case 'd':
	    debug += 1;
	    break;
	case 'n':
	    /* nonlinear posture regulator */
	    break;
	case 'l':
	    logfile = optarg;
	    break;
	case 'k':
	    plogfile = optarg;
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
	case 't':
	    if (sscanf(optarg, "%d", &mc_data.mcd_max_refine_retries) != 1) {
		error("-t option is not a number: %s\n", optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'm':
	    if (sscanf(optarg, "%f", &mc_data.mcd_meter_tolerance) != 1) {
		error("-m option is not a float: %s\n", optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'r':
	    if (sscanf(optarg, "%f", &mc_data.mcd_radian_tolerance) != 1) {
		error("-r option is not a float: %s\n", optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'a':
	    if (sscanf(optarg, "%f", &pp_data.ppd_max_distance) != 1) {
		error("-a option is not a float: %s\n", optarg);
		usage();
		exit(1);
	    }
	    break;
	case 'U':
	    emc_path = optarg;
	    break;
	case 's':
	    statsfile = optarg;
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

    if (plogfile) {
	if ((plogfilep = fopen(plogfile, "w")) == NULL) {
	    fprintf(stderr, "ERROR: could not open packet log file, %s\n", plogfile);
	}
    }

#if defined(SIGINFO)
    signal(SIGINFO, siginfo);
#endif

    signal(SIGPIPE, SIG_IGN);

    time(&start_time);
    info("RMCD %s\n"
	 "  start:\t%s"
	 "  max_refine_retries:\t%d\n"
	 "  meter_tolerance:\t%.2f\n"
	 "  radian_tolerance:\t%.2f\n"
	 "  max_distance:\t%.2f\n",
	 build_info,
	 ctime(&start_time),
	 mc_data.mcd_max_refine_retries,
	 mc_data.mcd_meter_tolerance,
	 mc_data.mcd_radian_tolerance,
	 pp_data.ppd_max_distance);

    if (emc_hostname != NULL || emc_path != NULL) {
	/* Connect to emcd and get the configuration. */
	if ((emc_handle = mtp_create_handle2(emc_hostname,
					     emc_port,
					     emc_path)) == NULL) {
	    pfatal("mtp_create_handle");
	}
	else if (mtp_send_packet2(emc_handle,
				  MA_Opcode, MTP_CONTROL_INIT,
				  MA_Role, MTP_ROLE_RMC,
				  MA_Message, "rmcd init",
				  MA_TAG_DONE) != MTP_PP_SUCCESS) {
	    pfatal("could not configure with emc");
	}
	else if (mtp_receive_packet(emc_handle, &rmp) != MTP_PP_SUCCESS) {
	    pfatal("could not get rmc config packet");
	}
	else {
	    int lpc;

	    mtp_print_packet(stderr, &rmp);

	    FD_SET(emc_handle->mh_fd, &pc_data.pcd_read_fds);
	    rmc_config = &rmp.data.mtp_payload_u.config_rmc;

	    ob_data.od_emc_handle = emc_handle;

	    pc_data.pcd_emc_handle = emc_handle;
	    pc_data.pcd_config = rmc_config;

	    pp_data.ppd_bounds = rmc_config->bounds.bounds_val;
	    pp_data.ppd_bounds_len = rmc_config->bounds.bounds_len;

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
	    }

	    for (lpc = 0; lpc < rmc_config->obstacles.obstacles_len; lpc++) {
		ob_add_obstacle(&rmc_config->obstacles.obstacles_val[lpc]);
	    }
	}
    }

    gettimeofday(&tv, NULL);
    next_time = tv;
    next_time.tv_sec += 1;

    while (looping) {
	fd_set rreadyfds = pc_data.pcd_read_fds;
	fd_set wreadyfds = pc_data.pcd_write_fds;
	struct timeval select_timeout;
	int rc;

#if defined(SIGINFO)
	if (got_siginfo) {
	    pc_dump_info();
	    ob_dump_info();
	    got_siginfo = 0;
	}
#endif

	timersub(&next_time, &tv, &select_timeout);
	rc = select(FD_SETSIZE, &rreadyfds, &wreadyfds, NULL, &select_timeout);

	if (rc > 0) {
	    if (FD_ISSET(emc_handle->mh_fd, &rreadyfds)) {
		do {
		    handle_emc_packet(emc_handle);
		} while (emc_handle->mh_remaining > 0);
	    }

	    pc_handle_signal(&rreadyfds, &wreadyfds);
	}
	else if (rc == -1 && errno != EINTR) {
	    errorc("select\n");
	}

	gettimeofday(&tv, NULL);
	if (timercmp(&tv, &next_time, >)) {
	    pc_handle_timeout(&tv);
	    next_time = tv;
	    next_time.tv_sec += 1;
	}
    }

    return retval;
}
