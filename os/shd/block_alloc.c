#include "trie.h"
#include "block_alloc.h"

void SetShadowSize (long size)
{
    shadow_size = size;
}

void InitBlockAllocator (int method, long range_start, long range_size)
{
    reclaim_method = method;
    block_range.start = range_start;
    block_range.end = range_start + range_size;
    block_range.ptr = range_start;
    printf ("Initialized block allocator to start = %ld, end = %ld\n", block_range.ptr, block_range.end);
    SetShadowSize (range_size);
}
 
void DeleteCheckpoint (int version)
{
    /* dummy function. write code to merge checkpoints */
    return;
}

void CopyTree (Trie * trie, long size)
{
  /* dummy function. Iterate through the trie and adjust each block 
     number by "size". Also Copy the blocks to new locations respectively */
      
     if (0 != trie)
     {
         TrieIterator pos;
         
     } 
}

long GetLastBlock ()
{
    /* dummy function. Returns the largest block number allocated to 
       a given checkpoint */
    return 0;
}

long CurrentFreeBlockSize ()
{
    if (block_range.end < block_range.ptr)
        return (block_range.ptr + shadow_size - block_range.end);
    else
        return (block_range.end - block_range.ptr);
}

int BlockFree (long start, long size)
{

/* called only for EXPLICIT_CKPT_DELETE. Shouldn't be called for 
   LAST_CKPT_AUTO_DELETE at all */

    struct FreeSpaceQueue* temp = head;
    long end = (start + size) % shadow_size;
    /*printf ("Freeing blocks %ld to %ld\n", start, end);*/
    while (temp != 0)
    {
        if (temp->start == (end + 1) % shadow_size)
        {
            temp->start = start;
            return 0;
        }
        else 
        if ((temp->end + 1) % shadow_size == start)
        {
            temp->end = end;
            return 0;
        }
        temp = temp->next;
    }
    AddFreeSpaceToQueue (start, end);
    return 0;
}

long BlockAlloc (int size)
{
    struct FreeSpaceQueue* temp = 0;
    long retVal;
    switch (reclaim_method)
    {
     case LAST_CKPT_AUTO_DELETE:
         while (CurrentFreeBlockSize () < size)
         {
             DeleteCheckpoint (first_checkpoint);
             block_range.end = GetLastBlock (first_checkpoint);
             /* Delete the corresponding trie and merge changes with
                next checkpoint */ 
             first_checkpoint++;
         }    
         break;
     case EXPLICIT_CKPT_DELETE:
         while (CurrentFreeBlockSize () < size)
         {
             /*printf ("size = %d, free_block_size = %d\n", size, CurrentFreeBlockSize());
             printf ("end = %ld, ptr = %ld\n", block_range.end, block_range.ptr);*/
             if (-1 == MergeWithNextFreeBlockRange ( CurrentFreeBlockSize() ))
             {
                 printf ("Error! No more free space on disk\n");     
                 return -1;
             }
         }
         break;
    }
    retVal = block_range.ptr;
    block_range.ptr += size;
    /*printf ("Allocating %d blocks starting %d\n", size, retVal);*/
    return retVal;
}

long MergeWithNextFreeBlockRange (long size)
{
    int done = 0;
    Trie * trie;
    struct FreeSpaceQueue* temp = GetNextFreeSpaceFromQueue ();
    if (0 == temp)
        return -1;
    while (1)
    {
        CopyTree (trie, size);
        if ((GetLastBlock() + size) % shadow_size  >= temp->start)
        {
            block_range.start = block_range.ptr = GetLastBlock();
            block_range.end = temp->end;
            DeleteFreeSpace (temp);
            break;
        }
    }
    return (size + temp->end - temp->start);
}

int initFreeSpaceQueue (void)
{
    head = tail = 0;
}

int AddFreeSpaceToQueue (long start, long end)
{
    struct FreeSpaceQueue* new = malloc (sizeof (struct FreeSpaceQueue),  M_DEVBUF, M_NOWAIT);
    new->start = start;
    new->end = end;
    if (0 == head)
        head = tail = new;
    else
    {
        new->next = head;
        head->prev = new;
        head = new;
    } 
    return 0;
}     

struct FreeSpaceQueue* GetNextFreeSpaceFromQueue (void)
{
    struct FreeSpaceQueue* temp = 0;
    if (0 == tail)
        return 0;
    else
    {
        temp = tail;
        tail = tail->prev; 
    } 
    return temp;
}

int PrintFreeSpaceQueue()
{
    struct FreeSpaceQueue* current = head;
    while (current != 0)
    {
        printf ("%ld %ld\n", current->start, current->end);
        current = current->next;
    } 
    return 0;
}

int DeleteFreeSpace (struct FreeSpaceQueue* temp)
{
    if (head == temp)
        head = 0;
    free (temp, M_DEVBUF);
    temp = temp->next = temp->prev = 0;
    return 0;
}

int main ()
{
    struct FreeSpaceQueue* temp;
    AddFreeSpaceToQueue (10, 15);
    AddFreeSpaceToQueue (2, 4);
    PrintFreeSpaceQueue();
    temp = GetNextFreeSpaceFromQueue();
    printf ("%d %d\n", temp->start, temp->end);
    DeleteFreeSpace (temp);
    temp = GetNextFreeSpaceFromQueue();
    printf ("%d %d\n", temp->start, temp->end);
    DeleteFreeSpace (temp);
    PrintFreeSpaceQueue();
    return 0;
}

