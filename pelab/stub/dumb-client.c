/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// dumb-client.c

#include "stub.h"

int main(int argc, char * argv[])
{
  char buffer[MAX_PAYLOAD_SIZE] = {0};
  int connection = -1;
  int error = 0;
  struct sockaddr_in address;

  if (argc != 3)
  {
    printf("Usage: %s <server-ip> <server-port>\n", argv[0]);
    return 1;
  }

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

  int count = 0;

  while (1)
  {
//    usleep(1000);
    count = send(connection, buffer, MAX_PAYLOAD_SIZE, MSG_DONTWAIT);
//    printf("Sent %d bytes\n", count);
  }

  return 0;
}
