
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "mtp.h"

int main(int argc, char *argv[])
{
    int fd, port = 0, retval = EXIT_SUCCESS;

    if ((argc == 2) && sscanf(argv[1], "%d", &port) != 1) {
	fprintf(stderr, "error: port argument is not a number: %s\n", argv[1]);
	retval = EXIT_FAILURE;
    }
    else if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("socket");
	retval = EXIT_FAILURE;
    }
    else {
	struct sockaddr_in sin;
	int on_off = 1;
	
	memset(&sin, 0, sizeof(sin));
	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = INADDR_ANY;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on_off, sizeof(int));
	
	if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
	    perror("bind");
	    retval = EXIT_FAILURE;
	}
	else if (listen(fd, 5) == -1) {
	    perror("listen");
	    retval = EXIT_FAILURE;
	}
	else {
	    struct sockaddr_in sin_peer;
	    socklen_t slen;
	    int fd_peer;

	    slen = sizeof(sin_peer);
	    
	    getsockname(fd, (struct sockaddr *)&sin, &slen);
	    printf("Listening for mtp packets on port: %d\n",
		   ntohs(sin.sin_port));
	    
	    while ((fd_peer = accept(fd,
				     (struct sockaddr *)&sin_peer,
				     &slen)) != -1) {
		struct mtp_packet *mp;
		
		while (mtp_receive_packet(fd_peer, &mp) == MTP_PP_SUCCESS) {
		    int lpc;
		    
		    printf("Packet: length %d; version %d; role %d\n",
			   mp->length,
			   mp->version,
			   mp->role);
		    
		    switch (mp->opcode) {
		    case MTP_CONTROL_ERROR:
		    case MTP_CONTROL_NOTIFY:
		    case MTP_CONTROL_INIT:
		    case MTP_CONTROL_CLOSE:
			switch (mp->opcode) {
			case MTP_CONTROL_ERROR:
			    printf(" opcode:\terror\n");
			    break;
			case MTP_CONTROL_NOTIFY:
			    printf(" opcode:\tnotify\n");
			    break;
			case MTP_CONTROL_INIT:
			    printf(" opcode:\tinit\n");
			    break;
			case MTP_CONTROL_CLOSE:
			    printf(" opcode:\tclose\n");
			    break;
			}
			printf("  id:\t\t%d\n"
			       "  code:\t\t%d\n"
			       "  msg:\t\t%s\n",
			       mp->data.control->id,
			       mp->data.control->code,
			       mp->data.control->msg);
			break;

		    case MTP_CONFIG_VMC:
			printf(" opcode:\tconfig-vmc\n"
			       "  num:\t\t%d\n",
			       mp->data.config_vmc->num_robots);
			for (lpc = 0;
			     lpc < mp->data.config_vmc->num_robots;
			     lpc++) {
			    printf("  robot[%d]:\t%d, %s\n",
				   lpc,
				   mp->data.config_vmc->robots[lpc].id,
				   mp->data.config_vmc->robots[lpc].hostname);
			}
			break;
			
		    case MTP_CONFIG_RMC:
			printf(" opcode:\tconfig-rmc\n"
			       "  num:\t%d\n"
			       "  horiz:\t%f\n"
			       "  vert:\t%f\n",
			       mp->data.config_rmc->num_robots,
			       mp->data.config_rmc->box.horizontal,
			       mp->data.config_rmc->box.vertical);
			for (lpc = 0;
			     lpc < mp->data.config_rmc->num_robots;
			     lpc++) {
			    printf("  robot[%d]:\t%d, %s\n",
				   lpc,
				   mp->data.config_rmc->robots[lpc].id,
				   mp->data.config_rmc->robots[lpc].hostname);
			}
			break;
			
		    case MTP_REQUEST_POSITION:
			printf(" opcode:\trequest-position\n"
			       "  id:\t\t%d\n",
			       mp->data.request_position->robot_id);
			break;
			
		    case MTP_REQUEST_ID:
			printf(" opcode:\trequest-id\n"
			       "  x:\t\t%f\n"
			       "  y:\t\t%f\n"
			       "  theta:\t%f\n"
			       "  timestamp:\t%d\n",
			       mp->data.request_id->position.x,
			       mp->data.request_id->position.y,
			       mp->data.request_id->position.theta,
			       mp->data.request_id->timestamp);
			break;
			
		    case MTP_UPDATE_POSITION:
			printf(" opcode:\tupdate-position\n"
			       "  id:\t\t%d\n"
			       "  x:\t\t%f\n"
			       "  y:\t\t%f\n"
			       "  theta:\t%f\n"
			       "  status:\t%d\n"
			       "  timestamp:\t%d\n",
			       mp->data.update_position->robot_id,
			       mp->data.update_position->position.x,
			       mp->data.update_position->position.y,
			       mp->data.update_position->position.theta,
			       mp->data.update_position->status,
			       mp->data.update_position->timestamp);
			break;
			
		    case MTP_UPDATE_ID:
			printf(" opcode:\tupdate-id\n"
			       "  id:\t%d\n",
			       mp->data.update_position->robot_id);
			break;

		    case MTP_COMMAND_GOTO:
			printf(" opcode:\tcommand-goto\n"
			       "  commid:\t%d\n"
			       "  id:\t\t%d\n"
			       "  x:\t\t%f\n"
			       "  y:\t\t%f\n"
			       "  theta:\t%f\n",
			       mp->data.command_goto->command_id,
			       mp->data.command_goto->robot_id,
			       mp->data.command_goto->position.x,
			       mp->data.command_goto->position.y,
			       mp->data.command_goto->position.theta);
			break;

		    case MTP_COMMAND_STOP:
			printf(" opcode:\tupdate-id\n"
			       "  commid:\t%d\n"
			       "  id:\t%d\n",
			       mp->data.command_stop->command_id,
			       mp->data.command_stop->robot_id);
			break;

		    default:
			assert(0);
			break;
		    }

		    mtp_free_packet(mp);
		    mp = NULL;
		}

		close(fd_peer);
		fd_peer = -1;
		
		slen = sizeof(sin_peer);
	    }
	    
	    perror("accept");
	}
	
	close(fd);
	fd = -1;
    }
    
    return retval;
}
