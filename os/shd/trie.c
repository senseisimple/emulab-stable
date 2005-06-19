
#include "trie.h"

#define SHD_NO_MEM -1
#define SHD_DISK_FULL -2

TrieKey getDefaultTrieKey(void)
{
    return 0;
}

TrieKey getMaxTrieKey(void)
{
    return 0xffffffff;
}

TrieValue getDefaultTrieValue(void)
{
    return 0;
}

static void setLeaf(TrieNode * current)
{
    if (current != 0)
    {
        current->isLeaf = 1;
    }
}

static void clearLeaf(TrieNode * current)
{
    if (current != 0)
    {
        current->isLeaf = 0;
    }
}

/* Returns 1 if the node is a leaf, 0 otherwise. */
static int isLeaf(TrieNode * current)
{
    if (current != 0 && current->isLeaf != 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int isBranch(TrieNode * current)
{
    if (current != 0 && current->isLeaf == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/* Returns 1 on success, 0 on failure */
int TrieInit(Trie ** trieOutPtr, BlockAllocateFunction blockAlloc,
             BlockFreeFunction blockFree, BlockCopyFunction blockCopy)
{
    int result = 0;
    if (trieOutPtr != 0)
    {
        Trie * trie = malloc(sizeof(Trie)
#ifdef KERNEL
, M_DEVBUF, M_NOWAIT
#endif
);
        result = 1;
        if (trie == 0)
        {
            printf ("Error allocating memory for TrieInit\n");
            result = SHD_NO_MEM;
        }
        else
        {
            TrieNodeInit(&(trie->root));
            clearLeaf(&(trie->root));
            trie->blockAlloc = blockAlloc;
            trie->blockFree = blockFree;
            trie->blockCopy = blockCopy;
            trie->first = 0;
            trie->second = 0;
            trie->third = 0;
            trie->tail = &(trie->root);
            *trieOutPtr = trie;
        }
    }
    return result;
}

static void TrieNodeInit(TrieNode * node)
{
    if (node != 0)
    {
        int i = 0;
        node->depth = 0;
        node->maxDepth = 0;
        setLeaf(node);
        node->key = getDefaultTrieKey();
        node->value = getDefaultTrieValue();
        node->parent = 0;
        node->next = 0;
        node->prev = 0;
        for (i = 0; i < CHILD_COUNT; i++)
        {
            node->child[i] = 0;
        }
    }
}

void TrieCleanup(Trie * triePtr)
{
    if (triePtr != 0)
    {
        freeChildren(triePtr, &(triePtr->root), DEFAULT_OVERLAP);
        free(triePtr
#ifdef KERNEL
, M_DEVBUF
#endif
);
    }
}

static void TrieNodeCleanup(TrieNode * node)
{
    if (node->next != 0)
    {
        TrieNodeCleanup(node->next);
        free(node->next
#ifdef KERNEL
, M_DEVBUF
#endif
            );
    }
}

static int roundDownToPow2(int num)
{
    num = num | (num >> 1);
    num = num | (num >> 2);
    num = num | (num >> 4);
    num = num | (num >> 8);
    num = num | (num >> 16);
    return num - (num >> 1);
}

/* This is the number of leading zeroes algorithm from 'Hacker's Delight'
   I modified it to print the ceiling of the log. */
int sizeToDepth(int num)
{
    int total = 0;
    if (num == 1 || num == 0)
    {
        return 32;
    }
    --num;
    if ((num >> 16) == 0)
    {
        total += 16;
        num <<= 16;
    }
    if ((num >> 24) == 0)
    {
        total += 8;
        num <<= 8;
    }
    if ((num >> 28) == 0)
    {
        total += 4;
        num <<= 4;
    }
    if ((num >> 30) == 0)
    {
        total += 2;
        num <<= 2;
    }
    if ((num >> 31) == 0)
    {
        total += 1;
        num <<= 1;
    }
    return /*32 - */total;
}

int depthToSize(int num)
{
    return 1 << (TOTAL_BITS - num);
}

int getBlockSize(TrieKey key, int size)
{
    /* Get the rightmost '1' in the key. */
    int biggestKeyBlock = key & (~key + 1);
    if (key == getDefaultTrieKey() || biggestKeyBlock < 0)
    {
        biggestKeyBlock = 0x40000000;
    }
    if (size <= 0)
    {
        return 0;
    }
    if (biggestKeyBlock < size)
    {
        return biggestKeyBlock;
    }
    else
    {
        return roundDownToPow2(size);
    }
}

int TrieInsertWeak(Trie * triePtr, TrieKey key, int size, FirstPtrT first,
                   SecondPtrT second, ThirdPtrT third)
{
    int temp = 0;
    int copyCount = -1;
    if (triePtr != 0 && size > 0)
    {
        int blockSize = 0;
        copyCount = 0;
        triePtr->first = first;
        triePtr->second = second;
        triePtr->third = third;
        while (size > 0)
        {
            blockSize = getBlockSize(key, size);
            temp = insertWeak(triePtr, &(triePtr->root), key,
                                    sizeToDepth(blockSize));
            if (temp < 0)
            {
                copyCount = temp;
                break;
            }
            else
            {
                copyCount += temp;
            }
            key += blockSize;
            size -= blockSize;
        }
    }
    return copyCount;
}

int TrieInsertStrong(Trie * triePtr, TrieKey key, int size, TrieValue value,
                     OverlapT overlap)
{
    int insertCount = -1;
    if (triePtr != 0 && size > 0)
    {
        int blockSize = 0;
        triePtr->first = 0;
        triePtr->second = 0;
        triePtr->third = 0;
        while (size > 0)
        {
            blockSize = getBlockSize(key, size);
            insertCount += insertStrong(triePtr, &(triePtr->root), key,
                                        sizeToDepth(blockSize), value,
                                        overlap);
            key += blockSize;
            size -= blockSize;
            value += blockSize;
        }
    }
    return insertCount;
}

static int insertWeak(Trie * triePtr, TrieNode * node, TrieKey key,
                      int maxDepth)
{
    int total = 0;
    int done = 0;
    if (node == 0)
    {
        node = &(triePtr->root);
    }
    while (!done)
    {
        node = search(node, key, maxDepth);
        if (node->depth == node->maxDepth && isLeaf(node))
        {
            /* The current node is larger than our insertion.
               Nothing to do. */
            total = 0;
            done = 1;
        }
        else if (node->depth == maxDepth)
        {
            /* We don't want to go below this level.
               We are weak, so we have to break up the range
               and insert smaller blocks */

            /* There are two possibilities at this point. Either the
               node is a leaf and we are a superset of a single
               range. Or the node is a branch and there are many
               possible subranges which are being used. */
            if (isLeaf(node))
            {
                /* The node is a leaf, so we have a single range to
                   work around. */
                total = weakOverlap(triePtr, node, key, maxDepth);
            }
            else
            {
                /* The node is a branch, so there are many ranges to
                   work around. The easiest way to solve this is to
                   split the problem in two and try to
                   re-insert. Eventually, this should resolve itself
                   to the other cases as we recurse. */
                insertWeak(triePtr, node, key, maxDepth+1);
                insertWeak(triePtr, node, key + depthToSize(maxDepth+1),
                           maxDepth+1);
            }
            done = 1;
        }
        else if (isBranch(node))
        {
            /* If the node is a branch, we just insert ourselves. */
            TrieValue value = (triePtr->blockAlloc)(depthToSize(maxDepth));
            if (value >= 0)
            {
                (triePtr->blockCopy)(triePtr->first, triePtr->second,
                                     key, value,
                                     depthToSize(maxDepth), 1);
                total = addChild(triePtr, node, key, maxDepth, value);
            }
            else
            {
                total = SHD_DISK_FULL;
            }
            done = 1;
        }
        else
        {
            /* The current node is a leaf. Push it down to the next level. */
            node = pushDown(triePtr, node);
        }
    }
    return total;
}

static int insertStrong(Trie * triePtr, TrieNode * node, TrieKey key,
                        int maxDepth, TrieValue value, OverlapT overlap)
{
    int total = 0;
    int done = 0;
    if (node == 0)
    {
        node = &(triePtr->root);
    }
    while (!done)
    {
        node = search(node, key, maxDepth);
        if (node->depth == maxDepth)
        {
            /* We don't want to go below this level.
               We are strong, just replace whatever is here because
               it must be a subset of this level. */
            total = replace(triePtr, node, key, maxDepth, value, overlap);
            done = 1;
        }
        else if (isBranch(node))
        {
            /* If the node is a branch, we just insert ourselves. */
            total = addChild(triePtr, node, key, maxDepth, value);
            done = 1;
        }
        else if (node->depth == node->maxDepth)
        {
            /* If the target node is already at its lowest level
               We need to over-ride just our portion and divide the rest up to
               be inserted seperately */
            total = strongOverlap(triePtr, node, key, maxDepth, value,
                                  overlap);
            done = 1;
        }
        else
        {
            /* The current node is a leaf. Push it down to the next level. */
            node = pushDown(triePtr, node);
        }
    }
    return total;
}

static int addChild(Trie * triePtr, TrieNode * parent, TrieKey key,
                    int maxDepth, TrieValue value)
{
    int result = 0;
    TrieNode * child = malloc(sizeof(TrieNode)
#ifdef KERNEL
, M_DEVBUF, M_NOWAIT
#endif
);
    if (child != 0)
    {
        result = depthToSize(maxDepth);
        TrieNodeInit(child);
        parent->child[extract(parent->depth, key)] = child;
        child->key = key;
        child->value = value;
        child->depth = parent->depth + 1;
        child->maxDepth = maxDepth;
        setLeaf(child);
        child->parent = parent;
        addToList(triePtr, child);
    }
    else
    {
        printf ("Error allocating memory for addChild\n");
        result = SHD_NO_MEM;
    }
    return result;
}

static int replace(Trie * triePtr, TrieNode * node, TrieKey key,
                   int maxDepth, TrieValue value, OverlapT overlap)
{
    if (isLeaf(node))
    {
        if (overlap == FREE_OVERLAP)
        {
            (triePtr->blockFree)(node->value, depthToSize(node->maxDepth));
        }
    }
    else
    {
        freeChildren(triePtr, node, overlap);
    }

    /* depth is already set correctly */
    node->maxDepth = maxDepth;
    setLeaf(node);
    node->key = key;
    node->value = value;
    /* parent is already set correctly */
    /* next is already set correctly */
    /* prev is already set correctly */
    return depthToSize(maxDepth);
}

static void freeChildren(Trie * triePtr, TrieNode * node, OverlapT overlap)
{
    int i = 0;
    for ( ; i < CHILD_COUNT; i++)
    {
        if (node->child[i] != 0)
        {
            freeChildren(triePtr, node->child[i], overlap);
            removeFromList(triePtr, node->child[i]);
            if (isLeaf(node->child[i]) && overlap == FREE_OVERLAP)
            {
                (triePtr->blockFree)(node->child[i]->value,
                                     depthToSize(node->child[i]->maxDepth));
            }
            free(node->child[i]
#ifdef KERNEL
, M_DEVBUF
#endif
            );
            node->child[i] = 0;
        }
    }
}

static int strongOverlap(Trie * triePtr, TrieNode * node, TrieKey key,
                         int maxDepth, TrieValue value, OverlapT overlap)
{
    /* the old key range starts before the new one and ends after it. */
    TrieKey oldKey = node->key;
    int oldMaxDepth = node->maxDepth;
    TrieValue oldValue = node->value;
    TrieKey i = oldKey;
    TrieKey limit = key;
    TrieValue currentValue = oldValue;

    if (overlap == FREE_OVERLAP)
    {
        (triePtr->blockFree)(oldValue, depthToSize(oldMaxDepth));
    }

    node->key = key;
    node->maxDepth = maxDepth;
    node->value = value;
    for ( ; i < limit; ++i, ++currentValue)
    {
        insertStrong(triePtr, &(triePtr->root), i, TOTAL_LEVELS,
                     currentValue, overlap);
    }

    i = node->key + depthToSize(node->maxDepth);
    limit = oldKey + depthToSize(oldMaxDepth);
    currentValue += depthToSize(node->maxDepth);
    for ( ; i < limit; ++i, ++currentValue)
    {
        insertStrong(triePtr, &(triePtr->root), i, TOTAL_LEVELS,
                     currentValue, overlap);
    }
    return depthToSize(maxDepth);
}

static int weakOverlap(Trie * triePtr, TrieNode * node, TrieKey key,
                       int maxDepth)
{
    /* The old key range starts after the new one and ends before it. */
    int total = 0;
    TrieKey i = key;
    TrieKey limit = node->key;
    for ( ; i < limit; ++i)
    {
        insertWeak(triePtr, &(triePtr->root), i, TOTAL_LEVELS);
        ++total;
    }

    i = node->key + depthToSize(node->maxDepth);
    limit = key + depthToSize(maxDepth);
    for ( ; i < limit; ++i)
    {
        insertWeak(triePtr, &(triePtr->root), i, TOTAL_LEVELS);
        ++total;
    }
    return total;
}

TrieNode * search(TrieNode * node, TrieKey key, int maxDepth)
{
    int index = extract(node->depth, key);
    if (node == 0)
    {
        return 0;
    }
    else if (isLeaf(node) || node->child[index] == 0
             || node->depth == (char)maxDepth)
    {
        return node;
    }
    else
    {
        return search(node->child[index], key, maxDepth);
    }
}

static TrieNode * pushDown(Trie * triePtr, TrieNode * node)
{
    TrieNode * middle = malloc(sizeof(TrieNode)
#ifdef KERNEL
, M_DEVBUF, M_NOWAIT
#endif
);
    if (middle != 0 && node != 0)
    {
        TrieNodeInit(middle);
        middle->depth = node->depth;
        middle->maxDepth = 0;
        clearLeaf(middle);
        middle->parent = node->parent;
        middle->child[extract(node->depth, node->key)] = node;
        node->parent->child[extract(node->depth - 1, node->key)] = middle;
        node->parent = middle;
        node->depth = node->depth + 1;
        addToList(triePtr, middle);
    }
    else
    {
        printf ("Error allocating memory for pushDown\n");
        middle = SHD_NO_MEM;
    }
    return middle;
}

static int extract(char inDepth, TrieKey key)
{
    int depth = inDepth;
    return (key >> (sizeof(TrieKey)*8 - depth * BITS_PER_LEVEL
                    - BITS_PER_LEVEL)) & LEVEL_MASK;
}

void logTrieStructure(Trie * triePtr)
{
    if (triePtr != 0)
    {
        logStructure(&(triePtr->root), 0, 1);
    }
}

static char bitTable[200];

static void logStructure(TrieNode * current, int tableSize, int isYoungest)
{
    int i = 0;
    int newYoungest = -1;
    /* print out the current node
    output << "PARSE_TREE: "; */
    for (i = 0; i < tableSize; ++i)
    {
        printf(" ");
        if (bitTable[i] == 1)
        {
            printf("|");
        }
        else
        {
            printf(" ");
        }
        printf("  ");
    }
    printf(" +->(%d : %d)", current->key, current->value);
    if (isLeaf(current))
    {
        printf(" LEAF: %d", current->maxDepth);
    }
    printf("\n");

    /* print out the child nodes */
    for (i = 0; i < CHILD_COUNT; ++i)
    {
        if (current->child[i] != 0)
        {
            newYoungest = i;
        }
    }
    if (newYoungest != -1)
    {
        if (isYoungest == 1)
        {
            bitTable[tableSize] = 0;
        }
        else
        {
            bitTable[tableSize] = 1;
        }
        for (i = 0; i < CHILD_COUNT; ++i)
        {
            if (current->child[i] != 0)
            {
                int isChildYoungest = 0;
                if (i >= newYoungest)
                {
                    isChildYoungest = 1;
                }
                logStructure(current->child[i], tableSize + 1,
                             isChildYoungest);
            }
        }
    }
}

int TrieIteratorInit(Trie * triePtr, TrieIterator * iterator)
{
    /* The root is never a leaf. */
    *iterator = &(triePtr->root);
    TrieIteratorAdvance(iterator);
    return 1;
}

void TrieIteratorCleanup(TrieIterator * iterator)
{
    /* No-op for now. Perhaps this will change later. */
}

/* Move the TrieIterator to the next position. */
void TrieIteratorAdvance(TrieIterator * iterator)
{
    if (*iterator != 0)
    {
        *iterator = (*iterator)->next;
        while (*iterator != 0 && isBranch(*iterator))
        {
            *iterator = (*iterator)->next;
        }
    }
}

/* Checks to see if the iterator points to a valid node.
   If the iterator is valid, this returns 1, otherwise 0. */
int TrieIteratorIsValid(TrieIterator iterator)
{
    return iterator != 0;
}

static void addToList(Trie * triePtr, TrieNode * node)
{
    node->next = 0;
    node->prev = triePtr->tail;
    triePtr->tail->next = node;
    triePtr->tail = node;
}

static void removeFromList(Trie * triePtr, TrieNode * node)
{
    if (node->prev != 0)
    {
        node->prev->next = node->next;
    }

    if (node->next != 0)
    {
        node->next->prev = node->prev;
    }
    else
    {
        triePtr->tail = node->prev;
    }

    node->next = 0;
    node->prev = 0;
}

int merge(Trie * dest, Trie * source, OverlapT overlap)
{
    if (dest != 0 && source != 0)
    {
        TrieIterator pos;
        int ok = TrieIteratorInit(source, &pos);
        if (ok)
        {
            for ( ; TrieIteratorIsValid(pos); TrieIteratorAdvance(&pos))
            {
                TrieInsertStrong(dest, pos->key, depthToSize(pos->maxDepth),
                                 pos->value, overlap);
            }
            TrieIteratorCleanup(&pos);
        }
        return ok;
    }
    return 0;
}

/* This is not transactional. */
void freeAllBlocks(Trie * trie)
{
    if (trie != 0)
    {
        TrieIterator pos;
        if (TrieIteratorInit(trie, &pos))
        {
            for ( ; TrieIteratorIsValid(pos); TrieIteratorAdvance(&pos))
            {
                (trie->blockFree)(pos->value, depthToSize(pos->maxDepth));
            }
            TrieIteratorCleanup(&pos);
        }
    }
}
