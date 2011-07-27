/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifdef __cplusplus
extern "C" {
#endif

int	PortLookup(char *service, char *hostname, int namelen, int *port);
int	PortRegister(char *service, int port);

#ifdef __cplusplus
}
#endif
