/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// saveload.cc

#include "lib.h"
#include "log.h"
#include "saveload.h"
#include "Command.h"

// Uncomment this to turn on the verbose input for log replay format.
// #define LOG_REPLAY_FORMAT

namespace
{
  #ifdef LOG_REPLAY_FORMAT
  const int DASHES_MAX = 200;
  char dashes[DASHES_MAX];
  int dashesLevel = 0;
  #endif

  int count = 0;
  int logFlag = LOG_NOTHING;

  class FileLevel
  {
  public:
    FileLevel()
    {
      #ifdef LOG_REPLAY_FORMAT
      if (dashesLevel == 0)
      {
        count = 0;
      }
      dashes[dashesLevel] = '-';
      ++dashesLevel;
      dashes[dashesLevel] = '\0';
      #endif
    }
    ~FileLevel()
    {
      #ifdef LOG_REPLAY_FORMAT
      dashes[dashesLevel] = '\0';
      --dashesLevel;
      #endif
    }
  };
}

#ifdef LOG_REPLAY_FORMAT
#define logReplay(str,afterCount) ((afterCount == 0) \
                                    ? (logWrite(logFlag, "%s> %s @ %d", \
                                                dashes, str, count)) \
                                    : (logWrite(logFlag, "%s> %s @ [%d, %d)", \
                                                dashes, str, count, \
                                                afterCount)))
#else
#define logReplay(str, afterCount)
#endif

int getLastSaveloadSize(void)
{
  return count;
}

char * saveChar(char * buffer, unsigned char value)
{
  FileLevel up;
  logReplay("Char", count + sizeof(value));
  count += sizeof(value);
  if (buffer != NULL)
  {
    memcpy(buffer, &value, sizeof(value));
    return buffer + sizeof(value);
  }
  else
  {
//    logWrite(ERROR, "saveChar() called with a NULL buffer");
    return buffer;
  }
}

char * saveShort(char * buffer, unsigned short value)
{
  FileLevel up;
  logReplay("Short", count + sizeof(value));
  count += sizeof(value);
  if (buffer != NULL)
  {
    unsigned short ordered = htons(value);
    memcpy(buffer, &ordered, sizeof(ordered));
    return buffer + sizeof(ordered);
  }
  else
  {
//    logWrite(ERROR, "saveShort() called with a NULL buffer");
    return buffer;
  }
}

char * saveInt(char * buffer, unsigned int value)
{
  FileLevel up;
  logReplay("Int", count + sizeof(value));
  count += sizeof(value);
  if (buffer != NULL)
  {
    unsigned int ordered = htonl(value);
    memcpy(buffer, &ordered, sizeof(ordered));
    return buffer + sizeof(ordered);
  }
  else
  {
//    logWrite(ERROR, "saveInt() called with a NULL buffer");
    return buffer;
  }
}

char * saveBuffer(char * buffer, void const * input, unsigned int size)
{
  FileLevel up;
  logReplay("Buffer", count + size);
  count += size;
  if (buffer != NULL)
  {
    memcpy(buffer, input, size);
    return buffer + size;
  }
  else
  {
    return buffer;
  }
}

char * saveHeader(char * buffer, Header const & value)
{
  FileLevel up;
  logReplay("Header", 0);
  char * pos = buffer;
  pos = saveChar(pos, value.type);
  pos = saveShort(pos, value.size);
  pos = saveChar(pos, value.version);
  logReplay("key", 0);
  if (global::CONTROL_VERSION < 1)
  {
    pos = saveBuffer(pos, value.key.id, Header::VERSION_0_SIZE);
  }
  else
  {
    pos = saveBuffer(pos, value.key.id, ElabOrder::idSize);
  }
//  pos = saveInt(pos, value.key.ip);
//  pos = saveShort(pos, value.key.localPort);
//  pos = saveShort(pos, value.key.remotePort);
  return pos;
}

