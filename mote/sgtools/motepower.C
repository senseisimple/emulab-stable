
/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * motepower.C - turn the power to a mote on a stargate "on" or "off" by
 * toggling its RSTN pin
 */

#include "SGGPIO.h"

// Pin used to reset the ATMega microcontroller
#define RSTN_PIN 77

// The RSTN pin is negative logic
#define MOTE_ON 1
#define MOTE_OFF 0

void usage() {
	fprintf(stderr,"Usage: motepower <on | off | cycle>\n");
	exit(1);
}

int main(int argc, char **argv) {
    /*
     * Handle command-line argument
     */
    if (argc != 2) {
	usage();
    }

    bool turnoff = false;
    bool turnon = false;
    if (!strcmp(argv[1],"on")) {
	turnon = true;
    } else if (!strcmp(argv[1],"off")) {
	turnoff = true;
    } else if (!strcmp(argv[1],"cycle")) {
	turnon = turnoff = true;
    } else {
	usage();
    }

    /*
     * Set the pin for output
     */
    SGGPIO_PORT sggpio;
    sggpio.setDir(RSTN_PIN,1);

    /*
     * Turn off the mote, if we're supposed to
     */
    if (turnoff) {
	sggpio.setPin(RSTN_PIN,MOTE_OFF);
    }
    
    /*
     * If cycling, give it a moment
     */
    if (turnon && turnoff) {
	usleep(500 * 1000); // .5 seconds
    }

    /*
     * Turn on the mote, if we're supposed to
     */
    if (turnon) {
	sggpio.setPin(RSTN_PIN,MOTE_ON);
    }
}
