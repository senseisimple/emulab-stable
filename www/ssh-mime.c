/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* Compile on cygwin with gcc -mwindows -mno-cygwin -o ssh-mime ssh-mime.c */
/* Set the env PUTTY_CMD to the path of where you have put putty */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/param.h>
#ifdef _WIN32
#include <windows.h>
#endif

char *usagestr = 
 "usage: ssh-mime <control-file>"
 "\n";

char *default_sshcmd = "\"c:\\putty.exe\" -ssh -1 -A -X";
char *tempfile = "c:\\windows\\temp\\puttycmd";
char *logfile = "c:\\windows\\temp\\ssh-mime.log";
char sshcmd[MAXPATHLEN];
FILE *fplog = 0;

void
usage()
{
	fprintf(fplog, usagestr);
	exit(1);
}

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInstance,
                HINSTANCE hPrevInstance,
                LPSTR lpCmdLine,
                int nCmdShow) {
#else
int main(int argc, char *argv[]) {
#endif

    FILE *fp = 0;
    char line[1024];
    char hostname[1024];
    char gateway[1024];
    char login[64];
    char port[10];
    char cmdline[MAXPATHLEN];
    char *str = 0;
#ifdef _WIN32
    char *argv[3];
    int argc=0;

    argv[0] = strtok( GetCommandLine(), " \t\n");
    if (argv[0]) {
	argc++;
    }
    if (lpCmdLine[0] != '\0') {
	argv[1] = lpCmdLine;
	argc++;
	argv[2] = 0;
    } else {
	argv[1] = 0;
    }
#endif
    fplog = fopen(logfile, "w");
    if (fplog == 0) {
	fplog = stderr;
    }

    if (argc != 2) {
	usage();
    }

    fp = fopen(argv[1], "r");
    if (fp == 0) {
	fprintf(fplog, "ssh-mime: Error \"%s\" opening file:%s", 
		strerror(errno), argv[1]);
	exit(2);
    }
    if( (str = getenv("PUTTY_CMD")) ) {
	snprintf(sshcmd, sizeof(sshcmd), "\"%s\" -ssh -1 -A -X", 
		 str );
    } else {
	strncpy( sshcmd, default_sshcmd, sizeof(sshcmd) );
    }

    memset(line, 0, sizeof(line));
    memset(hostname, 0, sizeof(hostname));
    memset(gateway, 0, sizeof(gateway));
    memset(login, 0, sizeof(login));
    memset(port, 0, sizeof(port));
    while( fgets(line, sizeof(line), fp) != NULL ) {
        char *name = strtok(line, ":\t \n");
	char *val = strtok(NULL, ":\t \n");
	if ( name && val ) {
	    if( strcmp(name, "hostname") == 0 ) {
		strncpy( hostname, val, sizeof(hostname) );
	    } else if( strcmp(name, "gateway") == 0 ) {
		strncpy( gateway, val, sizeof(gateway) );
	    } else if( strcmp(name, "login") == 0 ) {
		snprintf( login, sizeof(login), "-l %s", val );
	    } else if( strcmp(name, "port") == 0 ) {
		snprintf( port, sizeof(port), "-P %s", val );
	    }
	}
	memset(line, 0, sizeof(line));
    }
    if( hostname[0] == '\0' ) {
	fprintf(fplog, "ssh-mime: Config file must specify a hostname\n");
	exit(3);
    }
    if( gateway[0] != '\0' ) {
        FILE *tmpfp = fopen( tempfile, "w" );
        if (tmpfp == 0) {
		fprintf(fplog, "ssh-mime: Error \"%s\" opening file:%s", 
			strerror(errno), tempfile);
		exit(4);
	}
        fprintf( tmpfp, "ssh -tt -o StrictHostKeyChecking=no %s %s",
		 port, hostname );
        fclose(tmpfp);
	snprintf( cmdline, sizeof(cmdline), "%s -t %s -m %s %s", sshcmd,
		login, tempfile, gateway);
    } else {
	snprintf( cmdline, sizeof(cmdline), "%s %s %s %s", sshcmd,
		port, login, hostname);
    }
#ifdef _WIN32
    {
        /* Just cut'n'pasted from someplace */
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory( &si, sizeof(si) );
        si.cb = sizeof(si);
        ZeroMemory( &pi, sizeof(pi) );

        // Start the child process.
        if( !CreateProcess( NULL, // No module name (use command line).
                cmdline, 	  // Command line. 
                NULL,             // Process handle not inheritable.
                NULL,             // Thread handle not inheritable.
                FALSE,            // Set handle inheritance to FALSE.
                0,                // No creation flags.
                NULL,             // Use parent's environment block.
                NULL,             // Use parent's starting directory.
                &si,              // Pointer to STARTUPINFO structure.
                &pi )             // Pointer to PROCESS_INFORMATION structure.
        )
        {
		fprintf(fplog, "Could not run cmdline:%s\n", cmdline );
        } else {
		WaitForInputIdle( pi.hProcess, 1000 ); 
	}
    }
#else
    system(cmdline);
#endif
    exit(0);
}
