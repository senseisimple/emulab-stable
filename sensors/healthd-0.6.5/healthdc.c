/*-
 * Copyright (c) 1999-2000 James E. Housley <jim@thehousleys.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: healthdc.c,v 1.1 2001-12-05 18:45:09 kwebb Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

#if !defined(HAVE_GETADDRINFO) || !defined(HAVE_GETNAMEINFO)
#include "addrinfo.h"
#endif

#include "VERSION.h"

int main(int argc, char *argv[], char *envp[]);
void PrintNormalOutput(int , char *);
void PrintHTTPOutput(int , char *);
int OpenConnection(char *, char *, int);
int GetProtocolVersion(int sock);
int GetDaemonVersion(int sock);
void ParseLine(char *);
int hex2dec(char *ptr);
void BailOut(int code);

float VerProto = 1.0;
int VerDaemon_major = 0;
int VerDaemon_minor = 6;
int VerDaemon_sub = 0;

int tabout = 0;
int verbose = 0;
int debug = 0;
int html = 0;
int html_body = 0;

int CONTENT_LENGTH = 0;
int POST=1;

#define PrintMAX 10240
char InputLine[PrintMAX];

#define MAX_TARGETS 255
char *Target[MAX_TARGETS+1];
int TargetPtr;

char *query_string = NULL;

int
main(int argc, char *argv[], char *envp[]) {
  char ch;
  int x, y, z;
  int len;
  int sock;
  struct servent *sp;
  unsigned long port;
  char HostNameBuffer[MAXHOSTNAMELEN];
  char portstr[16];

  for (sock=0; sock<=MAX_TARGETS; sock++) {
    Target[sock] = NULL;
  }
  TargetPtr = 0;

  if ((sp = getservbyname("healthd", "tcp")) == NULL) {
    port = 1281;
    strcpy(portstr, "1281");
  } else {
    port = ntohs(sp->s_port);
    sprintf(portstr, "%ld", port);
  }
  while((ch = getopt(argc, argv, "HPVdhtv")) != -1) {
    switch(ch){
    case 'H':
      html = 1;
      POST = 0;
      break;

    case 'P':
      port = atoi(optarg);
      break;

    case 'V':
      fprintf(stderr, "Version %s\n", hdVERSION);
      exit(0);
      break;

    case 'd':
      debug = 1;
      break;

    case 'h':
      html = 1;
      html_body = 1;
      POST = 0;
      break;

    case 't':
      tabout = 1;
      break;

    case 'v':
      verbose = 1;
      break;

    default:
      fprintf(stderr, "Usage: %s [-H|h] [-P port] [-V] [-d] [-t|v] [hostname ...]\n", argv[0]);
      exit(1);
    }
  }

  while (optind < argc) {
    if ((Target[TargetPtr] = (char *)malloc(1+strlen(argv[optind]))) == NULL) {
      for (sock=0; sock<TargetPtr; sock++) {
	free(Target[TargetPtr]);
      }
      exit(1);
    }
    strcpy(Target[TargetPtr], argv[optind]);
    TargetPtr++;
    optind++;
  }
  sock = 0;
  while (envp[sock] != NULL) {
    ParseLine(envp[sock]);
    sock++;
  }
  if (html) {
    if (html_body == 0) {
      printf("Content-type: text/html\n\n<HTML><TITLE>healthd</TITLE><BODY>\n");
      x = 0;
      while (envp[x] != NULL) {
	x++;
      }
    }
    if (POST) {
      /*
       * The POST (origional) method was used
       */
      y = 0;
      len = 0;
      do {
	z = fgetc(stdin);
	len++;
	InputLine[y++] = z;
	if ((z == '&') || (z == EOF) || (z == '\n') || 
	    (z == '\r') || (z == ' ')) {
	  InputLine[y-1] = '\0';
	  ParseLine(InputLine);
	  y = 0;
	}
	if (CONTENT_LENGTH != 0) {
	  if (len == CONTENT_LENGTH) {
	    InputLine[y] = '\0';
	    ParseLine(InputLine);
	    break;
	  }
	}
	if (y > (PrintMAX - 4)) {
	  printf("<BR><CENTER><H1>ERROR!!!</H1><BR><BR><H2>INPUT TOO LARGE!!!!<BR>\n");
	  y = 0;
	}
      } while ((z != EOF) && (z != '\n') && (z != '\r') && (z != ' '));
    } else {
      if (query_string != NULL) {
	/*
	 * The GET method was used
	 */
	char *ptr;

	y = 0;
	len = 0;
	ptr = query_string;
	ptr += sizeof("QUERY_STRING=") - 1;
	do {
	  z = *ptr++;
	  len++;
	  InputLine[y++] = z;
	  if ((z == '&') || (z == EOF) || (z == '\n') || 
	      (z == '\r') || (z == ' ')) {
	    InputLine[y-1] = '\0';
	    ParseLine(InputLine);
	    y = 0;
	  }
	  if (*ptr == '\0') {
	    InputLine[y] = '\0';
	    ParseLine(InputLine);
	    break;
	  }
	  if (y > (PrintMAX - 4)) {
	    printf("<BR><CENTER><H1>ERROR!!!</H1><BR><BR><H2>INPUT TOO LARGE!!!!<BR>\n");
	    y = 0;
	  }
	} while ((z != '\0') && (z != '\n') && (z != '\r') && (z != ' '));
      }
    }
  }
  if (TargetPtr == 0) {
    if ((Target[TargetPtr] = (char *)malloc(1+strlen("localhost"))) == NULL) {
      for (sock=0; sock<TargetPtr; sock++) {
	free(Target[TargetPtr]);
      }
      exit(1);
    }
    strcpy(Target[TargetPtr], "localhost");
    TargetPtr++;
  }
  for (x=0; x<TargetPtr; x++) {
    if (*Target[x] == '[' && strrchr(Target[x], ']') != NULL) { 
      /* IPv6 address in [] */
      strncpy(HostNameBuffer, Target[x] + 1, strlen(Target[x]) - 2);
      HostNameBuffer[strlen(Target[x]) - 2] = '\0';
    } else {
      strncpy(HostNameBuffer, Target[x], strlen(Target[x]));
      HostNameBuffer[strlen(Target[x])] = '\0';
    }
    if ((sock = OpenConnection(HostNameBuffer, portstr, sizeof(HostNameBuffer))) == 0) {
      continue;
    }

    if (GetProtocolVersion(sock)) {
      close(sock);
      continue;
    }
    if (GetDaemonVersion(sock)) {
      close(sock);
      continue;
    }

    if (html) {
      PrintHTTPOutput(sock, HostNameBuffer);
    } else {
      PrintNormalOutput(sock, HostNameBuffer);
    }
  }

  for (sock=0; sock<=MAX_TARGETS; sock++) {
    if (Target[sock] != NULL) {
      free(Target[sock]);
    }
  }
  if (query_string != NULL) {
    free(query_string);
  }

  if ((html) && (html_body == 0)) {
    printf("</BODY></HTML>\n");
  }
  exit(0);
}

