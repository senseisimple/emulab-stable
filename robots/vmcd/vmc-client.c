
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "log.h"
#include "mtp.h"

#if defined(HAVE_MEZZANINE)
#include "mezz.h"
#else

#define MEZZ_MAX_OBJECTS 100

typedef struct {
    int class[2];       // Color class for the two blobs
    int valid;
    double max_disp;    // Maximum inter-frame displacement
    double max_sep;     // Maximum blob separation
    int max_missed;     // Max frames before looking for a new match
    int missed;         // Number of missed frames (object not seen)
    double px, py, pa;  // Object pose (world cs).
} mezz_object_t;

typedef struct {
    int count;
    mezz_object_t objects[MEZZ_MAX_OBJECTS];
} mezz_objectlist_t;

typedef struct {
    double time;
    mezz_objectlist_t objectlist;
} mezz_mmap_t;

#endif

#define VMCCLIENT_DEFAULT_PORT 6969

static int debug = 0;

static int looping = 1;

static unsigned int mezz_event_count = 0;

static void usage(void)
{
    fprintf(stderr,
            "Usage: vmcclient [-hd] [-p port] mezzfile\n"
            "Required arguments:\n"
            "  mezzfile\tThe name of the mezzanine shared file\n"
            "Options:\n"
            "  -h\t\tPrint this message\n"
            "  -d\t\tTurn on debugging messages and do not daemonize\n"
            "  -l logfile\tSpecify the log file\n"
            "  -p port\tSpecify the port number to listen on. (Default: %d)\n",
            VMCCLIENT_DEFAULT_PORT);
}

static void sigquit(int signal)
{
    looping = 0;
}

static void sigusr1(int signal)
{
    mezz_event_count += 1;
}

static int encode_packets(char *buffer, mezz_mmap_t *mm)
{
    struct mtp_update_position mup;
    mezz_objectlist_t *mol;
    struct mtp_packet mp;
    int lpc, retval;
    char *cursor;
    
    assert(buffer != NULL);
    assert(mm != NULL);
    
    mol = &mm->objectlist;
    
    mp.length = mtp_calc_size(MTP_UPDATE_POSITION, &mup);
    mp.opcode = MTP_UPDATE_POSITION;
    mp.version = MTP_VERSION;
    mp.role = MTP_ROLE_VMC;
    mp.data.update_position = &mup;
    
    cursor = buffer;
    for (lpc = 0; lpc < mol->count; lpc++) {
        // we don't want to send meaningless data!
        if (mol->objects[lpc].valid) {
            mup.robot_id = -1;
            mup.position.x = mol->objects[lpc].px;
            mup.position.y = mol->objects[lpc].py;
            mup.position.theta = mol->objects[lpc].pa;
            if (lpc == mol->count - 1) {
                /* this value being set tells vmc when it can delete stale
                 * tracks.
                 */
                mup.status = MTP_POSITION_STATUS_CYCLE_COMPLETE;
            }
            else {
                mup.status = MTP_POSITION_STATUS_UNKNOWN;
            }
            mup.position.timestamp = mm->time;
            
            cursor += mtp_encode_packet(&cursor, &mp);
        }
    }
    
    retval = cursor - buffer;
    
    return retval;
}

int main(int argc, char *argv[])
{
    int c, port = VMCCLIENT_DEFAULT_PORT, serv_sock, on_off = 1;
    char *mezzfile, *logfile = NULL, *pidfile = NULL;
    mezz_mmap_t *mezzmap = NULL;
    int retval = EXIT_FAILURE;
    struct sockaddr_in sin;
    struct sigaction sa;
    
    while ((c = getopt(argc, argv, "hdp:l:i:")) != -1) {
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
        case 'p':
            if (sscanf(optarg, "%d", &port) != 1) {
                error("-p option is not a number: %s\n", optarg);
                usage();
                exit(1);
            }
            break;
        default:
            break;
        }
    }
    
    argv += optind;
    argc -= optind;
    
    if (argc == 0) {
        error("missing mezzanine file argument\n");
        usage();
        exit(1);
    }
    
    signal(SIGQUIT, sigquit);
    signal(SIGTERM, sigquit);
    signal(SIGINT, sigquit);
    
    sa.sa_handler = sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
    
    mezzfile = argv[0];
    
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
    
