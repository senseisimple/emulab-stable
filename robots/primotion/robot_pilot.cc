
#include "config.h"

#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>

#include "acpGarcia.h"
#include "acpValue.h"

#include "dashboard.hh"

static int debug = 0;

static int looping = 1;

static void sigquit(int signal)
{
    looping = 0;
}

static void sigpanic(int signal)
{
    exit(1);
}

static void usage(void)
{
}

int find_range_start(unsigned long *range, unsigned long index)
{
    int lpc, retval = -1;

    assert(range != NULL);

    for (lpc = 0; (retval == -1) && (range[lpc + 1] != 0); lpc++) {
	if ((range[lpc] <= index) && (index < range[lpc + 1])) {
	    retval = lpc;
	}
    }

    return retval;
}

int main(int argc, char *argv[])
{
    char *logfile = NULL, *pidfile = NULL;
    int c, lpc, retval = EXIT_SUCCESS;
    unsigned long now;
    acpGarcia garcia;
    aIOLib ioRef;
    aErr err;

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
	}
    }
    
    if (getuid() != 0) {
	fprintf(stderr, "error: must run as root!\n");
	exit(1);
    }

    if (!debug) {
	/* Become a daemon */
	daemon(0, 0);

	if (logfile) {
	    FILE *file;

	    if ((file = fopen(logfile, "w")) != NULL) {
		dup2(fileno(file), 1);
		dup2(fileno(file), 2);
		stdout = file;
		stderr = file;
	    }
	}
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
    
    signal(SIGPIPE, SIG_IGN);
    
    aIO_GetLibRef(&ioRef, &err);

    if (debug) {
	printf("Waiting for link to robot\n");
    }
    
    for (lpc = 0;
	 (lpc < 30) && !garcia.getNamedValue("active")->getBoolVal();
	 lpc++) {
	printf(".");
	fflush(stdout);
	aIO_MSSleep(ioRef, 100, NULL);
    }
    printf("\n");

    if (lpc == 30) {
	fprintf(stderr,
		"error: could not connect to robot %d\n",
		garcia.getNamedValue("status")->getIntVal());
	exit(1);
    }

    aIO_MSSleep(ioRef, 500, NULL);
    
    if (debug) {
	printf("Established link to robot\n");
    }

    {
	dashboard db;
	
	do {
	    garcia.handleCallbacks(50);
	    aIO_GetMSTicks(ioRef, &now, NULL);
	} while (looping && db.update(garcia, now));
    }
    
    aIO_ReleaseLibRef(ioRef, &err);
    
    return retval;
}
