/* Robot Master Control Daemon
 *
 * Dan Flickinger
 *
 *
 * RMCD will immediately try to connect to EMCD, get configuration data,
 * and then open connections to all robots as sent by EMCD.
 *
 * Once connections are open, RMCD will wait for commands from EMCD.
 *
 * After a command is received, RMCD will wait for EMCD to send the
 * current position for the robot commanded to move by EMCD.  Once the
 * current position is received, the move command is relayed to the
 * appropriate robot after converting the coordinates into the local
 * robot's reference frame.
 *
 * Upon completion of a command, RMCD will send notice to EMCD, along
 * with a position update sent from the robot and converted into the
 * global coordinate frame.
 *
 *
 * 2004/12/01
 * 2004/12/06
 */
 
#include<stdio.h>
#include<sys/types.h>

#include<unistd.h>
#include<stdlib.h>
#include<signal.h>
#include<netiten/in.h>
#include<errno.h>

#include<string.h>
#include<sys/socket.h>

#include "rmcd.h"


 
int main(int argc, char **argv) {
  /* You are watching RMCD
   * Everything you beleave is wrong.
   */

  int quitmain = 0; // quit main loop
  int retval; // return value

  int FD_emc; // file descriptor for connection to EMC
  
  
  int cpacket_rcvd = 0;
  struct mtp_packet *read_packet;
  
  fd_set defaultset; 
  int max_fd = -1;
  struct timeval tv,default_tv;
  
  // socket to connect to EMCD (FIXME: this needs to be a client)
  struct sockaddr EMC_addr; // address for EMC server (IS THIS RIGHT?)
  EMC_addr.sin_family = AF_INET;
  EMC_addr.sin_port = htons(port); // FIXME: define 'port' somewhere
  EMC_addr.sin_addr.s_addr = INADDR_ANY; // FIXME: is this where to put EMCD's address??
  
  // parse options
  // FIXME
  
  
  // daemon mode
  // FIXME
  
         
  // set up file descriptors
  FD_ZERO(&defaultset);
  default_tv.tv_sec = 5; // 5 seconds
  default_tv.tv_usec = 0; // wah?
  
  
  
  // open connection to EMC server
  FD_emc = socket(PF_INET, SOCK_STREAM, 0);
  
  if (FD_emc == -1) {
    // fuckup
    printf("FATAL: Could not open socket to EMC\n");
    exit(1); 
  }
  
  // bind socket
  retval = connect(FD_emc, &remoteaddr, sizeof(struct sockaddr));
  if (retval == -1) {
    // fuckup
    printf("FATAL: Could not connect to EMC\n");
    exit(2);
  }
      
  // FIXME: send init packet to EMC
         
  

  
  // configuration loop
  while (cpacket_rcvd == 0) {
    // get configuration data from EMC
  
   read_packet = receive_packet(FD_emc);  
   if (read_packet == NULL) {
     // fuckup
     printf("FATAL: Could not read config data from EMC\n");
     exit(3);
   }
  
  
   if (read_packet->opcode == MTP_CONFIG_RMC) {
    // this is a configuration packet
      
    // store robot list data
      
    // global coordinate bound box
      
    cpacket_rcvd = 1;
   } else {
     printf("Still missing configuration packet from EMC\n");
    
     // send notification to EMC
     // where is CONFIG PACKET??
    
    
   }
  }

  // do something about configuration packet
  
  
  // setup list of robots
  
  
  
  // open connection to each robot listener
  
  
  
  
  // store data about connected robots
  
  
  // send packet to EMC saying that I'm ready to receive commands

  
  // set up select bitmap
  
  
  /** CONFIGURATION IS NOW COMPLETE **/

  // main loop
  while (quitmain == 0) {
   
    // select() on all sub-instance file descriptors and on emc fd
    
    
    // if goto command from EMC:
    // if this robot already moving, drop the packet
    // otherwise -- put requested position in "robot state array"
    // issue request to EMC for latest position
    // then implicitly wait to hear a position update for this robot id
    
    
    
    
    // if stop command from EMC:
    // if "stop all robots", issue commands to all subinstances saying
    // terminate all motion
    // if stop single robot:
    //   update state array saying we're trying to stop a robot.
    //   forward the stop to subinstance
    
    
    
    
    // if position update from EMC:
    // store the position in position array
    // IFF a goto command is currently executing:
    // compare intended position to current position (issue goto commands
    //    if necessary)
    
    //    if necessary: transform position to robot local coordinate frame
    //                  and send to robot
    //    else: send final status update for this command
    //      to EMC
    
    
    
    // if it's status update from subinstance:
    // send get position update request
    

      
    }
    
     
  } // end main loop


  printf("RMCD exiting.\n");
  return 0;
}
