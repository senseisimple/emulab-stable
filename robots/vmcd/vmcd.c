
#include <math.h>
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

static int debug = 0;

static int looping = 1;

static char *emc_hostname = NULL, *emc_path = NULL;
static int emc_port = 0;

static char *logfile = NULL;

static char *pidfile = NULL;

static mtp_handle_t emc_handle;

#define MAX_VMC_CLIENTS 10
#define MAX_TRACKED_OBJECTS 10

struct vmc_client_track {
    int updated;
    int valid;
    int robot_id;
    struct robot_position position;
    struct robot_position wiggle_expected_position;
    /* wiggle_valid indicates that the data in wiggle_expected_position 
     * actually means something -- this might not have otherwise always
     * been the case... (suppose a new vmc track is added after the wiggle
     * is started -- then the robot_id is -1, and we would consider this new
     * track as a potential wiggled object, when in fact, it is not eligible,
     * since there is no valid wiggle_expected_position)
     */
    int wiggle_valid;
    int request_id;
};

struct real_track {
    int updated;
    int valid;
    int robot_id;
    struct robot_position position;
    int vmc_client_ids[MAX_VMC_CLIENTS];
};

struct vmc_client {
    mtp_handle_t vc_handle;
    char *vc_hostname;
    int vc_port;
    int vc_fd;
    /* this is bad: this is the index into the vmc_clients global array
     * we need it so that vmc_handle_update_position can figure out
     * which vmc_client is talking to it.
     */
    int vc_idx;
    /* we store object positions from individual vmc-clients in this array */
    struct vmc_client_track tracks[MAX_TRACKED_OBJECTS];
};

static unsigned int vmc_client_count = 0;
static struct vmc_client vmc_clients[MAX_VMC_CLIENTS];

static struct mtp_config_vmc vmc_config;

/* for tracking */
static int next_request_id = 0;

int get_next_request_id() {
    return next_request_id++;
}

/* we store actual robot positions in this array */
/* these are made from updates to the individual vmc_clients' arrays */
static struct real_track real_robots[MAX_TRACKED_OBJECTS];

void vmc_handle_update_id(struct mtp_packet *p);
void vmc_handle_update_position(struct vmc_client *vc,struct mtp_packet *p);
void vmc_handle_wiggle_status(struct mtp_packet *p);
int find_real_robot_id(struct robot_position *p);
int match_position(struct robot_position *target,
		   struct robot_position *actual,
		   float max_disp,
		   float *actual_disp);