char * saveOptions(char * buffer, std::list<Option> const & options)
{
  FileLevel up;
  logReplay("Options", 0);
  char * pos = buffer;

  pos = saveInt(pos, options.size());
  std::list<Option>::const_iterator current = options.begin();
  std::list<Option>::const_iterator limit = options.end();
  for (; current != limit; ++current)
  {
    FileLevel up;
    logReplay("Option", 0);
    pos = saveChar(pos, current->type);
    pos = saveChar(pos, current->length);
    if (current->length > 0)
    {
      pos = saveBuffer(pos, current->buffer, current->length);
    }
  }

  return pos;
}

char * savePacket(char * buffer, PacketInfo const & value)
{
  FileLevel up;
  count = 0;
  if (buffer != NULL)
  {
    logFlag = REPLAY;
  }
  logReplay("savePacket", 0);
  char * pos = buffer;
  struct tcp_info const * kernel = value.kernel;

  // Save transport protocol
  logReplay("transport", 0);
  pos = saveChar(pos, value.transport);

  // Save packet time
  logReplay("packetTime", 0);
  pos = saveInt(pos, value.packetTime.getTimeval()->tv_sec);
  pos = saveInt(pos, value.packetTime.getTimeval()->tv_usec);

  // Save packet length
  logReplay("packetLength", 0);
  pos = saveInt(pos, value.packetLength);

  // Save tcp_info
/* Inefficient, but safe.
  pos = saveChar(pos, kernel->tcpi_state);
  pos = saveChar(pos, kernel->tcpi_ca_state);
  pos = saveChar(pos, kernel->tcpi_retransmits);
  pos = saveChar(pos, kernel->tcpi_probes);
  pos = saveChar(pos, kernel->tcpi_backoff);
  pos = saveChar(pos, kernel->tcpi_options);
  unsigned char  windowScale = (kernel->tcpi_snd_wscale & 0xf)
    | ((kernel->tcpi_rcv_wscale & 0xf) << 4);
  pos = saveChar(pos, windowScale);

  pos = saveInt(pos, kernel->tcpi_rto);
  pos = saveInt(pos, kernel->tcpi_ato);
  pos = saveInt(pos, kernel->tcpi_snd_mss);
  pos = saveInt(pos, kernel->tcpi_rcv_mss);

  pos = saveInt(pos, kernel->tcpi_unacked);
  pos = saveInt(pos, kernel->tcpi_sacked);
  pos = saveInt(pos, kernel->tcpi_lost);
  pos = saveInt(pos, kernel->tcpi_retrans);
  pos = saveInt(pos, kernel->tcpi_fackets);

  pos = saveInt(pos, kernel->tcpi_last_data_sent);
  pos = saveInt(pos, kernel->tcpi_last_ack_sent);
  pos = saveInt(pos, kernel->tcpi_last_data_recv);
  pos = saveInt(pos, kernel->tcpi_last_ack_recv);

  pos = saveInt(pos, kernel->tcpi_pmtu);
  pos = saveInt(pos, kernel->tcpi_rcv_ssthresh);
  pos = saveInt(pos, kernel->tcpi_rtt);
  pos = saveInt(pos, kernel->tcpi_rttvar);
  pos = saveInt(pos, kernel->tcpi_snd_ssthresh);
  pos = saveInt(pos, kernel->tcpi_snd_cwnd);
  pos = saveInt(pos, kernel->tcpi_advmss);
  pos = saveInt(pos, kernel->tcpi_reordering);
*/

  if (value.transport == TCP_CONNECTION)
  {
    logReplay("tcp_info", 0);
    pos = saveBuffer(pos, kernel, sizeof(struct tcp_info));
    //  memcpy(pos, kernel, sizeof(struct tcp_info));
    //  pos += sizeof(struct tcp_info);
  }

  // Save IP header
  logReplay("IP header", 0);
  pos = saveBuffer(pos, value.ip, sizeof(struct ip));
//  memcpy(pos, value.ip, sizeof(struct ip));
//  pos += sizeof(struct ip);

  // Save IP options
  logReplay("IP options", 0);
  pos = saveOptions(pos, * value.ipOptions);

  if (value.transport == TCP_CONNECTION)
  {
    // Save TCP header
    logReplay("TCP header", 0);
    pos = saveBuffer(pos, value.tcp, sizeof(struct tcphdr));
    //  memcpy(pos, value.tcp, sizeof(struct tcphdr));
    //  pos += sizeof(struct tcphdr);

    // Save TCP options
    logReplay("TCP options", 0);
    pos = saveOptions(pos, * value.tcpOptions);
  }

  if (value.transport == UDP_CONNECTION)
  {
    // Save UDP header
    logReplay("UDP header", 0);
    pos = saveBuffer(pos, value.udp, sizeof(struct udphdr));
  }

  // Save payload
  logReplay("payload size", 0);
  pos = saveInt(pos, value.payloadSize);

  logReplay("payload buffer", 0);
  pos = saveBuffer(pos, value.payload, value.payloadSize);

  // Save elab stuff
  logReplay("elab key", 0);
  pos = saveBuffer(pos, value.elab.id, ElabOrder::idSize);
//  pos = saveChar(pos, value.elab.transport);
//  pos = saveInt(pos, value.elab.ip);
//  pos = saveShort(pos, value.elab.localPort);
//  pos = saveShort(pos, value.elab.remotePort);

  // Save bufferFull measurement
  logReplay("bufferFull", 0);
  unsigned char bufferFull = value.bufferFull;
  pos = saveChar(pos, bufferFull);

  // Save packetType
  logReplay("packetType", 0);
  pos = saveChar(pos, value.packetType);

  logFlag = LOG_NOTHING;
  return pos;
}

