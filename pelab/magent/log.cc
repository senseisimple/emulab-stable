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
static void logPrefix(int which)
{
  switch(which)
  {
  case ERROR:
    fprintf(logFile, "ERROR ");
    break;
  case EXCEPTION:
    fprintf(logFile, "EXCEPTION ");
    break;
  case PEER_CYCLE:
    fprintf(logFile, "PEER_CYCLE ");
    break;
  case SENSOR:
    fprintf(logFile, "SENSOR ");
    break;
  case CONNECTION_MODEL:
    fprintf(logFile, "CONNECTION_MODEL ");
    break;
  case ROBUST:
    fprintf(logFile, "ROBUST ");
    break;
  case MAIN_LOOP:
    fprintf(logFile, "MAIN_LOOP ");
    break;
  case COMMAND_INPUT:
    fprintf(logFile, "COMMAND_INPUT ");
    break;
  case CONNECTION:
    fprintf(logFile, "CONNECTION ");
    break;
  case PCAP:
    fprintf(logFile, "PCAP ");
    break;
  case COMMAND_OUTPUT:
    fprintf(logFile, "COMMAND_OUTPUT ");
    break;
  default:
    fprintf(logFile, "LOG_ERROR ");
    break;
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
