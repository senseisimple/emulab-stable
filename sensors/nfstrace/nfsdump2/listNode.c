/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file listNode.c
 *
 * Implementation for the list node functions.
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "listNode.h"

void lnRemove(struct lnMinNode *node)
{
    node->ln_Pred->ln_Succ = node->ln_Succ;
    node->ln_Succ->ln_Pred = node->ln_Pred;
}

void lnNewList(struct lnMinList *list)
{
    list->lh_Head = (struct lnMinNode *)&(list->lh_Tail);
    list->lh_Tail = 0;
    list->lh_TailPred = (struct lnMinNode *)list;
}

void lnMoveList(struct lnMinList *dest, struct lnMinList *src)
{
    if( lnEmptyList(src) )
    {
	lnNewList(dest);
    }
    else
    {
	dest->lh_Tail = NULL;
	dest->lh_Head = src->lh_Head;
	dest->lh_TailPred = src->lh_TailPred;
	dest->lh_Head->ln_Pred = (struct lnMinNode *)dest;
	dest->lh_TailPred->ln_Succ = (struct lnMinNode *)&dest->lh_Tail;
	lnNewList(src);
    }
}

void lnAppendList(struct lnMinList *dest, struct lnMinList *src)
{
    if( !lnEmptyList(src) )
    {
	dest->lh_TailPred->ln_Succ = src->lh_Head;
	src->lh_Head->ln_Pred = dest->lh_TailPred;
	dest->lh_TailPred = src->lh_TailPred;
	dest->lh_TailPred->ln_Succ =
	    (struct lnMinNode *)&dest->lh_Tail;
	lnNewList(src);
    }
}

void lnPrependList(struct lnMinList *dest, struct lnMinList *src)
{
    if( !lnEmptyList(src) )
    {
	src->lh_TailPred->ln_Succ = dest->lh_Head;
	dest->lh_Head->ln_Pred = src->lh_TailPred;
	src->lh_Head->ln_Pred = (struct lnMinNode *)&dest->lh_Head;
	dest->lh_Head = src->lh_Head;
	lnNewList(src);
    }
}

void lnAddHead(struct lnMinList *list, struct lnMinNode *node)
{
    node->ln_Succ = list->lh_Head;
    node->ln_Pred = (struct lnMinNode *)list;
    list->lh_Head->ln_Pred = node;
    list->lh_Head = node;
}

void lnAddTail(struct lnMinList *list, struct lnMinNode *node)
{
    list->lh_TailPred->ln_Succ = node;
    node->ln_Pred = list->lh_TailPred;
    list->lh_TailPred = node;
    node->ln_Succ = (struct lnMinNode *)&(list->lh_Tail);
}

struct lnMinNode *lnRemHead(struct lnMinList *list)
{
    struct lnMinNode *remnode = 0;
    
    if( list->lh_Head->ln_Succ )
    {
	remnode = list->lh_Head;
	list->lh_Head = remnode->ln_Succ;
	list->lh_Head->ln_Pred = (struct lnMinNode *)list;
    }
    return( remnode );
}

struct lnMinNode *lnRemTail(struct lnMinList *list )
{
    struct lnMinNode *remnode = 0;
    
    if( list->lh_TailPred->ln_Pred )
    {
	remnode = list->lh_TailPred;
	list->lh_TailPred = remnode->ln_Pred;
	list->lh_TailPred->ln_Succ =
	    (struct lnMinNode *)&(list->lh_Tail);
    }
    return( remnode);
}

void lnInsert(struct lnMinNode *pred, struct lnMinNode *node)
{
    node->ln_Succ = pred->ln_Succ;
    pred->ln_Succ = node;
    node->ln_Pred = pred;
    node->ln_Succ->ln_Pred = node;
}

void lnEnqueue(struct lnList *list, struct lnNode *node)
{
    struct lnNode *curr;
    int done = 0;
    
    curr = list->lh_Head;
    while( curr->ln_Succ && !done )
    {
	if( node->ln_Pri > curr->ln_Pri )
	{
	    lnInsert((struct lnMinNode *)curr->ln_Pred,
		     (struct lnMinNode *)node);
	    done = 1;
	}
	curr = curr->ln_Succ;
    }
    if( !done )
	lnAddTail((struct lnMinList *)list, (struct lnMinNode *)node);
}

struct lnNode *lnFindName(struct lnList *list, const char *name)
{
    struct lnNode *curr, *retval = NULL;
    
    curr = list->lh_Head;
    while( (curr->ln_Succ != NULL) && (retval == NULL) )
    {
	if( strcmp(curr->ln_Name, name) == 0 )
	{
	    retval = curr;
	}
	curr = curr->ln_Succ;
    }
    return( retval );
}

unsigned int lnCountNodes(struct lnMinList *list)
{
    unsigned int retval = 0;
    struct lnMinNode *curr;

    curr = list->lh_Head;
    while( curr->ln_Succ != NULL )
    {
	retval += 1;
	curr = curr->ln_Succ;
    }
    return( retval );
}
