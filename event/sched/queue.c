/*
 * queue.c --
 *
 *      Priority queue implementation for testbed event scheduler.
 *
 * @COPYRIGHT@
 */

static char rcsid[] = "$Id: queue.c,v 1.1 2001-11-02 05:40:53 imurdock Exp $";

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sched.h>

/* The size of the event queue (i.e., the number of events that can be
   pending at any given time). */
#define EVENT_QUEUE_LENGTH 131072

/* The event queue.  The event queue is implemented as a priority
   queue using the implicit tree (array) representation.  The head of
   the queue is EVENT_QUEUE[1], and the tail is
   EVENT_QUEUE[EVENT_QUEUE_TAIL].  EVENT_QUEUE[0] is initialized to 0,
   since it should never be used; this makes it easier to catch errors. */
static sched_event_t event_queue[EVENT_QUEUE_LENGTH] = { { { 0, 0 } } };

/* The index of the event queue head (fixed). */
#define EVENT_QUEUE_HEAD 1

/* The index of the event queue tail. */
static int event_queue_tail = 0;

static void sched_event_queue_dump_node_and_descendents(FILE *fp,
                                                        int index,
                                                        int level);

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

/* Enqueue the event EVENT to the priority queue.  Returns non-zero if
   successful, 0 otherwise. */
int
sched_event_enqueue(sched_event_t event)
{
    int parent, child;

    if (event_queue_tail == EVENT_QUEUE_LENGTH - 1) {
        ERROR("queue full\n");
        return 0;
    }

    /* Add the event to the priority queue.  The event is first
       inserted as a leaf of the tree, then propogated up the tree
       until the heap property is again satisfied.  At each iteration,
       we check to see if the event being inserted is more
       recent that it's parent.  If it is, we swap parent and child,
       move up one level in the tree, and iterate again; if it
       isn't, we're as far up the tree as we're going to get and are
       done. */

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

    TRACE("enqueued event (event=(time=(tv_sec=%ld, tv_usec=%ld)), "
          "host=%s, type=%d)\n",
          event.time.tv_sec,
          event.time.tv_usec,
          event.host,
          event.type);

    /* Sanity check: Make sure the heap property is satisfied. */
    sched_event_queue_verify();

    return 1;
}

/* Dequeue the next event from the priority queue.  Stores the event
   at *EVENT and returns non-zero if successful, 0 otherwise. */
int
sched_event_dequeue(sched_event_t *event)
{
    int parent, child;

    if (event == NULL) {
        ERROR("invalid event pointer\n");
        return 0;
    }

    if (event_queue_tail == 0) {
        ERROR("queue empty\n");
        return 0;
    }

    /* Remove the next event from the priority queue. */

    /* Store the item at the head of the queue in *EVENT; this is the
       event that should fire next. */
    *event = event_queue[EVENT_QUEUE_HEAD];

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

    TRACE("dequeued event (event=(time=(tv_sec=%ld, tv_usec=%ld)), "
          "host=%s, type=%d)\n",
          event->time.tv_sec,
          event->time.tv_usec,
          event->host,
          event->type);

    /* Sanity check: Make sure the heap property is satisfied. */
    sched_event_queue_verify();

    return 1;
}

/* Dump the event queue. */
void
sched_event_queue_dump(FILE *fp)
{
    sched_event_queue_dump_node_and_descendents(fp, EVENT_QUEUE_HEAD, 0);
}

/* Dump an event queue node and its descendents, with indentation for
   readability. */
static void
sched_event_queue_dump_node_and_descendents(FILE *fp, int index, int level)
{
    int i;

    if (index < EVENT_QUEUE_HEAD || index > event_queue_tail) {
        ERROR("invalid index %d (valid range is %d-%d)\n", index,
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

/* Verify that the event queue satisfies the heap property. */
void
sched_event_queue_verify(void)
{
    int error, i;

    /* Slot 0 should never be used. */
    assert(event_queue[0].time.tv_sec == 0 &&
           event_queue[0].time.tv_usec == 0);

    /* Verify the heap property: The firing time of each node N should
       be more recent than the firing time of its children 2N and 2N+1. */
    error = 0;
    for (i = EVENT_QUEUE_HEAD; i <= event_queue_tail; i++) {
        if (2 * i <= event_queue_tail) {
            if (!event_is_more_recent(event_queue[i], event_queue[2 * i]))
                error = 1;
        }
        if (2 * i + 1 <= event_queue_tail) {
            if (!event_is_more_recent(event_queue[i], event_queue[2 * i + 1]))
                error = 1;
        }
        if (error) {
            ERROR("node %d violates the heap property:\n", i);
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

    /* Enqueue events. */
    printf("Enqueueing events...\n");
    fflush(stdout);
    for (i = 0; i < events; i++) {
        event.time.tv_sec = rand();
        event.time.tv_usec = rand();
        if (sched_event_enqueue(event) == 0) {
            ERROR("could not enqueue event\n");
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
        if (sched_event_dequeue(&event) == 0) {
            ERROR("could not dequeue event\n");
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
