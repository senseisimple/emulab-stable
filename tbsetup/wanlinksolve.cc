/**** 
 wanlinksolve - 
 A Reasonably Effective Algorithm for Genetically Assigning Nodes.
 Chad Barb, May 3, 2002

 applies a standard genetic algorithm (with crossover)
 to solve the problem of mapping a smaller "virtual" graph
 into a larger "physical" graph, such that the actual 
 weights between acquired nodes are similar to desired weights.

 The penalty function is the sum of error square roots.

 For the application this was designed for,
 "weights" are values of latency time in milliseconds.

 switches: none.

 takes input from stdin, outputs to stdout.

 input format:

 * One line containing a single number 'p'-- the number of physical nodes.
 * 'p' lines containing the name of each physical node
 * 'p' lines each containing 'p' space-delimited numbers;
       this is a P x P matrix of the _actual_ weight from pnodes to 
       pnodes. For latencies, there are zeros along the diagonal.

 * One line containing a single number 'v'-- the number of virtual nodes.
 * 'v' lines containing the name of each virtual node
 * 'v' lines each containing 'v' space delimited numbers;
       this is a V x V matrix of the _desired_ weight between vnodes.
       '-1's indicate "don't care"s.. (e.g. for the weight between two
       not-connected vnodes.) 
       If link weights are symmetrical, this matrix will be its own
       transpose (symmetric over the diagonal.)

 output format:
 
 Easily inferred. Note that the perl regexp:
 /(\S+)\smaps\sto\s(\S+)/
 will grab vnode to pnode mappings (in $1 and $2, respectively.)
 
****/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// To keep things lightweight, 
// structures are statically sized to
// accommodate up to MAX_NODES nodes.
// These could be changed to STL vectors in the future.
#define MAX_NODES 20

// The size of our "population."
// (100,000 seems to work well.)
#define SOLUTIONS 100000

// The minimum number of rounds to go before stopping.
// Note that the algorithm won't actually stop until
// round 3n, where "n" is the round which found the best solution so far.
#define ROUNDS 160

// The number of new children to produce each round.
// (20,000 seems to work well.)
#define CHILDREN_PER_ROUND 20000

// The probability a given child will NOT have a mutation.
// (out of 1000). 990 means 1% of children will mutate.
// (.01% will mutate twice, .0001% three times, etc.)
// 500 means 50% of children will mutate (25% twice, 12% three times, etc.)
// 500, incidentally, seems to be optimal.
#define MUTATE_PROB 500

// A score below Epsilon means we've found a perfect solution,
// so stop immediately.
#define EPSILON 0.0001f

// lets STL it up a notch.
#include <map>
#include <string>

// BAM!
using namespace std;

// keep freebsd from mumbling about gets(3) being unsafe.
#define gets( x ) fgets( x, sizeof( x ), stdin  )

// keep track of user names for nodes.
static map< int, string > pnodeNames;
static map< int, string > vnodeNames;

static int pnodes, vnodes;

static int pLatency[MAX_NODES][MAX_NODES];
static int vDesiredLatency[MAX_NODES][MAX_NODES];

class Solution
{
public:
  int pnode_mapping[MAX_NODES]; // -1 means nothin'
  float error;
};

static Solution pool[SOLUTIONS];

static inline int pickABest()
{
  // a hacky (but damn fast) probibility curve to
  // favor mating better solutions.

  switch (rand() % 4) {
  case 0: return rand() % (SOLUTIONS / 32);
  case 1: return rand() % (SOLUTIONS / 16);
  case 2: return rand() % (SOLUTIONS / 4);
  case 3: return rand() % (SOLUTIONS / 2);
  default: return 0; // can't happen, but appease -Wall.
  }
}

static inline int pickAWorst()
{
  return SOLUTIONS - pickABest();
}

