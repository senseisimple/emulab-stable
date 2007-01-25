/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include <signal.h>

#define MAX_SAMPLES    100000

/* Grab the TSC register. */
inline volatile unsigned long long RDTSC() {
   register unsigned long long TSC asm("eax");
   asm volatile (".byte 0xf, 0x31" : : : "eax", "edx");
   return TSC;
}

/* Global vars. */
char *progname;

void
usage()
{
	fprintf(stderr,
		"Usage: %s [-s usecs] [-n numsamples] [-t] [-l seconds] [-r numruns] [-b]\n",
		progname);
	exit(-1);
}

int main (int argc, char **argv) {

  unsigned long long tsc_values[MAX_SAMPLES];
  struct timeval tod_values[MAX_SAMPLES];
  struct timezone dummytz;
  struct timespec sleepintvl, sleptintvl;
  double loadavg[3];
  unsigned long run = 0;
  char c;
  int i;
  sigset_t block_sig;

  /* config variables */
  int sleepusecs = 0;
  int numsamples = MAX_SAMPLES;
  int usegettimeofday = 0;
  int loopdelay = 1;
  int numruns = 0;
  int block = 0;

  progname = argv[0];

  while ((c = getopt(argc, argv, "s:n:tl:r:b")) != -1) {
    switch (c) {
    case 's':
      sleepusecs = atoi(optarg);
      sleepintvl.tv_sec  =  sleepusecs / 1000000;
      sleepintvl.tv_nsec = (sleepusecs % 1000000) * 1000;
      printf("Interval: %u\n", sleepusecs);
      break;
    case 'n':
      numsamples = atoi(optarg);
      if (numsamples > MAX_SAMPLES) {
        fprintf(stderr, "Too many samples: %d (max %d).\n", 
                numsamples, MAX_SAMPLES);
        exit(-1);
      }
      else if (numsamples < 1) {
        fprintf(stderr, "Must specify a sample size of at least 1.\n");
        exit(-1);
      }
      printf("Numsamples: %u\n", numsamples);
      break;
    case 't':
      usegettimeofday = 1;
      break;
    case 'l':
      loopdelay = atoi(optarg);
      break;
    case 'r':
      numruns = atoi(optarg);
      printf("Runs: %u\n", numruns);
      break;
    case 'b':
      block = 1;
      break;
    default:
      usage();
    }
  }
  argc -= optind;
  argv += optind;

  sigemptyset (&block_sig);
  sigaddset (&block_sig, SIGTERM);
  sigaddset (&block_sig, SIGHUP);
 
  run = 1;
  while(!numruns || run <= numruns) {
    struct timeval now;

    if (block) 
      sigprocmask (SIG_BLOCK, &block_sig, NULL);
    gettimeofday(&now, &dummytz);
    printf("*** Run: %lu ***\n", run);
    printf("Time: %lu %lu\n", now.tv_sec, now.tv_usec);
    
    getloadavg(loadavg, 3);
    printf("Load: %f %f %f\n", loadavg[0], loadavg[1], loadavg[2]);
    fflush(stdout);

    for (i = 0; i < numsamples; ++i) {
      usegettimeofday ? 
        gettimeofday(&tod_values[i], &dummytz) : 
        (tsc_values[i] = RDTSC());
      sleepusecs ? 
        nanosleep(&sleepintvl, &sleptintvl) : 
        sched_yield();
    }

    for (i = 1; i < numsamples; ++i) {
      if (usegettimeofday) {
        long usecs, lastusecs, tmp;
        usecs = (tod_values[i].tv_sec * 1000000) + tod_values[i].tv_usec;
        lastusecs = (tod_values[i-1].tv_sec * 1000000) + tod_values[i-1].tv_usec;
        tmp = usecs - lastusecs;
        printf("GTOD_DIFF %ld-%d: %lu\n", run, i, tmp > 0 ? tmp : 0);
      } else {
        printf("TSC_DIFF %ld-%d: %llu\n", run, i, tsc_values[i] - tsc_values[i-1]);
      }
    }
    run++;
    fflush(stdout);
    if (block)
      sigprocmask (SIG_UNBLOCK, &block_sig, NULL);

    sleep(loopdelay);
  }

  return 0;
}




