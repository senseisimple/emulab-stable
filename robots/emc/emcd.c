#include "emcd.h"

/* emc sets up an interface for accepting commands over an internet socket;
 * listens on a well-known port for instances of vmc and rmc;
 * and maintains several key data structures.
 */

/* so emc reads config data, sets up data structures, listens on main port
 * every connecting client is configured or rejected, then is placed in
 * a working set to select() on.  Then commands are processed from either a 
 * file, or from an active user interface; requests/data are processed from
 * RMC and VMC, and appropriate responses are generated if necessary (whether
 * the response be to the requesting client, or to somebody else (i.e., if 
 * VMC pushes a position update, EMC does not need to respond to VMC -- but it
 * might need to push data to RMC!)
 */

// so we need to store a bunch of data for data from RMC, VMC, and initial
// all this data can reuse the same data structure, which includes a list of
//   robot id
//   position
//   orientation
//   status

// for the initial configuration of rmc, we need to send a list of id -> 
// hostname pairs (later we will also need to send a bounding box, but not
// until the vision system is a long ways along).

// for the initial configuration of vmc, we need to send a list of robot ids,
// nothing more (as the vision system starts picking up robots, it will query
// emc with known positions/orientations to get matching ids)

// so the emc config file is a bunch of lines with id number, hostname, initial
// position (x, y, and r floating point vals).  We read this file into the 
// initial position data structure, as well as a list of id to hostname
// mappings.

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>


#include "robot_list.h"
#include "mtp.h"


#define EMC_SERVER_PORT 2525


struct rmc_client {
  int sock_fd;
  struct robot_list *position_list;
};

struct vmc_client {
  int sock_fd;
  struct robot_list *position_list;
};


/* functions used in main */
void usage(char *name);
void parse_config_file();
void parse_movement_file();



/* global config data */
char *config_file = NULL;
char *movement_file = NULL;
int port = 2525;
int daemon_mode = 0;
int command_seq_no = 0;


/* initial data pulled from the config file */
/* it's somewhat non-intuitive to use lists here -- since we're doing maps,
 * we could just as well use hashtables -- but speed is not a big deal here,
 * and linked lists might be easier for everybody to understand
 */
/* the robot_id/hostname pairings as given in the config file */
struct robot_list *hostname_list = NULL;
/* the robot_id/initial positions as given in the config file */
struct robot_list *initial_position_list = NULL;
/* a list of positions read in from the movement file */
struct robot_list *position_queue = NULL;

/*
 * For getopt
 */
char *optarg;
int optind;
int optopt;
int opterr = 0;

