#include <stdio.h>
#include <string.h>

// From cracklib,2.7/cracklib/fascist.o
// char* FascistCheck(char* passwd, char* path, char* gecos);

int main(int ARGC, char* ARGV[]) {
  char gecos[256];
  char* path = "/usr/local/lib/pw_dict";
  char passwd[256];
  char* retval;

  if (ARGC < 4) {
    printf("Usage: checkpass <pass> <login> <fullname>\n");
    return 0;
  }

  strcpy(passwd,ARGV[1]);
  sprintf(gecos,"%s:%s",ARGV[2],ARGV[3]);

  retval = (char *) FascistCheck(passwd,path,gecos);

  if (retval!=NULL) {
    printf("Invalid Password: %s\n",retval);
    return 1;
  }
  printf("ok\n");
  return 0;
}