char * loadChar(char * buffer, unsigned char * value)
{
  FileLevel up;
  logReplay("Char", count + sizeof(*value));
  count += sizeof(*value);
  if (buffer == NULL)
  {
    logWrite(ERROR, "loadChar() called with a NULL buffer");
    return buffer;
  }
  else if (value == NULL)
  {
    logWrite(ERROR, "loadChar() called with a NULL value");
    return buffer;
  }
  else
  {
    memcpy(value, buffer, sizeof(unsigned char));
    return buffer + sizeof(unsigned char);
  }
}

char * loadShort(char * buffer, unsigned short * value)
{
  FileLevel up;
  logReplay("Short", count + sizeof(*value));
  count += sizeof(*value);
  if (buffer == NULL)
  {
    logWrite(ERROR, "loadShort() called with a NULL buffer");
    return buffer;
  }
  else if (value == NULL)
  {
    logWrite(ERROR, "loadShort() called with a NULL value");
    return buffer;
  }
  else
  {
    unsigned short ordered = 0;
    memcpy(&ordered, buffer, sizeof(ordered));
    *value = ntohs(ordered);
    return buffer + sizeof(ordered);
  }
}

char * loadInt(char * buffer, unsigned int * value)
{
  FileLevel up;
  logReplay("Int", count + sizeof(*value));
  count += sizeof(*value);
  if (buffer == NULL)
  {
    logWrite(ERROR, "loadInt() called with a NULL buffer");
    return buffer;
  }
  else if (value == NULL)
  {
    logWrite(ERROR, "loadInt() called with a NULL value");
    return buffer;
  }
  else
  {
    unsigned int ordered = 0;
    memcpy(&ordered, buffer, sizeof(ordered));
    *value = ntohl(ordered);
    return buffer + sizeof(ordered);
  }
}

char * loadBuffer(char * buffer, void * output, unsigned int size)
{
  FileLevel up;
  logReplay("Buffer", count + size);
  count += size;
  if (buffer == NULL)
  {
    logWrite(ERROR, "loadBuffer() called with a NULL buffer");
    return buffer;
  }
  else if (output == NULL)
  {
    logWrite(ERROR, "loadBuffer() called with a NULL output");
    return buffer;
  }
  else
  {
    memcpy(output, buffer, size);
    return buffer + size;
  }
}

