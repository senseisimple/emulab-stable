/*
 * This is a sample event generator to send TBEXAMPLE events to all nodes
 * in an experiment. Modify as needed of course.
 */

#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <termios.h>
#include <string.h>
#include "log.h"

#include "event.h"

static char	*progname;
static struct termios origterm, newterm;

static void restoreterm(void);

void
usage()
{
	fprintf(stderr,
		"Usage: %s [-s server] [-p port] [-k keyfile] "
                "[-f] [-c] [-l] [-w] [-i idleperiod] [-r retries] "
                "<event> [<expt>] [<host>]\n",
		progname);
	exit(-1);
}

int
main(int argc, char **argv)
{
	event_handle_t handle;
	event_notification_t notification;
	address_tuple_t	tuple;
	char *server = "localhost";
	char *port = NULL;
	char *keyfile = NULL;
	char buf[BUFSIZ], *bp;
	struct timeval	now;
	int c;
        int loopit    = 0;
        int scheduler = 0;
        int idleping  = 0;
        int failover  = 0;
        int retries   = 0;
        int wait      = 0;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "s:p:k:clwi:fr:")) != -1) {
		switch (c) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'k':
			keyfile = optarg;
			break;
                case 'c':
                        scheduler = 1;
                        break;
                case 'l':
                        loopit = 1;
                        break;
                case 'w':
                        wait = 1;
                        break;
                case 'i':
                        idleping = atoi(optarg);
                        break;
                case 'f':
                        failover = 1;
                        break;
                case 'r':
                        retries = atoi(optarg);
                        break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();
	
	loginit(0, 0);

	/*
	 * Uppercase event tags for now. Should be wired in list instead.
	 */
	bp = argv[0];
	while (*bp) {
		*bp = toupper(*bp);
		bp++;
	}

	/*
	 * Convert server/port to elvin thing.
	 *
	 * XXX This elvin string stuff should be moved down a layer. 
	 */
	if (server) {
		snprintf(buf, sizeof(buf), "elvin://%s%s%s",
			 server,
			 (port ? ":"  : ""),
			 (port ? port : ""));
		server = buf;
	}

	/*
	 * Construct an address tuple for generating the event.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	}
	tuple->objtype  = "TBEXAMPLE";
	tuple->eventtype= argv[0];
        if (argc > 1) {
          tuple->expt   = argv[1];
        }
        if (argc > 2) {
          tuple->host   = argv[2];
        } else {
          tuple->host	= ADDRESSTUPLE_ALL;
        }

        if (scheduler) {
          tuple->scheduler = 1;
        }

	/* Register with the event system: */
	handle = event_register_withkeyfile_withretry(server, 
                                                      0, 
                                                      keyfile,
                                                      retries);
	if (handle == NULL) {
		fatal("could not register with event system");
	}

        /* setup idle ping if requested */
        if (idleping) {
          event_set_idle_period(handle, idleping);
        }

        /* setup failover if requested */
        if (failover) {
          event_set_failover(handle, 1);
        }

        /* Allocate the event */
        notification = event_notification_alloc(handle, tuple);
        if (notification == NULL) {
          fatal("could not allocate notification");
        }

        /* 
         *  Make the terminal raw if in wait mode so we can capture
         *  individual keystrokes w/o waiting for an EOL.
         */
        if (wait) {
          if (tcgetattr(STDIN_FILENO, &origterm) != 0) {
            errorc("Can't get STDIN terminal settings.");
            exit(1);
          }
          memcpy(&newterm, &origterm, sizeof(struct termios));
          newterm.c_lflag &= ~(ICANON|ECHO);
          if (tcsetattr(STDIN_FILENO, TCSANOW, &newterm) != 0) {
            errorc("Can't set new STDIN terminal settings.");
            exit(1);
          }
        }

        /* Send in a loop if specified, or just once otherwise */
        do {
          gettimeofday(&now, NULL);

          if (event_notify(handle, notification) == 0) {
            error("could not send test event notification at: %lu:%d\n",
                  now.tv_sec, now.tv_usec);
          } else {
            info("Sent at time: %lu:%d\n", now.tv_sec, now.tv_usec);
          }

          if (wait) {
            int inchar = getchar();
            if (tolower(inchar) == 'q') {
              restoreterm();
              exit(0);
            }
          } else {
            sleep(1);
          }

        } while (loopit || wait);

        event_notification_free(handle, notification);
        
        /* Unregister with the event system: */
        if (event_unregister(handle) == 0) {
          fatal("could not unregister with event system");
        }
	
	return 0;
}

void restoreterm() {
  if (tcsetattr(STDIN_FILENO, TCSANOW, &origterm) != 0) {
    errorc("Error restoring STDIN's original terminal settings");
  }
}
