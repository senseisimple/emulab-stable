/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* Garcia motion program (command and interactive mode)
 *
 * Dan Flickinger
 *
 * 2004/09/13
 * 2004/11/17
 */
 
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <cmath>

#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/uio.h>


#include "acpGarcia.h"
#include "acpValue.h"


using namespace std;

#include "gcallbacks.h"
#include "grobot.h"



// path generators:
#ifdef PATH_SIMPLE
#include "simplepath.h"
#endif

#ifdef PATH_CUBIC
#include "cubicpath.h"
#endif
