/* gorobot.cc
 *
 * Sends the robot to points via the 
 * grobot::goto(float, float, float) method
 *
 * Dan Flickinger
 *
 * 2004/11/12
 * 2004/12/07
 */
 
#include "dgrobot/commotion.h"

#define GOR_SERVERPORT 2531

int main(int argc, char **argv) {

  float dx, dy, dr; // relative x, y coordinates and orientation
  
  int quitnow = 0;  // quit now
  int lsetup = 0;   // listener is setup
  
  grobot mrrobot;  // Mr. Robot
 
  int gor_sock;    // GoRobot socket
  int msg_sock;    // MSG socket
  
  struct sockaddr_in gor_sin; // listener sockaddr
  
  fd_set readyset;
  struct timeval timeout;
  
  // server port:
  int gor_port = GOR_SERVERPORT;
  
  
  // pre-malloc a packet for general usage
  struct mtp_packet *packet = (struct mtp_packet *)malloc(sizeof(struct mtp_packet));
  if (packet == NULL) {
    // fuckup
    fprintf(stdout, "FATAL: Could not allocate memory for general packet structure.\n");
    exit(1);
  }
  
  
  
  // welcome to the main loop
  while (quitnow == 0) {
    
    if (lsetup == 0) {
      // listener has not been setup
        
      /***********************/
      /*** create listener ***/
      /***********************/
      
      // server socket
      gor_sock = socket(AF_INET, SOCK_STREAM, 0);
      if (gor_sock < 0) {
        // fuckup
        fprintf(stdout, "FATAL: Could not open listener: %s\n", strerror(errno));
        exit(1); // There is no point of living now
      }
      
      gor_sin.sin_family = AF_INET;
      gor_sin.sin_port = htons(gor_port);
      gor_sin.sin_addr.s_addr = INADDR_ANY;
      
      // bind to socket
      if (bind(gor_sock, &gor_sin, sizeof(gor_sin))) {
        // fuckup
        fprintf(stdout, "FATAL: Could not bind to socket: %s\n", strerror(errno));
        exit(1);
      }
      
      // listen on socket
      if (-1 == listen(gor_sock, 32)) {
        // fuckup
        fprintf(stdout, "FATAL: Could not listen on socket: %s\n", strerror(errno));
        exit(1);
      }
      
      
      /*********************/
      /*** NOW LISTENING ***/
      /*********************/
      lsetup = 1;
    }
      
      
      // listen for activity
        
      // set up file descriptor set
      FD_ZERO(&readyset);
      FD_SET(gor_sock, &readyset);
      
      // and timeout value
      timeout.tv_sec = 5;
    
      
      
      
      /********************/
      /*** GET A PACKET ***/
      /********************/
      
      
      // wait for select
      if (select(gor_sock + 1, &readyset, 0, 0, &timeout) < 0) {
        // fuckup WHY??
        std::cout << "FUCKUP: Select" << std::endl;
        continue;
      }
      
      if (FD_ISSET(gor_sock, &readyset)) {
        // something came in, DEAL WITH IT!
        
        // accept the connection
        msg_sock = accept(gor_sock, (struct sockaddr *)0, (int *)0);
        if (-1 == msg_sock) {
          // fuckup
          std::cout << "FUCKUP: Accept" << std::endl;
        } else {
          // read in packet RIGHT NOW
          
          retval = mtp_receive_packet(gor_sock, &packet);
          if (packet == NULL) {
            // BAD PACKET: GO TO HELL
            close(gor_sock);
          } else if (packet->version != MTP_VERSION) {
            // write back a control error packet
            // Are we speaking the same protocol? NO.
            
            struct mtp_control ctrl_err;
            ctrl_err.id = -1;
            ctrl_err.code = -1;
            ctrl_err.msg = "DISCONNECT: protocol version mismatch";
            
            struct mtp_packet *wb = mtp_make_packet(MTP_CONTROL_ERROR, MTP_ROLE_GOR, &ctrl_err);
            mtp_send_packet(gor_sock, wb);
            close(gor_sock); // FUCK OFF        

          } else {
            // check the role: I only accept orders from RMCD
            if (packet->role != MTP_ROLE_RMC) {
              // packet is not from RMCD
              // who dares to demand that I do something??              
              
              // Send a control error packet, and close the connection.
              // We're not talking to YOU, bitch.
              
              struct mtp_control ctrl_err;
              ctrl_err.id = -1;
              ctrl_err.code = -1;
              ctrl_err.msg = "DISCONNECT: you are not RMCD";
            
              struct mtp_packet *wb = mtp_make_packet(MTP_CONTROL_ERROR, MTP_ROLE_GOR, &ctrl_err);
              mtp_send_packet(gor_sock, wb);
              close(gor_sock); // FUCK OFF
              
            } else {
              // I'm talking to RMCD
              // YES MASTER?
    
      
              if (packet->opcode == MTP_COMMAND_STOP) {
                // packet type is 'STOP':
                
                // don't need any additional data
                
                // execute an Estop (and clear the behavior queue)
                mrrobot.estop();
              }
        
      
      
              if (packet->opcode == MTP_COMMAND_GOTO) {
                // packet type is 'GOTO':
                
                // FIXME: does this work?
                // read in values for goto command (dx dy dr)
                dx = packet->data->position->x;
                dy = packet->data->position->y;
                dr = packet->data->position->theta;
                
                // (THIS IS IN THE LOCAL COORDINATE FRAME)
                // send the move to the robot
                mrrobot.dgoto(dx, dy, dr);
              }
    
        
              if (packet->opcode == MTP_UPDATE_POSITION) {
                // packet type is 'UPDATE POSITION'
                // RMCD is asking for a position estimate
                
                // Valid status fields:
                // MTP_POSITION_STATUS_IDLE, _MOVING, _ERROR, _COMPLETE
                
                // send back position estimate to RMCD
                
              }
        
        
        
              if (0) {
                // if packet type is 'HUP': (FIXME: NOT IMPLEMENTED)
                // RMCD is telling robot to go offline
                // shut everything down
              
                // quit now
                quitnow = 1;
              }
              
              
            } // end if packet role is RMCD 
          } // end if good packet
        } // end if packet accepted and read
      } // end if something came in (Do nothing if nothing comes in)
    
      
      
        
      /****************************/
      /*** Time for robot stuff ***/
      /****************************/   
    
      //     // wait for robot to finish (BAD!)
      //     while (!mrrobot.garcia.getNamedValue("idle")->getBoolVal()) {
      //       mrrobot.sleepy();
      //     }
    
      
      // give robot a chance to handle callbacks
      // if (mrrobot.garcia.getNamedValue("idle")->getBoolVal()) {
      if (mrrobot.sleepy() != 0) {
        // a callback has been sent
        // PROBLEM: three callbacks are expected for a goto command
        
      }
        
      // poll grobot for behavior completion
      // if behavior has completed: send back notification to RMCD
      
        
      
      
      
  } // end main loop 
      
  
  return 0;
}
