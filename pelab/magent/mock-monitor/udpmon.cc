/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// dumb-client.c

#include "../lib.h"
#include "../saveload.h"
#include "../DirectInput.h"

using namespace std;

int SEND_COUNT = 4200;
int PACKET_DELTA = 5;
int PACKET_SIZE = 1000;

void sendCommand(int conn, unsigned char commandId, ElabOrder const & key,
                 vector<char> const & command)
{
  static char buffer[8096] = {0};
  char * pos = buffer;
  Header head;
  head.type = commandId;
  head.size = command.size();
  head.version = global::CONTROL_VERSION;
  head.key = key;

  pos = saveHeader(pos, head);
  pos = saveBuffer(pos, & command[0], command.size());
  send(conn, buffer, Header::headerSize + command.size(), 0);
}

int main(int argc, char * argv[])
{
  int connection = -1;
  int error = 0;
  struct sockaddr_in address;
  ElabOrder key;
  vector<char> command;

  if (argc != 6)
  {
    printf("Usage: %s <server-ip> <server-port> <udpserver-ip> <num-sends> <send-delta>\n", argv[0]);
    return 1;
  }
  SEND_COUNT = atoi(argv[4]);
  PACKET_DELTA = atoi(argv[5]);

  connection = socket(AF_INET, SOCK_STREAM, 0);
  if (connection == -1)
  {
    printf("Failed socket()\n");
    return 1;
  }

  address.sin_family = AF_INET;
  address.sin_port = htons(atoi(argv[2]));
  inet_aton(argv[1], &address.sin_addr);
  memset(address.sin_zero, '\0', 8);

  error = connect(connection, (struct sockaddr *)&address, sizeof(address));
  if (error == -1)
  {
    printf("Failed connect()\n");
    return 1;
  }

  command.resize(sizeof(unsigned char));
  saveChar(& command[0], UDP_CONNECTION);
  sendCommand(connection, NEW_CONNECTION_COMMAND, key, command);

  command.resize(0);
  sendCommand(connection, TRAFFIC_MODEL_COMMAND, key, command);

  command.resize(sizeof(unsigned int));
  // PRAMOD: Replace with your own sensors - Done
  saveInt(& command[0], UDP_PACKET_SENSOR);
  sendCommand(connection, SENSOR_COMMAND, key, command);

//  saveInt(& command[0], UDP_THROUGHPUT_SENSOR);
//  sendCommand(connection, SENSOR_COMMAND, key, command);

  saveInt(& command[0], UDP_MINDELAY_SENSOR);
  sendCommand(connection, SENSOR_COMMAND, key, command);

  saveInt(& command[0], UDP_MAXDELAY_SENSOR);
  sendCommand(connection, SENSOR_COMMAND, key, command);

  saveInt(& command[0], UDP_RTT_SENSOR);
  sendCommand(connection, SENSOR_COMMAND, key, command);

  saveInt(& command[0], UDP_LOSS_SENSOR);
  sendCommand(connection, SENSOR_COMMAND, key, command);

  saveInt(& command[0], UDP_AVG_THROUGHPUT_SENSOR);
  sendCommand(connection, SENSOR_COMMAND, key, command);

  command.resize(sizeof(unsigned int));
  // PRAMOD: Replace with destination PlanetLab IP - Done
  struct in_addr destAddr;

  // Update the string below with the IP address of the 
  // planet lab host we are running on.
  inet_aton(argv[3], &destAddr);

  // Note: This ip value is in network byte order - convert it
  // to host order in the magent before using it.
  unsigned int ip = htonl(destAddr.s_addr);

  saveInt(& command[0], ip);
  sendCommand(connection, CONNECT_COMMAND, key, command);

  command.resize(2*sizeof(unsigned int));
  char * pos = & command[0];
  pos = saveInt(pos, PACKET_DELTA);
  pos = saveInt(pos, PACKET_SIZE);

  for (int i = 0; i < SEND_COUNT; ++i)
  {
    sendCommand(connection, TRAFFIC_WRITE_COMMAND, key, command);
    usleep(PACKET_DELTA*1000);
  }

  command.resize(0);
  sendCommand(connection, DELETE_CONNECTION_COMMAND, key, command);

  return 0;
}
