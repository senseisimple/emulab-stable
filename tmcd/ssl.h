/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002, 2004, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * SSL prototypes and definitions.
 */

int		tmcd_server_sslinit(void);
int		tmcd_client_sslinit(void);
int		tmcd_sslaccept(int sock, struct sockaddr *, socklen_t *, int);
int		tmcd_sslconnect(int sock, const struct sockaddr *, socklen_t);
int		tmcd_sslwrite(int sock, const void *buf, size_t nbytes);
int		tmcd_sslread(int sock, void *buf, size_t nbytes);
int		tmcd_sslclose(int sock);
int		tmcd_sslverify_client(char *, char *, char *, int);
int		isssl;
int		nousessl;

/*
 * The client sends this tag to indicate that it is SSL capable.
 * Only local nodes can skip SSL. Remote nodes must use SSL!
 */
#define SPEAKSSL	"ISPEAKSSL_TMCDV10"

/*
 * When compiled to use SSL, redefine the routines appropriately
 */
#ifdef WITHSSL
#define ACCEPT		tmcd_sslaccept
#define CONNECT		tmcd_sslconnect
#define WRITE		tmcd_sslwrite
#define READ		tmcd_sslread
#define CLOSE		tmcd_sslclose
#else
#ifdef _WIN32
inline int win_send(SOCKET s, const char FAR* buf, int len)
{
        return send(s,buf,len,0);
}
inline int win_recv(SOCKET s, char FAR* buf, int len)
{
        return recv(s,buf,len,0);
}
#define ACCEPT		tmcd_accept
#define CONNECT		connect
#define WRITE		win_send
#define READ		win_recv
#define CLOSE		close
#else
#define ACCEPT		tmcd_accept
#define CONNECT		connect
#define WRITE		write
#define READ		read
#define CLOSE		close
#endif /*_WIN32*/
#endif /*WITHSSL*/
