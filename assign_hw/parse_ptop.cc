/*
 * parse ptop files.  These are basic topologies
 * that are used to represent the physical topology.
 *
 * format:
 * node <name> <type> [<type2> ...]
 *   <type> = <t>[:<max>]
 *   <t>    = pc | switch | dnard
 *   <max>  = how many virtual entities of that type per physical entity.
 * link <src>[:<mac>] <dst>[:<mac>] <size> <number>
 */

#include <LEDA/graph_alg.h>
#include <LEDA/graphwin.h>
#include <LEDA/dictionary.h>
#include <LEDA/map.h>
#include <LEDA/graph_iterator.h>
#include <iostream.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "physical.h"

extern node pnodes[MAX_PNODES];	// int -> node map
node_array<int> switch_index;

int parse_ptop(tb_pgraph &PG, istream& i)
{
  int switchi=0;
  dictionary<string, node> nmap;
  node no1;
  edge ed1;
  string s1, s2;
  char inbuf[255];
  char n1[32], n2[32];
  int size, num;
  int n=1;
  int j;
  char *snext;
  char *snode;
  char *scur;
  int isswitch;

  switch_index.init(PG,MAX_PNODES,0);
  pnodes[0] = NULL;
  while (!i.eof()) {
    char *ret;
    i.getline(inbuf, 254);
    ret = strchr(inbuf, '\n');
    if (ret) *ret = 0;
    if (strlen(inbuf) == 0) { continue; }
    
    if (!strncmp(inbuf, "node", 4)) {
      isswitch = 0;
      snext = inbuf;
      scur = strsep(&snext," ");
      if (strcmp("node",scur) != 0) {
	fprintf(stderr, "bad node line: %s\n", inbuf);
      } else {
	scur = strsep(&snext," ");
	snode = scur;
	string s(snode);
	no1 = PG.new_node();
	PG[no1].name=strdup(snode);
	PG[no1].current_type = TYPE_UNKNOWN;
	PG[no1].max_load = 0;
	PG[no1].current_load = 0;
	PG[no1].pnodes_used=0;
	for (j = 0 ; j < MAX_TYPES; ++j) {
	  PG[no1].types[j].name = NULL;
	  PG[no1].types[j].max = 0;
	}
	while ((scur = strsep(&snext," ")) != NULL) {
	  char *t,*load=scur;
	  int iload;
	  t = strsep(&load,":");
	  if (load) {
	    if (sscanf(load,"%d",&iload) != 1) {
	      fprintf(stderr,"Bad load specifier: %s\n",load);
	      iload=1;
	    }
	  } else {
	    iload=1;
	  }
	  if (strcmp(t,"pc") == 0) {
	    PG[no1].types[TYPE_PC].name = "pc";
	    PG[no1].types[TYPE_PC].max = iload;
	  } else if (strcmp(t,"dnard") == 0) {
	    PG[no1].types[TYPE_DNARD].name = "dnard";
	    PG[no1].types[TYPE_DNARD].max = iload;
	  } else if (strcmp(t,"delay") == 0) {
	    PG[no1].types[TYPE_DELAY].name = "delay";
	    PG[no1].types[TYPE_DELAY].max = iload;
	  } else if (strcmp(t,"delay_pc") == 0) {
	    // this is an unsuported type.  Have types of both
	    // delay and pc.
	    fprintf(stderr,"Unsupported type: delay_pc\n");
	  } else if (strcmp(scur,"switch") == 0) {
	    isswitch = 1;
	    PG[no1].types[TYPE_SWITCH].name = "switch";
	    PG[no1].types[TYPE_SWITCH].max = 1;
	    PG[no1].the_switch = no1;
	    switch_index[no1] = switchi++;
	  }
	}
	if (! isswitch)
	  pnodes[n++]=no1;
	nmap.insert(s, no1);
      }
    }
    else if (!strncmp(inbuf, "link", 4)) {
      if (sscanf(inbuf, "link %s %s %d %d", n1, n2, &size, &num)
	  != 4) {
	fprintf(stderr, "bad link line: %s\n", inbuf);
      } else {
	char *snode,*smac;
	char *dnode,*dmac;
	smac = n1;
	dmac = n2;
	snode = strsep(&smac,":");
	dnode = strsep(&dmac,":");
	string s1(snode);
	string s2(dnode);
	if (nmap.lookup(s1) == nil) {
	  fprintf(stderr,"PTOP error: Unknown source node %s\n",snode);
	  exit(1);
	}
	if (nmap.lookup(s2) == nil) {
	  fprintf(stderr,"PTOP error: Unknown destination node %s\n",dnode);
	  exit(1);
	}
	node node1 = nmap.access(s1);
	node node2 = nmap.access(s2);
	for (int i = 0; i < num; ++i) {
	  ed1=PG.new_edge(node1, node2);
	  PG[ed1].bandwidth=size;
	  PG[ed1].bw_used=0;
	  PG[ed1].users=0;
	  if (smac)
	    PG[ed1].srcmac = strdup(smac);
	  else
	    PG[ed1].srcmac = NULL;
	  if (dmac)
	    PG[ed1].dstmac = strdup(dmac);
	  else
	    PG[ed1].dstmac = NULL;
	}
#define ISSWITCH(n) (PG[n].types[TYPE_SWITCH].max == 1)
	if (ISSWITCH(node1) &&
	    ! ISSWITCH(node2))
	  PG[node2].the_switch = node1;
	else if (ISSWITCH(node2) &&
		 ! ISSWITCH(node1))
	  PG[node1].the_switch = node2;
      }
    } else {
      fprintf(stderr, "unknown directive: %s\n", inbuf);
    }
  }
  return n-1;
}

