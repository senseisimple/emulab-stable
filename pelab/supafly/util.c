#include "util.h"
#include "defs.h"

void efatal(char *msg) {
    fprintf(stdout,"%s: %s\n",msg,strerror(errno));
    exit(-1);
}

void fatal(char *msg) {
    fprintf(stdout,"%s\n",msg);
    exit(-2);
}

void ewarn(char *msg) {
    fprintf(stdout,"WARNING: %s: %s\n",msg,strerror(errno));
}

void warn(char *msg) {
    fprintf(stdout,"WARNING: %s\n",msg);
}

int fill_sockaddr(char *hostname,int port,struct sockaddr_in *new_sa) {
    struct hostent *host_ptr;

    if (port < 1) {
	return -2;
    }

    new_sa->sin_family = AF_INET;
    new_sa->sin_port = htons((short) port);
 
    if (hostname != NULL && strlen(hostname) > 0) {
	host_ptr = gethostbyname(hostname);
    
	if (!host_ptr) {
	    return -3;
	}
	
	memcpy((char *) &new_sa->sin_addr.s_addr,
	       (char *) host_ptr->h_addr_list[0],
	       host_ptr->h_length);
    }
    else {
	new_sa->sin_addr.s_addr = INADDR_ANY;
    }

    return 0;
}
