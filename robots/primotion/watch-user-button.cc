
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "garciaUtil.hh"
#include "buttonManager.hh"

class myButtonCallback : public buttonCallback
{

public:
    
    virtual bool shortClick(acpGarcia &garcia, unsigned long now)
    {
	printf("shortClick(%ld)\n", now);
	return true;
    };

    virtual bool commandMode(acpGarcia &garcia, unsigned long now, bool on)
    {
	printf("commandMode(%ld, %d)\n", now, on);
	return true;
    };

    virtual bool shortCommandClick(acpGarcia &garcia, unsigned long now)
    {
	printf("shortCommandClick(%ld)\n", now);
	return true;
    };

    virtual bool longCommandClick(acpGarcia &garcia, unsigned long now)
    {
	printf("longCommandClick(%ld)\n", now);
	return true;
    };
    
};

static volatile int looping = 1;

static void sigquit(int signal)
{
    looping = 0;
}

int main(int argc, char *argv[])
{
    int retval = EXIT_SUCCESS;
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
	buttonManager bm(garcia, "user-button");
	unsigned long now;

	bm.setCallback(new myButtonCallback());
	while (looping) {
	    garcia.handleCallbacks(50);
	    aIO_GetMSTicks(ioRef, &now, NULL);
	    bm.update(now);
	}
    }
    
    aIO_ReleaseLibRef(ioRef, NULL);
    
    return retval;
}