// uses templates to avoid massive numbers
// of "if" choices.. compiler should generate two versions,
// one with dump and one without.
template <bool verbose>
static inline void calcError( Solution * t )
{ 
  float err = 0.0f;

  if (verbose) {
    printf("Error listing:\n");
  }

  int vnode_mapping[MAX_NODES];

  {
    for (int x = 0; x < pnodes; x++) {
      if (t->pnode_mapping[x] != -1) {
	vnode_mapping[ t->pnode_mapping[x] ] = x;
      }
    }
  }

  {
    for (int x = 0; x < vnodes; x++) {
      for (int y = 0; y < vnodes; y++) {
	int should = vDesiredLatency[x][y];
	if (should != -1) {
	  int is     = pLatency[ vnode_mapping[x] ][ vnode_mapping[y] ];
	  if (should != is) { 
	    if (verbose) {
	      printf("%s -> %s latency should be %i; is %i\n", 
		     vnodeNames[x].c_str(), vnodeNames[y].c_str(), should, is );
	    }
	    err += sqrtf((float)(abs(should - is))); 
	  }
	}
      }
    }
  }

  if (verbose) { printf("error (sum of roots) of %4.3f\n", err ); }
  t->error = err;
} 

static int compar( const void * a , const void * b )
{
  Solution * sa = (Solution *)a;
  Solution * sb = (Solution *)b;

  if (sa->error > sb->error) { return  1; } else
  if (sa->error < sb->error) { return -1; } else
  { return 0; }
}

static inline void sortByError()
{
  // Ahh.. sweet, sweet qsort.
  qsort( pool, SOLUTIONS, sizeof( Solution ), compar );
}

// "Mutating" is swapping what vnode a pnode maps to in
// the solution with what vnode a different pnode maps to.
// (if both are -1, e.g. not mapped to vnodes, nothing happens.)
static inline void mutate( Solution * t )
{
  while(1) {
    // forecast calls for a 1% chance of mutation...
    if ((rand() % 1000) < MUTATE_PROB) { break; }
    int a = rand() % pnodes;
    int b = rand() % pnodes;
    int temp = t->pnode_mapping[a];
    t->pnode_mapping[a] = t->pnode_mapping[b];
    t->pnode_mapping[b] = temp;
  }
}

// TODO: make sure that failed matings don't happen too
// often for difficult graphs.
// (perhaps have a limited retry)
static inline void splice( Solution * t, Solution * a, Solution * b)
{
  int vnode_mapping_a[MAX_NODES];
  int vnode_mapping_b[MAX_NODES];
  int vnode_mapping_t[MAX_NODES];
  int pnode_used[MAX_NODES];

  bzero( pnode_used, sizeof( pnode_used ) );

  {
    for (int x = 0; x < pnodes; x++) {
      if (a->pnode_mapping[x] != -1) {
	vnode_mapping_a[ a->pnode_mapping[x] ] = x;
      }
      if (b->pnode_mapping[x] != -1) {
	vnode_mapping_b[ b->pnode_mapping[x] ] = x;
      }
    }
  }

  // go through each mapping, and pick randomly
  // which one of the two parents' mappings the child
  // will inherit.
  // Inherit the one that makes sense if the other would
  // conflict with a previously chosen mapping.
  // If both would conflict with a previously chosen mapping.
  // its a failed mating, and we return.

  int pos = rand() % vnodes;
  for (int i = 0; i < vnodes; i++) {
    pos = (pos + 1) % vnodes;
    if (rand() % 2) {
      if (!pnode_used[ vnode_mapping_a[pos] ]) {
	vnode_mapping_t[pos] = vnode_mapping_a[pos];
	pnode_used[ vnode_mapping_a[pos] ]= 1;
      } else {
	if (!pnode_used[ vnode_mapping_b[pos] ]) {
	  vnode_mapping_t[pos] = vnode_mapping_b[pos];
	  pnode_used[ vnode_mapping_b[pos] ]= 1;
	} else {
	  // failed mating.
	  return;
	}
      }
    } else {
      if (!pnode_used[ vnode_mapping_b[pos] ]) {
	vnode_mapping_t[pos] = vnode_mapping_b[pos];
	pnode_used[ vnode_mapping_b[pos] ]= 1;
      } else {
	if (!pnode_used[ vnode_mapping_a[pos] ]) {
	  vnode_mapping_t[pos] = vnode_mapping_a[pos];
	  pnode_used[ vnode_mapping_a[pos] ]= 1;
	} else {
	  // failed mating.
	  return;
	}
      }
    }
  }

  // ok.. good one.

  for(int c = 0; c < pnodes; c++) {
    t->pnode_mapping[c] = -1;
  }

  for(int d = 0; d < vnodes; d++) {
    t->pnode_mapping[ vnode_mapping_t[d] ] = d;
  }

  // Mazeltov!

  mutate( t );
  calcError<false>( t );  
} 

