
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

static char *emc_hostname = NULL;
static int emc_port = 0;

static char *logfile = NULL;

static char *pidfile = NULL;

#define MAX_VMC_CLIENTS 10
#define MAX_TRACKED_OBJECTS 10

struct robot_track {
    int updated;
    int valid;
    int robot_id;
    struct position position;
    int request_id;
};

struct vmc_client {
    char *vc_hostname;
    int vc_port;
    int vc_fd;
    /* we store object positions from individual vmc-clients in this array */
    struct robot_track tracks[MAX_TRACKED_OBJECTS];
};

static unsigned int vmc_client_count = 0;
static struct vmc_client vmc_clients[MAX_VMC_CLIENTS];

static struct mtp_config_vmc *vmc_config = NULL;

/* for tracking */
static int next_request_id = 0;

int get_next_request_id() {
    return next_request_id++;
}

/* we store actual robot positions in this array */
/* these are made from updates to the individual vmc_clients' arrays */
static struct robot_track real_robots[MAX_TRACKED_OBJECTS];

void vmc_handle_update_id(struct mtp_packet *p);
void vmc_handle_update_position(struct vmc_client *vc,struct mtp_packet *p);
int find_real_robot_id(struct position *p);


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
    
    optreset = 1;
    optind = 1;
    
    return retval;
}

/* promoted to global so that handlers can access it */
int emc_fd;

