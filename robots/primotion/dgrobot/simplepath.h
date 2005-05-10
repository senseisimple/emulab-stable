/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* simplepath.h
 * class with methods for generating simple paths
 *
 * Dan Flickinger
 * 2004/10/26
 * 2004/11/16
 */

 
#ifndef SIMPLEPATH
#define SIMPLEPATH

#include "grobot.h"

class spathseg {
   public:
     // constructors:
     spathseg();
     ~spathseg();
     spathseg(grobot *g,
              float s_l,
              float s_r,
              float s_iv,
              float s_fv);
              
     
     
     int execute();
     void estop();
     
   private:
     float s_length;   // arc length of path
     float s_radius;   // Turning radius for this segment
     
     float s_Ivelocity; // Initial forward velocity for this segment
     float s_Fvelocity; // Final forward velocity for this segment
     
     grobot *pgrobot;
};

#endif
