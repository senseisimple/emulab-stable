/* Garcia robot behavior class methods
 *
 * Dan Flickinger
 *
 * 2004/11/19
 * 2004/11/19
 */
 
 
#include "gbehaviors.h"


gbehavior::gbehavior() {
  // constructor
  
  
}



gbehavior::~gbehavior() {
  // destructor
  
  
}



void gbehavior::getStatus(char *MSG) {
  // get the status of this behavior and translate to an english string
  
  if (b_status == aGARCIA_ERRFLAG_NORMAL) {
    strcpy(MSG, "no problems");
  } else if (b_status == aGARCIA_ERRFLAG_STALL) {
    strcpy(MSG, "stall condition detected");
  } else if (b_status == aGARCIA_ERRFLAG_FRONTR_LEFT) {
    strcpy(MSG, "object detected, front left IR sensor");
  } else if (b_status == aGARCIA_ERRFLAG_FRONTR_RIGHT) {
    strcpy(MSG, "object detected, front right IR sensor");
  } else if (b_status == aGARCIA_ERRFLAG_REARR_LEFT) {
    strcpy(MSG, "object detected, rear left IR sensor");
  } else if (b_status == aGARCIA_ERRFLAG_REARR_RIGHT) {
    strcpy(MSG, "object detected, rear right IR sensor");
  } else if (b_status == aGARCIA_ERRFLAG_SIDER_LEFT) {
    strcpy(MSG, "object detected, left side IR sensor");
  } else if (b_status == aGARCIA_ERRFLAG_SIDER_RIGHT) {
    strcpy(MSG, "object detected, right side IR sensor");
  } else if (b_status == aGARCIA_ERRFLAG_FALL_LEFT) {
    strcpy(MSG, "drop-off detected, front left side");
  } else if (b_status == aGARCIA_ERRFLAG_FALL_RIGHT) {
    strcpy(MSG, "drop-off detected, front right side");
  } else if (b_status == aGARCIA_ERRFLAG_ABORT) {
    strcpy(MSG, "aborted");
  } else if (b_status == aGARCIA_ERRFLAG_NOTEXECUTED) {
    strcpy(MSG, "not executed for some stupid reason");
  } else if (b_status == aGARCIA_ERRFLAG_WONTEXECUTE) {
    strcpy(MSG, "will not execute: bitching about something");
  } else if (b_status == aGARCIA_ERRFLAG_BATT) {
    strcpy(MSG, "LOW BATTERY: robot cry");
  } else if (b_status == aGARCIA_ERRFLAG_IRRX) {
    strcpy(MSG, "IR receiver override");
  } else {
    strcpy(MSG, "NO STATUS");
  }
  
  

}



float gbehavior::getDisp() {
  // get final displacement for this behavior
  return b_disp;
}
  


void gbehavior::setStatus(int stat) {
  // set the status for this behavior
  b_status = stat;
}



void gbehavior::setDisp(float disp) {
  // set the final displacement for this behavior
  b_disp = disp;
}



void gbehavior::setID(int ID) {
  // set the ID for this behavior
  b_ID = ID;
}


