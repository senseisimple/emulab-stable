
#ifndef TRIE_H_TRIE_1
#define TRIE_H_TRIE_1

#define KERNEL

#ifdef KERNEL

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/disklabel.h>
#include <ufs/ffs/fs.h>
#include <sys/devicestat.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>

#else

#include <stdio.h>
#include <stdlib.h>

#endif

/*--------------
  WARNING!!!!
  --------------
  The insertion functions are currently not transactional.*/

/* To change what the TrieKey and TrieValue types are, change the two
   typedefs and then change the definition of the two functions.
   TrieKey must not have a sign-extending shift.*/
typedef unsigned int TrieKey;
TrieKey getDefaultTrieKey(void);

typedef int TrieValue;
TrieValue getDefaultTrieValue(void);

/* pick one */
enum
{
    BITS_PER_LEVEL = 1
/* Don't pick the below options at this time. I'm assuming the first
   one for reasons of simplification, and because it provides the most
   compression. */
/*  BITS_PER_LEVEL = 2  */
/*  BITS_PER_LEVEL = 4  */
};

enum
{
    CHILD_COUNT = (1 << BITS_PER_LEVEL),
    TOTAL_BITS = sizeof(TrieKey) * 8,
    LEVEL_MASK = 0xffffffff >> (TOTAL_BITS - BITS_PER_LEVEL),
    TOTAL_LEVELS = TOTAL_BITS / BITS_PER_LEVEL
};

typedef struct TrieNodeTag
{
    char depth;
    char maxDepth;
    short isLeaf;
    TrieKey key;
    TrieValue value;
    struct TrieNodeTag * next;
/* Don't touch these */
    struct TrieNodeTag * parent;
    struct TrieNodeTag * child[CHILD_COUNT];
} TrieNode;

typedef void * FirstPtrT;
typedef void * SecondPtrT;
typedef void * ThirdPtrT;

typedef int (*BlockAllocateFunction)(int);
typedef int (*BlockFreeFunction)(int, int);
typedef int (*BlockCopyFunction)(FirstPtrT, SecondPtrT, 
                                 TrieKey source, TrieValue dest, 
                                 int size, int type);

typedef struct
{
    TrieNode root;
    BlockAllocateFunction blockAlloc;
    BlockFreeFunction blockFree;
    BlockCopyFunction blockCopy;
    FirstPtrT first;
    SecondPtrT second;
    ThirdPtrT third;
    TrieNode * tail;
} Trie;

typedef TrieNode * TrieIterator;

/* Call init before doing anything else. Send a pointer to a valid Trie *
   So it can allocate and setup a Trie for you and point your
   pointer to it.
   Returns 1 on success, 0 on failure */
int TrieInit(Trie ** trieOutPtr, BlockAllocateFunction blockAlloc,
             BlockFreeFunction blockFree, BlockCopyFunction blockCopy);

/* Call cleanup before exiting. Destroys the Trie. */
void TrieCleanup(Trie * triePtr);

/* Output the structure of the tree to stderr */
void logTrieStructure(Trie * triePtr);

/* Initialize an iterator based on a trie. Starts at the beginning.
   Send a pointer to a valid TrieIterator * so an iterator can be
   allocated.
   Returns 1 on success, 0 on failure */
int TrieIteratorInit(Trie * triePtr, TrieIterator * iterator);
void TrieIteratorCleanup(TrieIterator * iterator);

/* Move the TrieIterator to the next position. */
void TrieIteratorAdvance(TrieIterator * iterator);

/* Checks to see if the iterator points to a valid node.
   If the iterator is valid, this returns 1, otherwise 0. */
int TrieIteratorIsValid(TrieIterator iterator);

/* Inserts every key in [key, key+size) and skips any of these
   keys which are already inserted. This is transactional. If any insert
   fails, they all fail.
   Returns the number of keys actually inserted (non-skipped inserts).
   Returns <0 on failure */
int TrieInsertWeak(Trie * triePtr, TrieKey key, int size, FirstPtrT first,
                   SecondPtrT second, ThirdPtrT third);

/* Inserts every key in [key, key+size) and overwrites any of these
   keys which are already inserted. This inserts 'value' at key, 'value+1'
   at key+1, etc. up to size. This is transactional. If any of the inserts
   fail, they all fail.
   Returns the number of new keys inserted (non-overwrite inserts).
   Returns <0 on failure. */
int TrieInsertStrong(Trie * triePtr, TrieKey key, int size, TrieValue value);

/* This is not transactional.
   The return value determines whether it succeeded or not.
   1 for success, 0 for failure. */
int merge(Trie * dest, Trie * source);

/* This is not transactional. */
void freeAllBlocks(Trie * trie);

int depthToSize(int num);

#endif