// Generate a random solution 
// for the initial population.
static inline void generateRandomSolution( Solution * t )
{
  for (int i = 0; i < pnodes; i++) { t->pnode_mapping[i] = -1; }
  for (int j = 0; j < vnodes; j++) {
    while(1) {
      int r = rand() % pnodes;
      if (t->pnode_mapping[r] == -1) {
	t->pnode_mapping[r] = j;
	break;
      }
    }
  }
  calcError<false>( t );
}

int main()
{
  char line[1024];

  {
    printf("How many physical nodes?\n");
    gets( line );
    sscanf( line, "%i", &pnodes );

    printf("Okay, enter %i names for the physical nodes, one per line.\n", pnodes ); 
    for (int i = 0; i < pnodes; i++) {
      char name[1024];
      gets( line );
      sscanf( line, "%s", name );
      pnodeNames[i] = string( name );
    }

    printf("Enter %ix%i grid o' actual latency.\n", pnodes, pnodes);
    for (int y = 0; y < pnodes; y++) {
      char * linePos = line;
      gets( line );
      while (*linePos == ' ') { linePos++; } // skip leading whitespace
      for (int x = 0; x < pnodes; x++) {
	sscanf( linePos, "%i", &pLatency[x][y] );
	while (*linePos != ' ' && *linePos != '\n') { linePos++; }
	while (*linePos == ' ') { linePos++; }
      }
    }
  } 

  {
    printf("How many virtual nodes?\n");
    gets( line );
    sscanf( line, "%i", &vnodes );

    printf("Okay, enter %i names for the virtual nodes, one per line.\n", vnodes ); 
    for (int i = 0; i < vnodes; i++) {
      char name[1024];
      gets( line );
      sscanf( line, "%s", name );
      vnodeNames[i] = string( name );
    }

    printf("Enter %ix%i grid o' desired latency (-1 is don't care.)\n", vnodes, vnodes);
    for (int y = 0; y < vnodes; y++) {
      char * linePos = line;
      gets( line );
      while (*linePos == ' ') { linePos++; } // skip leading whitespace
      for (int x = 0; x < vnodes; x++) {
	sscanf( linePos, "%i", &vDesiredLatency[x][y] );
	while (*linePos != ' ' && *linePos != '\n') { linePos++; }
	while (*linePos == ' ') { linePos++; }
      }
    }
  } 

  printf("Thanks.. now running...\n");

  {
    for (int i = 0; i < SOLUTIONS; i++) {
      generateRandomSolution( &(pool[i]) );
    }
    sortByError();
  }

  {
    int highestFoundRound = 0;
    float last = pool[0].error;
    for (int i = 0; (i < ROUNDS) || (i < highestFoundRound * 3); i++) {
      if (!(i % (ROUNDS / 10))) {
	printf("Round %i. (best %4.3f)\n", i, pool[0].error);
      }

      if (pool[0].error < last) {
	printf("Better solution found in round %i (error %4.3f)\n", 
	       i, pool[0].error);
	last = pool[0].error;
	highestFoundRound = i;
      }

      for (int j = 0; j < CHILDREN_PER_ROUND; j++) {
	// Overwrite a "bad" solution with the child of two "good" ones.
	splice( &(pool[pickAWorst()]), 
		&(pool[pickABest()]), 
		&(pool[pickABest()]) ); 
      }
      sortByError();
      if (pool[0].error < EPSILON) { 
	printf("Found perfect solution.\n");
	break;
      }
    }
  }

  {
    printf("\nYour solution is as follows:\n");
    for (int x = 0; x < pnodes; x++) {
      if (pool[0].pnode_mapping[x] != -1) {
	printf("%s maps to %s\n", 
	       vnodeNames[pool[0].pnode_mapping[x]].c_str(),
	       pnodeNames[x].c_str() );
      }
    }
    printf("\n");

    // dump a detailed report of the returned solution's errors.
    calcError<true>( &(pool[0]) );
  }

  printf("Bye now.\n");
  return 0;
}














