/**** 
 wanlinksolve - 
 A Reasonably Effective Algorithm for Genetically Assigning Nodes.
 Chad Barb, May 3, 2002

 Applies a standard genetic algorithm (with crossover)
 to solve the problem of mapping a smaller "virtual" graph
 into a larger "physical" graph, such that the actual 
 weights between acquired nodes are similar to desired weights.

 The penalty function is the sum of error square roots.

 For the application this was designed for,
 "weights" are values of latency time in milliseconds.

 switches: 
   -v  Extra verbosity.

   -2 <weight>
       Solve for 2nd (bandwidth) dimension;
       Penalty for error is sum of differences of natural logs,
       multiplied by <weight>.
       A good weight seems to be around 7, but the appropriate
       value is pretty much application-dependent.

   -m <plexpenalty>
       Handle multiple virtual->physical mapping.
       For each vnode beyond 1 assigned to a pnode, <plexpenalty> will be assessed.
       Asks for 'p' lines of input, with the maximum number of virtual
       nodes allowed on a given physical node, per line (See below)

   -s <seed>
       Use a specific random seed.

   -r <minrounds>
       Specify the minimum number of rounds to go before stopping.
       Note that the algorithm won't actually stop until
       round 3n, where "n" is the round which found the best solution so far,
       or if it hits maxrounds.
       default: DEFAULT_ROUNDS

   -R <maxrounds>
       Specify the absolute maximum number of rounds.
       default: -1 (infinite)

 takes input from stdin, outputs to stdout.

 input format:

 * One line containing a single number 'p'-- the number of physical nodes.
 * 'p' lines containing the name of each physical node
 ? 'p' lines containing the maximum number of virtual nodes each 
       physical node can accommodate. ('-m' switch only)
 * 'p' lines each containing 'p' space-delimited numbers;
       this is a P x P matrix of the _actual_ weight from pnodes to 
       pnodes. For latencies, there are zeros along the diagonal.
 ? 'p' lines, a P x P matrix of actual bandwidth ('-2' switch only)


 * One line containing a single number 'v'-- the number of virtual nodes.
 * 'v' lines containing the name of each virtual node.
       If the name is prefaced with '*', then this node will be permanently
       mapped ("fix"ed) to the physical node of the same name. 
 * 'v' lines each containing 'v' space delimited numbers;
       this is a V x V matrix of the _desired_ weight between vnodes.
       '-1's indicate "don't care"s.. (e.g. for the weight between two
       not-connected vnodes.) 
       If link weights are symmetrical, this matrix will be its own
       transpose (symmetric over the diagonal.)
 ? 'v' lines, a V x V matrix of desired bandwidth ('-2' switch only)

 output format:
 
 Easily inferred. Note that the perl regexp:
 /(\S+)\smapsTo\s(\S+)/
 will grab vnode to pnode mappings (in $1 and $2, respectively.)
 
****/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>

// To keep things lightweight, 
// structures are statically sized to
// accommodate up to MAX_NODES nodes.
// These could be changed to STL vectors in the future.
#define MAX_NODES 20
#define MAX_DIMENSIONS 2

#define DEFAULT_ROUNDS 160

// The size of our "population."
// (100,000 seems to work well.)
#define SOLUTIONS 100000

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
static map< string, int > reversePNodeNames;
static map< int, string > vnodeNames;

static int pnodes, vnodes;

static int pLatency[MAX_DIMENSIONS][MAX_NODES][MAX_NODES];
static int vDesiredLatency[MAX_DIMENSIONS][MAX_NODES][MAX_NODES];

static float plexPenalty = 0.0f;
static int maxplex[MAX_NODES];
static int userSpecifiedMultiplex = 0;

static int fixed[MAX_NODES];

static int dimensions;
static float d2weight;

static int verbose = 0;

static int minrounds, maxrounds;

class Solution
{
public:
  int vnode_mapping[MAX_NODES];

  // Keep around a list of how many times each physical node is used
  // in this solution, rather than recomputing it each time it is needed
  // (which is often.)
  unsigned char pnode_uses[MAX_NODES];

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
  default: return 0; // can't happen, but -Wall doesn't realize this.
  }
}

static inline int pickAWorst()
{
  return SOLUTIONS - pickABest();
}