int main(int argc,char **argv) {
  int retval;
  char *optstring;
  int finished = 0;

  int emc_sock = -1;
  int emulab_sock = -1;
  struct rmc_client rmc_data;
  robot_list_create(rmc_data.position_list);
  struct vmc_client vmc_data;
  robot_list_create(vmc_data.position_list);
  //int client_fds[4];  // one for emc, rmc, vmc, and the emulab interface...
  fd_set readset,defaultset;
  int max_fd = -1;
  struct timeval tv,default_tv;
  

  /* parse options */
  optstring = "c:f:dp:";
  while ((retval = getopt(argc,argv,optstring)) != -1) {
	switch(retval) {
	case 'c':
	  config_file = optarg;
	  break;
	case 'f':
	  movement_file = optarg;
	  break;
	case 'd':
	  daemon_mode = 1;
	  break;
	case 'p':
	  port = atoi(optarg);
	  if (port == -1) {
		fprintf(stdout,"WARNING: illegible port, switching to default %d\n",
				EMC_SERVER_PORT);
		port = EMC_SERVER_PORT;
	  }
	  break;
	default:
	  fprintf(stdout,"ERROR: unknown argument '%c'\n",(char)retval);
	  usage(argv[0]);
	  exit(-1);
	  break;
	}
  }

  if (config_file == NULL) {
	fprintf(stdout,"ERROR: you must specify a config file!\n");
	usage(argv[0]);
	exit(-1);
  }
  
  // initialize the global lists
  hostname_list = robot_list_create();
  initial_position_list = robot_list_create();
  position_queue = robot_list_create();
  
  /* read config file into the above lists*/
  parse_config_file();
  /* read movement file into the position queue */
  parse_movement_file();

  // now break off into daemon mode

  // fork
  if (daemon_mode) {
	int retval;
	if ((retval = fork()) == -1) {
	  fprintf(stderr,
			  "Could not fork the child to enter daemon mode, exiting.\n");
	  exit(1);
	}
	else if (retval > 0) {
	  // in parent, exit
	  exit(0);
	}
  }

  // in child
  // close all file descriptors
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  
  chdir("/");
  
  // make init process our group leader (and thus dump any possibility
  // of our controlling tty sending us signals...)
  setpgrp();

  // in child: catch necessary signals
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGTSTP,SIG_IGN);
  signal(SIGHUP,SIG_IGN);

  umask(077);

  // done with "daemonizing"


  // set up our fdsets and select vars
  FD_ZERO(&defaultset);
  default_tv.tv_sec = 5;
  default_tv.tv_usec = 0;
  
  // in child: set up a socket to read interactive commands, a socket to 
  // listen for RMC and VMC.
  
  // first, EMC server sock:
  emc_sock = socket(AF_INET,SOCK_STREAM,0);
  if (emc_sock == -1) {
	fprintf(stdout,"Could not open socket for EMC: %s\n",strerror(errno));
	exit(1);
  }
  
  // add new socket to fdset
  FD_SET(emc_sock,&defaultset);
  max_fd = (emc_sock > max_fd)?emc_sock:max_fd;
  
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = INADDR_ANY;
  
  if (bind(emc_sock,(struct sockaddr *)(&sin),sizeof(struct sockaddr_in)) == -1) {
	fprintf(stdout,"Could not bind to socket: %s\n",strerror(errno));
	exit(1);
  }
  
  if (listen(emc_sock,32) == -1) {
	fprintf(stdout,"Could not listen on socket: %s\n",strerror(errno));
	exit(1);
  }
  
  // we use a single pre-malloc'ed packet for all uses in the while loop
  struct mtp_packet *packet = (struct mtp_packet *)malloc(sizeof(struct mtp_packet));
  if (packet == NULL) {
	fprintf(stdout,"Could not malloc packet structure.\n");
	exit(1);
  }
  
  // now we listen in our select loop for the vmc, rmc, and emulab clients.
  while (!finished) {
	readset = defaultset;
	tv = default_tv;
	
	retval = select(max_fd + 1,&readset,NULL,NULL,&tv);
	if (retval == -1) {
	  fprintf(stdout,"EMC: error in select: %s\n",strerror(errno));
	}
	else if (retval == 0) {
	  // timeout was reached; do nothing.
	}
	else {
	  // check all the descriptors:
	  // so we check emc_sock, emulab_sock, rmc_data->sock_fd, and
	  // vmc_sock_fd
	  if (FD_ISSET(emc_sock,&readset)) {
		// we have a new client -- but who?
		struct sockaddr_in client_sin;
		int addrlen = sizeof(struct sockaddr_in);
		int client_sock = accept(emc_sock,(struct sockaddr *)(&client_sin),&addrlen);
		// now, we have to read a packet right away so we know what kind of
		// client we've got
		retval = mtp_receive_packet(client_sock,&packet);
		if (packet == NULL) {
		  // bogus packet OR some malloc went wrong...
		  // we just drop on floor and close connection if it can't
		  // send the right stuff immediately
		  close(client_sock);
		}
		else {
		  if (packet->version != MTP_VERSION 
			  || packet->opcode != MTP_CONTROL_INIT) {
			// write back a control error packet
			struct mtp_control c;
			c.id = -1;
			c.code = -1;
			c.msg = "protocol error";
			struct mtp_packet *wb = mtp_make_packet(MTP_CONTROL_ERROR,
													MTP_ROLE_EMC,
													&c
													);
			mtp_send_packet(client_sock,wb);
													
			// and close
			close(client_sock);
		  }
		  else {
			// check the role to see what kind of client this is:
			if (packet->role == MTP_ROLE_RMC) {
			  if (rmc_data.sock_fd == -1) {
				// write back an RMC_CONFIG packet
				struct mtp_config_rmc r;
				r.box.horizontal = 12*12*2.54/100.0;
				r.box.vertical = 8*12*2.54/100.0;

				r.num_robots = initial_position_list->item_count;
				r.robots = (struct robot_config *)malloc(sizeof(struct robot_config)*(r.num_robots));
				if (r.robots == NULL) {
				  struct mtp_control c;
				  c.id = -1;
				  c.code = -1;
				  c.msg = "internal server error";
				  struct mtp_packet *wb = mtp_make_packet(MTP_CONTROL_ERROR,
														  MTP_ROLE_EMC,
														  &c
														  );
				  mtp_send_packet(client_sock,wb);
				  
				  // and close
				  close(client_sock);
				}
				else {
				  struct robot_list_enum *e = robot_list_enum(initial_position_list);
				  struct robot_config *rc = NULL;
				  int i = 0;
				  while ((rc = (struct robot_config *)robot_list_enum_next_element(e)) != NULL) {
					r.robots[i] = *rc;
				  }

				  // write back an rmc_config packet
				  struct mtp_packet *wb = mtp_make_packet(MTP_CONFIG_RMC,
														  MTP_ROLE_EMC,
														  &r
														  );
				  retval = mtp_send_packet(client_sock,wb);
				  if (retval == MTP_PP_SUCCESS) {
					// add descriptor to list, etc:
					rmc_data.sock_fd = client_sock;
					rmc_data.position_list = robot_list_create();
					
					// add this descriptor to the select list:
					FD_SET(rmc_data.sock_fd,&defaultset);
					max_fd = (rmc_data.sock_fd > max_fd)?(rmc_data.sock_fd):max_fd;
				  }
				  else {
					close(client_sock);
				  }
				}
			  }
			  else {
				// send back a control error packet, with a message if we
				// wish
				struct mtp_control c;
				c.id = -1;
				c.code = -1;
				c.msg = "protocol error";
				struct mtp_packet *wb = mtp_make_packet(MTP_CONTROL_ERROR,
														MTP_ROLE_EMC,
														&c
														);
				mtp_send_packet(client_sock,wb);
				
				// and close
				close(client_sock);
			  }
			}
			else if (packet->role == MTP_ROLE_EMULAB) {
			  if (emulab_sock == -1) {
				emulab_sock = client_sock;
				FD_SET(emulab_sock,&defaultset);
				max_fd = (emulab_sock > max_fd)?emulab_sock:max_fd;
			  }
			  else {
				// send back a control error packet

				close(client_sock);
			  }
			}
			else if (packet->role == MTP_ROLE_VMC) {
			  if (vmc_data.sock_fd == -1) {
				vmc_data.sock_fd = client_sock;
				vmc_data.position_list = robot_list_create();

				// add this descriptor to the select list:
				FD_SET(vmc_data.sock_fd,&defaultset);
				max_fd = (vmc_data.sock_fd > max_fd)?(vmc_data.sock_fd):max_fd;

				// write back a VMC_CONFIG packet
				  

			  }
			  else {
				// send back a control error packet, with a message if we
				// wish

				close(client_sock);
			  }
			}
			else {
			  // bogus role, drop connection on floor
			  close(client_sock);
			}
		  }
		}
	  }

	  // now check rmc:
	  if (rmc_data.sock_fd != -1 && FD_ISSET(rmc_data.sock_fd,&readset)) {
		// we can legitimately read packets with these opcodes:
		// MTP_REQUEST_POSITION, MTP_UPDATE_POSITION
		retval = mtp_receive_packet(rmc_data.sock_fd,&packet);
		if (packet == NULL) {
		  // bogus packet OR some malloc went wrong...
		  // we just drop on floor and close connection if it can't
		  // send the right stuff immediately
		  close(rmc_data.sock_fd);
		  FD_CLR(rmc_data.sock_fd,&defaultset);
		  rmc_data.sock_fd = -1;
		}
		else if (packet->version != MTP_VERSION) {
		  // return control error msg
		  struct mtp_control c;
		  c.id = -1;
		  c.code = -1;
		  c.msg = "protocol error: version";
		  struct mtp_packet *wb = mtp_make_packet(MTP_CONTROL_ERROR,
												  MTP_ROLE_EMC,
												  &c
												  );
		  mtp_send_packet(rmc_data.sock_fd,wb);
		}
		else {
		  if (packet->opcode == MTP_REQUEST_POSITION) {
			// find the latest position data for the robot:
			//   supply init pos if no positions in rmc_data or vmc_data;
			//   otherwise, take the position with the most recent timestamp
			//     from rmc_data or vmc_data
			int my_id = packet->data.request_position->robot_id;

			struct mtp_update_position *vmc_up = (struct mtp_update_position *)robot_list_search(vmc_data.position_list,my_id);
			struct mtp_update_position *rmc_up = (struct mtp_update_position *)robot_list_search(vmc_data.position_list,my_id);

			// since VMC isn't hooked in, we simply write back the rmc posit
			struct mtp_update_position up_copy = *rmc_up;
			// the status field has no meaning when this packet is being sent
			up_copy.status = -1;
			// construct the packet
			struct mtp_packet *wb = mtp_make_packet(MTP_UPDATE_POSITION,
													MTP_ROLE_EMC,
													&up_copy
													);
			mtp_send_packet(rmc_data.sock_fd,wb);
		  }
		  else if (packet->opcode == MTP_UPDATE_POSITION) {
			// store the latest data in teh robot position/status list
			// in rmc_data
			int my_id = packet->data.update_position->robot_id;
			struct mtp_update_position *up = (struct mtp_update_position *)robot_list_remove_by_id(rmc_data.position_list,my_id);
			if (up == NULL) {
			  // add teh updated one to the list :-)
			  struct mtp_update_position *up_copy = (struct mtp_update_position *)malloc(sizeof(struct mtp_update_position));
			  *up_copy = *(packet->data.update_position);
			  robot_list_append(rmc_data.position_list,my_id,up_copy);
			}
			else {
			  free(up);
			   // add teh updated one to the list :-)
			  struct mtp_update_position *up_copy = (struct mtp_update_position *)malloc(sizeof(struct mtp_update_position));
			  *up_copy = *(packet->data.update_position);
			  robot_list_append(rmc_data.position_list,my_id,up_copy);
			}
			  
			// also, if status is MTP_POSITION_STATUS_COMPLETE || 
			// MTP_POSITION_STATUS_ERROR, notify emulab, or issue the next
			// command to rmc from the movement list.

			// later ....
		  }
		  else {
			fprintf(stdout,"Received bogus packet from RMC\n");
			struct mtp_control c;
			c.id = -1;
			c.code = -1;
			c.msg = "protocol error: bad opcode";
			struct mtp_packet *wb = mtp_make_packet(MTP_CONTROL_ERROR,
													MTP_ROLE_EMC,
													&c
													);
			mtp_send_packet(rmc_data.sock_fd,wb);
		  }
		}
	  }

	  // now check vmc_data.sock_fd:

	  // now check emulab_sock:

	}
  }

  return 0;
}

