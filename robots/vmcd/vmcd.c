
#include <math.h>
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

static struct robot_object ro_pool_nodes[16];

static struct lnMinList known_bots, unknown_bots, wiggling_bots, lost_bots;
static struct lnMinList last_frame, current_frame, wiggle_frame;

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

static void dump_robot(struct robot_object *ro)
{
    info("  %d: %s; flags=%x\n", ro->ro_id, ro->ro_name, ro->ro_flags);
}

static void dump_robot_list(struct lnMinList *list)
{
    struct robot_object *ro;
    
    ro = (struct robot_object *)list->lh_Head;
    while (ro->ro_link.ln_Succ != NULL) {
	dump_robot(ro);
	ro = (struct robot_object *)ro->ro_link.ln_Succ;
    }
}

static void dump_vision_track(struct vision_track *vt)
{
    info("  %d: %.2f %.2f %.2f; age=%d; %s\n",
	 vt->vt_client->vc_port,
	 vt->vt_position.x,
	 vt->vt_position.y,
	 vt->vt_position.theta,
	 vt->vt_age,
	 (vt->vt_userdata == NULL) ?
	 "(unknown)" :
	 ((struct robot_object *)vt->vt_userdata)->ro_name);
}

static void dump_vision_list(struct lnMinList *list)
{
    struct vision_track *vt;
    
    vt = (struct vision_track *)list->lh_Head;
    while (vt->vt_link.ln_Succ != NULL) {
	dump_vision_track(vt);
	vt = (struct vision_track *)vt->vt_link.ln_Succ;
    }
}

static void dump_info(void)
{
    info("Unknown robots:\n");
    dump_robot_list(&unknown_bots);
    info("Wiggling robots:\n");
    dump_robot_list(&wiggling_bots);
    info("Lost robots:\n");
    dump_robot_list(&lost_bots);

    info("Wiggle frame:\n");
    dump_vision_list(&wiggle_frame);
    info("Last frame:\n");
    dump_vision_list(&last_frame);
    info("Current frame:\n");
    dump_vision_list(&current_frame);
}
#endif

static void usage(void)
{
    fprintf(stderr,
            "Usage: vmcd [-hd] [-l logfile] [-e emchost] [-p emcport]\n"
            "            [-c clienthost] [-P clientport]\n"
            "Vision master control daemon\n"
            "  -h\tPrint this message\n"
            "  -d\tTurn on debugging messages and do not daemonize\n"
            "  -l logfile\tSpecify the log file to use\n"
            "  -i pidfile\tSpecify the pid file name\n"
            "  -e emchost\tSpecify the host name where emc is running\n"
            "  -p emcport\tSpecify the port that emc is listening on\n"
            "  -c clienthost\tSpecify the host name where a vmc-client is\n"
            "  -P clientport\tSpecify the port the client is listening on\n"
            "\n"
            "Example:\n"
            "  $ vmcd -c foo -P 7070 -- -c foo -P 7071\n");
}