int match_orientation(struct robot_position *target,
		      struct robot_position *actual,
		      float max_disp,
		      float *actual_disp);


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
	    /* this sucks */
	    vmc_clients[vmc_client_count].vc_idx = vmc_client_count;
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
    int lpc, i, j, retval = EXIT_SUCCESS;
    struct sockaddr_in sin;
    fd_set readfds;
    
    FD_ZERO(&readfds);
    
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
            loginit(1, "vmcclient");
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

		real_robots[lpc].robot_id = 
		    vmc_config.robots.robots_val[lpc].id;

            }
            for (lpc = 0; lpc < vmc_config.cameras.cameras_len; lpc++) {
		vmc_clients[vmc_client_count].vc_hostname =
		    vmc_config.cameras.cameras_val[lpc].hostname;
		vmc_clients[vmc_client_count].vc_port =
		    vmc_config.cameras.cameras_val[lpc].port;
		vmc_client_count += 1;
		
                info(" camera[%d] = %s:%d\n",
                     lpc,
                     vmc_config.cameras.cameras_val[lpc].hostname,
                     vmc_config.cameras.cameras_val[lpc].port);
            }

	    mtp_free_packet(&rmp);
        }
    }

    /* setup real_robots */
    for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
        real_robots[i].valid = 0;
        real_robots[i].updated = 0;

        for (j = 0; j < MAX_VMC_CLIENTS; ++j) {
	    real_robots[i].vmc_client_ids[0] = 0;
        }
    }
    
    /* connect to all specified clients */
    for (lpc = 0; lpc < vmc_client_count; lpc++) {
        struct vmc_client *vc = &vmc_clients[lpc];
        
        if (mygethostbyname(&sin, vc->vc_hostname, vc->vc_port) == 0) {
            pfatal("bad vmc-client hostname: %s\n", vc->vc_hostname);
        }
        else if ((vc->vc_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            pfatal("socket");
        }
        else if (connect(vc->vc_fd,
                         (struct sockaddr *)&sin,
                         sizeof(sin)) == -1) {
            pfatal("vmc-client connect");
        }
        else if ((vc->vc_handle = mtp_create_handle(vc->vc_fd)) == NULL) {
	    pfatal("mtp_create_handle");
	}
	else {
            int i;
	    
            FD_SET(vc->vc_fd, &readfds);

            // we now init the robot_track list for this struct:
            for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
                vc->tracks[i].updated = 0;
                vc->tracks[i].valid = 0;
                vc->tracks[i].robot_id = -1;
		vc->tracks[i].request_id = -1;
            }
        }
    }

    /* handle input from emc and vmc */
    while (looping) {
        fd_set rreadyfds;
        int rc;
        
        rreadyfds = readfds;
        rc = select(FD_SETSIZE, &rreadyfds, NULL, NULL, NULL);
        if (rc > 0) {
            if ((emc_handle != NULL) &&
		FD_ISSET(emc_handle->mh_fd, &rreadyfds)) {
		do {
		    struct mtp_packet mp;
		    
		    if ((rc = mtp_receive_packet(emc_handle,
						 &mp)) != MTP_PP_SUCCESS) {
			fatal("bad packet from emc %d\n", rc);
		    }
		    else {
			/* XXX Need to handle update_id msgs here. */
			mtp_print_packet(stdout, &mp);

			switch (mp.data.opcode) {
			case MTP_UPDATE_ID:			
			    vmc_handle_update_id(&mp);
			    break;
			case MTP_WIGGLE_STATUS:
			    vmc_handle_wiggle_status(&mp);
			    break;
			default:
			    error("bogus packet from emc\n");
			    break;
			}
		    }
		    
		    mtp_free_packet(&mp);
		} while (emc_handle->mh_remaining > 0);
	    }
            
            for (lpc = 0; lpc < vmc_client_count; lpc++) {
                struct vmc_client *vc = &vmc_clients[lpc];
                
                if (FD_ISSET(vc->vc_fd, &rreadyfds)) {
		    do {
			struct mtp_packet mp;
			
			if (mtp_receive_packet(vc->vc_handle,
					       &mp) != MTP_PP_SUCCESS) {
			    fatal("bad packet from vmc-client\n");
			}
			else {
			    if (debug > 1) {
				mtp_print_packet(stdout, &mp);
				fflush(stdout);
			    }
			    
			    vmc_handle_update_position(vc, &mp);
			}
			
			mtp_free_packet(&mp);
		    } while (vc->vc_handle->mh_remaining > 0);
		    //info("done reading\n");
		}
            }
        }
        else if (rc == -1 && errno != EINTR) {
            perror("select");
        }
    }
    
    return retval;
}

void vmc_handle_update_id(struct mtp_packet *p) {
    int i,j,k;
    if (p == NULL) {
        return;
    }
    // we have to search all of the vmc_clients' tracks arrays for this
    // request_id, IF the returned robot_id isn't -1
    if (p->data.mtp_payload_u.update_id.robot_id == -1) {
	error("no robot id?\n");
        return;
    }

    info("update id %d\n", p->data.mtp_payload_u.update_id.request_id);

    // now check through all the possible tracks:
    for (i = 0; i < MAX_VMC_CLIENTS; ++i) {
        if (vmc_clients[i].vc_fd > -1) {
            struct vmc_client *vc = &(vmc_clients[i]);
            for (j = 0; j < MAX_TRACKED_OBJECTS; ++j) {
                if (vc->tracks[j].valid &&
                    vc->tracks[j].request_id ==
		    p->data.mtp_payload_u.update_id.request_id) {
                    int first_invalid_idx = -1;
		    
                    vc->tracks[j].robot_id =
			p->data.mtp_payload_u.update_id.robot_id;

                    // before we return, we should also try to update
                    // real_robots, and make a new track if one isn't there
                    // for this robot
                    // in truth, there should not be a real_robot matching
                    // this id in the list, or we would've found it... but
                    // not everything happens in perfect sync, so we have to
                    // check for this possibility
                    for (k = 0; k < MAX_TRACKED_OBJECTS; ++k) {
                        if (real_robots[k].valid == 1) {
                            if (real_robots[k].robot_id == vc->tracks[j].robot_id) {
                                // update the posit and return.
                                real_robots[k].position = vc->tracks[j].position;
                                return;
                            }
                        }
                        else if (first_invalid_idx == -1) {
                            first_invalid_idx = k;
                        }
                    }

                    // if we get here, we have to add a new track to the 
                    // real_robots list:
                    if (first_invalid_idx != -1) {
                        real_robots[first_invalid_idx].valid = 1;
                        real_robots[first_invalid_idx].robot_id = vc->tracks[j].robot_id;
                        real_robots[first_invalid_idx].position = vc->tracks[j].position;
                    }

                    return;
                }
            }
        }
    }

}

