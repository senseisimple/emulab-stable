/* Callback functions for garcia robot
 *
 * Dan Flickinger
 *
 * 2004/10/04
 * 2004/11/17
 *
 */

#ifndef GCALLBACKS_H
#define GCALLBACKS_H

#include <iostream>

using namespace std;


class CallbackComplete;
class CallbackExecute;

#include "grobot.h"

// a simple callback completion class
class CallbackComplete : public acpCallback {
   public:
     CallbackComplete(acpObject *b, grobot *g);
     ~CallbackComplete();
  
     aErr call();
  
     int getStatus() { return blast_status; }

   private:
     acpObject *behavior;
     grobot *pgrobot;
  
     int blast_status; // Behavior Last _STATUS
     int blast_id;     // Behavior Last _ID
};



class CallbackExecute : public acpCallback {
  public:
    CallbackExecute(acpObject *b, grobot *g);
    ~CallbackExecute();
    
    aErr call();
    
  private:
    acpObject *behavior;
    grobot *pgrobot;

    int blast_id;     // Behavior Last _ID
    
};

#endif
