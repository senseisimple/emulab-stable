#include <unistd.h>
#include <sys/time.h>

#include "f_common.h"
#include "f_timer.h"
#include <signal.h>

typedef struct { 
  int alarmTime; /* the time when the alarm will go off */
  uint wentOff;  /* bool, whether the alarm has gone off */
} Timer;

/* just one of these. */
static Timer t;

/* handles system alarm signal for when timer goes off. */
static void sigalrm_handler( int useless )
{
  /* take off every sig for great justice */
  /*
  traceprintf("time: Alarm went off!\n");
  */

  t.wentOff = 1;

  /* signal( SIGALRM, sigalrm_handler ); */
}

/* return the number of milliseconds since.. umm.. something. */
uint t_getMSec()
{
  struct timeval tv;
  
  gettimeofday( &tv, NULL );  

  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000); 
}

/* set up signal handler */
void t_init()
{
  signal( SIGALRM, sigalrm_handler );
}

void t_setTimer( int msec )
{
#ifdef OSKIT
  /* OSKIT doesnt have millisecond alarm timing, 
     so round up to the nearest second. */
  msec = ((msec / 1000) + 1) * 1000;
#endif
  t.wentOff = 0;
  t.alarmTime = t_getMSec() + msec;  

#ifdef OSKIT
  /*
  traceprintf("time: calling alarm( %i )..\n", (msec/1000));
  */
  alarm( msec / 1000 );
#else
  /*
  traceprintf("time: calling ualarm( %i )..\n", (1000 * msec) );
  */
  ualarm( 1000 * msec, 0 );
#endif
}

/* return time until timer goes off */
int t_getRemainingTime()
{
  return (t.alarmTime - t_getMSec());
}

/* returns whether the timer has gone off */
int t_getWentOff()
{
  return t.wentOff;
}