int wiggle_robot_id = -1;
int wiggle_type = -1;
#define MAX_WIGGLE_POSIT_DISP    0.05f    // 5 cm
#define MAX_WIGGLE_ORIENT_DISP   M_PI/10  // 8 deg

/* called by vmc_handle_update_position, each time a vmc_client spews data */
/* checks to see if we need to wiggle, or if we're already wiggling, etc */
void wiggle_manager() {
    int i;
    struct mtp_packet mp;

    if (wiggle_robot_id > -1) {
	/* already wiggling */
	return;
    }

    for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
	// find the first one that needs wiggling
	if (real_robots[i].valid != 1) {
	    wiggle_robot_id = real_robots[i].robot_id;
	    break;
	}
    }

    if (wiggle_robot_id == -1) {
	// nobody needs wiggling...
	return;
    }

    /* otherwise, we wiggle wiggle_robot_id */

    /* to do this, we must first stop the current motion of robot X, wait
     * for rmcd to tell us that it's stopped.  Only then may we issue the 
     * wiggle command; this ensures that we use a valid, stopped position
     * as the base of detecting a wiggle.
     */

    mtp_init_packet( &mp,
		     MA_Opcode, MTP_WIGGLE_REQUEST,
		     MA_Role, MTP_ROLE_VMC,
		     MA_RobotID, wiggle_robot_id,
		     MA_WiggleType, MTP_WIGGLE_180_R,
		     MA_TAG_DONE
		    );

    mtp_send_packet(emc_handle,&mp);

    mtp_free_packet(&mp);

    wiggle_type = MTP_WIGGLE_180_R;

    info("sent wiggle request for robot id %d\n",wiggle_robot_id);

}

