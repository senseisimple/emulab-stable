
#ifndef _SCORE_H
#define _SCORE_H

// Cost of a direct link
const float SCORE_DIRECT_LINK = 0.01;
// Cost of overusing a direct link
const float SCORE_DIRECT_LINK_PENALTY = 0.5;

// Cost of an intraswitch link
const float SCORE_INTRASWITCH_LINK=0.02;

// Cost of an interswitch link
const float SCORE_INTERSWITCH_LINK=0.05;

// Cost of being unable to fulfill a virtual link
const float SCORE_NO_CONNECTION = 0.5;

// Cost of using a pnode
const float SCORE_PNODE = 0.05;
// Cost of overusing a pnode
const float SCORE_PNODE_PENALTY = 0.5;

// Cost of using a switch.
const float SCORE_SWITCH = 0.5;

// Cost of an unassigned node
const float SCORE_UNASSIGNED = 1;


extern float score;
extern int violations;

void init_score();
void remove_node(node n);
void add_node(node n);

#endif