char * loadHeader(char * buffer, Header * value)
{
  FileLevel up;
  logReplay("Header", 0);
  if (buffer == NULL)
  {
    logWrite(ERROR, "loadHeader() called with a NULL buffer");
    return buffer;
  }
  else if (value == NULL)
  {
    logWrite(ERROR, "loadHeader() called with a NULL value");
    return buffer;
  }
  else
  {
    char * pos = buffer;
    pos = loadChar(pos, &(value->type));
    pos = loadShort(pos, &(value->size));
    pos = loadChar(pos, &(value->version));
    switch(value->version)
    {
    case 0:
      logReplay("Version 0", 0);
      pos = loadBuffer(pos, value->key.id, Header::VERSION_0_SIZE);
      memset(value->key.id + Header::VERSION_0_SIZE, '\0',
             ElabOrder::idSize - Header::VERSION_0_SIZE);
      break;
    case 1:
      logReplay("Version 1", 0);
      pos = loadBuffer(pos, value->key.id, ElabOrder::idSize);
      break;
    default:
      logWrite(ERROR, "Unknown version: %d", value->version);
      logWrite(ERROR, "!!!! Undefined behaviour will follow !!!!");
      break;
    }
//      pos = loadInt(pos, &(value->key.ip));
//      pos = loadShort(pos, &(value->key.localPort));
//      pos = loadShort(pos, &(value->key.remotePort));
    return pos;
  }
}

// Assumption: The input buffer will not be destroyed or changed
// before the list of optins is destroyed.
char * loadOptions(char * buffer, std::list<Option> * options)
{
  FileLevel up;
  logReplay("Options", 0);
  options->clear();
  char * pos = buffer;

  Option current;
  size_t i = 0;
  size_t limit = 0;

  pos = loadInt(pos, &limit);
  for (i = 0; i < limit; ++i)
  {
    FileLevel up;
    logReplay("Option", 0);
    pos = loadChar(pos, & current.type);
    pos = loadChar(pos, & current.length);
    if (current.length > 0)
    {
      current.buffer = reinterpret_cast<unsigned char *>(pos);
      pos += current.length;
      count += current.length;
    }
    else
    {
      current.buffer = NULL;
    }
    options->push_back(current);
  }

  return pos;
}

std::auto_ptr<Command> loadCommand(Header * head, char * body)
{
  FileLevel up;
  std::auto_ptr<Command> result;
  switch (head->type)
  {
  case NEW_CONNECTION_COMMAND:
  {
    NewConnectionCommand * newConnect = new NewConnectionCommand();
    if (head->version < 1)
    {
      newConnect->transport = TCP_CONNECTION;
    }
    else
    {
      unsigned char temp = 0;
      char * buffer = body;
      loadChar(buffer, &temp);
      newConnect->transport = temp;
    }
    result.reset(newConnect);
    break;
  }
  case TRAFFIC_MODEL_COMMAND:
    result.reset(new TrafficModelCommand());
    break;
  case CONNECTION_MODEL_COMMAND:
  {
    ConnectionModelCommand * model = new ConnectionModelCommand();
    unsigned int temp = 0;
    char * buffer = body;
    buffer = loadInt(buffer, &temp);
    model->type = temp;
    buffer = loadInt(buffer, &temp);
    model->value = temp;
    result.reset(model);
    break;
  }
  case SENSOR_COMMAND:
  {
    SensorCommand * model = new SensorCommand();
    char * buffer = body;
    unsigned int temp = 0;
    buffer = loadInt(buffer, &temp);
    model->type = temp;
    result.reset(model);
    break;
  }
  case CONNECT_COMMAND:
  {
    ConnectCommand * connect = new ConnectCommand();
    unsigned int tempIp = 0;
    char * buffer = NULL;
    if (head->version < 1)
    {
      buffer = head->key.id;
    }
    else
    {
      buffer = body;
    }
    buffer = loadInt(buffer, &tempIp);
    connect->ip = tempIp;
    result.reset(connect);
    break;
  }
  case TRAFFIC_WRITE_COMMAND:
  {
    TrafficWriteCommand * model = new TrafficWriteCommand();
    char * buffer = body;
    unsigned int temp;
    buffer = loadInt(buffer, &temp);
    model->delta = temp;
    buffer = loadInt(buffer, &temp);
    model->size = temp;
    result.reset(model);
    break;
  }
  case DELETE_CONNECTION_COMMAND:
    result.reset(new DeleteConnectionCommand());
    break;
  default:
    logWrite(ERROR, "Unknown command type encountered: %d", head->type);
    break;
  }
  result->key = head->key;
  return result;
}

