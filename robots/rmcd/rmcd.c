/* Robot Master Control Daemon
 *
 * Dan Flickinger
 *
 *
 * RMCD will immediately try to connect to EMCD, get configuration data,
 * and then open connections to all robots as sent by EMCD. RMCD only heeds
 * one master; in other words, it will connect to only a single EMCD.
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
 * 2004/12/07
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

#define EMC_SERVERPORT 2525
#define EMC_HOSTNAME "blah.flux.utah.edu"
 
int main(int argc, char **argv) {
  /* You are watching RMCD */

  // miscellaneous administration junk
  int quitmain = 0;     // quit main loop
  int retval = 0;       // return value
  int cpacket_rcvd = 0; // configuration packet received

  
  // file descriptors
  int FD_emc; // connection to EMC server
  int max_fd = -1;
  

  // file descriptor set and time value for select() loop
  fd_set defaultset, readset;
  struct timeval default_tv, tv;


  // set up file descriptors and select variables
  FD_ZERO(&defaultset);
  default_tv.tv_sec = 5; // 5 seconds
  default_tv.tv_usec = 0; // wah?
  

  struct sockaddr_in EMC_serv;
  struct hostent *EMC_host;
  
  struct mtp_packet *read_packet;
  
  
  
  
 
  // parse options
  // FIXME
         
  int EMC_port = EMC_SERVERPORT; // EMC server port (SET DEFAULT VALUE)
  char EMC_hostname[128]; // FIXME: what is the default hostname?

  
  
  
  // daemon mode -- go to hell
  // FIXME
  

  
         
         
  
  
  
  /*************************************/
  /*** Connect to EMCD and configure ***/
  /*************************************/
  
  // create socket for connection to EMC server
  FD_emc = socket(AF_INET, SOCK_STREAM, 0);
  if (FD_emc == -1) {
    // fuckup
    fprintf(stdout, "FATAL: Could not open socket to EMC: %s\n", strerror(errno));
    exit(1); 
  }
  
  
  // add this new socket to the default FD set
  FD_SET(FD_emc, &defaultset);
  max_fd = (FD_emc > max_fd)?FD_emc:max_fd;

  
  // client socket to connect to EMCD
  EMC_serv.sin_family = AF_INET; // TCP/IP
  EMC_serv.sin_port = htons(port); // port
  EMC_host = gethostbyname(&EMC_hostname);
  bcopy(EMC_host->h_addr, &EMC_serv, EMC_host->h_length);
  
    
  // connect socket
  if (-1 == connect(FD_emc, (struct sockaddr *)(&EMC_serv), sizeof(struct sockaddr))) {
    // fuckup
    fprintf(stdout, "FATAL: Could not connect to EMC: %s\n", strerror(errno));
    exit(1);
  }
  // now connected to EMCD
  
  
  
  
  // send configuration request packet to EMCD
  // FIXME: need to look at structure in the protocol
  struct mtp_config cfg;
  cfg.id = -1; // ID: FIXME
  cfg.code = -1; // code: FIXME
  cfg.msg = ""; // message: FIXME
  
  struct mtp_packet *cfg_packet = mtp_make_packet(MTP_CONFIG, MTP_ROLE_RMC, &cfg); // FIXME: is it MTP_CONFIG??
  mtp_send_packet(FD_emc, cfg_packet);
  
  
  
  
  while (cpacket_rcvd == 0) {
    // wait for configuration data from EMC
    
  
    // select??
    
    
    // read a packet in
    read_packet = receive_packet(FD_emc);
    if (read_packet == NULL) {
      // fuckup
      fprintf(stdout, "FATAL: Could not read config data from EMC\n");
      exit(1);
    }
  
  
    if (read_packet->opcode == MTP_CONFIG_RMC) {
      // this is a configuration packet
      
      // store robot list data
      // FIXME: need data structure here
      
      // FIXME: global coordinate bound box
      //cfg.box.horizontal = read_packet.box.horizontal;
      //cfg.box.vertical = read_packet.box.vertical;
      
      
      cpacket_rcvd = 1;
    } else {
      printf("Still missing configuration packet from EMC\n");
    
      // send notification to EMC
      // Hello? Where is CONFIG PACKET??
     
      // send the configuration request packet AGAIN
      mtp_send_packet(FD_emc, cfg);
    
    }
  }

  /********************************************/
  /*** Now connected to EMCD and configured ***/
  /********************************************/

  
  
  
  /************************/
  /*** Configure robots ***/
  /************************/
  
  
  // open a connection to each robot listener
  // FIXME: monkey
         
  // {loop}
  
  // create packet for this robot
  // send packet to robot
         
  // configure
         
  // add connection to default set

  // add robot to list as configured
  
  // notify that robot is configured
  
  // {endloop}
  
         
         
         
  /*********************************/
  /*** Robots are now configured ***/
  /*********************************/

         
         


  /**********************************************************/
  /*** Notify EMCD that RMCD is ready to receive commands ***/
  /**********************************************************/   
  
  // FIXME: is this right??
  // send packet to EMC updating ready status
  struct mtp_control rc_packet;
  rc_packet.id = -1; // ID: ?? FIXME
  rc_packet.code = -1; // Code: ?? FIXME
  rc_packet.msg = "RMCD ready";
  
  struct mtp_packet *rc = mtp_make_packet(MTP_CONFIG, MTP_ROLE_RMC, &rc_packet); // FIXME: not MTP_CONFIG
  
  mtp_send_packet(FD_emc, rc);
  


  /*** EMCD has been notified of status ***/  

  // set up select bitmap


  

  /*****************************************/  
  /*** ALL CONFIGURATION IS NOW COMPLETE ***/
  /*** Let's get down to business        ***/
  /*****************************************/

  
  // malloc packet structure for use in the main loop
  struct mtp_packet *packet = (struct mtp_packet *)malloc(sizeof(struct mtp_packet));
  if (packet == NULL) {
    fprintf(stdout, "FATAL: Could not malloc packet structure.\n");
    exit(1);
  }

  
  
  
  // main loop, whoo!
  while (quitmain == 0) {
   
    
    readset = defaultset; // select tramples defaultset otherwise
    tv = default_tv; // same here (shame!)
    
    // select() on all EMC and sub-instance file descriptors
    retval = select(max_fd + 1, &readset, NULL, NULL, &tv);
    if (retval == -1) {
      // fuckup
      fprintf(stdout, "RMCD: Error in select loop: %s\n", strerror(errno));
    } else if (retval == 0) {
      // timeout was reached
      // do nothing!
    } else {
      // check all file descriptors for activity
      
      if (FD_ISSET(FD_emc, &readset)) {
        // Uh oh, something came back from EMCD
        
        
    
    
    // receive packet(s) in
    
    // if packet is from EMCD:
    
    if (packet->role == MTP_ROLE_EMC) {
      
      
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
      // compare intended position to current position (issue goto commands
      //    IF necessary)
    
      // transform position to robot local coordinate frame and send
      // to robot
      // ELSE: tell EMCD that this robot is at it's intended position

    
    } else if (packet->role == MTP_GOROBOT) {
      // FIXME: what is the correct role here??
      // it's from a robot
      
      // send get position update request
                 
             
      // IF robot says that it thinks goal position has been reached
      
             
      // IF robot says that it cannot get to commanded position
    

      
    }
    
     
  } // end main loop


  // have a nice day, and good bye.
  printf("RMCD exiting without complaint.\n");
  return 0; // hope to see you soon
}