#if defined(HAVE_MEZZANINE)
    {
        if (mezz_init(0, mezzfile) == -1) {
            errorc("unable to initialize mezzanine\n");
            exit(2);
        }
        mezzmap = mezz_mmap();
    }
#else
    {
        mezzmap = calloc(1, sizeof(mezz_mmap_t));
        
        mezzmap->time = 20.0;
        mezzmap->objectlist.count = 2;
        
        mezzmap->objectlist.objects[0].valid = 1;
        mezzmap->objectlist.objects[0].px = 2.5;
        mezzmap->objectlist.objects[0].py = 5.5;
        mezzmap->objectlist.objects[0].pa = 0.48;
        
        mezzmap->objectlist.objects[1].valid = 1;
        mezzmap->objectlist.objects[1].px = 4.5;
        mezzmap->objectlist.objects[1].py = 6.5;
        mezzmap->objectlist.objects[1].pa = 0.54;
    }
#endif
    
    if (pidfile) {
        FILE *fp;
        
        if ((fp = fopen(pidfile, "w")) != NULL) {
            fprintf(fp, "%d\n", getpid());
            (void) fclose(fp);
        }
    }
    
    memset(&sin, 0, sizeof(sin));
    sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = INADDR_ANY;
    
    if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        error("cannot create socket\n");
    }
    else if (setsockopt(serv_sock,
                        SOL_SOCKET,
                        SO_REUSEADDR,
                        &on_off,
                        sizeof(on_off)) == -1) {
        errorc("setsockopt");
    }
    else if (bind(serv_sock, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
        errorc("bind");
    }
    else if (listen(serv_sock, 5) == -1) {
        errorc("listen");
    }
    else {
        fd_set readfds, clientfds;
        int last_mezz_event = 0;
        
        FD_ZERO(&readfds);
        FD_ZERO(&clientfds);
        
        FD_SET(serv_sock, &readfds);
        
        while (looping) {
            fd_set rreadyfds;
            int rc;
            
            rreadyfds = readfds;
            rc = select(FD_SETSIZE, &rreadyfds, NULL, NULL, NULL);
            if (rc > 0) {
                int lpc;
                
                if (FD_ISSET(serv_sock, &rreadyfds)) {
                    struct sockaddr_in peer_sin;
                    socklen_t slen;
                    int fd;
                    
                    slen = sizeof(peer_sin);
                    if ((fd = accept(serv_sock,
                                     (struct sockaddr *)&peer_sin,
                                     &slen)) == -1) {
                        errorc("accept");
                    }
                    else {
                        FD_SET(fd, &readfds);
                        FD_SET(fd, &clientfds);
                    }
                }
                
                /* we assume that if somebody connected to us tries to write
                 * us some data, they're screwing up; we're just a data source
                 */
                for (lpc = 0; lpc < FD_SETSIZE; lpc++) {
                    if ((lpc != serv_sock) && FD_ISSET(lpc, &rreadyfds)) {
                        info("dead connection %d\n", lpc);
                        close(lpc);
                        FD_CLR(lpc, &readfds);
                        FD_CLR(lpc, &clientfds);
                    }
                }
            }
            else if (rc == -1) {
                switch (errno) {
                case EINTR:
                    /* this is how we check to make sure it was mezzanine
                     * who signaled us
                     */
                    if (mezz_event_count > last_mezz_event) {
                        char buffer[8192];
                        
                        if (debug) {
                            info("sending updates\n");
                        }
                        
                        if ((rc = encode_packets(buffer, mezzmap)) == -1) {
                            errorc("unable to encode packets");
                        }
                        else {
                            int lpc;
                            
                            for (lpc = 0; lpc < FD_SETSIZE; lpc++) {
                                if (FD_ISSET(lpc, &clientfds)) {
                                    write(lpc, buffer, rc);
                                }
                            }
                        }
                        last_mezz_event = mezz_event_count;
                    }
                    break;
                default:
                    errorc("unhandled select error\n");
                    break;
                }
            }
        }
    }
    
#if defined(HAVE_MEZZANINE)
    mezz_term(0);
#endif
    
    return retval;
}
