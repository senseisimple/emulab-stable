/* rdp-mime-winxp.exe - Exe wrapper for a Perl script, making
 *                        it acceptable as a file-type opener.
 *
 * Compile with gcc -o rdp-mime-winxp.{exe,c}
 *
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
  if (argc != 2)
    printf("Usage: rdp-mime-winxp.exe pcxxx.tbrdp\n");
  else
  {
    /* Just pass the file argument on to the Perl script. */
    argv[0] = "rdp-mime-winxp.pl";
    execvp(argv[0], argv);       /* Shouldn't return. */
    perror("rdp-mime-winxp.pl");
  }
  sleep(5);
  return 1;
}
