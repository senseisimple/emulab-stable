/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#ifdef __CYGWIN__
#include <w32api/windows.h>
#include <sys/cygwin.h>
#endif /* __CYGWIN__ */

#include "be_user.h"
#include "log.h"

int be_user(const char *user)
{
	int retval = 1;
	struct passwd *pw;

	if ((pw = getpwnam(user)) == NULL) {
		fatal("invalid user: %s", user);
	}

	if (getuid() == 0) {

#ifdef __CYGWIN__
	/* 
	 * On Windows, present the plain-text password from the tmcc accounts
	 * file so remote Samba directory mounts like /proj can be accessed.
	 */
	FILE *pwd_file = fopen("/var/emulab/boot/tmcc/accounts", "r");
	static char line[255], name[30], password[30];
	int matched = 0;
	while (pwd_file && fgets(line, 255, pwd_file)) {
		if (sscanf(line, "ADDUSER LOGIN=%30s PSWD=%30s ",
			   name, password) == 2 &&
		   (matched = (strncmp(user, name, 30) == 0)))
			break; /* Found it. */
	}
	fclose(pwd_file);
	if (matched) {
		info("cygwin_logon_user: name %s, password '%s'...",
		     pw->pw_name, 
		     0 ? password : "(hidden)");
		HANDLE hToken = cygwin_logon_user(pw, password);
		if (hToken != INVALID_HANDLE_VALUE) {
			info(" suceeded\n");
			/* This sets context for setuid() below. */
			cygwin_set_impersonation_token(hToken);

			retval = 0;
		}
		else
			info(" failed\n");
	}
	else
		info("user %s, %s", pw->pw_name, "password not found\n");
#endif /* __CYGWIN__ */

		/*
		 * Initialize the group list, and then flip to uid.
		 */
		if (setgid(pw->pw_gid) ||
		    initgroups(user, pw->pw_gid) ||
		    setuid(pw->pw_uid)) {
			fatal("Could not become user: %s", user);
		}
	}

	return retval;
}
