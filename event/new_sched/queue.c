/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002, 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * queue.c --
 *
 *      Priority queue implementation for testbed event scheduler.
 *
 */

#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include "event-sched.h"


#ifndef TIMEVAL_TO_TIMESPEC
#define TIMEVAL_TO_TIMESPEC(tv, ts) {	  \
		(ts)->tv_sec = (tv)->tv_sec; \
		(ts)->tv_nsec = (tv)->tv_usec * 1000; \
	}
#endif

/* The size of the event queue (i.e., the number of events that can be
   pending at any given time). */
#define EVENT_QUEUE_LENGTH (1024 * 32)

/* The event queue.  The event queue is implemented as a priority
   queue using the implicit tree (array) representation.  The head of
   the queue is EVENT_QUEUE[1], and the tail is
   EVENT_QUEUE[EVENT_QUEUE_TAIL].  EVENT_QUEUE[0] is initialized to 0,
   since it should never be used; this makes it easier to catch errors. */
static sched_event_t event_queue[EVENT_QUEUE_LENGTH];

/* The index of the event queue head (fixed). */
#define EVENT_QUEUE_HEAD 1

/* The index of the event queue tail. */
static int event_queue_tail = 0;

static pthread_mutex_t event_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t event_queue_cond = PTHREAD_COND_INITIALIZER;

/* Producer/consumer semaphores. */
static sem_t event_to_consume;

static int initialized = 0;

static void sched_event_queue_dump_node_and_descendents(FILE *fp,
                                                        int index,
                                                        int level);

static void sched_event_queue_verify(void);

/* Returns non-zero if EVENT1 is more recent than EVENT2, 0 otherwise. */
static inline int
event_is_more_recent(sched_event_t event1, sched_event_t event2)
{
    if (event1.time.tv_sec != event2.time.tv_sec) {
        return event1.time.tv_sec <= event2.time.tv_sec ? 1 : 0;
    } else {
        return event1.time.tv_usec <= event2.time.tv_usec ? 1 : 0;
    }
}

/* Initialize priority queue semaphores. */
void
sched_event_init(void)
{
    sem_init(&event_to_consume, 0, 0);
    initialized = 1;
}

/* Enqueue the event EVENT to the priority queue.  Returns non-zero if
   successful, 0 otherwise. */
int
sched_event_enqueue(sched_event_t event)
{
    int parent, child;

    assert(initialized);

    pthread_mutex_lock(&event_queue_mutex);

    assert(event_queue_tail < EVENT_QUEUE_LENGTH - 1);

    /* Add the event to the priority queue.  The event is first
       inserted as a leaf of the tree, then propogated up the tree
       until the heap property is again satisfied.  At each iteration,
       we check to see if the event being inserted is more
       recent that its parent.  If it is, we swap parent and child,
       move up one level in the tree, and iterate again; if
       it isn't, we're as far up the tree as we're going to get and
       are done. */

    event_queue[++event_queue_tail] = event;

    child = event_queue_tail;
    parent = child / 2;
    while (parent >= EVENT_QUEUE_HEAD) {
        if (event_is_more_recent(event_queue[child], event_queue[parent])) {
            sched_event_t tmp;
            /* Swap PARENT and CHILD. */
            tmp = event_queue[parent];
            event_queue[parent] = event_queue[child];
            event_queue[child] = tmp;
            /* Move up one level in the tree and iterate again. */
            child = parent;
            parent /= 2;
        } else {
            /* Done. */
            break;
        }
    }

    if (debug > 2) {
	    fprintf(stderr,
		    "enqueued event (event=(notification=%p, "
		    "time=(tv_sec=%ld, tv_usec=%ld)))\n",
		    event.notification,
		    event.time.tv_sec,
		    event.time.tv_usec);
    }

    /* Sanity check: Make sure the heap property is satisfied. */
    sched_event_queue_verify();

    pthread_mutex_unlock(&event_queue_mutex);

    /* Signal that there is now an event to be consumed. */
    sem_post(&event_to_consume);

    /* Signal that there is a new event */
    pthread_cond_signal(&event_queue_cond);

    return 1;
}

