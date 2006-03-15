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
// 'timestamp' is a pointer to a timestamp which will be prepended
// onto the logging message. If it is NULL, the current time is
// used. Note that whether or not the timestamp is printed at all is
// set elsewhere.
// 'format' printf-style formatting string. It and any extra variables
// act just like a normal printf call. This is where you actually
// write your message. It will be written to the logging file.
void logWrite(int flags, struct timeval const * timestamp,
	      char const * format, ...);

enum LOG_TYPE
{
  // Information about control messages being sent to the monitor
  CONTROL_SEND     = 0x1,
  // Information about control messages being received from the monitor
  CONTROL_RECEIVE  = 0x2,
  // Information about sent control messages in a format used in
  // tcptrace graphs.
  TCPTRACE_SEND    = 0x4,
  // Information about received control messages in a format used in
  // tcptrace graphs.
  TCPTRACE_RECEIVE = 0x8,
  // Information about sent packets which were sniffed by stub-pcap.
  SNIFF_SEND       = 0x10,
  // Information about received packets which were sniffed by stub-pcap.
  SNIFF_RECEIVE    = 0x20,
  // Information about writing to peers.
  PEER_WRITE       = 0x40,
  // Information about reading from peers.
  PEER_READ        = 0x80,
  // Which stages are being run in the main loop.
  MAIN_LOOP        = 0x100,
  // Database storage and reterieval
  LOOKUP_DB        = 0x200,
  DELAY_DETAIL     = 0x400,
  // Shortcuts for common cases.
  LOG_NOTHING      = 0x00,
  LOG_EVERYTHING   = 0x7ff
};

#ifdef __cplusplus
}
#endif

#endif
