/* hcmdgrab - utility to obtain stored data in hcmd */

#include "hmcd.h"

int main(int argc, char **argv) {

  int usd, numbytes;
  char lbuf[256], *msg, *enttime, *curtok;
  struct sockaddr_un unixaddr;
  
  if (argc < 2 || argc > 3) {
    printf("Usage: %s <node_name>\n", argv[0]);
    return 1;
  }

  bzero(lbuf, sizeof(lbuf));

  msg = (char*)malloc(strlen(argv[1])+5);
  sprintf(msg, "GET %s", argv[1]);

  bzero(&unixaddr, sizeof(struct sockaddr_un));
  unixaddr.sun_family = AF_LOCAL;
  strcpy(unixaddr.sun_path, SOCKPATH);
  
  if ((usd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
    perror("Can't create UNIX domain socket");
    return 1;
  }

  if (connect(usd, (struct sockaddr*)&unixaddr, SUN_LEN(&unixaddr)) < 0) {
    perror("Couldn't connect to hcmd");
    return 1;
  }

  if (send(usd, msg, strlen(msg)+1, 0) < 0) {
    perror("Problem sending message");
    return 1;
  }

  if ((numbytes = recv(usd, lbuf, sizeof(lbuf), 0)) > 0) {
    lbuf[numbytes-1] = '\0';
    if (!strncmp(lbuf, "NULL", 4)) {
      return 1;
    }
    enttime = strtok(lbuf, " |");
    while ((curtok = strtok(NULL, " |"))) {
      printf("%s@%s\n", curtok, enttime);
    }
  }

  return 0;
}
