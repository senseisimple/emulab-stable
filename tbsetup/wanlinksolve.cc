/**** 
 wanlinksolve - 
 A Reasonably Effective Algorithm for Genetically Assigning Nodes.
 Chad Barb, May 3, 2002

 Applies a standard genetic algorithm (with crossover and elitism)
 to solve the problem of mapping a smaller "virtual" graph
 into a larger "physical" graph, such that the actual 
 weights between acquired nodes are similar to desired weights.

 The penalty function is the sum of error square roots.

 For the application this was designed for,
 "weights" are values of latency time in milliseconds.

 switches: 
   -v     Extra verbosity
   -v -v  Jumbo verbosity 

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

   -r <roundswobetter>
       Specify the number of rounds to continue going without a better solution.
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
       appending a space then the name of a physical node will permanently
       "fix" the mapping of this vnode to the named pnode.
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


 Some references on GA's:

 Goldberg 89
    D. E. Goldberg. Genetic Algorithms in Search, Optimization, and Machine Learning. Addison-Wesley, Reading, MA, 1989.

 Holland 75
    J. Holland. Adaptation In Natural and Artificial Systems. The University of Michigan Press, Ann Arbour, 1975.


 
****/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

// To keep things lightweight, 
// structures are statically sized to
// accommodate up to MAX_NODES nodes.
// These could be changed to STL vectors in the future.
#define MAX_NODES 20
#define MAX_DIMENSIONS 2

// default rounds to keep going without finding a better solution.
#define DEFAULT_ROUNDS 512

// The size of our "population."
// (number of phenotypes)
#define SOLUTIONS 400

// The probability a given child will mutate (out of 1000)
//#define MUTATE_PROB 40
#define MUTATE_PROB 10

// probability crossover will happen (out of 1000)
#define CROSSOVER_PROB 700

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

// some code to enable when the solver wedges and we don't know why.
#ifdef DEBUG
int wedgecount;
static inline void BEGIN_LOOP() { wedgecount = 0; } 
static inline void IN_LOOP_hlp( int line ) 
{ 
  ++wedgecount;
  if (wedgecount == 66666) { 
    printf( "possibly wedged in line %i.\n", line );
    assert(false);
  }
}
#define IN_LOOP() IN_LOOP_hlp(__LINE__)
#else

#define BEGIN_LOOP()
#define IN_LOOP()

#endif

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

static bool himutate = false;

class Solution
{
public:
  int vnode_mapping[MAX_NODES];

  // Keep around a list of how many times each physical node is used
  // in this solution, rather than recomputing it each time it is needed
  // (which is often.)
  unsigned char pnode_uses[MAX_NODES];

  float error;
  unsigned int prob;
};

// the population pool.
// at any one time, one of these is the "current" pool (where the parents come from)
// and one is the "next" pool (where the children go)
static Solution pool[SOLUTIONS];
static Solution pool2[SOLUTIONS];

// determines probabilities of selecting each solution.
static inline void prepick( Solution * currentPool )
{
  float totalFitness = 0.0f;
  int i;

  for (i = 0; i < SOLUTIONS; i++) {
    totalFitness += 1.0f / currentPool[i].error;
  }

  unsigned int probsofar = 0;
  for (i = 0; i < SOLUTIONS; i++) {
    probsofar += (unsigned int)(((float)0xFFFFFFFF / currentPool[i].error) / totalFitness);
    currentPool[i].prob = probsofar;
  }

  currentPool[SOLUTIONS - 1].prob = 0xFFFFFFFF;
}

