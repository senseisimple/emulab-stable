/* gorobot.cc
 *
 * Sends the robot to points via the 
 * grobot::goto(float, float, float) method
 *
 * Dan Flickinger
 *
 * 2004/11/12
 * 2004/12/09
 */
 
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include <string.h>
#include <sys/socket.h>

#include "mtp.h"
#include "dgrobot/grobot.h"

#define GOR_SERVERPORT 2531

static int debug = 0;

static int looping = 1;

static int robot_id = -1;

static void sigquit(int signal)
{
    looping = 0;
}

static void usage(void)
{
    fprintf(stderr,
	    "Usage: gorobot ...\n");
}

int main(int argc, char *argv[])
{
    int c, serv_sock = -1, port = GOR_SERVERPORT, on_off = 1;
    char *logfile = NULL, *pidfile = NULL;
    int retval = EXIT_SUCCESS;
    struct sockaddr_in saddr;
    
    while ((c = getopt(argc, argv, "hdp:l:i:")) != -1) {
	switch (c) {
	case 'h':
	    usage();
	    exit(0);
	    break;
	case 'd':
	    debug += 1;
	    break;
	case 'l':
	    logfile = optarg;
	    break;
	case 'i':
	    pidfile = optarg;
	    break;
	case 'p':
	    if (sscanf(optarg, "%d", &port) != 1) {
		fprintf(stderr,
			"error: -p option is not a number: %s\n",
			optarg);
		usage();
		exit(1);
	    }
	    break;
	}
    }
    
    if (!debug) {
	/* Become a daemon */
	daemon(0, 0);

	if (logfile) {
	    FILE *file;

	    if ((file = fopen(logfile, "w")) != NULL) {
		dup2(fileno(file), 1);
		dup2(fileno(file), 2);
		stdout = file;
		stderr = file;
	    }
	}
    }

    if (pidfile) {
	FILE *fp;
	
	if ((fp = fopen(pidfile, "w")) != NULL) {
	    fprintf(fp, "%d\n", getpid());
	    (void) fclose(fp);
	}
    }

    printf("Listening on %d\n", port);

    signal(SIGQUIT, sigquit);
    signal(SIGTERM, sigquit);
    signal(SIGINT, sigquit);
    
    signal(SIGPIPE, SIG_IGN);

    memset(&saddr, 0, sizeof(saddr));
#if !defined(linux)
    saddr.sin_len = sizeof(saddr);
#endif
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;

    if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("socket");
    }
    else if (setsockopt(serv_sock,
			SOL_SOCKET,
			SO_REUSEADDR,
			&on_off,
			sizeof(on_off)) == -1) {
	perror("setsockopt");
    }
    else if (bind(serv_sock, (struct sockaddr *)&saddr, sizeof(saddr)) == -1) {
	perror("bind");
    }
    else if (listen(serv_sock, 5) == -1) {
	perror("listen");
    }
    else {
	fd_set readfds, clientfds;
	grobot bot;
	
	FD_ZERO(&readfds);
	FD_ZERO(&clientfds);
	FD_SET(serv_sock, &readfds);

	while (looping) {
	    struct timeval tv_zero = { 0, 0 };
	    fd_set rreadyfds = readfds;
	    int rc;

	    /* Poll the file descriptors, don't block */
	    rc = select(FD_SETSIZE, &rreadyfds, NULL, NULL, &tv_zero);
	    if (rc > 0) {
		int lpc;

		if (FD_ISSET(serv_sock, &rreadyfds)) {
                    struct sockaddr_in peer_sin;
                    socklen_t slen;
                    int cfd;
                    
                    slen = sizeof(peer_sin);
                    if ((cfd = accept(serv_sock,
				      (struct sockaddr *)&peer_sin,
				      &slen)) == -1) {
                        perror("accept");
                    }
                    else {
                        FD_SET(cfd, &readfds);
                        FD_SET(cfd, &clientfds);
                    }
		}

		for (lpc = 0; lpc < FD_SETSIZE; lpc++) {
		    if ((lpc != serv_sock) && FD_ISSET(lpc, &rreadyfds)) {
			struct mtp_packet *mp = NULL;

			if (mtp_receive_packet(lpc, &mp) != MTP_PP_SUCCESS) {
			    perror("receive");
			    close(lpc);
			    FD_CLR(lpc, &clientfds);
			    FD_CLR(lpc, &readfds);
			}
			else {
			    // XXX handle client request.
			    switch (mp->opcode) {
			    case MTP_COMMAND_GOTO:
				printf("goin\n");
				
				robot_id = mp->data.command_goto->robot_id;
				bot.estop();
				bot.dgoto(mp->data.command_goto->position.x,
					  mp->data.command_goto->position.y,
					  mp->data.command_goto->position.
					  theta);
				break;
			    case MTP_COMMAND_STOP:
				{
				    struct mtp_update_position mup;
				    struct mtp_packet *ump;

				    printf("stoppin\n");
				    
				    bot.estop();
				    
				    mup.robot_id = robot_id;
				    bot.getDisplacement(mup.position.x,
							mup.position.y);
				    mup.position.theta = 0;
				    mup.status = MTP_POSITION_STATUS_IDLE;
				    if ((ump = mtp_make_packet(
					MTP_UPDATE_POSITION,
					MTP_ROLE_RMC,
					&mup)) == NULL) {
					fprintf(stderr,
						"error: unable to make update position packet\n");
				    }
				    else {
					char buffer[1024], *buf = buffer;
					int lpc, len;
					
					len = mtp_encode_packet(&buf, ump);
					for (lpc = 0; lpc < FD_SETSIZE; lpc++) {
					    if (FD_ISSET(lpc, &clientfds)) {
						write(lpc, buffer, len);
					    }
					}
					
					mtp_free_packet(ump);
					ump = NULL;
				    }
				}
				break;
			    }
			}
			mtp_free_packet(mp);
			mp = NULL;
		    }
		}
	    }

	    bot.sleepy();

	    if ((rc = bot.getGOTOstatus()) != 0) {
		struct mtp_update_position mup;
		struct mtp_packet *mp;

		mup.robot_id = robot_id;
		bot.getDisplacement(mup.position.x, mup.position.y);
		mup.position.theta = 0;
		if (rc < 0) {
		    mup.status = MTP_POSITION_STATUS_ERROR;
		}
		else {
		    mup.status = MTP_POSITION_STATUS_COMPLETE;
		}
		if ((mp = mtp_make_packet(MTP_UPDATE_POSITION,
					  MTP_ROLE_RMC,
					  &mup)) == NULL) {
		    fprintf(stderr,
			    "error: unable to make update position packet\n");
		}
		else {
		    char buffer[1024], *buf = buffer;
		    int lpc, len;

		    len = mtp_encode_packet(&buf, mp);
		    for (lpc = 0; lpc < FD_SETSIZE; lpc++) {
			if (FD_ISSET(lpc, &clientfds)) {
			    write(lpc, buffer, len);
			}
		    }
		    
		    mtp_free_packet(mp);
		    mp = NULL;
		}
	    }
	}
    }

    return retval;
}

#if 0
int main(int argc, char **argv) {

  float dx, dy, dr; // relative x, y coordinates and orientation
  
  int quitnow = 0;  // quit now
  int lsetup = 0;   // listener is setup
  
  int gstatus;      // status of goto move
  grobot mrrobot;   // Mr. Robot
  
 
  int gor_sock;     // GoRobot socket
  int msg_sock;     // MSG socket
  
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
      
      mrrobot.sleepy();
        
      // poll grobot for behavior completion
      gstatus = mrrobot.getGOTOstatus();
      if (0 != Gstatus) {
        // send back notification to RMCD about goto behavior completion
        
        // determine if the move was successful
        if (gstatus > 0) {
          // move was good
          // FIXME 
        } else {
          // move was bad
          // FIXME
        }
        
        // send back position
        // FIXME
      }
        
      
      
      
  } // end main loop 
      
  
  return 0;
}
#endif
