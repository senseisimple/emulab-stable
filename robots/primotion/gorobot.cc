/* gorobot.cc
 *
 * Sends the robot to points via the 
 * grobot::goto(float, float, float) method
 *
 * Dan Flickinger
 *
 * 2004/11/12
 * 2004/12/02
 */
 
#include "dgrobot/commotion.h"

int main(int argc, char **argv) {

  float dx, dy, dr; // relative x, y coordinates and orientation
  
  int quitnow = 0;  // quit now
  int lsetup = 0;   // listener is setup
  
  grobot mrrobot;  // Mr. Robot
 
  // main loop
  while (quitnow == 0) {
    
    if (lsetup == 0) {
      // listener has not been setup
        
      // create listener
     
      // (if)
      // listener created
      // printf("Listening for RMCD on port %d.\n", port);
      lsetup = 1;
        
      // (else)
      // listener could not be created
      // FAIL
        
           
    } else {
      // listen for activity
        
      // select()
        
        
      // (if) command received  
    
      
      // if packet type is 'STOP':
      // execute an Estop (clear the behavior queue)
      mrrobot.estop();
        
      
      
      // if packet type is 'GOTO':
      // read in values for goto command (dx dy dr)
      // (THIS IS IN THE LOCAL COORDINATE FRAME)
      // send the move to the robot
      mrrobot.dgoto(dx, dy, dr);
    
      // (endif) command received
          
    
      //     // wait for robot to finish (BAD!)
      //     while (!mrrobot.garcia.getNamedValue("idle")->getBoolVal()) {
      //       mrrobot.sleepy();
      //     }
    
      
      // give robot a chance to handle callbacks
      // if (mrrobot.garcia.getNamedValue("idle")->getBoolVal()) {
         if (mrrobot.sleepy() != 0) {
         // a callback has been sent
        // PROBLEM: three callbacks are expected for a goto command
        
        
        // send back notification to RMCD that behavior has completed
        
      }
      
      
    }
  } // end main loop  
      
  
  return 0;
}