void usage(char *name) {
  char *usage = "USAGE: %s -c filename [-f filename] [-d] [-p port]\n"
                "  -c filename  Path to a valid config file\n"
                "  -f filename  Path to a file containing lists of positions\n"
	            "  -d           Run in the background\n"
                "  -p port      Port to listen on\n";

  fprintf(stdout,usage,name);
}

char *read_line(FILE *fp) {
  char *retval = NULL;
  int retval_size = 10;
  int current_index = 0;
  char *status = NULL;
  int len;

  if (fp == NULL) {
	return NULL;
  }

  retval = (char *)malloc(sizeof(char)*retval_size);
  if (retval == NULL) {
	return NULL;
  }

  status = fgets((char*)(retval+current_index),retval_size - current_index,fp);
  //printf("fgets read '%s'\n",retval+current_index);
  len = strlen(retval);
  current_index = len;
  while (status != NULL && retval[(len > 0)?len-1:len] != '\n') {
	// realloc & add more crap.
	if (len == (retval_size - 1)) {
	  // need to realloc:
	  //printf("realloc'ing:\n");
	  status = (char*)realloc(retval,retval_size*2);
	  if (status == NULL) {
		free(retval);
		return NULL;
	  }
	  else {
		//retval = status;
		retval_size *= 2;
	  }
	}

	//printf("wl %d %d\n",len,retval_size);
	status = fgets((char*)(retval+current_index),retval_size - current_index,fp);
	len = strlen(retval);
	current_index = len;
  }

  // terminate the string.
  if (status == NULL) {
	free(retval);
	//printf("returning null\n");
	return NULL;
  }
  else {
	retval[(len > 0)?len-1:len] = '\0';
	
	return retval;
  }
}

