/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <float.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "log.h"
#include "mtp.h"
#include "vmcd.h"
#include "robotObject.h"
#include "visionTrack.h"

int debug = 0;

/**
 * Signal handler for SIGUSR1, just toggles the debugging level.
 *
 * @param sig The actual signal number received.
 */
static void sigdebug(int sig)
{
    if (debug == 2)
	debug = 1;
    else
	debug = 2;
}

static volatile int looping = 1;

static char *emc_hostname = NULL, *emc_path = NULL;
static int emc_port = 0;

static char *logfile = NULL;

static char *pidfile = NULL;

static mtp_handle_t emc_handle;

#define MAX_VMC_CLIENTS 16

static unsigned int vmc_client_count = 0;
static struct vmc_client vmc_clients[MAX_VMC_CLIENTS];

static struct mtp_config_vmc vmc_config;

#define TRACK_POOL_SIZE 128

static struct vision_track vt_pool_nodes[TRACK_POOL_SIZE];
static struct lnMinList vt_pool;

static unsigned long long frame_count = 0;

static struct robot_object ro_pool_nodes[16];

static struct lnMinList last_frame, current_frame, wiggle_frame;

static struct robot_object *wiggle_bot = NULL;

#define DEFAULT_SMOOTH_WINDOW 10
#define DEFAULT_SMOOTH_ALPHA 0.10
 
/* by default, smoothing is off */
typedef enum { 
    ST_NONE = 0, 
    ST_SMA, 
    ST_EWMA 
} smoothing_t;

static smoothing_t smoothing_type = ST_NONE;
static int smoothing_window = DEFAULT_SMOOTH_WINDOW;
double smoothing_alpha = DEFAULT_SMOOTH_ALPHA;

static mtp_handle_t client_dump_handle = NULL;
static mtp_handle_t frame_dump_handle = NULL;
static mtp_handle_t coalesce_dump_handle = NULL;

/**
 * Here's how the algorithm will lay out: when we get update_position packets
 * from a vmc-client, we will attempt to find a preexisting track for the given
 * position in vc_objects[n].  If there's nothing within the threshold, we 
 * start a new track for this object, and send a query to emc to get its id 
 * (this requires a request_id, which we create, and a client fd) (we continue
 * to update this new track, of course).  When the request comes back, IF there
 * is still a track for this client fd and request id, we associate the 
 * returned id with the track.  
 * Otherwise, we search for the track whose posit
 * and pose deltas are least for the new object.  If there are tracks that 
 * are not updated for some time (a two timestamp difference), we remove them.
 * The two-timestamp business: since we're getting update_position packets
 * from the vmc-clients for EACH id, we don't process everything at once.  
 * Thus, we have to ensure that all update_position packets for a specific
 * timestamp have been handled before we even try deleting stale tracks.  If
 * we place a MTP_POSITION_STATUS_CYCLE_COMPLETE in the status field for the 
 * last position update in a 
 * mezzanine cycle, we know we've seen the last packet for this iteration,
 * and we can clean out stale tracks.
 *
 * Every time we get 5 updates from each client, we update the robots array;
 * essentially, for each robot id we're supposed to know about (passed in the
 * config struct), we scan all vc_objects subarrays for the best position 
 * estimate for that id (given by confidence value, which we will add later --
 * for now, we just take the first one).
 */

/* TODO:
 * 1. fix timestamp encoding issue (change in protocol from int to double)
 * 2. add above hooks below
 * 3. add some support to emc for more vision querying and updating
 *
 * 4. test!
 *
 * 5. merge mezzanine-usb with mezzanine-current
 *
 * 6. test!
 *
 */

static void sigquit(int signal)
{
    looping = 0;
}

