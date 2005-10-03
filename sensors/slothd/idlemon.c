/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* idlemon is started up at user login on Windows to monitor desktop events
 * for slothd under RDP logins.  It has to be in the user session context for
 * GetLastInputInfo() to work.  Output is just touching a time tag file.
 */
char *rdp_time_file = "/var/run/rdp_input";

#include "slothd.h"
#include "config.h"

void lerror(const char* msgstr) {
  if (msgstr) {
    syslog(LOG_ERR, "%s: %m", msgstr);
    fprintf(stderr, "idlemon: %s: %s\n", msgstr, strerror(errno));
  }
}
char err_buff[BUFSIZ];

int main(int argc, char **argv) {
  int ch;
  int debug = 0;
  int input_occurred_fd;
  time_t last_time = 0;
  
  while ((ch = getopt(argc, argv, "d")) != -1) {
    switch (ch) {
    case 'd':
      debug = 1;
      break;
    }
  }

  input_occurred_fd = open(
    rdp_time_file, O_CREAT | O_TRUNC | O_WRONLY, 0666);
  if (input_occurred_fd == -1) {
    sprintf(err_buff, "Failed to open rdp_time_file %s.", rdp_time_file);
    lerror(err_buff);
    exit(1);
  }
  fchmod(input_occurred_fd, 0666); /* Everyone can write, if umask blocked it. */

  /* Poll the time of last keyboard or mouse event, once every few seconds. */
  for (; 1; sleep(5)) {
    LASTINPUTINFO windows_input;
    windows_input.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&windows_input) == 0) {
      lerror("Failed GetLastInputInfo().");
      exit(2);
    }
    else {

      /* Windows keeps time in millisecond ticks since boot time. */
      DWORD windows_ticks = GetTickCount() - windows_input.dwTime;
      time_t windows_time = time(0) - windows_ticks/1000;
      if (debug)
	printf("Windows input event time: %s", ctime(&windows_time));
      
      if (windows_time > last_time) {
	last_time = windows_time;

	/* It would be nice to use futimes(), but there's not one
	 * on Cygwin.  Instead, use ftruncate() to set the modtime.
	 */
	if (ftruncate(input_occurred_fd, 0)) {
	  sprintf(err_buff, "Error touching rdp input time file, %s.",
		strerror(errno));
	  lerror(err_buff);
	  exit(3);
	}
      }
    }
  }
}