int 
OpenConnection(char *HostName, char *portstr, int size) {
  char *hostptr;
  struct addrinfo hints, *res, *res0;
  const char *cause = NULL;
  int error;
  int sock;

  hostptr = HostName;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_CANONNAME;
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  error = getaddrinfo(hostptr, portstr, &hints, &res0);
  if (error) {
    warnx("%s: %s", hostptr, gai_strerror(error));
    if (error == EAI_SYSTEM)
      warnx("%s: %s", hostptr, strerror(errno));
    return (0);
  }

  res = res0;
  if (res->ai_canonname) {
    (void)strncpy(HostName, res->ai_canonname, size);
  }
  sock = -1;
  for (res = res0; res; res = res->ai_next) {
    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
      cause = "socket";
      continue;
    }
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
      cause = "connect";
      close(sock);
      sock = -1;
      continue;
    }

    /* We made a connection */
    break;
  }
  if (sock < 0) {
    err(1, cause);
  }
  freeaddrinfo(res0);
  return(sock);
}

void
PrintNormalOutput(int sock, char *HostNameBuffer) {
  int x;
  int rval;
  char query[16];
  char seprt[8];
  char buf[1024];

  if (verbose) {
    sprintf(query, "GTV");
    sprintf(seprt, ", ");
  } else {
    sprintf(query, "GET");
    sprintf(seprt, "\t");
  }

  printf("%s%s", HostNameBuffer, seprt);
  for (x=1; x<=3; x++) {
    sprintf(buf, "%s T%d", query, x);
    if (write(sock, buf, strlen(buf)) < 0) {
      perror("sending stream message");
    }
    bzero(buf, sizeof(buf));
    if ((rval = read(sock, buf, 1024)) < 0) {
      perror("reading stream message");
    }
    if (rval == 0) {
      printf("Ending connections\n");
      goto lost_connection;
    } else {
      printf("%s%s", buf, seprt);
    }
  }
  for (x=1; x<=3; x++) {
    sprintf(buf, "%s S%d", query, x);
    if (write(sock, buf, strlen(buf)) < 0) {
      perror("sending stream message");
    }
    bzero(buf, sizeof(buf));
    if ((rval = read(sock, buf, 1024)) < 0) {
      perror("reading stream message");
    }
    if (rval == 0) {
      printf("Ending connections\n");
      goto lost_connection;
    } else {
      printf("%s%s", buf, seprt);
    }
  }
  for (x=1; x<=7; x++) {
    sprintf(buf, "%s V%d", query, x);
    if (write(sock, buf, strlen(buf)) < 0) {
      perror("sending stream message");
    }
    bzero(buf, sizeof(buf));
    if ((rval = read(sock, buf, 1024)) < 0) {
      perror("reading stream message");
    }
    if (rval == 0) {
      printf("Ending connections\n");
      goto lost_connection;
    } else {
      printf("%s%s", buf, seprt);
    }
  }
lost_connection:
  close(sock);
  printf("\n");
}