/* Dequeue the next event from the priority queue.  If WAIT is
   non-zero, block until there is an event to dequeue, otherwise
   return immediately if the queue is empty.  Stores the event
   at *EVENT and returns non-zero if successful, 0 otherwise. */
int
sched_event_dequeue(sched_event_t *event, int wait)
{
    int parent, child;

    assert(initialized);

// XXX
// info("queue.c: sched_event_dequeue() called\n");
// XXX
    if (event == NULL) {
        error("invalid event pointer\n");
        return 0;
    }

    if (wait) {
        /* Wait until there is an event to be consumed. */
        sem_wait(&event_to_consume);
    } else {
        /* Return immediately if the queue is empty. */
        int val;
        sem_getvalue(&event_to_consume, &val);
        if (val == 0) {
            fprintf(stderr, "queue empty\n");
            return 0;
        }
    }

    pthread_mutex_lock(&event_queue_mutex);

    assert(event_queue_tail > 0);

    /*
     * Wait for the time to arrive of the first event. We will get
     * woken up early if a new event comes in, so must recheck each
     * time. This allows newer events to sneak in, obviously.
     */
// info("queue.c: sched_event_dequeue(): Waiting for an event\n");
    while (1) {
	    struct timespec      fireme;
	    int			 err;
		    
	    *event = event_queue[EVENT_QUEUE_HEAD];

	    TIMEVAL_TO_TIMESPEC(&event->time, &fireme);

	    if (debug > 3) {
		    fprintf(stderr,
			    "sleeping until time=(tv_sec=%ld, tv_usec=%ld).\n",
			    event->time.tv_sec, event->time.tv_usec);
	    }

	    if ((err = pthread_cond_timedwait(&event_queue_cond,
					      &event_queue_mutex, &fireme))
		!= 0) {
		    if (err != ETIMEDOUT) {
			    error("pthread_cond_timedwait failed: %d", err);
			    return -1;
		    }
		    
		    /*
		     * Timed out. Still want to check to make sure
		     * the head has not changed.
		     */
		    if (event->notification ==
			event_queue[EVENT_QUEUE_HEAD].notification)
			    break;
	    }
    }

    /* Restore the heap property.  To do this, we move the leaf
       EVENT_QUEUE[EVENT_QUEUE_TAIL] to the root and propogate it down
       the tree.  At each iteration, we check to see if either of the
       node's children are more recent than the node being propogated.
       If either are, we swap parent and the most recent child, move
       down one level in the tree, and interate again; if not,
       we're as far down the tree as we're going to get and are done. */

    event_queue[EVENT_QUEUE_HEAD] = event_queue[event_queue_tail--];

    parent = EVENT_QUEUE_HEAD;
    child = parent * 2;
    while (child <= event_queue_tail) {
        if (event_is_more_recent(event_queue[child + 1], event_queue[child])) {
            /* PARENT's other child is more recent, so descend down
               that subtree instead. */
            child = child + 1;
        }
        if (event_is_more_recent(event_queue[child], event_queue[parent])) {
            sched_event_t tmp;
            /* Swap PARENT and CHILD. */
            tmp = event_queue[parent];
            event_queue[parent] = event_queue[child];
            event_queue[child] = tmp;
            /* Move down one level in the tree and iterate again. */
            parent = child;
            child *= 2;
        } else {
            /* Done. */
            break;
        }
    }

    if (debug > 2) {
	    fprintf(stderr,
		    "dequeued event (event=(notification=%p, "
		    "time=(tv_sec=%ld, tv_usec=%ld)))\n",
		    event->notification,
		    event->time.tv_sec,
		    event->time.tv_usec);
    }

    /* Sanity check: Make sure the heap property is satisfied. */
    sched_event_queue_verify();

    pthread_mutex_unlock(&event_queue_mutex);

    return 1;
}

