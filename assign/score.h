/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */


#ifndef _SCORE_H
#define _SCORE_H


typedef struct {
  int unassigned;
  int pnode_load;
  int no_connection;
  int link_users;
  int bandwidth;
  int desires;
  int vclass;
} violated_info;

extern float score;
extern int violated;
extern violated_info vinfo;
extern int allow_trivial_links;

void init_score();
void remove_node(node n);
int add_node(node n,int loc);
float get_score();
double fd_score(tb_vnode &vnoder,tb_pnode &pnoder,int *fd_violated);

#endif