void
PrintHTTPOutput(int sock, char *HostNameBuffer) {
  int x;
  int rval;
  char buf[1024];
  int active;
  int Ival;
  float Fval;

  printf("<CENTER><H1>%s</H1></CENTER>", HostNameBuffer);
  printf("<CENTER><TABLE BORDER=2>\n");
  if (VerProto < 2.0) {
    for (x=1; x<=3; x++) {
      sprintf(buf, "GTV T%d", x);
      if (write(sock, buf, strlen(buf)) < 0) {
	perror("sending stream message");
      }
      bzero(buf, sizeof(buf));
      if ((rval = read(sock, buf, 1024)) < 0) {
	perror("reading stream message");
      }
      if (rval == 0) {
	printf("Ending connections\n");
	goto lost_connection;
      } else {
	sscanf(buf, "%f|%d|", &Fval, &active);
	if (active) {
	  printf("<TR><TD>Temp #%d</TD><TD>%4.1f</TD></TR>\n", x, Fval);
	}
      }
    }
    for (x=1; x<=3; x++) {
      sprintf(buf, "GTV S%d", x);
      if (write(sock, buf, strlen(buf)) < 0) {
	perror("sending stream message");
      }
      bzero(buf, sizeof(buf));
      if ((rval = read(sock, buf, 1024)) < 0) {
	perror("reading stream message");
      }
      if (rval == 0) {
	printf("Ending connections\n");
	goto lost_connection;
      } else {
	sscanf(buf, "%d|%d|", &Ival, &active);
	if (active) {
	  printf("<TR><TD>Fan Speed #%d</TD><TD>%d</TD></TR>\n", x, Ival);
	}
      }
    }
    for (x=1; x<=7; x++) {
      sprintf(buf, "GTV V%d", x);
      if (write(sock, buf, strlen(buf)) < 0) {
	perror("sending stream message");
      }
      bzero(buf, sizeof(buf));
      if ((rval = read(sock, buf, 1024)) < 0) {
	perror("reading stream message");
      }
      if (rval == 0) {
	printf("Ending connections\n");
	goto lost_connection;
      } else {
	sscanf(buf, "%f|%d|", &Fval, &active);
	if (active) {
	  printf("<TR><TD>Voltage #%d</TD><TD>%4.2f</TD></TR>\n", x, Fval);
	}
      }
    }
    printf("</TABLE></CENTER>\n");
  } else {
    /*
     * Version 2.0 or newer
     */
    for (x=1; x<=3; x++) {
      sprintf(buf, "CFG T%d_ACT", x);
      if (write(sock, buf, strlen(buf)) < 0) {
	perror("sending stream message");
      }
      bzero(buf, sizeof(buf));
      if ((rval = read(sock, buf, 1024)) < 0) {
	perror("reading stream message");
      }
      if (rval == 0) {
	printf("Ending connections\n");
	goto lost_connection;
      }
      if (strncasecmp(buf, "yes", strlen("yes")) == 0) {
	sprintf(buf, "CFG T%d_LBL", x);
	if (write(sock, buf, strlen(buf)) < 0) {
	  perror("sending stream message");
	}
	bzero(buf, sizeof(buf));
	if ((rval = read(sock, buf, 1024)) < 0) {
	  perror("reading stream message");
	}
	if (rval == 0) {
	  printf("Ending connections\n");
	  goto lost_connection;
	} else {
	  printf("<TR><TD ALIGN=RIGHT>%s</TD>", buf);
	}
	sprintf(buf, "GET T%d", x);
	if (write(sock, buf, strlen(buf)) < 0) {
	  perror("sending stream message");
	}
	bzero(buf, sizeof(buf));
	if ((rval = read(sock, buf, 1024)) < 0) {
	  perror("reading stream message");
	}
	if (rval == 0) {
	  printf("Ending connections\n");
	  goto lost_connection;
	} else {
	    printf("<TD>%s</TD></TR>\n", buf);
	}
      }
    }
    for (x=1; x<=3; x++) {
      sprintf(buf, "CFG F%d_ACT", x);
      if (write(sock, buf, strlen(buf)) < 0) {
	perror("sending stream message");
      }
      bzero(buf, sizeof(buf));
      if ((rval = read(sock, buf, 1024)) < 0) {
	perror("reading stream message");
      }
      if (rval == 0) {
	printf("Ending connections\n");
	goto lost_connection;
      }
      if (strncasecmp(buf, "yes", strlen("yes")) == 0) {
	sprintf(buf, "CFG F%d_LBL", x);
	if (write(sock, buf, strlen(buf)) < 0) {
	  perror("sending stream message");
	}
	bzero(buf, sizeof(buf));
	if ((rval = read(sock, buf, 1024)) < 0) {
	  perror("reading stream message");
	}
	if (rval == 0) {
	  printf("Ending connections\n");
	  goto lost_connection;
	} else {
	  printf("<TR><TD ALIGN=RIGHT>%s</TD>", buf);
	}
	sprintf(buf, "GET S%d", x);
	if (write(sock, buf, strlen(buf)) < 0) {
	  perror("sending stream message");
	}
	bzero(buf, sizeof(buf));
	if ((rval = read(sock, buf, 1024)) < 0) {
	  perror("reading stream message");
	}
	if (rval == 0) {
	  printf("Ending connections\n");
	  goto lost_connection;
	} else {
	    printf("<TD>%s</TD></TR>\n", buf);
	}
      }
    }
    for (x=1; x<=7; x++) {
      sprintf(buf, "CFG V%d_ACT", x);
      if (write(sock, buf, strlen(buf)) < 0) {
	perror("sending stream message");
      }
      bzero(buf, sizeof(buf));
      if ((rval = read(sock, buf, 1024)) < 0) {
	perror("reading stream message");
      }
      if (rval == 0) {
	printf("Ending connections\n");
	goto lost_connection;
      }
      if (strncasecmp(buf, "yes", strlen("yes")) == 0) {
	sprintf(buf, "CFG V%d_LBL", x);
	if (write(sock, buf, strlen(buf)) < 0) {
	  perror("sending stream message");
	}
	bzero(buf, sizeof(buf));
	if ((rval = read(sock, buf, 1024)) < 0) {
	  perror("reading stream message");
	}
	if (rval == 0) {
	  printf("Ending connections\n");
	  goto lost_connection;
	} else {
	  printf("<TR><TD ALIGN=RIGHT>%s</TD>", buf);
	}
	sprintf(buf, "GET V%d", x);
	if (write(sock, buf, strlen(buf)) < 0) {
	  perror("sending stream message");
	}
	bzero(buf, sizeof(buf));
	if ((rval = read(sock, buf, 1024)) < 0) {
	  perror("reading stream message");
	}
	if (rval == 0) {
	  printf("Ending connections\n");
	  goto lost_connection;
	} else {
	    printf("<TD>%s</TD></TR>\n", buf);
	}
      }
    }
    printf("</TABLE></CENTER>\n");
  }
lost_connection:
  close(sock);
  printf("\n");
}