// uses a template to avoid massive numbers
// of "if" choices.. compiler should generate two versions,
// one with dump and one without.
template <bool verbose>
static inline void calcError( Solution * t )
{ 
  float err = 0.0f;

  if (verbose) {
    printf("Error listing:\n");
  }

  if (userSpecifiedMultiplex) {
    for (int x = 0; x < pnodes; x++) {
      if (t->pnode_uses[x] > 1) { 
	float errDelta = (float)(t->pnode_uses[x] - 1) * plexPenalty;
	if (verbose) {
	  printf("%i nodes multiplexed on physical node %s, (err [(vnodes - 1) * plexPenalty] %4.3f)\n",
		 t->pnode_uses[x],
		 pnodeNames[x].c_str(),
		 errDelta );
	}
	err += errDelta;
      }
    }
  }

  {
    for (int z = 0; z < dimensions; z++) {
      for (int x = 0; x < vnodes; x++) {
	for (int y = 0; y < vnodes; y++) {
	  int should = vDesiredLatency[z][x][y];
	  if (should != -1) {
	    int is     = pLatency[z][ t->vnode_mapping[x] ][ t->vnode_mapping[y] ];
	    if (should != is) { 
	      float errDelta;

	      if (z == 0) {
		errDelta = sqrtf((float)(abs(should - is)));
	      } else {
		errDelta = d2weight * (float)(abs(should - is));
		if (should < is) { errDelta *= 0.5f; }
	      }

	      if (verbose) {
		if (z == 0) {
		  printf("%s -> %s latency (ms) should be %i; is %i (err [sqrt |a-b|] %4.3f)\n", 
			 vnodeNames[x].c_str(), vnodeNames[y].c_str(), 
			 should, is, errDelta );
		} else {
		  printf("%s -> %s bandwidth (kB/s) should be ~%i; is ~%i (err [wt %s* ln (a/b)] %4.3f)\n", 
			 vnodeNames[x].c_str(), vnodeNames[y].c_str(), 
			 (int)expf((float)should / 1000.0f), 
			 (int)expf((float)is / 1000.0f),
			 (should < is) ? "* 0.5 " : "",
			 errDelta );
		}
	      }
	      err += errDelta;
	    }
	  }
	}
      }
    }
  }

  if (verbose) { printf("error of %4.3f\n", err ); }
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
    
    if ((rand() % 3)) {
      // swap mutation. 
      int a, b;
      do {
	a = rand() % vnodes;
	b = rand() % vnodes;
      } while (fixed[a] != -1 || fixed[b] != -1);
      int temp = t->vnode_mapping[a];
      t->vnode_mapping[a] = t->vnode_mapping[b];
      t->vnode_mapping[b] = temp;
    } else {
      // reassign mutation
      /*
      int pnode_uses[MAX_NODES];

      bzero( pnode_uses, sizeof( pnode_uses ) );

      for (int i = 0; i < vnodes; i++) {
	++pnode_uses[ t->vnode_mapping[i] ];
      }
      */

      int a, b;
      // find which vnode to remap
      do {
	a = rand() % vnodes;
      } while (fixed[a] != -1);

      // de-map it.
      --t->pnode_uses[ t->vnode_mapping[a] ];

      // now find a suitable physical node.
      do {
	b = rand() % pnodes;
      } while (t->pnode_uses[b] >= maxplex[b]);

      // now map it.
      t->vnode_mapping[a] = b;
      ++t->pnode_uses[b];
    }
  }
}

