/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _GEN_NAM_FILE_H_
#define _GEN_NAM_FILE_H_

#define HOSTNAME_LEN 10
#define MAC_ADDR_STR_LEN 15
typedef struct link_struct
{
	u_char *pointA;
	u_char *pointB;
	struct lan_node *lan_if_list;
	struct link_struct *next;
}link_type;

typedef struct lan_node
{
	u_char node[ETHADDRSIZ];
	struct lan_node *next;
}lan_node_type;

void 
add_link(struct topd_nbor *p);
int 
get_id(u_char *mac_addr);
void
gen_nam_file(const char *mesg, size_t nbytes, char *);
void
get_hostname(char *addr,char *hostname);
void
read_mac_db();
void
print_links_list();
int 
not_in_node_list(u_char *mac_addr);


#endif