void vmc_handle_wiggle_status(struct mtp_packet *p) {
    int i,j;
    
    if (p == NULL) {
	return;
    }

    switch (p->data.mtp_payload_u.wiggle_status.status) {
    /* if we have a current wiggle, and this is an MTP_POSITION_STATUS_IDLE
     * status msg, we can copy the latest positions for any candidate tracks,
     * then use these tracks as the baseline to determine which track wiggled.
     */
    /* candidate tracks are those who are not currently mapped to a robotid */
    case MTP_POSITION_STATUS_IDLE:
	{
	    info("wiggle starting for robot id %d\n",wiggle_robot_id);
	    
	    for (i = 0; i < MAX_VMC_CLIENTS; ++i) {
		for (j = 0; j < MAX_TRACKED_OBJECTS; ++j) {
		    if (vmc_clients[i].tracks[j].valid &&
			vmc_clients[i].tracks[j].robot_id == -1) {

			vmc_clients[i].tracks[j].wiggle_valid = 1;
			vmc_clients[i].tracks[j].wiggle_expected_position = 
			    vmc_clients[i].tracks[j].position;
			
			info("recorded init posit for vmc-client %d, "
			     "track %d\n",
			     i,j);

			if (wiggle_type == MTP_WIGGLE_180_R) {
			    info("client%d/track%d theta BEFORE ADD = %f\n",
				 i,j,
				 vmc_clients[i].tracks[j].position.theta
				 );
			    vmc_clients[i].tracks[j].position.theta += M_PI;
			    if (vmc_clients[i].tracks[j].position.theta >
				2*M_PI) {
				vmc_clients[i].tracks[j].position.theta -=
				    2*M_PI;
			    }
			    info("wiggle was single 180\n");
			}
			else {
			    // XXX fix for other wiggle types later...
			    error("unrecognized wiggle type, can't calc new"
				  " posit\n"
				  );
			}

		    }
		    else {
			vmc_clients[i].tracks[j].wiggle_valid = 0;
		    }
		}
	    }
	}
	break;
    /* if we have a current wiggle, and this is a MTP_POSITION_STATUS_COMPLETE
     * status msg,
     * we grab the current state of the positions for eligible tracks, and
     * determine who wiggled.  if it's equivocal in one vmc_client only,
     * we'll have to re-wiggle.  if it's equivocal between multiple 
     * vmc_clients, if the positions of the tracks indicate a match, there is
     * really no equivocation; both tracks are the ID of the robot that we 
     * wiggled.  otherwise there is equivocation and we have to re-wiggle.
     */
    case MTP_POSITION_STATUS_COMPLETE:
	{
	    int pos_val,orient_val;
	    int real_robot_idx = -1;
	    float pd,ad;
	    int single_vmc_track_match = -1;

	    for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
		if (real_robots[i].robot_id == wiggle_robot_id) {
		    real_robot_idx = i;
		    break;
		}
	    }

	    info("checking to see who wiggled for robot id %d\n",
		 wiggle_robot_id
		 );

	    /* of the tracks whose wiggle_valid bit is set, and who still
	     * have robot_id == -1, check if they underwent the wiggle
	     */
	    for (i = 0; i < MAX_VMC_CLIENTS; ++i) {
		single_vmc_track_match = -1;
		for (j = 0; j < MAX_TRACKED_OBJECTS; ++j) {
		    if (vmc_clients[i].tracks[j].valid &&
			vmc_clients[i].tracks[j].wiggle_valid &&
			vmc_clients[i].tracks[j].robot_id == -1) {

			info ("trying to match client%d/track%d\n",i,j);
			
			pos_val =
			    match_position(&(vmc_clients[i].tracks[j].wiggle_expected_position),
					   &(vmc_clients[i].tracks[j].position),
					   MAX_WIGGLE_POSIT_DISP,
					   &pd);
			orient_val = match_orientation(&(vmc_clients[i].tracks[j].wiggle_expected_position),
						       &(vmc_clients[i].tracks[j].position),
						       MAX_WIGGLE_POSIT_DISP,
						       &ad);
			
			info("pos_disp = %f; orient_disp = %f\n",
			     pd,ad
			     );
			info("pos_val = %d; orient_val = %d\n",
			     pos_val,orient_val
			     );

			if (pos_val && orient_val) {
			    if (single_vmc_track_match == -1) {
				single_vmc_track_match = j;
			    }
			    else {
				error("multiple internal vmc-client wiggle"
				      " match; must re-wiggle\n"
				      );
				single_vmc_track_match = -1;
				break;
			    }
			}

		    }
		    else {
			//info("client%d/track%d invalid\n",i,j);
			vmc_clients[i].tracks[j].wiggle_valid = 0;
		    }
		}
		if (single_vmc_track_match != -1) {
		    info("wiggle match for id %d\n",wiggle_robot_id);
		    // so we have a match...
		    // XXX make intra-vmc-client equivocation fixes later.
		    vmc_clients[i].tracks[single_vmc_track_match].wiggle_valid = 0;
		    vmc_clients[i].tracks[single_vmc_track_match].robot_id =
			wiggle_robot_id;
		    real_robots[real_robot_idx].vmc_client_ids[i] = 1;
		    real_robots[real_robot_idx].position =
			vmc_clients[i].tracks[single_vmc_track_match].position;
		    real_robots[real_robot_idx].valid = 1;
		}
	    }

	}
	break;
    default:
	error("bogus MTP_POSITION_STATUS_* while handling wiggle status "
	      "packet\n");
	break;
    }
    

}

/* we allow a track to be continued if there is a match within 5 cm */
#define DIST_THRESHOLD 0.10
/* and if pose difference is less than 45 degress */
#define POSE_THRESHOLD 0.785

