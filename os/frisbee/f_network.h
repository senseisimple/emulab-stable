
/* 
 * network.h
 *
 * Network abstractions for frisbee
 *
 * (handles ip broadcast and packet (de)marshalling)
 *
 * Chad Barb (barb@cs) 
 * March 20, 2001
 * 
 */  
#define NPT_DATA    0x01
#define NPT_REQUEST 0x02
#define NPT_POLL    0x04
#define NPT_ALIVE   0x08
#define NPT_GONE    0x10
#define NPT_ANY     0xFF

#define MAXUINT 0xffffffff

#define PACKET_PAYLOAD_SIZE 1024

typedef struct {
  unsigned char type;
  uint packetId;
  uchar data[PACKET_PAYLOAD_SIZE];
} PacketData;

typedef struct {
  unsigned char type;
  uint pollSequence;
  uint packetId;
  /* uchar padding[64]; */
} PacketRequest;

typedef struct {
  unsigned char type;
  uint packetTotal;
  uint pollSequence;
  uint addressMask;
  uint address;
  /* uchar padding[64]; */
} PacketPoll;

typedef struct {
  unsigned char type;
  uint clientId;
  /* uchar padding[64]; */
} PacketAlive;

typedef struct {
  unsigned char type;
  uint clientId;
  /* uchar padding[64]; */
} PacketGone;


typedef struct {
  unsigned char type;
  uchar filler[ sizeof( PacketData ) - 1 ]; /* PacketData is by far the largest */
} Packet;



void n_init( ushort receivePort, ushort sendPort, uint sendAddress );
void n_initLookup( ushort receivePort, ushort sendPort, const char * sendName );
void n_finish();

int  n_packetRecv( Packet * p, uint timeout, uchar mask );
void n_packetSend( Packet * p );
