#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * Maximum groups for a server process.
 * We don't use NGROUPS_MAX because it is huge on Linux (64K).
 */
#define MAXGIDS	(16+1)

#define NOUID (-1)

/*
 * Config info for a single image
 * XXX needs to be extended for REs.
 */
struct config_imageinfo {
	char *imageid;		/* unique name of image */
	char *dir;		/* directory to which path must resolve */
	char *path;		/* path where image is stored */
	void *sig;		/* signature of image */
	int flags;		/* */
	int uid;		/* UID to run server process as */
	gid_t gids[MAXGIDS];	/* GIDs to run server process as */
	int ngids;		/* number of valid GIDs */
	char *get_options;	/* command line options for GET server */
	int get_methods;	/* allowed GET transfer mechanisms */
	int get_timeout;	/* max time to allow GET server to run */
	char *put_options;	/* command line options for PUT server */
	uint64_t put_maxsize;	/* maximum size for this image */
	int put_timeout;	/* max time to allow PUT server to run */
	char *put_oldversion;	/* where to save the old version */
	void *extra;		/* config-type specific info */
};

/* flags */
#define CONFIG_PATH_ISFILE	0x1	/* path is an image file */
#define CONFIG_PATH_ISDIR	0x2	/* path is a directory */
#define CONFIG_PATH_ISGLOB	0x4	/* path is a file glob */
#define CONFIG_PATH_ISRE	0x8	/* path is a perl RE */
#define CONFIG_PATH_RESOLVE	0x10	/* path needs resolution at use */
#define CONFIG_PATH_EXISTS	0x20	/* imaged named by path arg exists */
#define CONFIG_SIG_ISMTIME	0x1000	/* sig is path mtime */
#define CONFIG_SIG_ISMD5	0x2000	/* sig is MD5 hash of path */
#define CONFIG_SIG_ISSHA1	0x4000	/* sig is SHA1 hash of path */

/* methods */
#define CONFIG_IMAGE_UNKNOWN	0x0
#define CONFIG_IMAGE_UCAST	0x1
#define CONFIG_IMAGE_MCAST	0x2
#define CONFIG_IMAGE_BCAST	0x4
#define CONFIG_IMAGE_ANY	0x7

struct config_host_authinfo {
	char *hostid;		/* unique name of host */
	int numimages;		/* number of images in info array */
	struct config_imageinfo *imageinfo; /* info array */
	void *extra;		/* config-type specific info */
};

/*
 * Config file functions
 */
struct config {
	void (*config_deinit)(void);
	int (*config_read)(void);
	int (*config_get_host_authinfo)(struct in_addr *,
					struct in_addr *, char *,
					struct config_host_authinfo **,
					struct config_host_authinfo **);
	void (*config_free_host_authinfo)(struct config_host_authinfo *);
	int (*config_get_server_address)(struct config_imageinfo *, int, int,
					 in_addr_t *, in_port_t *, int *);
	void *(*config_save)(void);
	int (*config_restore)(void *);
	void (*config_free)(void *);
	void (*config_dump)(FILE *);
};

extern int	config_init(char *, int);
extern void	config_deinit(void);
extern int	config_read(void);
extern int	config_get_host_authinfo(struct in_addr *,
					 struct in_addr *, char *,
					 struct config_host_authinfo **,
					 struct config_host_authinfo **);
extern void	config_dump_host_authinfo(struct config_host_authinfo *);
extern void	config_free_host_authinfo(struct config_host_authinfo *);
extern int	config_auth_by_IP(int, struct in_addr *, struct in_addr *,
				  char *, struct config_host_authinfo **);
extern int	config_get_server_address(struct config_imageinfo *, int, int,
					  in_addr_t *, in_port_t *, int *);
extern void *	config_save(void);
extern int	config_restore(void *);
extern void	config_dump(FILE *);

/* Common utility functions */
extern char *	isindir(char *dir, char *path);
extern char *	myrealpath(char *path, char rpath[PATH_MAX]);
extern char *	resolvepath(char *path, char *dir, int create);

