/* ssh-mime-winxp.exe - Exe wrapper for a Perl script, making
 *                      it acceptable as a file-type opener.
 *
 * Compile with gcc -o ssh-mime-winxp.{exe,c}
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
    printf("Usage: ssh-mime-winxp.exe pcxxx.tbssh\n");
  else
  {
    /* Just pass the file argument on to the Perl script. */
    argv[0] = "ssh-mime-winxp.pl";
    execvp(argv[0], argv);       /* Shouldn't return. */
    perror("ssh-mime-winxp.pl");
  }
  sleep(5);
  return 1;
}
