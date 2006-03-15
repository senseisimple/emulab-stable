// log.c

#include "stub.h"
#include "log.h"

static FILE * logFile;
static int logFlags;
static int logTimestamp;

void logInit(FILE * destFile, int flags, int useTimestamp)
{
  if (destFile == NULL || flags == LOG_NOTHING)
  {
    logFile = NULL;
    logFlags = LOG_NOTHING;
    logTimestamp = 0;
  }
  else
  {
    logFile = destFile;
    logFlags = flags;
    logTimestamp = useTimestamp;
  }
}

void logCleanup(void)
{
}

// Print the timestamp and type of logging to the logFile.
static void logPrefix(int flags, struct timeval const * timestamp)
{
  if (flags & CONTROL_SEND)
  {
    fprintf(logFile, "CONTROL_SEND ");
  }
  if (flags & CONTROL_RECEIVE)
  {
    fprintf(logFile, "CONTROL_RECEIVE ");
  }
  if (flags & TCPTRACE_SEND)
  {
    fprintf(logFile, "TCPTRACE_SEND ");
  }
  if (flags & TCPTRACE_RECEIVE)
  {
    fprintf(logFile, "TCPTRACE_RECEIVE ");
  }
  if (flags & SNIFF_SEND)
  {
    fprintf(logFile, "SNIFF_SEND ");
  }
  if (flags & SNIFF_RECEIVE)
  {
    fprintf(logFile, "SNIFF_RECEIVE ");
  }
  if (flags & PEER_WRITE)
  {
    fprintf(logFile, "PEER_WRITE ");
  }
  if (flags & PEER_READ)
  {
    fprintf(logFile, "PEER_READ ");
  }
  if (flags & MAIN_LOOP)
  {
    fprintf(logFile, "MAIN_LOOP ");
  }
  if (flags & LOOKUP_DB)
  {
    fprintf(logFile, "LOOKUP_DB ");
  }
  if (flags & DELAY_DETAIL)
  {
      fprintf(logFile, "DELAY_DETAIL ");
  }
  if (logTimestamp)
  {
    struct timeval now;
    struct timeval const * timeptr = timestamp;
    if (timeptr == NULL)
    {
      gettimeofday(&now, NULL);
      timeptr = &now;
    }
    fprintf(logFile, "%d.%d ", (int)(timeptr->tv_sec),
	    (int)((timeptr->tv_usec)/1000));
  }
  fprintf(logFile, ": ");
}

void logWrite(int flags, struct timeval const * timestamp,
	      char const * format, ...)
{
  va_list va;
  va_start(va, format);
  if ((flags & logFlags) != 0)
  {
    logPrefix(flags & logFlags, timestamp);
    vfprintf(logFile, format, va);
    fprintf(logFile, "\n");
  }
  va_end(va);
}
