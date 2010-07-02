/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file listNode.h
 *
 * Amiga-style doubly linked list functions.
 */

#ifndef _list_node_h
#define _list_node_h

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Basic node structure that is used for doubly linked list.
 */
struct lnMinNode {
	struct lnMinNode *ln_Succ;
	struct lnMinNode *ln_Pred;
};

/**
 * Extended node structure that is used for doubly linked lists that can be
 * ordered by priority.
 */
struct lnNode {
	struct lnNode *ln_Succ;
	struct lnNode *ln_Pred;
	int ln_Pri;
	const char *ln_Name;
};

/**
 * A combined head and tail for lists that contain lnMinNode's.
 */
struct lnMinList {
	struct lnMinNode *lh_Head; /**< points to the head node */
	struct lnMinNode *lh_Tail; /**< always == 0 */
	struct lnMinNode *lh_TailPred; /**< points to the tail node */
};

/**
 * A combined head and tail for lists that contain lnNode's.
 */
struct lnList {
	struct lnNode *lh_Head;
	struct lnNode *lh_Tail;
	struct lnNode *lh_TailPred;
	int lh_Pri;
	const char *lh_Name;
};

/**
 * Check a list to make sure its structure is sane.
 */
#define lnCheck(list) { \
	assert((list)->lh_Head != NULL); \
	assert((list)->lh_Tail == NULL); \
	assert((list)->lh_TailPred != NULL); \
}

/**
 * Remove a node from the list
 *
 * @param node The node to remove.
 */
void lnRemove(struct lnNode *node);

/**
 * Initialize a list
 *
 * @param list The list object to initialize.
 */
void lnNewList(struct lnList *list);

/**
 * Transfer the nodes on src to dest, overwriting any nodes on dest
 *
 * @param dest The destination list object.
 * @param src The source list object.
 */
void lnMoveList(struct lnList *dest, struct lnList *src);

/**
 * Append the nodes from src to dest.
 *
 * @param dest The destination list object.
 * @param src The source list object.
 */
void lnAppendList(struct lnList *dest, struct lnList *src);

/**
 * Add a node to the head of the list.
 *
 * @param list The list object the node is to be added to.
 * @param node The node to add.
 */
void lnAddHead(struct lnList *list, struct lnNode *node);

/**
 * Add a node to the tail of the list.
 *
 * @param list The list object the node is to be added to.
 * @param node The node to add.
 */
void lnAddTail(struct lnList *list, struct lnNode *node);

/**
 * Remove and return the node at the head of the list, or NULL if the list is
 * empty.
 *
 * @param list A valid list object.
 * @return The node at the head of 'list', or NULL if the list is empty.
 */
struct lnNode *lnRemHead(struct lnList *list);

/**
 * Remove and return the node at the tail of the list, or NULL if the list is
 * empty
 *
 * @param list A valid list object.
 * @return The node at the tail of 'list', or NULL if the list is empty.
 */
struct lnNode *lnRemTail(struct lnList *list);

/**
 * Check if a list is empty.
 *
 * @param list The list to check.
 * @return True if the list is empty.
 */
#define lnEmptyList(list) \
    ((struct lnNode *)(list)->lh_TailPred == (struct lnNode *)(list))

/**
 * Insert a node at a specific position in a list.
 *
 * @param pred The node in the list that the new node should be inserted after.
 * @param node The node to insert.
 */
void lnInsert(struct lnNode *pred, struct lnNode *node);

/**
 * Insert a node into a prioritized list.
 *
 * @param list An ordered list.
 * @param node The node to insert.
 */
void lnEnqueue(struct lnList *list, struct lnNode *node);

/**
 * Find a node with the given name.
 *
 * @param list The list to search.
 * @param name The name to search for.
 * @return The first node in the list that matches the given name or NULL if no
 *         match could be found.
 */
struct lnNode *lnFindName(struct lnList *list, const char *name);

/**
 * Count the number of nodes in a list.
 *
 * @param list The list to scan.
 * @return The number of nodes in the list.
 */
unsigned int lnCountNodes(struct lnList *list);

#ifdef __cplusplus
}
#endif

#endif