void vmc_handle_update_position(struct vmc_client *vc,struct mtp_packet *p) {
    int i,j;

    float best_dist_delta = DIST_THRESHOLD;
    float best_pose_delta = POSE_THRESHOLD;
    int best_track_idx = -1;
    int first_invalid_track = -1;

    if (p == NULL || vc == NULL) {
        return;
    }

    info("THETA WAS %f\n",
	 p->data.mtp_payload_u.update_position.position.theta
	 );

    // first we run through the current track list for this vmc_client:
    for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
        if (vc->tracks[i].valid) {
            float dx,dy;
            float my_dist_delta;
            
            dx = fabsf(p->data.mtp_payload_u.update_position.position.x -
                       vc->tracks[i].position.x);
            dy = fabsf(p->data.mtp_payload_u.update_position.position.y -
                       vc->tracks[i].position.y);
            my_dist_delta = sqrt(dx*dx + dy*dy);
            //my_pose_delta = p->data.update_position->position.theta - 
            //    vc->tracks[i].position.theta;
            
            if (fabsf(my_dist_delta) > best_dist_delta) {
                continue;
            }
            //if (fabsf(my_pose_delta) > POSE_THRESHOLD ||
            //    fabsf(my_pose_delta) > best_pose_delta) {
            //    continue;
            //}
            
            // if we've gotten here, we've beat the current best!
            best_dist_delta = my_dist_delta;
            //best_pose_delta = my_pose_delta;
            best_track_idx = i;
        }
        else if (first_invalid_track == -1) {
            first_invalid_track = i;
        }
        
    }
    
    // now check to see if we found a match.
    // if we did, update the track:
    if (best_track_idx > -1) {
        //info("got a match\n");
        
        vc->tracks[best_track_idx].position =
            p->data.mtp_payload_u.update_position.position;
        // now we try to update real_robots
        if (vc->tracks[best_track_idx].robot_id != -1) {
            // we want to update the real_robots list!
            for (j = 0; j < MAX_TRACKED_OBJECTS; ++j) {
                if (real_robots[j].robot_id ==
                    vc->tracks[best_track_idx].robot_id) {
                    real_robots[j].position =
                        vc->tracks[best_track_idx].position;
                    real_robots[j].updated = 1;
                }
            }
        }
        
        vc->tracks[best_track_idx].updated = 1;
    }
    else {
        // need to start a new track
        if (first_invalid_track == -1) {
            // no more room to store tracks!
            printf("OUT OF VC TRACKING ROOM IN VMCD!\n");
        }
        else {
            int retval;
            
            vc->tracks[first_invalid_track].valid = 1;
            vc->tracks[first_invalid_track].position =
                p->data.mtp_payload_u.update_position.position;
            vc->tracks[first_invalid_track].robot_id = -1;
            vc->tracks[first_invalid_track].request_id = -1;
            
            // now, if we can find a match for this track close enough to
            // a track in the real_robots list, we can steal the robot_id from
            // there!
            retval = find_real_robot_id(&(vc->tracks[first_invalid_track].position));
            if (retval == -1) {
              /* now, instead of sending request_id packets, we can simply
               * send wiggle packets...
               */
              /* as soon as the wiggle request is received in rmcd, it will
               * terminate whatever behavior that robot is currently undergoing
               * so that I can assume that the wiggle will start right away.
               * this way, rmcd doesn't have to notify me to start watching
               * for a wiggle -- I'll just start looking.
               */

              /* actually, we don't send a wiggle; we just mark this track
               * as unidentified
               */

		vc->tracks[first_invalid_track].robot_id = -1;
		vc->tracks[first_invalid_track].wiggle_valid = -1;

/*                struct mtp_packet mp; */
/*                mtp_init_packet(&mp, */
/*                                MA_Opcode, MTP_WIGGLE_REQUEST, */
/*                                MA_Role, MTP_ROLE_VMC, */
/*                                MA_RobotID, "vmcd init", */
/*                                MA_TAG_DONE); */
              


/*                  // we have to send our request_id packet, now: */
/*                  struct mtp_packet mp; */
/*                  int request_id; */
                
/*                  request_id = get_next_request_id(); */
/*                  mtp_init_packet(&mp, */
/*                                  MA_Opcode, MTP_REQUEST_ID, */
/*                                  MA_Role, MTP_ROLE_VMC, */
/*                                  MA_Position, &vc->tracks[first_invalid_track]. */
/*                                  position, */
/*                                  MA_RequestID, request_id, */
/*                                  MA_TAG_DONE); */
/*                  retval = mtp_send_packet(emc_handle, &mp); */
/*                  if (retval == MTP_PP_SUCCESS) { */
/*                    info("sent request %d\n", request_id); */
/*                    vc->tracks[first_invalid_track].request_id = request_id; */
/*                  } */
/*                  else { */
/*                    vc->tracks[first_invalid_track].valid = 0; */
/*                  } */
            }
            else {
              // we can update the real_robots list!
              real_robots[retval].position =
                p->data.mtp_payload_u.update_position.position;
              real_robots[retval].updated = 1;
              vc->tracks[first_invalid_track].robot_id =
                real_robots[retval].robot_id;
	      real_robots[retval].vmc_client_ids[vc->vc_idx] = 1;
            }
            
            vc->tracks[first_invalid_track].updated = 1;
            
        }
    }
    
    // now, if the update_position.status field is MTP_POSITION_STATUS_CYCLE_
    // COMPLETE, we want to go though and weed out any valid tracks that
    // were not updated:
    if (p->data.mtp_payload_u.update_position.status == MTP_POSITION_STATUS_CYCLE_COMPLETE) {
        for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
            if (vc->tracks[i].valid && !(vc->tracks[i].updated)) {
                vc->tracks[i].valid = -1;
		if (vc->tracks[i].robot_id > -1) {
		    int rr_idx = -1;
		    for (j = 0; j < MAX_TRACKED_OBJECTS; ++j) {
			if (real_robots[j].robot_id ==
			    vc->tracks[i].robot_id) {
			    rr_idx = j;
			    break;
			}
		    }
		    if (rr_idx > -1) {
			real_robots[rr_idx].vmc_client_ids[vc->vc_idx] = 0;
		    }
		    vc->tracks[i].robot_id = -1;
		}
            }
            else if (vc->tracks[i].valid && vc->tracks[i].robot_id != -1) {
		struct mtp_packet mp;
		
                info("sending update for %d (idx %d)-> %f %f\n",
                     vc->tracks[i].robot_id,
                     i,
                     vc->tracks[i].position.x,
                     vc->tracks[i].position.y);
                
		mtp_init_packet(&mp,
				MA_Opcode, MTP_UPDATE_POSITION,
				MA_Role, MTP_ROLE_VMC,
				MA_RobotID, vc->tracks[i].robot_id,
				MA_Position, &vc->tracks[i].position,
				MA_Status, MTP_POSITION_STATUS_UNKNOWN,
				MA_TAG_DONE);
		
                mtp_send_packet(emc_handle, &mp);
            }
        }

        // now we null out all teh updated bits and let the next cycle remove
        // dups
        for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
            vc->tracks[i].updated = 0;
        }

    }
    
    // finis!

    // now we call wiggle_manager to do some housekeeping and see
    // if we need to wiggle any of the robots (i.e., if no vmc_clients can
    // see one of the real_robots
    wiggle_manager();

}