static int parse_client_options(int *argcp, char **argvp[])
{
    int c, argc, retval = EXIT_SUCCESS;
    char **argv;
    
    assert(argcp != NULL);
    assert(argvp != NULL);
    
    argc = *argcp;
    argv = *argvp;
    
    while ((c = getopt(argc, argv, "hdp:U:l:i:e:c:P:")) != -1) {
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
        default:
            break;
        }
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

int main(int argc, char *argv[])
{
    int lpc, current_client = 0, retval = EXIT_SUCCESS;
    struct robot_object *wiggle_bot = NULL;
    fd_set readfds;
    
    FD_ZERO(&readfds);

    lnNewList(&vt_pool);
    for (lpc = 0; lpc < TRACK_POOL_SIZE; lpc++) {
	lnAddTail(&vt_pool, &vt_pool_nodes[lpc].vt_link);
    }

    lnNewList(&last_frame);
    lnNewList(&current_frame);
    lnNewList(&wiggle_frame);
    
    lnNewList(&known_bots);
    lnNewList(&unknown_bots);
    lnNewList(&wiggling_bots);
    lnNewList(&lost_bots);
    
    do {
        retval = parse_client_options(&argc, &argv);
    } while ((argc > 0) && strcmp(argv[0], "--") == 0);
    
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
                info(" robot[%d] = %d %s\n",
                     lpc,
                     vmc_config.robots.robots_val[lpc].id,
                     vmc_config.robots.robots_val[lpc].hostname);

		ro_pool_nodes[lpc].ro_name =
		    vmc_config.robots.robots_val[lpc].hostname;
		ro_pool_nodes[lpc].ro_id =
		    vmc_config.robots.robots_val[lpc].id;
		lnAddTail(&unknown_bots, &ro_pool_nodes[lpc].ro_link);
            }
	    
            for (lpc = 0; lpc < vmc_config.cameras.cameras_len; lpc++) {
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
        struct vmc_client *vc = &vmc_clients[lpc];
        
        if ((vc->vc_handle = mtp_create_handle2(vc->vc_hostname,
						vc->vc_port,
						NULL)) == NULL) {
	    pfatal("mtp_create_handle");
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
	    struct vision_track *vt, *vt_unknown = NULL;
	    int unknown_track_count = 0;
	    struct robot_object *ro;

	    /* We've got all of the camera tracks, start processing. */
	    
	    vtCoalesce(&vt_pool,
		       &current_frame,
		       vmc_clients,
		       vmc_client_count); // Get rid of duplicates

	    /*
	     * Match the current frame to the last frame and throw any old
	     * frames (vt_age > 5) in the pool.
	     */
	    vtMatch(&vt_pool, &last_frame, &current_frame);

	    /* Reset the list of known/unknown bots. */
	    lnAppendList(&unknown_bots, &known_bots);

	    /*
	     * Detect the robots that have references in the current frame and
	     * send an MTP_UPDATE_POSITION.  All others are left in the
	     * unknown_bots list.
	     */
	    vt = (struct vision_track *)current_frame.lh_Head;
	    while (vt->vt_link.ln_Succ != NULL) {
		if ((ro = vt->vt_userdata) != NULL) {
		    struct mtp_packet ump;
		    
		    lnRemove(&ro->ro_link);
		    lnAddTail(&known_bots, &ro->ro_link);
		    mtp_init_packet(&ump,
				    MA_Opcode, MTP_UPDATE_POSITION,
				    MA_Role, MTP_ROLE_VMC,
				    MA_RobotID, ro->ro_id,
				    MA_Position, &vt->vt_position,
				    MA_Status, MTP_POSITION_STATUS_UNKNOWN,
				    MA_TAG_DONE);
		    mtp_send_packet(emc_handle, &ump);
		}
		else {
		    unknown_track_count += 1;
		    vt_unknown = vt;
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

#if 0
	    if ((lnCountNodes(&unknown_bots) == 1) && (vt_unknown != NULL)) {
		ro = (struct robot_object *)lnRemHead(&unknown_bots);
		lnAddTail(&known_bots, &ro->ro_link);
		vt_unknown->vt_userdata = ro;
	    }
#endif

	    /*
	     * Send MTP_WIGGLE_STARTs for any unknown robots, this should stop
	     * them and cause an IDLE wiggle_status to come back.
	     */

	    ro = (struct robot_object *)unknown_bots.lh_Head;
	    while (ro->ro_link.ln_Succ != NULL) {
		if (!(ro->ro_flags & RF_WIGGLING)) {
		    struct mtp_packet wmp;

		    info("sending wiggle request for %d\n", ro->ro_id);
		    
		    mtp_init_packet(&wmp,
				    MA_Opcode, MTP_WIGGLE_REQUEST,
				    MA_Role, MTP_ROLE_VMC,
				    MA_RobotID, ro->ro_id,
				    MA_WiggleType, MTP_WIGGLE_START,
				    MA_TAG_DONE);
		    mtp_send_packet(emc_handle, &wmp);

		    ro->ro_flags |= RF_WIGGLING;
		}
		ro = (struct robot_object *)ro->ro_link.ln_Succ;
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
	if (!lnEmptyList(&lost_bots)) {
	    struct robot_object *ro = NULL;
	    struct robot_object *ro_next = NULL;
	    struct timeval current;

	    gettimeofday(&current,NULL);

	    ro = (struct robot_object *)lost_bots.lh_Head;
	    while (ro->ro_link.ln_Succ != NULL) {
		if ((current.tv_sec - ro->ro_lost_timestamp.tv_sec) > 10) {
		    // remove this guy
		    ro_next = (struct robot_object *)ro->ro_link.ln_Succ;
		    lnRemove(&ro->ro_link);
		    // add him to unknown list
		    lnAddTail(&unknown_bots,&ro->ro_link);
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
	if ((wiggle_bot == NULL) && !lnEmptyList(&wiggling_bots)) {
	    /*
	     * Take a snapshot of the tracks with no robot attached from the
	     * last frame so we can compare it against the frame when the robot
	     * has finished its wiggle.
	     */
	    vtUnknownTracks(&wiggle_frame, &last_frame);

	    if (!lnEmptyList(&wiggle_frame)) {
		struct mtp_packet wmp;
		
		wiggle_bot = (struct robot_object *)lnRemHead(&wiggling_bots);
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
			struct mtp_wiggle_status *mws;
			struct vision_track *vt;
			struct robot_object *ro;
			
			if (debug > 0) {
			    mtp_print_packet(stdout, &mp);
			}

			switch (mp.data.opcode) {
			case MTP_WIGGLE_STATUS:
			    mws = &mp.data.mtp_payload_u.wiggle_status;
			    ro = roFindRobot(&unknown_bots, mws->robot_id);
			    switch (mws->status) {
			    case MTP_POSITION_STATUS_IDLE:
				/* Got a response to an MTP_WIGGLE_START. */
				if (ro == NULL) {
				    fatal("unknown bot %d\n", mws->robot_id);
				}
				else {
				    /*
				     * Add him to the list of robots ready
				     * to be wiggled.
				     */
				    lnRemove(&ro->ro_link);
				    lnAddTail(&wiggling_bots, &ro->ro_link);
				    ro->ro_flags &= ~RF_WIGGLING;
				}
				break;
				
			    case MTP_POSITION_STATUS_COMPLETE:
				/*
				 * The robot finished his wiggle, check the
				 * current frame against the snapshot we took
				 * to find a robot in the same position, but
				 * with a 180 degree difference in orientation.
				 */
				if ((vt = vtFindWiggle(&wiggle_frame,
						       &last_frame)) == NULL) {
				    /*
				     * Couldn't locate the robot, add him to
				     * the list of "lost" robots which we'll
				     * try to rediscover after a short time.
				     */
				    lnAddTail(&lost_bots,
					      &wiggle_bot->ro_link);

				    gettimeofday(&(wiggle_bot->ro_lost_timestamp),
						 NULL);
				    info("took a lost timestamp for robot"
					 " %d\n",
					 wiggle_bot->ro_id
					 );

				    wiggle_bot = NULL;
				}
				else {
				    info("found wiggle ! %p\n", vt);
				    vt->vt_userdata = wiggle_bot;
				    lnAddTail(&known_bots,
					      &wiggle_bot->ro_link);
				    wiggle_bot = NULL;
				}
				/*
				 * Return the frame we snapshotted awhile ago
				 * to the pool.
				 */
				lnAppendList(&vt_pool, &wiggle_frame);
				break;
				
			    default:
				info("got an unknown wiggle status!\n");

				if ((wiggle_bot != NULL) &&
				    (wiggle_bot->ro_id == mws->robot_id)) {
				    ro = wiggle_bot;
				    wiggle_bot = NULL;
				}
				else if (ro != NULL) {
				    lnRemove(&ro->ro_link);
				}
				else if ((ro = roFindRobot(&wiggling_bots,
							   mws->robot_id)) != NULL) {
				    lnRemove(&ro->ro_link);
				}
				if (ro != NULL) {
				    gettimeofday(&(ro->ro_lost_timestamp), NULL);
				    lnAddTail(&lost_bots, &ro->ro_link);
				}
				break;
			    }
			    break;
			default:
			    break;
			}
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
		    else {
			if (debug > 2) {
			    mtp_print_packet(stdout, &mp);
			    fflush(stdout);
			}
			
			if (vtUpdate(&current_frame, vc, &mp, &vt_pool)) {
			    if (debug > 2) {
				printf("got frame from client %s:%d\n",
				       vc->vc_hostname,
				       vc->vc_port);
			    }

			    /*
			     * Got all of the tracks from this client, clear
			     * his bit and
			     */
			    FD_CLR(vc->vc_handle->mh_fd, &readfds);
			    /* ... try to wait for the next one. */
			    current_client += 1;
			    if (current_client < vmc_client_count)
				FD_SET(vc[1].vc_handle->mh_fd, &readfds);
			    break; // XXX
			}
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
