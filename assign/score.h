
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
  int delay;
#ifdef FIX_PLINK_ENDPOINTS
  int incorrect_endpoints;
#endif
} violated_info;

extern double score;
extern int violated;
extern violated_info vinfo;

void init_score();
void remove_node(vvertex vv);
int add_node(vvertex vv,pvertex pv,bool deterministic);
double get_score();
double fd_score(tb_vnode &vnoder,tb_pnode &pnoder,int *fd_violated);
pvertex make_lan_node(vvertex vv);
void delete_lan_node(pvertex pv);

#endif
