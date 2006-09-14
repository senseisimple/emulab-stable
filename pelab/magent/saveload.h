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

int getLastSaveloadSize(void);

// These take in a current position in the buffer and return the
// position after the saved or loaded item.

// If buffer is NULL, then nothing is actually written, but the write
// size is still updated. Use this to determine the size of the buffer
// to write.
char * saveChar(char * buffer, unsigned char value);
char * saveShort(char * buffer, unsigned short value);
char * saveInt(char * buffer, unsigned int value);
char * saveBuffer(char * buffer, void const * input, unsigned int size);
char * saveHeader(char * buffer, Header const & value);
char * saveOptions(char * buffer, std::list<Option> const & options);
char * savePacket(char * buffer, PacketInfo const & value);

// If buffer is NULL, that is an error.
char * loadChar(char * buffer, unsigned char * value);
char * loadShort(char * buffer, unsigned short * value);
char * loadInt(char * buffer, unsigned int * value);
char * loadBuffer(char * buffer, void * output, unsigned int size);
char * loadHeader(char * buffer, Header * value);
char * loadOptions(char * buffer, std::list<Option> * options);
std::auto_ptr<Command> loadCommand(Header * head, char * body);
// It is presumed that value contains pointers to the various
// substructures that need to be filled.
char * loadPacket(char * buffer, PacketInfo * value, struct tcp_info & kernel,
                  struct ip & ip, struct tcphdr & tcp,
                  std::list<Option> & ipOptions,
                  std::list<Option> & tcpOptions);

#endif