char * loadPacket(char * buffer, PacketInfo * value, struct tcp_info & kernel,
                  struct ip & ip, struct tcphdr & tcp, struct udphdr & udp,
                  std::list<Option> & ipOptions,
                  std::list<Option> & tcpOptions,
                  std::vector<unsigned char> & payload,
                  unsigned char version)
{
  count = 0;
  FileLevel up;
  if (buffer != NULL)
  {
    logFlag = REPLAY;
  }
  logReplay("loadPacket", 0);
  if (version == 0)
  {
    logReplay("Backwards compatability to version 0", 0);
  }
  char * pos = buffer;
  value->kernel = &kernel;
  value->ip = &ip;
  value->ipOptions = &ipOptions;
  value->tcp = &tcp;
  value->tcpOptions = &tcpOptions;
  value->udp = &udp;
  value->payload = & payload[0];

  if (version < 1)
  {
    value->transport = TCP_CONNECTION;
  }
  else
  {
    logReplay("transport", 0);
    unsigned char transport = TCP_CONNECTION;
    pos = loadChar(pos, & transport);
    value->transport = transport;
  }

  // Load packet time
  logReplay("packetTime", 0);
  unsigned int timeSeconds = 0;
  pos = loadInt(pos, & timeSeconds);
  value->packetTime.getTimeval()->tv_sec = timeSeconds;
  unsigned int timeMicroseconds = 0;
  pos = loadInt(pos, & timeMicroseconds);
  value->packetTime.getTimeval()->tv_usec = timeMicroseconds;

  // Load packet length
  logReplay("packetLength", 0);
  unsigned int packetLength = 0;
  pos = loadInt(pos, & packetLength);
  value->packetLength = static_cast<int>(packetLength);

  // Load tcp_info
/* Inefficient, but safe
  pos = loadChar(pos, & kernel.tcpi_state);
  pos = loadChar(pos, & kernel.tcpi_ca_state);
  pos = loadChar(pos, & kernel.tcpi_retransmits);
  pos = loadChar(pos, & kernel.tcpi_probes);
  pos = loadChar(pos, & kernel.tcpi_backoff);
  pos = loadChar(pos, & kernel.tcpi_options);
  unsigned char  windowScale = 0;
  pos = loadChar(pos, &windowScale);
  kernel.tcpi_snd_wscale = windowScale & 0xf;
  kernel.tcpi_rcv_wscale = (windowScale >> 4) & 0xf;

  pos = loadInt(pos, & kernel.tcpi_rto);
  pos = loadInt(pos, & kernel.tcpi_ato);
  pos = loadInt(pos, & kernel.tcpi_snd_mss);
  pos = loadInt(pos, & kernel.tcpi_rcv_mss);

  pos = loadInt(pos, & kernel.tcpi_unacked);
  pos = loadInt(pos, & kernel.tcpi_sacked);
  pos = loadInt(pos, & kernel.tcpi_lost);
  pos = loadInt(pos, & kernel.tcpi_retrans);
  pos = loadInt(pos, & kernel.tcpi_fackets);

  pos = loadInt(pos, & kernel.tcpi_last_data_sent);
  pos = loadInt(pos, & kernel.tcpi_last_ack_sent);
  pos = loadInt(pos, & kernel.tcpi_last_data_recv);
  pos = loadInt(pos, & kernel.tcpi_last_ack_recv);

  pos = loadInt(pos, & kernel.tcpi_pmtu);
  pos = loadInt(pos, & kernel.tcpi_rcv_ssthresh);
  pos = loadInt(pos, & kernel.tcpi_rtt);
  pos = loadInt(pos, & kernel.tcpi_rttvar);
  pos = loadInt(pos, & kernel.tcpi_snd_ssthresh);
  pos = loadInt(pos, & kernel.tcpi_snd_cwnd);
  pos = loadInt(pos, & kernel.tcpi_advmss);
  pos = loadInt(pos, & kernel.tcpi_reordering);
*/

  if (value->transport == TCP_CONNECTION)
  {
    logReplay("tcp_info", 0);
    pos = loadBuffer(pos, &kernel, sizeof(struct tcp_info));
    //  memcpy(&kernel, pos, sizeof(struct tcp_info));
    //  pos += sizeof(struct tcp_info);
  }
  else
  {
    value->kernel = NULL;
  }

  // Load IP header
  logReplay("IP header", 0);
  pos = loadBuffer(pos, &ip, sizeof(struct ip));
//  memcpy(&ip, pos, sizeof(struct ip));
//  pos += sizeof(struct ip);

  //logWrite(REPLAY,"IP v=%i l=%i src=%s", ip.ip_v, ip.ip_hl,
  //        inet_ntoa(ip.ip_src));

  // Load IP options
  logReplay("IP options", 0);
  pos = loadOptions(pos, &ipOptions);

  // Load TCP header
  if (value->transport == TCP_CONNECTION)
  {
    logReplay("TCP header", 0);
    pos = loadBuffer(pos, &tcp, sizeof(struct tcphdr));
    //  memcpy(&tcp, pos, sizeof(struct tcphdr));
    //  pos += sizeof(struct tcphdr);

    //logWrite(REPLAY,"TCP sport=%i dport=%i",htons(tcp.source),
    //        htons(tcp.dest));

    // Load TCP options
    logReplay("TCP options", 0);
    pos = loadOptions(pos, &tcpOptions);
  }
  else
  {
    value->tcp = NULL;
    value->tcpOptions = NULL;
  }

  if (value->transport == UDP_CONNECTION)
  {
    logReplay("UDP header", 0);
    pos = loadBuffer(pos, &udp, sizeof(struct udphdr));
  }
  else
  {
    value->udp = NULL;
  }

  if (version < 1)
  {
    value->payloadSize = 0;
    value->payload = NULL;
  }
  else
  {
    logReplay("payloadSize", 0);
    pos = loadInt(pos, & value->payloadSize);

    logReplay("payload", 0);
    payload.resize(value->payloadSize);
    pos = loadBuffer(pos, & payload[0], value->payloadSize);
  }

  // Load elab
  logReplay("elab key", 0);
  switch(version)
  {
  case 0:
    pos = loadBuffer(pos, value->elab.id, Header::VERSION_0_SIZE);
    memset(value->elab.id + Header::VERSION_0_SIZE, '\0',
           ElabOrder::idSize - Header::VERSION_0_SIZE);
    break;
  case 1:
    pos = loadBuffer(pos, value->elab.id, ElabOrder::idSize);
    break;
  default:
    logWrite(ERROR, "Unknown version: %d", version);
    logWrite(ERROR, "!!!! Undefined behaviour will follow !!!!");
    break;
  }
  //logWrite(REPLAY,"ELAB trans=%i IP=%i.%i.%i.%.i lp=%i rp=%i",
  //        value->elab.transport,
  //        ((value->elab.ip) >> 24) & 0xff,
  //        ((value->elab.ip) >> 16) & 0xff,
  //        ((value->elab.ip) >> 8) & 0xff,
  //        (value->elab.ip) & 0xff,
  //        (value->elab.localPort),
  //        (value->elab.remotePort));

  // Load bufferFull
  logReplay("bufferFull", 0);
  unsigned char bufferFull = 0;
  pos = loadChar(pos, &bufferFull);
  //logWrite(REPLAY,"BFULL=%i",bufferFull);
  value->bufferFull = (bufferFull == 1);

  // Load packet type
  logReplay("packetType", 0);
  pos = loadChar(pos, & value->packetType);
  //logWrite(REPLAY,"TYPE=%d",value->packetType);

  logFlag = LOG_NOTHING;
  return pos;
}