int match_position(struct robot_position *target,
		   struct robot_position *actual,
		   float max_disp,
		   float *actual_disp) {
    float dx,dy,disp;
    
    dx = actual->x - target->x;
    dy = actual->y - target->y;
    
    disp = sqrt(dx*dx + dy*dy);
    *actual_disp = disp;

    if (fabsf(disp) <= max_disp)
	return 1;
    else
	return 0;
}

int match_orientation(struct robot_position *target,
		      struct robot_position *actual,
		      float max_disp,
		      float *actual_disp) {

    info("actual_theta = %f; target_theta  = %f\n",
	 actual->theta,target->theta
	 );
    *actual_disp = actual->theta - target->theta;

    if (fabsf(*actual_disp) <= max_disp)
	return 1;
    else
	return 0;
}

int find_real_robot_id(struct robot_position *p) {
    int i;

    // we look through real_robots:
    float best_dist_delta = DIST_THRESHOLD;
    float best_pose_delta = POSE_THRESHOLD;
    int best_track_idx = -1;

    if (p == NULL) {
        return -1;
    }

    for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
        if (real_robots[i].valid) {
            float dx,dy;
            float my_dist_delta;
            float my_pose_delta;

            dx = p->x - real_robots[i].position.x;
            dy = p->y - real_robots[i].position.y;
            my_dist_delta = sqrt(dx*dx + dy*dy);
            my_pose_delta = p->theta - real_robots[i].position.theta;

            if (fabsf(my_dist_delta) > DIST_THRESHOLD || 
                fabsf(my_dist_delta) > best_dist_delta) {
                continue;
            }
            if (fabsf(my_pose_delta) > POSE_THRESHOLD ||
                fabsf(my_pose_delta) > best_pose_delta) {
                continue;
            }

            // if we've gotten here, we've beat the current best!
            best_dist_delta = my_dist_delta;
            best_pose_delta = my_pose_delta;
            best_track_idx = i;
        }   
    }

    return best_track_idx;
}
