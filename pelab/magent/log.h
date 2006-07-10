// log.h

#ifndef _LOG_H
#define _LOG_H

#ifdef __cplusplus
extern "C"
{
#endif

// The logging framework. Use logInit() during initialization to set
// up logging. Then use log() to generate messages. logCleanup()
// should be called during program shutdown.

// logInit() opens a file, sets what types of logging messages to
// actually print, and determines whether timestamps should be
// prepended onto messages. Logging is disabled if 'filename' is NULL
// or 'flags' is 0.
//
// 'filename' is the name of the file to be used for logging. It is
// opened in append mode.
//
// 'flags' is the logical or of one or more of the LOG_TYPEs defined
// below or 0. This determines which of the message types to print out.
//
// If 'useTimestamp' is 1, then timestamps are prepended to each
// message, otherwise, they are ignored.
void logInit(FILE * destFile, int flags, int useTimestamp);

// logCleanup() cleans up the logging state.
void logCleanup(void);

// The logWrite() function is used to print a logging message.
//
// 'flags' is the logical or of one or more of the LOG_TYPEs defined
// below. This tags the logging message so it can be selectively
// filtered. This should never be '0'. If it is '0', the message will
// not be printed.
// 'format' printf-style formatting string. It and any extra variables
// act just like a normal printf call. This is where you actually
// write your message. It will be written to the logging file.
void logWrite(int flags, char const * format, ...);

enum LOG_TYPE
{
  ERROR            = 0x01,
  EXCEPTION        = 0x02,
  PEER_CYCLE       = 0x04,
  // Shortcuts for common cases.
  LOG_NOTHING      = 0x00,
  LOG_EVERYTHING   = 0x07
};

#ifdef __cplusplus
}
#endif

#endif
