/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// dumb-server.c

#include "stub.h"

int main(int argc, char * argv[])
{
  char buffer[MAX_PAYLOAD_SIZE] = {0};
  int connection = -1;
  int error = 0;
  struct sockaddr_in address;
  int client = -1;
  int address_size = sizeof(address);

  if (argc != 2)
  {
    printf("Usage: %s <port-number>\n", argv[0]);
    return 1;
  }

  connection = socket(AF_INET, SOCK_STREAM, 0);
  if (connection == -1)
  {
    printf("Failed socket()\n");
    return 1;
  }

  address.sin_family = AF_INET;
  address.sin_port = htons(atoi(argv[1]));
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(address.sin_zero, '\0', 8);

  error = bind(connection, (struct sockaddr *)&address, sizeof(address));
  if (error == -1)
  {
    printf("Failed bind()\n");
    return 1;
  }

  error = listen(connection, 1);
  if (error == -1)
  {
    printf("Failed listen()\n");
    return 1;
  }

  client = accept(connection, (struct sockaddr *)&address, &address_size);
  if (client == -1)
  {
    printf("Failed accept()\n");
    return 1;
  }

  while (1)
  {
    recv(client, buffer, MAX_PAYLOAD_SIZE, 0);
  }

  return 0;
}