void parse_config_file() {
  FILE *fp;

  if (config_file == NULL) {
	return;
  }

  fp = fopen(config_file,"r");
  if (fp == NULL) {
	fprintf(stderr,"Could not open config file!\n");
	exit(-1);
  }

  // read line by line
  char *line = NULL;
  int line_no = 0;
  while ((line = read_line(fp)) != NULL) {
	int id;
	char *hostname;
	float init_x;
	float init_y;
	float init_theta;

	++line_no;

	// parse the line
	// sorry, i ain't using regex.h to do this simple crap
	// lines look like '5 garcia1.flux.utah.edu 6.5 4.234582 0.38'
	// the regex would be '\s*\S+\s+\S+\s+\S+\s+\S+\s+\S+'
	// so basically, 5 non-whitespace groups separated by whitespace (tabs or
	// spaces)

	// we use strtok_r because i'm complete
	char *token = NULL;
	char *delim = " \t";
	char *state = NULL;
	
	token = strtok_r(line,delim,&state);
	if (token == NULL) {
	  fprintf(stdout,"Syntax error in config file, line %d.\n",line_no);
	  free(line);
	  continue;
	}
	id = atoi(token);

	token = strtok_r(NULL,delim,&state);
	if (token == NULL) {
	  fprintf(stdout,"Syntax error in config file, line %d.\n",line_no);
	  free(line);
	  continue;
	}
	hostname = strdup(token);

	token = strtok_r(NULL,delim,&state);
	if (token == NULL) {
	  fprintf(stdout,"Syntax error in config file, line %d.\n",line_no);
	  free(line);
	  continue;
	}
	init_x = (float)atof(token);
	  
	token = strtok_r(NULL,delim,&state);
	if (token == NULL) {
	  fprintf(stdout,"Syntax error in config file, line %d.\n",line_no);
	  free(line);
	  continue;
	}
	init_y = (float)atof(token);

	token = strtok_r(NULL,delim,&state);
	if (token == NULL) {
	  fprintf(stdout,"Syntax error in config file, line %d.\n",line_no);
	  free(line);
	  continue;
	}
	init_theta = (float)atof(token);

	
	// now we save this data to the lists:
	struct robot_config *rc = (struct robot_config *)malloc(sizeof(struct robot_config));
	rc->id = id;
	rc->hostname = hostname;
	robot_list_append(hostname_list,id,(void*)rc);
	struct position *p = (struct position *)malloc(sizeof(struct position *));
	p->x = init_x;
	p->y = init_y;
	p->theta = init_theta;
	robot_list_append(initial_position_list,id,(void*)p);


	free(line);
	// next line!
  }

}


