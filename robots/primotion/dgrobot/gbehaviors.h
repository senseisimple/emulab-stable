/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* Garcia robot behavior data class
 *
 * Dan Flickinger
 *
 * 2004/11/19
 * 2004/11/19
 */
 
#ifndef GBEHAVIORS_H
#define GBEHAVIORS_H

#include <string>

// shouldn't this already be defined???!!
#define aGARCIA_ERRFLAG_NORMAL		 0x0000
#define aGARCIA_ERRFLAG_STALL		 0x0001
#define aGARCIA_ERRFLAG_FRONTR_LEFT	 0x0002
#define aGARCIA_ERRFLAG_FRONTR_RIGHT 	 0x0003
#define aGARCIA_ERRFLAG_REARR_LEFT	 0x0004
#define aGARCIA_ERRFLAG_REARR_RIGHT	 0x0005
#define aGARCIA_ERRFLAG_SIDER_LEFT	 0x0008
#define aGARCIA_ERRFLAG_SIDER_RIGHT	 0x0009
#define aGARCIA_ERRFLAG_FALL_LEFT	 0x0010
#define aGARCIA_ERRFLAG_FALL_RIGHT	 0x0011
#define aGARCIA_ERRFLAG_ABORT		 0x0012
#define aGARCIA_ERRFLAG_NOTEXECUTED	 0x0013
#define aGARCIA_ERRFLAG_WONTEXECUTE	 0x0014
#define aGARCIA_ERRFLAG_BATT		 0x0020
#define aGARCIA_ERRFLAG_IRRX		 0x0040

class gbehavior;


class gbehavior {
  public:
    gbehavior();
    ~gbehavior();
    
    void getStatus(char *MSG);
    float getDisp();
    
    void setStatus(int stat);
    void setDisp(float disp);
    void setID(int ID);
    
  private:
    int b_ID;     // behavior ID
    int b_status; // behavior return status
    
    float b_disp; // behavior final displacement
};

#endif