static void sigpanic(int sig)
{
    char buf[32];
    
    sprintf(buf, "sigpanic %d\n", sig);
    write(1, buf, strlen(buf));
    sleep(120);
    exit(1);
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

static void dump_robot(struct robot_object *ro)
{
    assert(ro != NULL);
    
    info("  %d: %s; status=%s\n",
	 ro->ro_id,
	 ro->ro_name,
	 ro_status_names[ro->ro_status]);
}

static void dump_robot_list(struct lnMinList *list)
{
    struct robot_object *ro;

    assert(list != NULL);
    
    ro = (struct robot_object *)list->lh_Head;
    while (ro->ro_link.ln_Succ != NULL) {
	dump_robot(ro);
	ro = (struct robot_object *)ro->ro_link.ln_Succ;
    }
}

static void dump_vision_track(struct vision_track *vt)
{
    int lpc;
    
    assert(vt != NULL);
    
    info("  %d: %.2f %.2f %.2f; age=%d; ts=%f; %s\n",
	 vt->vt_client->vc_port,
	 vt->vt_position.x,
	 vt->vt_position.y,
	 vt->vt_position.theta,
	 vt->vt_age,
	 vt->vt_position.timestamp,
	 (vt->vt_userdata == NULL) ?
	 "(unknown)" :
	 ((struct robot_object *)vt->vt_userdata)->ro_name);
    for (lpc = 0; lpc < vt->ma.positions_len; lpc++) {
	struct robot_position *rp;

	rp = &vt->ma.positions[lpc];
	info("\t%.2f %.2f %.2f %.3f\n",
	     rp->x, rp->y, rp->theta, rp->timestamp);
    }
}

void dump_vision_list(struct lnMinList *list)
{
    struct vision_track *vt;

    assert(list != NULL);
    
    vt = (struct vision_track *)list->lh_Head;
    while (vt->vt_link.ln_Succ != NULL) {
	dump_vision_track(vt);
	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }
}

static void dump_smoothing_info(struct lnMinList *list) {
    struct vision_track *vt;
    int i;
    
    assert(list != NULL);
    
    vt = (struct vision_track *)(list->lh_Head);
    
    while (vt->vt_link.ln_Succ != NULL) {
	if (vt->vt_userdata != NULL) {
	    struct robot_object *ro;
	    struct moving_average *ma;

	    ro = (struct robot_object *)vt->vt_userdata;
	    ma = &(vt->ma);

	    info("smooth buffer for robot id %d:\n",ro->ro_id);

	    /* print the valid positions first */
	    for (i = 0; i < ma->number_valid_positions; ++i) {
		info("(%f,%f,%f),",
		     ma->positions[i].x,
		     ma->positions[i].y,
		     ma->positions[i].theta);
	    }
	    
	    info("\n");
	}

	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }
}

static void dump_info(void)
{
    static unsigned long long last_frame_count = 0;

    int lpc;
    
    info("Frame: %qd (%qd - %qd)\n",
	 frame_count - last_frame_count,
	 frame_count,
	 last_frame_count);
    last_frame_count = frame_count;

    for (lpc = 0; lpc < vmc_client_count; lpc++) {
	struct vmc_client *vc = &vmc_clients[lpc];
	
	info(" Client: %s/%d; frames=%qd; l=%.2f r=%.2f t=%.2f b=%.2f\n",
	     vc->vc_hostname,
	     vc->vc_port,
	     vc->vc_frame_count,
	     vc->vc_left,
	     vc->vc_right,
	     vc->vc_top,
	     vc->vc_bottom);
    }

    for (lpc = 0; lpc < ROS_MAX; lpc++) {
	struct lnMinList *list = &ro_data.rd_lists[lpc];
	
	if (!lnEmptyList(list)) {
	    info(" %s robots:\n", ro_status_names[lpc]);
	    dump_robot_list(list);
	}
    }

    info(" Wiggle frame:\n");
    dump_vision_list(&wiggle_frame);
    info(" Last frame:\n");
    dump_vision_list(&last_frame);
    info(" Current frame:\n");
    dump_vision_list(&current_frame);

    info("Smoothing info:\n");
    dump_smoothing_info(&current_frame);

}

static void usage(void)
{
    fprintf(stderr,
            "Usage: vmcd [-hdS] [-l logfile] [-e emchost] [-p emcport]\n"
            "            [-c clienthost] [-P clientport] [-s #samples]\n"
            "Vision master control daemon\n"
            "  -h\tPrint this message\n"
            "  -d\tTurn on debugging messages and do not daemonize\n"
	    "  -S\tTurn on position smoothing\n"
            "  -l logfile\tSpecify the log file to use\n"
            "  -i pidfile\tSpecify the pid file name\n"
            "  -e emchost\tSpecify the host name where emc is running\n"
            "  -p emcport\tSpecify the port that emc is listening on\n"
            "  -c clienthost\tSpecify the host name where a vmc-client is\n"
            "  -P clientport\tSpecify the port the client is listening on\n"
	    "  -w #samples\tSmooth over the last #samples positions (default "
	    "             \tis %d)\n"
	    "  -a alpha for EWMA\n"
            "\n"
            "Example:\n"
            "  $ vmcd -c foo -P 7070 -- -c foo -P 7071\n",
	    DEFAULT_SMOOTH_WINDOW);
}

static int parse_client_options(int *argcp, char **argvp[])
{
    int c, fd, argc, retval = EXIT_SUCCESS;
    char **argv;
    
    assert(argcp != NULL);
    assert(argvp != NULL);
    
    argc = *argcp;
    argv = *argvp;
    
    while ((c = getopt(argc, argv, "hdSEp:U:l:i:e:c:P:w:f:F:C:")) != -1) {
        switch (c) {
	case 'S':
	    smoothing_type = ST_SMA;
	    fprintf(stdout,"DATA WILL BE SMOOTHED with SMA!\n");
	    break;
	case 'E':
	    smoothing_type = ST_EWMA;
	    fprintf(stdout,"DATA WILL BE SMOOTHED with EWMA!\n");
	    break;
	case 'w':
	    smoothing_window = atoi(optarg);
	    if (smoothing_window < 2) {
		fprintf(stdout,
			"WARNING: smoothing window must be at least 2; setting"
			" to default of %d.\n",
			DEFAULT_SMOOTH_WINDOW);
		smoothing_window = DEFAULT_SMOOTH_WINDOW;
	    }
	    break;
	case 'a':
	    smoothing_alpha = atof(optarg);
	    if (smoothing_alpha <= 0.0) {
		fprintf(stdout,
			"WARNING: smoothing alpha must be greater than 0; "
			"setting to default of %f.\n",
			DEFAULT_SMOOTH_ALPHA);
		smoothing_alpha = DEFAULT_SMOOTH_ALPHA;
	    }
	    break;
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
        case 'c':
            vmc_clients[vmc_client_count].vc_hostname = optarg;
            break;
        case 'P':
            if (sscanf(optarg,
                       "%d",
                       &vmc_clients[vmc_client_count].vc_port) != 1) {
                error("-P option is not a number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
	case 'f':
	    if ((fd = open(optarg, O_WRONLY|O_CREAT|O_TRUNC, 0664)) == -1) {
		errorc("-f option is not a valid file name: %s\n", optarg);
		usage();
		exit(1);
	    }
	    else if ((client_dump_handle = mtp_create_handle(fd)) == NULL) {
		errorc("unable to create MTP handle\n");
		exit(1);
	    }
	    break;
	case 'F':
	    if ((fd = open(optarg, O_WRONLY|O_CREAT|O_TRUNC, 0664)) == -1) {
		errorc("-F option is not a valid file name: %s\n", optarg);
		usage();
		exit(1);
	    }
	    else if ((frame_dump_handle = mtp_create_handle(fd)) == NULL) {
		errorc("unable to create MTP handle\n");
		exit(1);
	    }
	    break;
	case 'C':
	    if ((fd = open(optarg, O_WRONLY|O_CREAT|O_TRUNC, 0664)) == -1) {
		errorc("-C option is not a valid file name: %s\n", optarg);
		usage();
		exit(1);
	    }
	    else if ((coalesce_dump_handle = mtp_create_handle(fd)) == NULL) {
		errorc("unable to create MTP handle\n");
		exit(1);
	    }
	    break;
        default:
            break;
        }
    }

    if (smoothing_alpha < 0 && smoothing_type == ST_EWMA) {
	/* pick a sane value and notify user. */
	smoothing_alpha = 2/(double)(smoothing_window+1);
	fprintf(stdout,
		"WARNING: calculating EWMA alpha as 2/(window+1) = %f!\n",
		smoothing_alpha);
    }
    
    if (vmc_clients[vmc_client_count].vc_port != 0) {
        vmc_client_count += 1;
    }
    
    argc -= optind;
    argv += optind;
    
    *argcp = argc + 1;
    *argvp = argv - 1;
    
#ifndef linux
    optreset = 1;
#endif
    optind = 1;
    
    return retval;
}

static int wiggle_idle(struct robot_object *ro, mtp_packet_t *mp)
{
    int retval = 0;
    
    assert(ro != NULL);
    assert(mp != NULL);

    roMoveRobot(ro, ROS_WIGGLE_QUEUE);
    
    return retval;
}

static int wiggle_complete(struct robot_object *ro, mtp_packet_t *mp)
{
    struct vision_track *vt;
    int retval = 0;
    
    assert(ro != NULL);
    assert(mp != NULL);

    if (wiggle_bot == NULL) {
	warning("received spurious wiggle complete from %d\n", ro->ro_id);
    }
    else if ((vt = vtFindWiggle(&wiggle_frame, &last_frame)) == NULL) {
	roMoveRobot(ro, ROS_LOST);
    }
    else {

	/* zero out the smoothing data for this 
	 * track!
	 */
	info("WC: RESET ma data\n");
	vt->ma.oldest_index = 0;
	vt->ma.number_valid_positions = 0;

	vt->vt_userdata = wiggle_bot;
	roMoveRobot(ro, ROS_KNOWN);
    }
    lnAppendList(&vt_pool, &wiggle_frame);

    wiggle_bot = NULL;
    
    return retval;
}

static int wiggle_error(struct robot_object *ro, mtp_packet_t *mp)
{
    int retval = 0;
    
    assert(ro != NULL);
    assert(mp != NULL);

    if (ro->ro_status == ROS_WIGGLING) {
	lnAppendList(&vt_pool, &wiggle_frame);
	wiggle_bot = NULL;
    }
    roMoveRobot(ro, ROS_LOST);
    
    return retval;
}

static int handle_robot_packet(void *arg, mtp_packet_t *mp)
{
    struct mtp_wiggle_status *mws;
    struct robot_object *ro;
    int retval = -1;

    mws = &mp->data.mtp_payload_u.wiggle_status;
    if ((ro = roFindRobot(mws->robot_id)) == NULL) {
	error("unknown robot %d\n", mws->robot_id);
    }
    else {
	retval = mtp_dispatch(ro, mp,

			      MD_Integer, ro->ro_status,

			      MD_OnInteger, ROS_STARTED_WIGGLING,
			      MD_OnStatus, MTP_POSITION_STATUS_IDLE,
			      MD_Call, wiggle_idle,

			      MD_OnInteger, ROS_WIGGLING,
			      MD_OnStatus, MTP_POSITION_STATUS_COMPLETE,
			      MD_Call, wiggle_complete,

			      MD_OR | MD_OnStatus, MTP_POSITION_STATUS_ERROR,
			      MD_OnStatus, MTP_POSITION_STATUS_ABORTED,
			      MD_Call, wiggle_error,

			      MD_TAG_DONE);
    }
    
    return retval;
}

static int handle_emc_packet(mtp_packet_t *mp)
{
    int rc, retval = 0;

    if (debug > 0)
	mtp_print_packet(stdout, mp);
    
    rc = mtp_dispatch(NULL, mp,
		      
		      MD_OnOpcode, MTP_WIGGLE_STATUS,
		      MD_Call, handle_robot_packet,
		      
		      MD_TAG_DONE);
    
    return retval;
}

static void send_snapshots(void)
{
    int lpc;

    for (lpc = 0; lpc < vmc_client_count; lpc++) {
        struct vmc_client *vc = &vmc_clients[lpc];

	mtp_send_packet2(vc->vc_handle,
			 MA_Opcode, MTP_SNAPSHOT,
			 MA_Role, MTP_ROLE_VMC,
			 MA_TAG_DONE);
    }
}

int main(int argc, char *argv[])
{
    int lpc, current_client = 0, retval = EXIT_SUCCESS;
    time_t start_time;
    fd_set readfds;

    roInit();
    
    time(&start_time);
    info("VMCD %s\n"
	 "  start:\t%s",
	 build_info,
	 ctime(&start_time));
    
    FD_ZERO(&readfds);

    lnNewList(&vt_pool);
    for (lpc = 0; lpc < TRACK_POOL_SIZE; lpc++) {
	lnAddTail(&vt_pool, &vt_pool_nodes[lpc].vt_link);
    }

    lnNewList(&last_frame);
    lnNewList(&current_frame);
    lnNewList(&wiggle_frame);
    
    do {
        retval = parse_client_options(&argc, &argv);
    } while ((argc > 0) && strcmp(argv[0], "--") == 0);
    
 
    /* set up smoothing */
    if (smoothing_type > ST_NONE) {
	
	for (lpc = 0; lpc < TRACK_POOL_SIZE; ++lpc) {
#if 0
	    vt_pool_nodes[lpc].ma.positions = (struct robot_position *)
		malloc(sizeof(struct robot_position)*smoothing_window);
#endif
	    bzero(vt_pool_nodes[lpc].ma.positions,
		  sizeof(struct robot_position)*smoothing_window);
	    vt_pool_nodes[lpc].ma.positions_len = smoothing_window;
	    vt_pool_nodes[lpc].ma.number_valid_positions = 0;
	    vt_pool_nodes[lpc].ma.oldest_index = 0;
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
            loginit(1, "vmcd");
    }
    
    if (pidfile) {
        FILE *fp;
        
        if ((fp = fopen(pidfile, "w")) != NULL) {
            fprintf(fp, "%d\n", getpid());
            (void) fclose(fp);
        }
    }
    
    signal(SIGHUP, sigdebug);
    
    signal(SIGQUIT, sigquit);
    signal(SIGTERM, sigquit);
    signal(SIGINT, sigquit);
    
    signal(SIGSEGV, sigpanic);
    signal(SIGBUS, sigpanic);

#if defined(SIGINFO)
    signal(SIGINFO, siginfo);
#endif

    /* connect to emc and send init packet */
    /* in future, vmc will be a separate identity that emc can connect to
     * as necessary; vmc will not connect to emc.
     */
    if (emc_hostname != NULL || emc_path != NULL) {
        struct mtp_packet mp, rmp;

	mtp_init_packet(&mp,
			MA_Opcode, MTP_CONTROL_INIT,
			MA_Role, MTP_ROLE_VMC,
			MA_Message, "vmcd init",
			MA_TAG_DONE);
        if ((emc_handle = mtp_create_handle2(emc_hostname,
					     emc_port,
					     emc_path)) == NULL) {
	    pfatal("mtp_create_handle");
	}
        else if (mtp_send_packet(emc_handle, &mp) != MTP_PP_SUCCESS) {
            pfatal("could not configure with emc");
        }
        else if (mtp_receive_packet(emc_handle, &rmp) != MTP_PP_SUCCESS) {
            pfatal("could not get vmc config packet");
        }
        else {
            FD_SET(emc_handle->mh_fd, &readfds);
            vmc_config = rmp.data.mtp_payload_u.config_vmc;
            
            for (lpc = 0; lpc < vmc_config.robots.robots_len; lpc++) {
		struct robot_object *ro = &ro_pool_nodes[lpc];
		
                info(" robot[%d] = %d %s\n",
                     lpc,
                     vmc_config.robots.robots_val[lpc].id,
                     vmc_config.robots.robots_val[lpc].hostname);

		ro->ro_name = vmc_config.robots.robots_val[lpc].hostname;
		ro->ro_id = vmc_config.robots.robots_val[lpc].id;
		lnAddTail(&ro_data.rd_lists[ROS_UNKNOWN], &ro->ro_link);
		ro->ro_next = ro_data.rd_all;
		ro_data.rd_all = ro;
            }
	    
            for (lpc = 0; lpc < vmc_config.cameras.cameras_len; lpc++) {
		lnNewList(&vmc_clients[vmc_client_count].vc_frame);
		lnNewList(&vmc_clients[vmc_client_count].vc_last_frame);
		vmc_clients[vmc_client_count].vc_hostname =
		    vmc_config.cameras.cameras_val[lpc].hostname;
		vmc_clients[vmc_client_count].vc_port =
		    vmc_config.cameras.cameras_val[lpc].port;
		vmc_clients[vmc_client_count].vc_left =
		    vmc_config.cameras.cameras_val[lpc].x;
		vmc_clients[vmc_client_count].vc_top = 
		    vmc_config.cameras.cameras_val[lpc].y;
		vmc_clients[vmc_client_count].vc_right =
		    vmc_config.cameras.cameras_val[lpc].x +
		    vmc_config.cameras.cameras_val[lpc].width;
		vmc_clients[vmc_client_count].vc_bottom =
		    vmc_config.cameras.cameras_val[lpc].y +
		    vmc_config.cameras.cameras_val[lpc].height;

		vmc_client_count += 1;
		
                info(" camera[%d] = %s:%d\n",
                     lpc,
                     vmc_config.cameras.cameras_val[lpc].hostname,
                     vmc_config.cameras.cameras_val[lpc].port);
            }
        }
    }

    /* connect to all specified clients */
    for (lpc = 0; lpc < vmc_client_count; lpc++) {
	struct camera_config *cc = &vmc_config.cameras.cameras_val[lpc];
        struct vmc_client *vc = &vmc_clients[lpc];
        
        if ((vc->vc_handle = mtp_create_handle2(vc->vc_hostname,
						vc->vc_port,
						NULL)) == NULL) {
	    pfatal("mtp_create_handle");
	}
	else if (mtp_send_packet2(vc->vc_handle,
				  MA_Opcode, MTP_CONFIG_VMC_CLIENT,
				  MA_Role, MTP_ROLE_VMC,
				  MA_X, cc->fixed_x,
				  MA_Y, cc->fixed_y,
				  MA_TAG_DONE) != MTP_PP_SUCCESS) {
		pfatal("could not configure vmc-client");
	}
    }

    /*
     * The main loop consists of:
     *
     *   1. Gathering all of the tracks from the cameras;
     *   2. Coalescing the separate tracks into a single "frame" that has all
     *      of the tracks without any duplicates caused by overlapping cameras.
     *   3. Matching the tracks in the current frame to those in the last
     *      frame, and copying the robot references from the old frame into
     *      the new frame.
     *   4. Sort the list of robots into known and unknown based on whether
     *      or not they are referenced in the current frame.
     *   5. Send MTP_WIGGLE_START to any unknown robots to stop them.
     *   6. Move the current frame to the last frame and start reading tracks
     *      from the cameras again.
     *
     * Notes:
     *
     *   + We will always get a packet from the vmc-clients, if there are no
     *     tracks, it will send an error.  This behavior allows us to keep all
     *     of the cameras in sync.
     *
     *   + I don't actually sync the cameras to the same frame, so there might
     *     be a few frames difference between tracks received from one camera
     *     and another.
     *
     *   + We read the tracks from each camera in the same order each time so
     *     the coalesce operation always picks the same track when merging.
     *
     *   + XXX I don't actually do anything with lost_bots at the moment, they
     *     should be rewiggled after a short timeout.
     */

    FD_SET(vmc_clients[0].vc_handle->mh_fd, &readfds);
    
    /* handle input from emc and vmc */
    while (looping) {
        fd_set rreadyfds;
        int rc;

#if defined(SIGINFO)
	if (got_siginfo) {
	    dump_info();
	    got_siginfo = 0;
	}
#endif

	if (current_client == vmc_client_count) {
	    struct vision_track *vt;
	    struct robot_object *ro;
	    int sent_snapshot = 0;

	    /* We've got all of the camera tracks, start processing. */
	    frame_count += 1;
	    
	    vtCoalesce(&vt_pool,
		       &current_frame,
		       vmc_clients,
		       vmc_client_count); // Get rid of duplicates

	    vt = (struct vision_track *)current_frame.lh_Head;
	    while (vt->vt_link.ln_Succ != NULL) {
		if (coalesce_dump_handle) {
		    mtp_send_packet2(coalesce_dump_handle,
				     MA_Opcode, MTP_UPDATE_POSITION,
				     MA_Role, MTP_ROLE_VMC,
				     MA_RobotID, -1,
				     MA_Position, &vt->vt_position,
				     MA_Timestamp, (double)frame_count,
				     MA_Status, MTP_POSITION_STATUS_UNKNOWN,
				     MA_TAG_DONE);
		}
		vt = (struct vision_track *)vt->vt_link.ln_Succ;
	    }
	    
	    /**
	     * Remove any unknown tracks before doing the match so that they
	     * are not candidates for matching against tracks in the current
	     * frame.
	     */
	    vtUnknownTracks(&vt_pool, &last_frame);

	    /* Match the current frame to the last frame. */
	    vtMatch(&vt_pool, &last_frame, &current_frame);


	    /* (maybe) Smooth the matched bot posits */
  	    if (smoothing_type == ST_SMA) { 
  		vtSmoothSMA(&current_frame); 
	    }
	    else if (smoothing_type == ST_EWMA) {
		vtSmoothEWMA(&current_frame);
	    }


	    /* Reset the list of known/unknown bots. */
	    lnAppendList(&ro_data.rd_lists[ROS_UNKNOWN],
			 &ro_data.rd_lists[ROS_KNOWN]);

	    /*
	     * Detect the robots that have references in the current frame and
	     * send an MTP_UPDATE_POSITION.  All others are left in the
	     * unknown_bots list.
	     */
	    vt = (struct vision_track *)current_frame.lh_Head;
	    while (vt->vt_link.ln_Succ != NULL) {
		if ((ro = vt->vt_userdata) != NULL) {
		    struct mtp_packet ump;

		    roMoveRobot(ro, ROS_KNOWN);
		    mtp_init_packet(&ump,
				    MA_Opcode, MTP_UPDATE_POSITION,
				    MA_Role, MTP_ROLE_VMC,
				    MA_RobotID, ro->ro_id,
				    MA_Position, &vt->vt_position,
				    MA_Status, MTP_POSITION_STATUS_UNKNOWN,
				    MA_TAG_DONE);
		    mtp_send_packet(emc_handle, &ump);
		    if (frame_dump_handle) {
			ump.data.mtp_payload_u.update_position.position.
			    timestamp = frame_count;
			mtp_send_packet(frame_dump_handle, &ump);
		    }
		}
		
		if (debug > 2) {
		    printf("track %f %f %f - %d - %p %s\n",
			   vt->vt_position.x,
			   vt->vt_position.y,
			   vt->vt_position.theta,
			   vt->vt_age,
			   vt->vt_userdata,
			   vt->vt_userdata == NULL ?
			   "" :
			   ((struct robot_object *)vt->vt_userdata)->ro_name);
		}
		
		vt = (struct vision_track *)vt->vt_link.ln_Succ;
	    }

	    /*
	     * Send MTP_WIGGLE_STARTs for any unknown robots, this should stop
	     * them and cause an IDLE wiggle_status to come back.
	     */

	    ro = (struct robot_object *)ro_data.rd_lists[ROS_UNKNOWN].lh_Head;
	    while (ro->ro_link.ln_Succ != NULL) {
		struct robot_object *ro_next;
		
		ro_next = (struct robot_object *)ro->ro_link.ln_Succ;
		
		info("sending wiggle request for %d\n", ro->ro_id);
		dump_info();

		if (!sent_snapshot) {
		    send_snapshots();
		    sent_snapshot = 1;
		}
		
		mtp_send_packet2(emc_handle,
				 MA_Opcode, MTP_WIGGLE_REQUEST,
				 MA_Role, MTP_ROLE_VMC,
				 MA_RobotID, ro->ro_id,
				 MA_WiggleType, MTP_WIGGLE_START,
				 MA_TAG_DONE);
		
		roMoveRobot(ro, ROS_STARTED_WIGGLING);
		ro = ro_next;
	    }

	    /* Throw any frames left from the last frame in the pool and */
	    lnAppendList(&vt_pool, &last_frame);

	    /*
	     * ... move the current frame to the last frame for the next
	     * iteration.
	     */
	    lnMoveList(&last_frame, &current_frame);
	    
	    FD_SET(vmc_clients[0].vc_handle->mh_fd, &readfds);
	    current_client = 0;
	}

	/*
	 * Check which bots in the lost list have exceeded the timeout;
	 * if any have, add them back to the unknown_bots list, and they
	 * will be rewiggled once the next set of updates comes from the
	 * vision system.
	 */
	if (!lnEmptyList(&ro_data.rd_lists[ROS_LOST])) {
	    struct robot_object *ro = NULL;
	    struct robot_object *ro_next = NULL;
	    struct timeval current;

	    gettimeofday(&current,NULL);

	    ro = (struct robot_object *)ro_data.rd_lists[ROS_LOST].lh_Head;
	    while (ro->ro_link.ln_Succ != NULL) {
		if ((current.tv_sec - ro->ro_lost_timestamp.tv_sec) > 10) {
		    ro_next = (struct robot_object *)ro->ro_link.ln_Succ;
		    roMoveRobot(ro, ROS_UNKNOWN);
		    info("robot %d exceeded the lost timeout, trying to "
			 "reacquire.\n",
			 ro->ro_id
			 );
		    ro = ro_next;
		}
		else {
		    //info("robot %d did not exceed the lost timeout!\n",
		    // ro->ro_id
		    // );
		    ro = (struct robot_object *)ro->ro_link.ln_Succ;
		}
	    }
	}

	/*
	 * Check if we need to identify any bots, assuming we're not in the
	 * process of finding one.
	 */
	if ((wiggle_bot == NULL) &&
	    !lnEmptyList(&ro_data.rd_lists[ROS_WIGGLE_QUEUE])) {
	    /*
	     * Take a snapshot of the tracks with no robot attached from the
	     * last frame so we can compare it against the frame when the robot
	     * has finished its wiggle.
	     */
	    vtUnknownTracks(&wiggle_frame, &last_frame);

	    if (!lnEmptyList(&wiggle_frame)) {
		struct mtp_packet wmp;
		
		wiggle_bot = roDequeueRobot(ROS_WIGGLE_QUEUE, ROS_WIGGLING);
		
		info("starting wiggle for %s\n", wiggle_bot->ro_name);
		dump_info();
		
		mtp_init_packet(&wmp,
				MA_Opcode, MTP_WIGGLE_REQUEST,
				MA_Role, MTP_ROLE_VMC,
				MA_RobotID, wiggle_bot->ro_id,
				MA_WiggleType, MTP_WIGGLE_180_R,
				MA_TAG_DONE);
		mtp_send_packet(emc_handle, &wmp);
	    }
	}

        rreadyfds = readfds;
        rc = select(FD_SETSIZE, &rreadyfds, NULL, NULL, NULL);
        if (rc > 0) {
	    struct vmc_client *vc = &vmc_clients[current_client];
	    
            if ((emc_handle != NULL) &&
		FD_ISSET(emc_handle->mh_fd, &rreadyfds)) {
		do {
		    struct mtp_packet mp;
		    
		    if ((rc = mtp_receive_packet(emc_handle,
						 &mp)) != MTP_PP_SUCCESS) {
			fatal("bad packet from emc %d\n", rc);
		    }
		    else {
			handle_emc_packet(&mp);
		    }
		    
		    mtp_free_packet(&mp);
		} while (emc_handle->mh_remaining > 0);
	    }

	    if (FD_ISSET(vc->vc_handle->mh_fd, &rreadyfds)) {
		do {
		    struct mtp_packet mp;
		    mtp_error_t rc;
		    
		    if ((rc = mtp_receive_packet(vc->vc_handle,
						 &mp)) != MTP_PP_SUCCESS) {
			fatal("bad packet from vmc-client %d\n", rc);
		    }
		    else if (vtUpdate(&vc->vc_frame, vc, &mp, &vt_pool)) {
			vc->vc_frame_count += 1;
			
			if (vc->vc_handle->mh_remaining > 0) {
			    lnAppendList(&vt_pool, &vc->vc_frame);
			}
			else {
			    if (debug > 2) {
				info("debug: "
				     "got frame from client %s:%d\n",
				     vc->vc_hostname,
				     vc->vc_port);
			    }
			    
			    vtAgeTracks(&vc->vc_frame,
					&vc->vc_last_frame,
					&vt_pool);
			    vtCopyTracks(&vc->vc_last_frame,
					 &vc->vc_frame,
					 &vt_pool);
			    lnAppendList(&current_frame, &vc->vc_frame);
			    
			    /*
			     * Got all of the tracks from this client, clear
			     * his bit and
			     */
			    FD_CLR(vc->vc_handle->mh_fd, &readfds);
			    /* ... try to wait for the next one. */
			    current_client += 1;
			    if (current_client < vmc_client_count)
				FD_SET(vc[1].vc_handle->mh_fd, &readfds);
			}
		    }

		    if (client_dump_handle) {
			mp.data.mtp_payload_u.update_position.command_id =
			    vc - vmc_clients;
			mp.data.mtp_payload_u.update_position.position.
			    timestamp = frame_count + 1;
			mtp_send_packet(client_dump_handle, &mp);
		    }
		    
		    mtp_free_packet(&mp);
		} while (vc->vc_handle->mh_remaining > 0);
            }
        }
        else if (rc == -1 && errno != EINTR) {
            perror("select");
        }
    }
    
    return retval;
}
