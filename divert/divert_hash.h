/*
 * Manage the pid / process hash table map.
 */

#ifndef DIVERT_HASH_H
#define DIVERT_HASH_H

#define ADDRTABLE_NUM_BUCKETS 263

void addrtable_init();

int addrtable_insert(unsigned long addr, int islocal);

int addrtable_remove(unsigned long addr);

int addrtable_find(unsigned long addr);

struct addrtable_entry {
	unsigned long addr;
	int islocal;
	struct addrtable_entry *next;
};
	

#endif /* DIVERT_HASH_H */
