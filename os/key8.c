#include <stdio.h>

#define TONUM(x) (x >= '0' && x <= '9' ? x - '0' : 10 + (x - 'a'))

int main()
{
  char buf[1024*32];
  int size;
  int i;
  size = fread(buf,1,1024*32,stdin);
  for (i=0;i<size;i+=2) {
    printf("%c",TONUM(buf[i])*16+TONUM(buf[i+1]));
  }
  fflush(stdout);
  return 0;
}
