#include <stdlib.h>
#include "divert_hash.h"
#include <assert.h>
#include <errno.h>

static int addrtable_initted = 0;

static struct addrtable_entry *addrtable[ADDRTABLE_NUM_BUCKETS];

int hashaddr(unsigned long addr)
{
	return addr % ADDRTABLE_NUM_BUCKETS; /* Lame */
}

void addrtable_init()
{
	int i;
	if (addrtable_initted) return;
	addrtable_initted = 1;

	for (i = 0; i < ADDRTABLE_NUM_BUCKETS; i++)
		addrtable[i] = NULL;
}

/*
 * -1 --> no entry
 *  0 --> Not local
 *  1 --> Local
 */
int bucket_contains(int bucket, unsigned long addr)
{
	struct addrtable_entry *pte;
	
	if (addrtable[bucket] == NULL)
		return -1;
	/* else walk the list in the bucket */
	pte = addrtable[bucket];
	while (pte) {
		if (pte->addr == addr) return pte->islocal;
		pte = pte->next;
	}
	return -1;
}

int addrtable_find(unsigned long addr)
{
	int bucket;
	int rc;

	bucket = hashaddr(addr);
	rc = bucket_contains(bucket, addr);
	return rc;
}

int addrtable_insert(unsigned long addr, int islocal)
{
	int bucket;
	struct addrtable_entry *newent;

	bucket = hashaddr(addr);
	if (bucket_contains(bucket, addr) != -1) {
		return EEXIST;
	}
	newent = malloc(sizeof(*newent));
	if (newent == NULL) {
		return ENOMEM;
	}
	newent->addr = addr;
	newent->islocal = islocal;
	newent->next = addrtable[bucket];
	addrtable[bucket] = newent;
	return 0;
}

int addrtable_remove(unsigned long addr)
{
	int bucket;
	struct addrtable_entry *pte;
	struct addrtable_entry *prev;

	bucket = hashaddr(addr);
	if (addrtable[bucket] == NULL) {
		return -1;
	}

	if (addrtable[bucket]->addr == addr) {
		pte = addrtable[bucket];
		addrtable[bucket] = addrtable[bucket]->next;
	} else {
		/* else walk the list in the bucket */
		prev = addrtable[bucket];
		pte = prev->next;
		while (pte) {
			if (pte->addr == addr)
				break;
			prev = pte;
			pte = pte->next;
		}
		/* At this point, we have it, or it's null  */
		if (pte == NULL) {
			return -1;
		}
		prev->next = pte->next;
	}
	free(pte);
	return 0;
}
