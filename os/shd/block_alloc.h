/* Change to correct value */
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

#define LAST_CKPT_AUTO_DELETE 1
#define EXPLICIT_CKPT_DELETE 2
#define SRC_TO_SHADOW 1
#define SHADOW_TO_SRC 2

int first_checkpoint;
int reclaim_method;
long shadow_size;

struct CurrentFreeBlockRange
{ 
    long start;
    long end;
    long ptr; 
} block_range;

struct FreeSpaceQueue
{  
    long start;
    long end;
    struct FreeSpaceQueue *next;
    struct FreeSpaceQueue *prev;
} *head, *tail;

void SetShadowSize (long size);
void InitBlockAllocator (int method, long range_start, long range_size);
void DeleteCheckpoint (int version);
void CopyTree (Trie * trie, long size);
long GetLastBlock ();
long CurrentFreeBlockSize ();
int BlockFree (long start, long end);
long BlockAlloc (int size);
long MergeWithNextFreeBlockRange (long size);
int initFreeSpaceQueue (void);
int AddFreeSpaceToQueue (long start, long end);
struct FreeSpaceQueue* GetNextFreeSpaceFromQueue (void);
int PrintFreeSpaceQueue();
int DeleteFreeSpace (struct FreeSpaceQueue* temp);