// optimized to 1. not use fp 2. use a binary search, rather than a linear one.
// win: according to gprof, was taking 75% of the total time, now takes 17%.
static inline int pickABest( Solution * currentPool )
{
  unsigned int x = rand();

  int lo = 0;
  int hi = SOLUTIONS;
  int mid;

  while (1) {
    mid = (hi + lo) / 2;
    if (x <= currentPool[mid].prob) { 
      if (mid == 0 || x > currentPool[mid - 1].prob) break; 
      hi = mid; 
    } else { 
      lo = mid; 
    }
  } 

  return mid;
  /*
  int i = 0;
  while (1) { 
    if (x < currentPool[i].prob) break;
    i++;
    if (i == SOLUTIONS) { printf("Ick.\n"); i = 0; break;}
  }
  return i - 1;
  */
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
	    if (is == -1) {
	      if (verbose) {
		printf("%s -> %s link nonexistant! Super-icky penality of 10000 assessed.\n",
		       vnodeNames[x].c_str(), vnodeNames[y].c_str() ); 
	      }
	      err += 10000.0f;
	    } else if (should != is) { 
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

static inline void sortByError( Solution * t )
{
  // Ahh.. sweet, sweet qsort.
  qsort( t, SOLUTIONS, sizeof( Solution ), compar );
}

// "Mutating" is swapping what vnode a pnode maps to in
// the solution with what vnode a different pnode maps to.
// (if both are -1, e.g. not mapped to vnodes, nothing happens.)
static inline void mutate( Solution * t )
{
  BEGIN_LOOP();
  while(1) {
    IN_LOOP();
    // forecast calls for a 1% chance of mutation...
    if ((rand() % 1000) > (himutate ? 500 : MUTATE_PROB)) { break; }
    
    if ((rand() % 3)) {
      // swap mutation. 
      int a, b;

      BEGIN_LOOP();
      do {
	IN_LOOP();
	a = rand() % vnodes;
      } while (fixed[a] != -1);

      BEGIN_LOOP();
      do {
	IN_LOOP();
	b = rand() % vnodes;
      } while (fixed[b] != -1);

      if (a != b) {
	int temp = t->vnode_mapping[a];
	t->vnode_mapping[a] = t->vnode_mapping[b];
	t->vnode_mapping[b] = temp;
      }
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
      BEGIN_LOOP();
      do {
	IN_LOOP();
	a = rand() % vnodes;
      } while (fixed[a] != -1);

      // de-map it.
      --t->pnode_uses[ t->vnode_mapping[a] ];

      // now find a suitable physical node.
      BEGIN_LOOP();
      do {
	IN_LOOP();
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
  if ((rand() % 1000) > CROSSOVER_PROB) {
    if (rand() % 2) {
      for (int x = 0; x < vnodes; x++) {
	t->vnode_mapping[x] = a->vnode_mapping[x];
      }
      for (int y = 0; y < vnodes; y++) {
	t->pnode_uses[y] = a->pnode_uses[y];
      }
    } else {
      for (int x = 0; x < vnodes; x++) {
	t->vnode_mapping[x] = b->vnode_mapping[x];
      }
      for (int y = 0; y < vnodes; y++) {
	t->pnode_uses[y] = b->pnode_uses[y];
      }
    }
  } else {

    bzero( t->pnode_uses, sizeof( t->pnode_uses ) );
    
    for (int i = 0; i < vnodes; i++) {
      if (fixed[i] != -1) {
	t->vnode_mapping[i] = fixed[i];
	++t->pnode_uses[ fixed[i] ];
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
	if (t->pnode_uses[ a->vnode_mapping[pos] ] < maxplex[ a->vnode_mapping[pos] ]) {
	  t->vnode_mapping[pos] = a->vnode_mapping[pos];
	  ++t->pnode_uses[ a->vnode_mapping[pos] ];
	} else {
	  if (t->pnode_uses[ b->vnode_mapping[pos] ] < maxplex[ b->vnode_mapping[pos] ]) {
	    t->vnode_mapping[pos] = b->vnode_mapping[pos];
	    ++t->pnode_uses[ b->vnode_mapping[pos] ];
	  } else {
	    // failed mating (asexually clone a parent)
	    for (int x = 0; x < vnodes; x++) {
	      t->vnode_mapping[x] = a->vnode_mapping[x];
	    }
	    for (int y = 0; y < vnodes; y++) {
	      t->pnode_uses[y] = a->pnode_uses[y];
	    }
	    break;
	  }
	}
      } else {
	if (t->pnode_uses[ b->vnode_mapping[pos] ] < maxplex[ b->vnode_mapping[pos] ]) {
	  t->vnode_mapping[pos] = b->vnode_mapping[pos];
	  ++t->pnode_uses[ b->vnode_mapping[pos] ];
	} else {
	  if (t->pnode_uses[ a->vnode_mapping[pos] ] < maxplex[ a->vnode_mapping[pos] ]) {
	    t->vnode_mapping[pos] = a->vnode_mapping[pos];
	    ++t->pnode_uses[ a->vnode_mapping[pos] ];
	  } else {
	    // failed mating (asexually clone a parent)
	    for (int x = 0; x < vnodes; x++) {
	      t->vnode_mapping[x] = b->vnode_mapping[x];
	    }
	    for (int y = 0; y < vnodes; y++) {
	      t->pnode_uses[y] = b->pnode_uses[y];
	    }
	    break;
	  }
	}
      }
    }
  }

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
      BEGIN_LOOP();
      do {
	IN_LOOP();
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
	  "  -v -v           jumbo verbosity\n"
	  "  -m <penalty>    enable many-to-one virtual to physical mappings\n"
	  "  -s <seed>       seed the random number generator with a specific value\n"
	  "  -2 <weight>     enable solving for bandwidth;\n"
	  "                  multiply bandwidth penalties by <weight>.\n"
	  "  -r <minrounds>  number of rounds to go without a solution before stopping\n"
	  "                  default: %i\n"
	  "  -R <maxrounds>  set maximum rounds\n"
	  "                  default: infinite (-1)\n\n", appname, DEFAULT_ROUNDS );
  exit(1);
}

int main( int argc, char ** argv )
{
  int available = 0;
  int fixedNodeCount = 0;

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
    if (verbose > 1) { printf("How many physical nodes?\n"); }
    gets( line );
    sscanf( line, "%i", &pnodes );

    if (verbose > 1) { printf("Okay, enter %i names for the physical nodes, one per line.\n", pnodes ); }
    for (int i = 0; i < pnodes; i++) {
      char name[1024];
      gets( line );
      sscanf( line, "%s", name );
      pnodeNames[i] = string( name );
      reversePNodeNames[string(name)] = i;
    }

    if (userSpecifiedMultiplex) {
      if (verbose > 1) { printf("Enter %i numbers, one per line, indicating the"
			    " maximum number of virtual nodes allowed on each"
			    " physical node.\n", pnodes ); }
      for (int i = 0; i < pnodes; i++) {
	gets( line );
	sscanf( line, "%i", &(maxplex[i]));
	available += maxplex[i];
      }
    } else {
      for (int i = 0; i < pnodes; i++) {
	maxplex[i] = 1;
	++available;
      }
    }

    for (int z = 0; z < dimensions; z++) {
      if (verbose > 1) {
	printf("Enter %ix%i grid o' actual %s.\n", pnodes, pnodes, 
	     (z == 0) ? "latency" : "bandwidth");
      }
      for (int y = 0; y < pnodes; y++) {
	char * linePos = line;
	gets( line );
	while (*linePos == ' ') { linePos++; } // skip leading whitespace
	for (int x = 0; x < pnodes; x++) {
	  sscanf( linePos, "%i", &pLatency[z][x][y] );
	  if (z == 1) { 
	    if (pLatency[z][x][y] != -1) {
	      pLatency[z][x][y] = (int)( logf(pLatency[z][x][y]) * 1000.0f ); 
	    }
	  }
	  while (*linePos != ' ' && *linePos != '\n') { linePos++; }
	  while (*linePos == ' ') { linePos++; }
	}
      }
    }
  } 

  {
    if (verbose > 1) { printf("How many virtual nodes?\n"); }
    gets( line );
    sscanf( line, "%i", &vnodes );

    if (vnodes > available) {
      fprintf(stderr, 
	      "More nodes required (%i) than can be mapped (%i).\n", 
	      vnodes, available );
      exit(2);
    }

    if (verbose > 1) { printf("Okay, enter %i names for the virtual nodes, one per line.\n"
			      "to fix a node, suffix a space then the name of the physical node.\n", 
			      vnodes ); }

    for (int i = 0; i < vnodes; i++) {
      char name[1024];
      char pname[1024];
      gets( line );
      if (sscanf( line, "%s %s", name, pname ) == 2) {
	map< string, int >::iterator it = reversePNodeNames.find( string( pname ) );
	if (it == reversePNodeNames.end()) {
	  fprintf(stderr,"pnode '%s' does not exist; can't fix.\n", pname );
	  exit(3);
	}
	if (verbose > 1) { 
	  printf("Fixing assignment from vnode %s to pnode %s.\n", name, pname );
	}
	fixed[i] = reversePNodeNames[ string( pname ) ];
	fixedNodeCount++;
      } else {
	fixed[i] = -1;
      }
      vnodeNames[i] = string( name );
    }

    for (int z = 0; z < dimensions; z++) {
      if (verbose > 1) {
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

  Solution * currentPool = pool;
  Solution * nextPool = pool2;

  if (fixedNodeCount < vnodes) {
    if (verbose) { printf("Now running...\n"); }

    {
      for (int i = 0; i < SOLUTIONS; i++) {
	generateRandomSolution( &(currentPool[i]) );
      }
      sortByError( currentPool );
    }

    int highestFoundRound = 0;
    float last = currentPool[0].error;

    himutate = false;
    for (int i = 0; i != maxrounds && i < (highestFoundRound + minrounds); i++) {
      /*

      */
     
      float avg = 0.0f;
      int xy;
      for (xy = 0; xy < SOLUTIONS; xy++) { avg += currentPool[xy].error; }
      avg /= (float)SOLUTIONS;
      
      float stddev = 0.0f;
      for (xy = 0; xy < SOLUTIONS; xy++) { 
	float diff = currentPool[xy].error - avg;
	stddev += diff * diff;
      }
      stddev = sqrtf(stddev / (float)(SOLUTIONS - 1));
      
      if (verbose && !(i % 100)) {
	printf("Round %i. (best %4.3f; avg %4.3f; sdev %4.3f)\n", i, currentPool[0].error, avg, stddev);
      }
      
      if (currentPool[0].error < last) {
	if (verbose) {
	  printf("Better solution found in round %i (error %4.3f)\n", 
		 i, currentPool[0].error);
	}
	last = currentPool[0].error;
	highestFoundRound = i;
      }

      // this is "elitism" -- the best solution is guaranteed to survive.
      // (otherwise, a good solution may be found, but may be lost by the next generation.)
      memcpy( &(nextPool[0]), &(currentPool[0]), sizeof( Solution ) );
      prepick( currentPool );

      int j;

      // if there is not enough variety, more mutation!
      if (stddev < (avg / 10.0f)) { 
	himutate = true;
      }

      for (j = 1; j < SOLUTIONS; j++) {
	splice( &(nextPool[j]),
		&(currentPool[pickABest( currentPool )]),
		&(currentPool[pickABest( currentPool )]) );
      }

      if (stddev < 0.1f) {
	himutate = false;
      }
      /*
	printf("NUKE!\n");
	for (j = SOLUTIONS - 10; j != SOLUTIONS; j++) {
	  generateRandomSolution( &(nextPool[j]) );	
	}
      }
      */
      /*      
      for (int j = 0; j < CHILDREN_PER_ROUND; j++) {
	// Overwrite a "bad" solution with the child of two "good" ones.
	splice( &(pool[pickAWorst()]), 
		&(pool[pickABest()]), 
		&(pool[pickABest()]) ); 
      }
      */
      sortByError( nextPool );

      Solution * temp = currentPool;
      currentPool = nextPool;
      nextPool = temp;

      if (currentPool[0].error < EPSILON) { 
	if (verbose > 1) { printf("Found perfect solution.\n"); }
	break;
      }
    }
  } else {
    if (verbose) { printf("No non-fixed vnodes (or no vnodes at all,) so nothing to be done!\n"); }
  }

  {
    if (verbose) { printf("\nYour solution is as follows:\n"); }
    for (int x = 0; x < vnodes; x++) {
      if (pnodeNames.find( currentPool[0].vnode_mapping[x] ) == pnodeNames.end()) {
	pnodeNames[ currentPool[0].vnode_mapping[x] ] = "CRAP";
      }
      printf("%s mapsTo %s\n", 
	     vnodeNames[x].c_str(),
	     pnodeNames[currentPool[0].vnode_mapping[x]].c_str() );
    }

    printf("\n");

    // dump a detailed report of the returned solution's errors.
    if (verbose > 1) { calcError<true>( &(currentPool[0]) ); }
  }

  if (verbose > 1) { printf("Bye now.\n"); }
  return 0;
}
