/* Garcia robot class
 *
 * Dan Flickinger
 *
 * 2004/10/04
 * 2004/12/07
 */

 
// FIXME: have dgoto register a goto command, check all 3 callbacks,
// and create a function to poll for completion of a goto command.
// Also, have dgoto give an error if a goto command is currently executing
// (NO QUEUEING!!!)
 
#ifndef GROBOT_H
#define GROBOT_H

#include "acpGarcia.h"
#include "acpValue.h"

class grobot;

#include "gcallbacks.h"
#include "gbehaviors.h"

#include <math.h>
#include <iostream>
#include <string>



// track width, in meters
#define TRACK_WIDTH 0.1778f

class grobot {
  public:
    grobot();
    ~grobot();
    
    void estop();
    void setWheels(float Vl, float Vr);
    void setvPath(float Wv, float Wr);
    
    void pbMove(float mdisplacement);
    void pbPivot(float pangle);
    void dgoto(float Dx, float Dy, float Rf);

    void resetPosition();
    void updatePosition();
    float getArclen();
    
    int getGstatus();
    int sleepy();
    
    void setCBexec(int id);
    void setCBstatus(int id, int stat);

     
    acpGarcia garcia;
    
  private:
    void createNULLbehavior();
    void createPRIMbehavior();
        
    float Vl; // left wheel velocity
    float Vr; // right wheel velocity
    
    acpObject *pBehavior;
     
    CallbackComplete *completeCB;
    CallbackExecute *executeCB;
    
    acpValue completeCBacpV;
    acpValue executeCBacpV;
    
    aIOLib ioRef;
    aErr err;
    
};


#endif


























