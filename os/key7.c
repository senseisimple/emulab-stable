#include <stdio.h>

int main()
{
  char buf[1024*32];
  int size;
  int i;
  size = fread(buf,1,1024*32,stdin);
  for (i=0;i<size;++i) {
    printf("%02x",buf[i]);
  }
  fflush(stdout);
  return 0;
}