/* Dump the event queue. */
void
sched_event_queue_dump(FILE *fp)
{
    pthread_mutex_lock(&event_queue_mutex);
    sched_event_queue_dump_node_and_descendents(fp, EVENT_QUEUE_HEAD, 0);
    pthread_mutex_unlock(&event_queue_mutex);
}

/* Dump an event queue node and its descendents, with indentation for
   readability.  Expects EVENT_QUEUE_MUTEX to be locked. */
static void
sched_event_queue_dump_node_and_descendents(FILE *fp, int index, int level)
{
    int i;

    if (index < EVENT_QUEUE_HEAD || index > event_queue_tail) {
        error("invalid index %d (valid range is %d-%d)\n", index,
              EVENT_QUEUE_HEAD,
              event_queue_tail);
        return;
    }

    /* Print this node, with appropriate indentation. */
    for (i = 0; i < level; i++) {
        fprintf(fp, " ");
    }
    fprintf(fp, "node %d: event=(time=(tv_sec=%ld, tv_usec=%ld))\n", index,
            event_queue[index].time.tv_sec,
            event_queue[index].time.tv_usec);
    fflush(fp);

    /* If this node has children, recursively print those nodes too. */
    if (2 * index <= event_queue_tail) {
        sched_event_queue_dump_node_and_descendents(fp, 2 * index,
                                                    level + 2);
    }
    if (2 * index + 1 <= event_queue_tail) {
        sched_event_queue_dump_node_and_descendents(fp, 2 * index + 1,
                                                    level + 2);
    }
}

/* Verify that the event queue satisfies the heap property.  Expects
   EVENT_QUEUE_MUTEX to be locked. */
static void
sched_event_queue_verify(void)
{
    int err, i;

    /* Slot 0 should never be used. */
    assert(event_queue[0].time.tv_sec == 0 &&
           event_queue[0].time.tv_usec == 0);

    /* Verify the heap property: The firing time of each node N should
       be more recent than the firing time of its children 2N and 2N+1. */
    err = 0;
    for (i = EVENT_QUEUE_HEAD; i <= event_queue_tail; i++) {
        if (2 * i <= event_queue_tail) {
            if (!event_is_more_recent(event_queue[i], event_queue[2 * i]))
                err = 1;
        }
        if (2 * i + 1 <= event_queue_tail) {
            if (!event_is_more_recent(event_queue[i], event_queue[2 * i + 1]))
                err = 1;
        }
        if (err) {
            error("node %d violates the heap property:\n", i);
            sched_event_queue_dump_node_and_descendents(stderr, i, 2);
            abort();
        }
    }
}

/* Define to compile with a main that tests event queues. */
/* #define TEST_EVENT_QUEUES */

#ifdef TEST_EVENT_QUEUES
/* Main routine for testing event queues. */
int
main(int argc, char **argv)
{
    sched_event_t event;
    struct timeval now;
    int events, i;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s EVENTS\n", argv[0]);
        return 1;
    }

    events = atoi(argv[1]);

    /* Seed random number generator. */
    gettimeofday(&now, NULL);
    srand((int) now.tv_usec);

    sched_event_init();

    /* Enqueue events. */
    printf("Enqueueing events...\n");
    fflush(stdout);
    for (i = 0; i < events; i++) {
        event.time.tv_sec = rand();
        event.time.tv_usec = rand();
        if (sched_event_enqueue(event) == 0) {
            error("could not enqueue event\n");
            return 1;
        }
    }

#ifdef DEBUG
    printf("Event queue:\n");
    sched_event_queue_dump(stdout);
#endif /* DEBUG */

    /* Dequeue events. */
    printf("Dequeueing events...\n");
    fflush(stdout);
    for (i = 0; i < events; i++) {
        if (sched_event_dequeue(&event, 0) == 0) {
            error("could not dequeue event\n");
            return 1;
        }
    }

    /* Verify event queue, to make sure all is well. */
    printf("Verifying event queue...\n");
    sched_event_queue_verify();
    printf("Success!\n");

    return 0;
}
#endif /* TEST_EVENT_QUEUES */
