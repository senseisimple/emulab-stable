
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "garciaUtil.hh"
#include "ledManager.hh"

static volatile int looping = 1;

static void sigquit(int signal)
{
    looping = 0;
}

static void usage(void)
{
    fprintf(stderr,
	    "usage: flash-user-led "
	    "<dot|fast-dot|dash|dash-dot|dash-dot-dot|line>\n");
}

int main(int argc, char *argv[])
{
    ledClient::lm_pattern_t lmp;
    int retval = EXIT_SUCCESS;

    if (argc != 2) {
	usage();
	
	retval = EXIT_FAILURE;
    }
    else if ((lmp = ledClient::findPattern(argv[1])) == ledClient::LMP_MAX) {
	fprintf(stderr, "error: invalid pattern - %s\n", argv[1]);
	usage();
	
	retval = EXIT_FAILURE;
    }
    else {
	acpGarcia garcia;
	aIOLib ioRef;
    
	signal(SIGQUIT, sigquit);
	signal(SIGTERM, sigquit);
	signal(SIGINT, sigquit);
	
	aIO_GetLibRef(&ioRef, NULL);
	
	if (!wait_for_brainstem_link(ioRef, garcia)) {
	    fprintf(stderr, "error: cannot establish link to robot\n");
	    
	    retval = EXIT_FAILURE;
	}
	else {
	    ledManager lm(garcia, "user-led");
	    ledClient lc(0, lmp);
	    unsigned long now;

	    lm.addClient(&lc);
	    while (looping) {
		garcia.handleCallbacks(50);
		aIO_GetMSTicks(ioRef, &now, NULL);
		lm.update(now);
	    }
	}
	
	aIO_ReleaseLibRef(ioRef, NULL);
    }
    
    return retval;
}
