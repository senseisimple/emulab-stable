// saveload.h

#ifndef SAVE_LOAD_H_STUB_2
#define SAVE_LOAD_H_STUB_2

class Command;

class Header
{
public:
  Header() : type(0), size(0) {}

  unsigned char type;
  unsigned short size;
  Order key;

  enum { headerSize = sizeof(unsigned char)*2
         + sizeof(unsigned short)*3
         + sizeof(unsigned int) };
};

// These take in a current position in the buffer and return the
// position after the saved or loaded item.

char * saveChar(char * buffer, unsigned char value);
char * saveShort(char * buffer, unsigned short value);
char * saveInt(char * buffer, unsigned int value);
char * saveHeader(char * buffer, Header const & value);
char * savePacket(char * buffer, PacketInfo const & value);

char * loadChar(char * buffer, unsigned char * value);
char * loadShort(char * buffer, unsigned short * value);
char * loadInt(char * buffer, unsigned int * value);
char * loadHeader(char * buffer, Header * value);
std::auto_ptr<Command> loadCommand(Header * head, char * body);
// It is presumed that value contains pointers to the various
// substructures that need to be filled.
char * loadPacket(char * buffer, PacketInfo * value, struct tcp_info & kernel,
                  IpHeader & ip, struct tcphdr & tcp);

#endif
