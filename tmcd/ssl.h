/*
 * SSL prototypes and definitions.
 */

int		tmcd_server_sslinit(void);
int		tmcd_client_sslinit(void);
int		tmcd_sslaccept(int sock, struct sockaddr *, socklen_t *);
int		tmcd_sslconnect(int sock, const struct sockaddr *, socklen_t);
int		tmcd_sslwrite(int sock, const void *buf, size_t nbytes);
int		tmcd_sslread(int sock, void *buf, size_t nbytes);
int		tmcd_sslclose(int sock);
int		tmcd_sslverify(int sock, char *host);

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
#define ACCEPT		accept
#define CONNECT		connect
#define WRITE		write
#define READ		read
#define CLOSE		close
#endif
