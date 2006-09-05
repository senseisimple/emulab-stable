// log.c

#include "lib.h"
#include "log.h"

static FILE * logFile;
static int logFlags;
static int logTimestamp;
static int startSeconds;

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
    struct timeval now;
    gettimeofday(&now, NULL);
    startSeconds = now.tv_sec;
  }
}

void logCleanup(void)
{
}

// Print the timestamp and type of logging to the logFile.
static void logPrefix(int which)
{
  char * messageClass = "LOG_ERROR ";
  switch(which)
  {
  case ERROR:
    messageClass = "ERROR ";
    break;
  case EXCEPTION:
    messageClass = "EXCEPTION ";
    break;
  case PEER_CYCLE:
    messageClass = "PEER_CYCLE ";
    break;
  case SENSOR:
    messageClass = "SENSOR ";
    break;
  case CONNECTION_MODEL:
    messageClass = "CONNECTION_MODEL ";
    break;
  case ROBUST:
    messageClass = "ROBUST ";
    break;
  case MAIN_LOOP:
    messageClass = "MAIN_LOOP ";
    break;
  case COMMAND_INPUT:
    messageClass = "COMMAND_INPUT ";
    break;
  case CONNECTION:
    messageClass = "CONNECTION ";
    break;
  case PCAP:
    messageClass = "PCAP ";
    break;
  case COMMAND_OUTPUT:
    messageClass = "COMMAND_OUTPUT ";
    break;
  default:
    messageClass = "LOG_ERROR ";
    break;
  }
  if (logTimestamp)
  {
    struct timeval now;
    gettimeofday(&now, NULL);
    now.tv_sec -= startSeconds;
    fprintf(logFile, "\n%s %u:%u: ", messageClass,
            now.tv_sec - startSeconds, (now.tv_usec)/1000);
  }
  else
  {
    fprintf(logFile, "\n%s: ", messageClass);
  }
}

void logWrite(int flags, char const * format, ...)
{
  va_list va;
  va_start(va, format);
  if ((flags & logFlags) != 0)
  {
    logPrefix(flags & logFlags);
    vfprintf(logFile, format, va);
  }
  va_end(va);
}
