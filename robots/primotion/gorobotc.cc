/* gorobotc.cc
 *
 * Console application to drive robot using grobot::goto()
 *
 * Dan Flickinger
 *
 * 2004/12/08
 * 2004/12/09
 */

 
#include "dgrobot/commotion.h"


int main() {

  float dx, dy, dr;
  float dxe, dye;
  
  int quitnow = 0;
  int gstatus = 0;
  
  grobot mrrobot;
  
  
  while (quitnow == 0) {
    
    std::cout << "? " << std::flush;
    std::cin >> dx >> dy >> dr;
    
    if ((float)(dx) == 0.0f && (float)(dy) == 0.0f) {
      if ((float)(dr) == 0.0f) {
        // send an estop
        std::cout << "ESTOP" << std::endl;
        mrrobot.estop();
      } else {
        std::cout << "Quiting..." << std::endl;
        quitnow = 1;
      }
    } else {
      mrrobot.dgoto((float)(dx), (float)(dy), (float)(dr));
    
    
      // wait for moves to complete
      while (!mrrobot.garcia.getNamedValue("idle")->getBoolVal()) {
        mrrobot.sleepy();
      }
    
      // get the status
      gstatus = mrrobot.getGOTOstatus();
      mrrobot.getDisplacement(dxe, dye);
      
      std::cout << "Goto move finished with status: " << gstatus
                << std::endl
                << "(Estimated position: " << dxe << ", "
                << dye << ".)" << std::endl;
      
      
      
    }
  }
 
  return 0; 
}
