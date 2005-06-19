#include "trie.h"
#include "block_alloc.h"
#include "shd.h"

int bPrintBlocks;
long int log_size_used = 0;

void SetShadowSize (long size)
{
    shadow_size = size;
}

void SetShadowStart (long start)
{
    shadow_start = start;
}

void InitBlockAllocator (int method, long range_start, long range_size)
{
    reclaim_method = method;
    block_range.start = range_start;
    /*range_size = 2000;*/
    bPrintBlocks = 0;
    block_range.end = range_start + range_size;
    block_range.ptr = range_start;
    printf ("Initialized block allocator to start = %ld, end = %ld\n", block_range.ptr, block_range.end);
    SetShadowStart (range_start);
    SetShadowSize (range_size);
}
 
long CurrentFreeBlockSize ()
{
    if (block_range.end < block_range.ptr)
    {
        printf ("Error in free block list\n");
        return -1;
    }
    return (block_range.end - block_range.ptr);
}

int BlockFree (long start, long size)
{

/* called only for EXPLICIT_CKPT_DELETE. Shouldn't be called for 
   LAST_CKPT_AUTO_DELETE at all */

    struct FreeSpaceQueue* temp = shd_fs_head;
    long end;
    long next_to_end;
    long prev_to_start;

    if ((start + size) > shadow_size)
        end = (start + size) - shadow_size + shadow_start; 
    else
        end = start + size;   
    if ((end == shadow_size) || (start == shadow_start))
    {
        AddFreeSpaceToQueue (start, end);
        return 0;
    }
    next_to_end = end + 1;
    prev_to_start = start - 1;
    while (temp != 0)
    {
        if (temp->start == next_to_end)     
        {
            temp->start = start;
            /*printf ("Merging ranges (%ld, %ld) and (%ld, %ld)\n", start, end, temp->start, temp->end);*/
           temp->size += size;
            return 0;
        }
        else 
        if (temp->end == prev_to_start)
        {
            temp->end = end;
            /*printf ("Merging ranges (%ld, %ld) and (%ld, %ld)\n", start, end, temp->start, temp->end);*/
            temp->size += size;
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
    /*if (bPrintBlocks) 
    {  
        printf ("Free space queue before allocating block = \n");
        PrintFreeSpaceQueue ();
    }
    printf ("CurrentFreeBlockSize = %ld, Requested size = %ld\n", CurrentFreeBlockSize(), size);*/
    if (CurrentFreeBlockSize () < size) {
        switch (reclaim_method)
        {
         case LAST_CKPT_AUTO_DELETE:
             /* Not implemented yet */
             break;
         case EXPLICIT_CKPT_DELETE:
             if (SHD_DISK_FULL == MergeWithNextFreeBlockRange (size))
             {
                 printf ("Error! No more free space on disk\n");     
                 return SHD_DISK_FULL;
             }
             break;
        }
    }
    retVal = block_range.ptr;
    block_range.ptr += size;
    /*if (bPrintBlocks)
    {
        printf ("Allocating %d blocks starting %d\n", size, retVal);
        PrintFreeSpaceQueue ();
    }*/
    log_size_used += size;
    return retVal;
}

long MergeWithNextFreeBlockRange (long size)
{
    int done = 0;
    struct FreeSpaceQueue* shd_temp, *shd_prev;
    shd_temp = shd_prev = shd_fs_head;
    while (shd_temp != 0)
    {
        if (shd_temp->size >= size)
        {
            if (CurrentFreeBlockSize() > 0)
                AddFreeSpaceToQueue (block_range.ptr, block_range.end);
            /*printf ("Old range = (%ld, %ld) New range = (%ld, %ld)\n", block_range.ptr, block_range.end, shd_temp->start, shd_temp->end);*/
            block_range.start = block_range.ptr = shd_temp->start;
            block_range.end = shd_temp->end;

            /* Delete shd_temp from queue */
            if (shd_temp == shd_fs_head)
            {
                shd_fs_head = shd_fs_head->next;
                free (shd_temp, M_DEVBUF);
            }
            else 
            {
                shd_prev->next = shd_temp->next;
                free (shd_temp, M_DEVBUF);
            }
            return 0;
        }
        shd_prev = shd_temp;
        shd_temp = shd_temp->next;
    }
    return SHD_DISK_FULL;
}

int initFreeSpaceQueue (void)
{
    shd_fs_head = 0;
}

int AddFreeSpaceToQueue (long start, long end)
{
    struct FreeSpaceQueue* new = malloc (sizeof (struct FreeSpaceQueue),  M_DEVBUF, M_NOWAIT);
    struct FreeSpaceQueue* temp;
    new->start = start;
    new->end = end;
    new->next = 0;
    new->size = end - start;
    if (0 == shd_fs_head)
        shd_fs_head = new;
    else
    {
        temp = shd_fs_head;
        while (temp->next != 0)
            temp = temp->next;
        temp->next = new;
    } 
    return 0;
}     

int PrintFreeSpaceQueue()
{
    struct FreeSpaceQueue* current = shd_fs_head;
    while (current != 0)
    {
        printf ("%ld<->%ld  ", current->start, current->end);
        current = current->next;
    } 
    printf ("\n");
    return 0;
}

/*int main ()
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
*/
