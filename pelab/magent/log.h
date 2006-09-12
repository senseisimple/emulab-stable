// log.h

#ifndef _LOG_H
#define _LOG_H

// HOWTO: Add a new logging option
// (1) Add a constant to the logging enum below. Set the next available bit.
// (2) Set that bit in the LOG_EVERYTHING constant.
// (3) Add the text to print in the prefix for that option in the switch
//     statement in log.cc

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
  ERROR            = 0x0001,
  EXCEPTION        = 0x0002,
  PEER_CYCLE       = 0x0004,
  SENSOR           = 0x0008,
  CONNECTION_MODEL = 0x0010,
  ROBUST           = 0x0020,
  MAIN_LOOP        = 0x0040,
  COMMAND_INPUT    = 0x0080,
  CONNECTION       = 0x0100,
  PCAP             = 0x0200,
  COMMAND_OUTPUT   = 0x0400,
  SENSOR_DETAIL    = 0x0800,
  SENSOR_COMPLETE  = 0x1000,
  REPLAY           = 0x2000,
  // Shortcuts for common cases.
  LOG_NOTHING      = 0x0000,
  LOG_EVERYTHING   = 0x3fff
};

#endif