int main(int argc, char *argv[])
{
    int lpc, i , retval = EXIT_SUCCESS;
    struct sockaddr_in sin;
    fd_set readfds;

    emc_fd = 0;
    
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
    
    /* connect to emc and send init packet */
    /* in future, vmc will be a separate identity that emc can connect to
     * as necessary; vmc will not connect to emc.
     */
    if (emc_hostname != NULL) {
        struct mtp_packet *mp = NULL, *rmp = NULL;
        struct mtp_control mc;
        
        mc.id = -1;
        mc.code = -1;
        mc.msg = "vmcd init";
        if (mygethostbyname(&sin, emc_hostname, emc_port) == 0) {
            pfatal("bad emc hostname: %s\n", emc_hostname);
        }
        else if ((emc_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            pfatal("socket");
        }
        else if (connect(emc_fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
            pfatal("connect");
        }
        else if ((mp = mtp_make_packet(MTP_CONTROL_INIT,
                                       MTP_ROLE_VMC,
                                       &mc)) == NULL) {
            pfatal("mtp_make_packet");
        }
        else if (mtp_send_packet(emc_fd, mp) != MTP_PP_SUCCESS) {
            pfatal("could not configure with emc");
        }
        else if (mtp_receive_packet(emc_fd, &rmp) != MTP_PP_SUCCESS) {
            pfatal("could not get vmc config packet");
        }
        else {
            FD_SET(emc_fd, &readfds);
            vmc_config = rmp->data.config_vmc;
            
            for (lpc = 0; lpc < vmc_config->num_robots; lpc++) {
                info(" robot[%d] = %d %s\n",
                     lpc,
                     vmc_config->robots[lpc].id,
                     vmc_config->robots[lpc].hostname);
            }
        }
        
        free(mp);
        mp = NULL;
    }

    /* setup real_robots */
    for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
        real_robots[i].valid = 0;
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
            pfatal("connect");
        }
        else {
            FD_SET(vc->vc_fd, &readfds);

            // we now init the robot_track list for this struct:
            int i;
            vc->updating = 0;
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
            if ((emc_fd != 0) && FD_ISSET(emc_fd, &rreadyfds)) {
                struct mtp_packet *mp = NULL;
                
                if ((rc = mtp_receive_packet(emc_fd, &mp)) != MTP_PP_SUCCESS) {
                    fatal("bad packet from emc %d\n", rc);
                }
                else {
                    /* XXX Need to handle update_id msgs here. */
                    //mtp_print_packet(stdout, mp);

                    vmc_handle_update_id(mp);
                }
                
                mtp_free_packet(mp);
                mp = NULL;
            }
            
            for (lpc = 0; lpc < vmc_client_count; lpc++) {
                struct vmc_client *vc = &vmc_clients[lpc];
                
                if (FD_ISSET(vc->vc_fd, &rreadyfds)) {
                    struct mtp_packet *mp = NULL;
                    
                    if (mtp_receive_packet(vc->vc_fd, &mp) != MTP_PP_SUCCESS) {
                        fatal("bad packet from vmc-client\n");
                    }
                    else {
                        //mtp_print_packet(stdout, mp);

                        vmc_handle_update_position(vc,mp);

                    }
                    
                    mtp_free_packet(mp);
                    mp = NULL;
                }
            }
        }
        else if (rc == -1) {
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
    if (p->data.update_id->robot_id == -1) {
        return;
    }

    // now check through all the possible tracks:
    for (i = 0; i < MAX_VMC_CLIENTS; ++i) {
        if (vmc_clients[i].fd > -1) {
            struct vmc_client *vc = &(vmc_clients[i]);
            for (j = 0; j < MAX_TRACKED_OBJECTS; ++j) {
                if (vc->tracks[j].valid &&
                    vc->tracks[j].request_id == p->data.update_id->request_id) {
                    vc->tracks[j].robot_id = p->data.update_id->robot_id;

                    // before we return, we should also try to update
                    // real_robots, and make a new track if one isn't there
                    // for this robot
                    // in truth, there should not be a real_robot matching
                    // this id in the list, or we would've found it... but
                    // not everything happens in perfect sync, so we have to
                    // check for this possibility
                    int first_invalid_idx = -1;
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
                    real_robots[first_invalid_idx].valid = 1;
                    real_robots[first_invalid_idx].robot_id = vc->tracks[j].robot_id;
                    real_robots[first_invalid_idx].position = vc->tracks[j].position;

                    return;
                }
            }
        }
    }

}
/* we allow a track to be continued if there is a match within 5 cm */
#define DIST_THRESHOLD 0.05
/* and if pose difference is less than 45 degress */
#define POSE_THRESHOLD 0.785

void vmc_handle_update_position(struct vmc_client *vc,struct mtp_packet *p) {
    int i,j;

    if (p == NULL || vc == NULL) {
        return;
    }

    // first we run through the current track list for this vmc_client:
    float best_dist_delta = DIST_THRESHOLD;
    float best_pose_delta = POSE_THRESHOLD;
    int best_track_idx = -1;
    int first_invalid_track = -1;

    for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
        if (vc->tracks[i].valid) {
            float dx,dy;
            float my_dist_delta;
            float my_pose_delta;

            dx = p->data.update_position->position.x - vc->tracks[i].position.x;
            dy = p->data.update_position->position.y - vc->tracks[i].position.y;
            my_dist_delta = sqrt(dx*dx + dy*dy);
            my_pose_delta = p->data.update_position->position.theta - 
                vc->tracks[i].position.theta;

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
        else if (first_invalid_track == -1) {
            first_invalid_track = i;
        }
            
    }

    // now check to see if we found a match.
    // if we did, update the track:
    if (best_track_idx > -1) {
        vc->tracks[best_track_idx].position = p->data.update_position->position;
        // now we try to update real_robots
        if (vc->tracks[best_track_idx].robot_id != -1) {
            // we want to update the real_robots list!
            for (j = 0; j < MAX_TRACKED_OBJECTS; ++j) {
                if (real_robots[j].robot_id == 
                    vc->tracks[best_track_idx].robot_id) {
                    real_robots[j].position = vc->tracks[best_track_idx].position;
                }
            }
        }

        vc->tracks[best_track_idx].updated = 1;
    }
    else {
        // need to start a new track and possibly issue a request_id request:
        if (first_invalid_track == -1) {
            // no more room to store tracks!
            printf("OUT OF TRACKING ROOM IN VMCD!\n");
        }
        else {
            vc->tracks[first_invalid_track].valid = 1;
            vc->tracks[first_invalid_track].position = p->data.update_position->position;
            vc->tracks[first_invalid_track].robot_id = -1;
            vc->tracks[first_invalid_track].request_id = -1;

            // now, if we can find a match for this track close enough to
            // a track in the real_robots list, we can steal the robot_id from
            // there!
            int retval = find_real_robot_id(&(vc->tracks[i].position));
            if (retval == -1) {
                // we have to send our request_id packet, now:
                struct mtp_request_id rid;
                rid.position = vc->tracks[i].position;
                rid.request_id = get_next_request_id();
                struct mtp_packet *mp = mtp_make_packet(MTP_REQUEST_ID,
                                                        MTP_ROLE_VMC,
                                                        &rid);
                retval = mtp_write_packet(emc_fd,mp);
                if (retval == MTP_PP_SUCCESS) {
                    vc->request_id = rid.request_id;
                }
                else {
                    vc->tracks[first_invalid_track].valid = 0;
                }
            }
            else {
                // we can update the real_robots list!
                real_robots[retval].position = p->data.update_position->position;
                vc->tracks[first_invalid_track].robot_id = real_robots[retval].robot_id;
            }

            vc->tracks[first_invalid_track].updated = 1;

        }
    }

    // now, if the update_position.status field is MTP_POSITION_STATUS_CYCLE_
    // COMPLETE, we want to go though and weed out any valid tracks that
    // were not updated:
    if (mp->data.update_position->status == MTP_POSITION_STATUS_CYCLE_COMPLETE) {
        for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
            if (vc->tracks[i].valid && !(vc->tracks[i].updated)) {
                vc->tracks[i].valid = -1;
            }
        }
    }

    // finis!
                                                        
            
}

int find_real_robot_id(struct position *p) {
    int i;

    if (p == NULL) {
        return -1;
    }

    // we look through real_robots:
    float best_dist_delta = DIST_THRESHOLD;
    float best_pose_delta = POSE_THRESHOLD;
    int best_track_idx = -1;

    for (i = 0; i < MAX_TRACKED_OBJECTS; ++i) {
        if (real_robots[i].valid) {
            float dx,dy;
            float my_dist_delta;
            float my_pose_delta;

            dx = p->data.update_position->position.x - real_robots[i].position.x;
            dy = p->data.update_position->position.y - real_robots[i].position.y;
            my_dist_delta = sqrt(dx*dx + dy*dy);
            my_pose_delta = p->data.update_position->position.theta - 
                real_robots[i].position.theta;

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

        