int 
GetProtocolVersion(int sock) {
  char buf[1024];
  int rval;
  float value;

  VerProto = 1.0;
  sprintf(buf, "VER P");
  if (write(sock, buf, strlen(buf)) < 0) {
    perror("sending stream message");
  }
  bzero(buf, sizeof(buf));
  if ((rval = read(sock, buf, 1024)) < 0) {
    perror("reading stream message");
  }
  if (rval == 0) {
    printf("Ending connections\n");
    return 1;
  } else {
    if (strncmp(buf, "ERROR", 5) != 0) {
      if (sscanf(buf, "%f", &value) == 1) {
	VerProto = value;
      }
    }
  }
  return 0;
}

int 
GetDaemonVersion(int sock) {
  char buf[1024];
  int rval;
  int major, minor, sub;

  VerDaemon_major = 0;
  VerDaemon_minor = 6;
  VerDaemon_sub = 0;
  sprintf(buf, "VER d");
  if (write(sock, buf, strlen(buf)) < 0) {
    perror("sending stream message");
  }
  bzero(buf, sizeof(buf));
  if ((rval = read(sock, buf, 1024)) < 0) {
    perror("reading stream message");
  }
  if (rval == 0) {
    printf("Ending connections\n");
    return 1;
  } else {
    if (strncmp(buf, "ERROR", 5) != 0) {
      if (sscanf(buf, "%d.%d.%d", &major, &minor, &sub) == 3) {
	VerDaemon_major = major;
	VerDaemon_minor = minor;
	VerDaemon_sub = sub;
      }
    }
  }
  return 0;
}

