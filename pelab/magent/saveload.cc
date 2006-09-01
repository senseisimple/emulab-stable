// saveload.cc

#include "lib.h"
#include "log.h"
#include "saveload.h"
#include "Command.h"

using namespace std;


char * saveChar(char * buffer, unsigned char value)
{
  if (buffer != NULL)
  {
    memcpy(buffer, &value, sizeof(value));
    return buffer + sizeof(value);
  }
  else
  {
    logWrite(ERROR, "saveChar() called with a NULL buffer");
    return buffer;
  }
}

char * saveShort(char * buffer, unsigned short value)
{
  if (buffer != NULL)
  {
    unsigned short ordered = htons(value);
    memcpy(buffer, &ordered, sizeof(ordered));
    return buffer + sizeof(ordered);
  }
  else
  {
    logWrite(ERROR, "saveShort() called with a NULL buffer");
    return buffer;
  }
}

char * saveInt(char * buffer, unsigned int value)
{
  if (buffer != NULL)
  {
    unsigned int ordered = htonl(value);
    memcpy(buffer, &ordered, sizeof(ordered));
    return buffer + sizeof(ordered);
  }
  else
  {
    logWrite(ERROR, "saveInt() called with a NULL buffer");
    return buffer;
  }
}

char * saveHeader(char * buffer, Header const & value)
{
  if (buffer != NULL)
  {
    char * pos = buffer;
    pos = saveChar(pos, value.type);
    pos = saveShort(pos, value.size);
    pos = saveChar(pos, value.key.transport);
    pos = saveInt(pos, value.key.ip);
    pos = saveShort(pos, value.key.localPort);
    pos = saveShort(pos, value.key.remotePort);
    return pos;
  }
  else
  {
    logWrite(ERROR, "saveHeader() called with a NULL buffer");
    return buffer;
  }
}

char * saveOptions(char * buffer, std::list<Option> const & options)
{
  char * pos = buffer;

  pos = saveInt(pos, options.size());
  list<Option>::const_iterator current = options.begin();
  list<Option>::const_iterator limit = options.end();
  for (; current != limit; ++current)
  {
    pos = saveChar(pos, current->type);
    pos = saveChar(pos, current->length);
    if (current->length > 0)
    {
      memcpy(pos, current->buffer, current->length);
      pos += current->length;
    }
  }

  return pos;
}

char * savePacket(char * buffer, PacketInfo const & value)
{
  char * pos = buffer;
  struct tcp_info const * kernel = value.kernel;

  // Save packet time
  pos = saveInt(pos, value.packetTime.getTimeval()->tv_sec);
  pos = saveInt(pos, value.packetTime.getTimeval()->tv_usec);

  // Save packet length
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

  memcpy(pos, kernel, sizeof(struct tcp_info));
  pos += sizeof(struct tcp_info);

  // Save IP header
  memcpy(pos, value.ip, sizeof(struct ip));
  pos += sizeof(struct ip);

  // Save IP options
  pos = saveOptions(pos, * value.ipOptions);

  // Save TCP header
  memcpy(pos, value.tcp, sizeof(struct tcphdr));
  pos += sizeof(struct tcphdr);

  // Save TCP options
  pos = saveOptions(pos, * value.tcpOptions);

  // Save elab stuff
  pos = saveChar(pos, value.elab.transport);
  pos = saveInt(pos, value.elab.ip);
  pos = saveShort(pos, value.elab.localPort);
  pos = saveShort(pos, value.elab.remotePort);

  // Save bufferFull measurement
  unsigned char bufferFull = value.bufferFull;
  pos = saveChar(pos, bufferFull);

 // Save packetType
  pos = saveChar(pos, value.packetType);

  return pos;
}

char * loadChar(char * buffer, unsigned char * value)
{
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

char * loadHeader(char * buffer, Header * value)
{
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
    pos = loadChar(pos, &(value->key.transport));
    pos = loadInt(pos, &(value->key.ip));
    pos = loadShort(pos, &(value->key.localPort));
    pos = loadShort(pos, &(value->key.remotePort));
    return pos;
  }
}

// Assumption: The input buffer will not be destroyed or changed
// before the list of optins is destroyed.
char * loadOptions(char * buffer, std::list<Option> * options)
{
  options->clear();
  char * pos = buffer;

  Option current;
  size_t i = 0;
  size_t limit = 0;

  pos = loadInt(pos, &limit);
  for (i = 0; i < limit; ++i)
  {
    pos = loadChar(pos, & current.type);
    pos = loadChar(pos, & current.length);
    if (current.length > 0)
    {
      current.buffer = reinterpret_cast<unsigned char *>(pos);
      pos += current.length;
    }
    else
    {
      current.buffer = NULL;
    }
    options->push_back(current);
  }

  return pos;
}

auto_ptr<Command> loadCommand(Header * head, char * body)
{
  auto_ptr<Command> result;
  switch (head->type)
  {
  case NEW_CONNECTION_COMMAND:
    result.reset(new NewConnectionCommand());
    break;
  case TRAFFIC_MODEL_COMMAND:
    result.reset(new TrafficModelCommand());
    break;
  case CONNECTION_MODEL_COMMAND:
  {
    ConnectionModelCommand * model = new ConnectionModelCommand();
    unsigned int temp;
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
    unsigned int temp;
    buffer = loadInt(buffer, &temp);
    model->type = temp;
    result.reset(model);
    break;
  }
  case CONNECT_COMMAND:
    result.reset(new ConnectCommand());
    break;
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
                  struct ip & ip, struct tcphdr & tcp,
                  list<Option> & ipOptions, list<Option> & tcpOptions)
{
  char * pos = buffer;
  value->kernel = &kernel;
  value->ip = &ip;
  value->ipOptions = &ipOptions;
  value->tcp = &tcp;
  value->tcpOptions = &tcpOptions;

  // Load packet time
  unsigned int timeSeconds = 0;
  pos = loadInt(pos, & timeSeconds);
  value->packetTime.getTimeval()->tv_sec = timeSeconds;
  unsigned int timeMicroseconds = 0;
  pos = loadInt(pos, & timeMicroseconds);
  value->packetTime.getTimeval()->tv_usec = timeMicroseconds;

  // Load packet length
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

  memcpy(&kernel, pos, sizeof(struct tcp_info));
  pos += sizeof(struct tcp_info);

  // Load IP header
  memcpy(&ip, pos, sizeof(struct ip));
  pos += sizeof(struct ip);

  // Load IP options
  pos = loadOptions(pos, &ipOptions);

  // Load TCP header
  memcpy(&tcp, pos, sizeof(struct tcphdr));
  pos += sizeof(struct tcphdr);

  // Load TCP options
  pos = loadOptions(pos, &tcpOptions);

  // Load elab
  pos = loadChar(pos, & value->elab.transport);
  pos = loadInt(pos, & value->elab.ip);
  pos = loadShort(pos, & value->elab.localPort);
  pos = loadShort(pos, & value->elab.remotePort);

  // Load bufferFull
  unsigned char bufferFull = 0;
  pos = loadChar(pos, &bufferFull);
  value->bufferFull = (bufferFull == 1);

  // Load packet type
  pos = loadChar(pos, & value->packetType);

  return pos;
}