void parse_movement_file() {
  FILE *fp;

  if (movement_file == NULL) {
	return;
  }

  fp = fopen(movement_file,"r");
  if (fp == NULL) {
	fprintf(stderr,"Could not open movement file!\n");
	exit(-1);
  }

  // read line by line
  char *line = NULL;
  int line_no = 0;
  while ((line = read_line(fp)) != NULL) {
	int id;
	float init_x;
	float init_y;
	float init_theta;

	++line_no;

	// parse the line
	// sorry, i ain't using regex.h to do this simple crap
	// lines look like '5  6.5 4.234582 0.38'
	// the regex would be '\s*\S+\s+\S+\s+\S+\s+\S+'
	// so basically, 4 non-whitespace groups separated by whitespace (tabs or
	// spaces)

	// we use strtok_r because i'm complete
	char *token = NULL;
	char *delim = " \t";
	char *state = NULL;
	
	token = strtok_r(line,delim,&state);
	if (token == NULL) {
	  fprintf(stdout,"Syntax error in movement file, line %d.\n",line_no);
	  free(line);
	  continue;
	}
	id = atoi(token);

	token = strtok_r(NULL,delim,&state);
	if (token == NULL) {
	  fprintf(stdout,"Syntax error in movement file, line %d.\n",line_no);
	  free(line);
	  continue;
	}
	init_x = (float)atof(token);
	  
	token = strtok_r(NULL,delim,&state);
	if (token == NULL) {
	  fprintf(stdout,"Syntax error in movement file, line %d.\n",line_no);
	  free(line);
	  continue;
	}
	init_y = (float)atof(token);

	token = strtok_r(NULL,delim,&state);
	if (token == NULL) {
	  fprintf(stdout,"Syntax error in movement file, line %d.\n",line_no);
	  free(line);
	  continue;
	}
	init_theta = (float)atof(token);


	// now we save this data to the lists:
	struct position *p = (struct position *)malloc(sizeof(struct position *));
	p->x = init_x;
	p->y = init_y;
	p->theta = init_theta;
	robot_list_append(position_queue,id,(void*)p);


	free(line);
	// next line!
  }

}