int 
hex2dec(char *ptr) {
  int rtn;

  if (('0' <= ptr[0]) && (ptr[0] <= '9'))
    rtn = ptr[0] -'0';
  else if (('A' <= ptr[0]) && (ptr[0] <= 'F'))
    rtn = ptr[0] -'A' + 10;
  else if (('a' <= ptr[0]) && (ptr[0] <= 'f'))
    rtn = ptr[0] -'a' + 10;
  else 
    rtn = 0;
  rtn *= 16;
  if (('0' <= ptr[1]) && (ptr[1] <= '9'))
    rtn += ptr[1] -'0';
  else if (('A' <= ptr[1]) && (ptr[1] <= 'F'))
    rtn += ptr[1] -'A' + 10;
  else if (('a' <= ptr[1]) && (ptr[1] <= 'f'))
    rtn += ptr[1] -'a' + 10;
  else 
    rtn += 0;
  return rtn;
}

void 
BailOut(int code) {
  if (html) {
    printf("<BR><CENTER><H1>ERROR!!!</H1><BR><BR><H2>");
  }
  printf("Internal Error #%d\n", code);
  if (html) {
    printf("</HTML>\n\n");
  }
  exit(code);
}

void 
ParseLine(char *line) {
  int x, y, len;

  if (strncmp("QUERY_STRING=", line, strlen("QUERY_STRING=")) == 0) {
    html = 1;
    if (strlen(line) != strlen("QUERY_STRING=")) {
      if ((query_string = (char *)malloc(1+strlen(line))) != NULL) {
	strcpy(query_string, line);
      } else {
	BailOut(1007);
      }
    }
  }
  else if (strncmp("HTTP_REFERER=", line, strlen("HTTP_REFERER=")) == 0) {
    html = 1;
  }
  else if (strncmp("REQUEST_METHOD=", line, strlen("REQUEST_METHOD=")) == 0) {
    html = 1;
    if (strlen(line) != strlen("REQUEST_METHOD=")) {
      if (strncmp("REQUEST_METHOD=GET", line, strlen("REQUEST_METHOD=GET")) == 0) {
	POST = 0;
      }
    }
  }
  else if (strncmp("HTTP_CONTENT_LENGTH=", line, strlen("HTTP_CONTENT_LENGTH=")) == 0) {
    int length;

    html = 1;
    if (sscanf(line, "HTTP_CONTENT_LENGTH=%d", &length) == 1) {
      CONTENT_LENGTH = length;
    }
  }
  else if (strncmp("CONTENT_LENGTH=", line, strlen("CONTENT_LENGTH=")) == 0) {
    int length;

    html = 1;
    if (sscanf(line, "CONTENT_LENGTH=%d", &length) == 1) {
      CONTENT_LENGTH = length;
    }
  }
  else if (strncmp("SERVER_ADMIN=", line, strlen("SERVER_ADMIN=")) == 0) {
    html = 1;
  } else {
    /*
     * Process the rest
     */
    len = strlen(line);
    y = 0;
    for (x=0; x<=len; x++) {
      if (line[x] == '+') {
	line[y++] = ' ';
      }
      else if (line[x] == '%') {
	line[y++] = hex2dec(line+(x+1));
	x += 2;
	if ((line[y-2] == 0x0D) && (line[y-1] == 0x0A)) {
	  line[y-2] = 0x0A;
	  y -= 1;
	}
      } else {
	line[y++] = line[x];
      }
    }
    y--;
    line[y] = '\0';
    if (strncmp("HEALTHD_TARGET=", line, strlen("HEALTHD_TARGET=")) == 0) {
      if (strlen(line) != strlen("HEALTHD_TARGET=")) {
	/* Not the first, already data in user->uname */
	if ((Target[TargetPtr] = (char *)malloc(1+strlen(line))) != NULL) {
	  strcpy(Target[TargetPtr], line+sizeof("HEALTHD_TARGET"));
	  TargetPtr++;
	} else {
	  BailOut(1007);
	}
      }
    }
  }
}
