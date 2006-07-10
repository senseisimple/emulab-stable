// log.c

#include "lib.h"
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
static void logPrefix(int flags)
{
  if (flags & ERROR)
  {
    fprintf(logFile, "ERROR ");
  }
  if (flags & EXCEPTION)
  {
    fprintf(logFile, "EXCEPTION ");
  }
  if (flags & PEER_CYCLE)
  {
    fprintf(logFile, "PEER_CYCLE ");
  }
  if (logTimestamp)
  {
    struct timeval now;
    gettimeofday(&now, NULL);
    fprintf(logFile, "%f ", (double)(now.tv_sec) +
            ((now.tv_usec)/1000)/1000.0);
  }
  fprintf(logFile, ": ");
}

void logWrite(int flags, char const * format, ...)
{
  va_list va;
  va_start(va, format);
  if ((flags & logFlags) != 0)
  {
    logPrefix(flags & logFlags);
    vfprintf(logFile, format, va);
    fprintf(logFile, "\n");
  }
  va_end(va);
}