// TODO: make sure that failed matings don't happen too
// often for difficult graphs.
// (perhaps have a limited retry)
static inline void splice( Solution * t, Solution * a, Solution * b)
{
  // temp storage so we don't clobber t unless the child turns out to be
  // cromulent.
  int vnode_mapping_t[MAX_NODES]; 
  int pnode_uses_t[MAX_NODES];

  bzero( pnode_uses_t, sizeof( pnode_uses_t ) );

  for (int i = 0; i < vnodes; i++) {
    if (fixed[i] != -1) {
      vnode_mapping_t[i] = fixed[i];
      ++pnode_uses_t[ fixed[i] ];
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
    if (fixed[pos] != -1) continue;
    if (rand() % 2) {
      if (pnode_uses_t[ a->vnode_mapping[pos] ] < maxplex[ a->vnode_mapping[pos] ]) {
	vnode_mapping_t[pos] = a->vnode_mapping[pos];
	++pnode_uses_t[ a->vnode_mapping[pos] ];
      } else {
	if (pnode_uses_t[ b->vnode_mapping[pos] ] < maxplex[ b->vnode_mapping[pos] ]) {
	  vnode_mapping_t[pos] = b->vnode_mapping[pos];
	  ++pnode_uses_t[ b->vnode_mapping[pos] ];
	} else {
	  // failed mating.
	  return;
	}
      }
    } else {
      if (pnode_uses_t[ b->vnode_mapping[pos] ] < maxplex[ b->vnode_mapping[pos] ]) {
	vnode_mapping_t[pos] = b->vnode_mapping[pos];
	++pnode_uses_t[ b->vnode_mapping[pos] ];
      } else {
	if (pnode_uses_t[ a->vnode_mapping[pos] ] < maxplex[ a->vnode_mapping[pos] ]) {
	  vnode_mapping_t[pos] = a->vnode_mapping[pos];
	  ++pnode_uses_t[ a->vnode_mapping[pos] ];
	} else {
	  // failed mating.
	  return;
	}
      }
    }
  }

  // ok.. good one..
  // since we have a cromulent child, we now clobber t.
  for(int d = 0; d < vnodes; d++) {
    t->vnode_mapping[d] = vnode_mapping_t[d];
  }

  for(int e = 0; e < pnodes; e++) {
    t->pnode_uses[e] = pnode_uses_t[e];
  }

  // Mazeltov!

  mutate( t );
  calcError<false>( t );  
} 

// Generate a random solution 
// for the initial population.
static inline void generateRandomSolution( Solution * t )
{
  //int pnode_uses[MAX_NODES];
  bzero( t->pnode_uses, sizeof( t->pnode_uses ) );

  for (int i = 0; i < vnodes; i++) {
    if (fixed[i] != -1) {
      t->vnode_mapping[i] = fixed[i];
      ++t->pnode_uses[ fixed[i] ];
    }
  }

  for (int i = 0; i < vnodes; i++) {
    if (fixed[i] == -1) {
      int x;
      do {
	x = rand() % pnodes;
      } while( t->pnode_uses[x] >= maxplex[x] );
      ++t->pnode_uses[x];
      t->vnode_mapping[i] = x;
    }
  }

  calcError<false>( t );
}

void usage( char * appname )
{
  fprintf(stderr,
	  "Usage:\n"
	  "%s [-v] [-2 <weight>] [-r <minrounds>] [-R <maxrounds>]\n\n"
	  "  -v              extra verbosity\n"
	  "  -m <penalty>    enable many-to-one virtual to physical mappings\n"
	  "  -s <seed>       seed the random number generator with a specific value\n"
	  "  -2 <weight>     enable solving for bandwidth;\n"
	  "                  multiply bandwidth penalties by <weight>.\n"
	  "  -r <minrounds>  set minimum rounds (a.k.a. generations)\n"
	  "                  default: %i\n"
	  "  -R <maxrounds>  set maximum rounds\n"
	  "                  default: infinite (-1)\n\n", appname, DEFAULT_ROUNDS );
  exit(-2);
}

int main( int argc, char ** argv )
{
  verbose = 0;
  dimensions = 1;
  d2weight = 1.0f / 1000.0f;

  minrounds = DEFAULT_ROUNDS;
  maxrounds = -1;

  int ch;

  while ((ch = getopt(argc, argv, "v2:r:R:s:m:")) != -1) {
    switch (ch) {
    case 's':
      srand( atoi( optarg ) );
      break;
    case 'v': 
      verbose++; 
      break;
    case 'm':
      plexPenalty = atof( optarg );
      userSpecifiedMultiplex++;
      break;
    case '2': 
      dimensions = 2;
      d2weight = atof( optarg ) / 1000.0f; 
      break;
    case 'r':
      minrounds = atoi( optarg );
      break;
    case 'R':
      maxrounds = atoi( optarg );
      break;
    default: 
      usage( argv[0] );
    }
  }


  char line[1024];

  {
    if (verbose) { printf("How many physical nodes?\n"); }
    gets( line );
    sscanf( line, "%i", &pnodes );

    if (verbose) { printf("Okay, enter %i names for the physical nodes, one per line.\n", pnodes ); }
    for (int i = 0; i < pnodes; i++) {
      char name[1024];
      gets( line );
      sscanf( line, "%s", name );
      pnodeNames[i] = string( name );
      reversePNodeNames[string(name)] = i;
    }

    if (userSpecifiedMultiplex) {
      if (verbose) { printf("Enter %i numbers, one per line, indicating the"
			    " maximum number of virtual nodes allowed on each"
			    " physical node.\n", pnodes ); }
      for (int i = 0; i < pnodes; i++) {
	gets( line );
	sscanf( line, "%i", &(maxplex[i]));
      }
    } else {
      for (int i = 0; i < pnodes; i++) {
	maxplex[i] = 1;
      }
    }

    for (int z = 0; z < dimensions; z++) {
      if (verbose) {
	printf("Enter %ix%i grid o' actual %s.\n", pnodes, pnodes, 
	     (z == 0) ? "latency" : "bandwidth");
      }
      for (int y = 0; y < pnodes; y++) {
	char * linePos = line;
	gets( line );
	while (*linePos == ' ') { linePos++; } // skip leading whitespace
	for (int x = 0; x < pnodes; x++) {
	  sscanf( linePos, "%i", &pLatency[z][x][y] );
	  if (z == 1) { pLatency[z][x][y] = (int)( logf(pLatency[z][x][y]) * 1000.0f ); }
	  while (*linePos != ' ' && *linePos != '\n') { linePos++; }
	  while (*linePos == ' ') { linePos++; }
	}
      }
    }
  } 

  {
    if (verbose) { printf("How many virtual nodes?\n"); }
    gets( line );
    sscanf( line, "%i", &vnodes );

    if (verbose) { printf("Okay, enter %i names for the virtual nodes, one per line.\n"
			  "(Preface name with '*' to fix assignment to pnode of the same name.\n", vnodes ); }
    for (int i = 0; i < vnodes; i++) {
      char name[1024];
      char *namep = name;
      gets( line );
      sscanf( line, "%s", name );
      if (namep[0] == '*') {
	namep++;
	map< string, int >::iterator it = reversePNodeNames.find( string( namep ) );
	if (it == reversePNodeNames.end()) {
	  fprintf(stderr,"vnode '%s' does not match any pnode; can't fix.\n", namep );
	  exit(-1);
	}
	if (verbose) { 
	  printf("Fixing assignment from vnode %s to pnode %s.\n", namep, namep );
	}
	fixed[i] = reversePNodeNames[ string( namep ) ];
      } else {
	fixed[i] = -1;
      }
      vnodeNames[i] = string( namep );
    }

    for (int z = 0; z < dimensions; z++) {
      if (verbose) {
	printf("Enter %ix%i grid o' desired %s (-1 is don't care.)\n", vnodes, vnodes, 
	       (z == 0) ? "latency" : "bandwidth");
      }
      for (int y = 0; y < vnodes; y++) {
	char * linePos = line;
	gets( line );
	while (*linePos == ' ') { linePos++; } // skip leading whitespace
	for (int x = 0; x < vnodes; x++) {
	  sscanf( linePos, "%i", &vDesiredLatency[z][x][y] );
	  if (z == 1) { 
	    if (vDesiredLatency[z][x][y] != -1) {
	      vDesiredLatency[z][x][y] = (int)(logf(vDesiredLatency[z][x][y]) * 1000.0f); 
	    }
	  }
	  while (*linePos != ' ' && *linePos != '\n') { linePos++; }
	  while (*linePos == ' ') { linePos++; }
	}
      }
    } 
  }

  if (verbose) { printf("Thanks.. now running...\n"); }

  {
    for (int i = 0; i < SOLUTIONS; i++) {
      generateRandomSolution( &(pool[i]) );
    }
    sortByError();
  }

  {
    int highestFoundRound = 0;
    float last = pool[0].error;
    for (int i = 0; i != maxrounds && (i < minrounds || i < highestFoundRound * 3); i++) {
      if (verbose && !(i % (minrounds / 10))) {
	printf("Round %i. (best %4.3f)\n", i, pool[0].error);
      }

      if (pool[0].error < last) {
	if (verbose) {
	  printf("Better solution found in round %i (error %4.3f)\n", 
	       i, pool[0].error);
	}
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
	if (verbose) { printf("Found perfect solution.\n"); }
	break;
      }
    }
  }

  {
    if (verbose) { printf("\nYour solution is as follows:\n"); }
    for (int x = 0; x < vnodes; x++) {
      if (pnodeNames.find( pool[0].vnode_mapping[x] ) == pnodeNames.end()) {
	pnodeNames[ pool[0].vnode_mapping[x] ] = "CRAP";
      }
      printf("%s mapsTo %s\n", 
	     vnodeNames[x].c_str(),
	     pnodeNames[pool[0].vnode_mapping[x]].c_str() );
    }

    printf("\n");

    // dump a detailed report of the returned solution's errors.
    if (verbose) { calcError<true>( &(pool[0]) ); }
  }

  if (verbose) { printf("Bye now.\n"); }
  return 0;
}














