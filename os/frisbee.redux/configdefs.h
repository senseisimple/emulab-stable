#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * Config info for a single image
 * XXX needs to be extended for REs.
 */
struct config_imageinfo {
	char *imageid;		/* unique name of image */
	char *path;		/* path where image is stored */
	int flags;		/* */
	char *get_options;	/* options for GET operation */
	int get_methods;	/* allowed GET transfer mechanisms */
	char *put_options;	/* options for PUT operation */
	void *extra;		/* config-type specific info */
};

/* flags */
#define CONFIG_IMAGE_ISFILE	0x1	/* path is an image file */
#define CONFIG_IMAGE_ISDIR	0x2	/* path is a directory */
#define CONFIG_IMAGE_ISGLOB	0x4	/* path is a file glob */
#define CONFIG_IMAGE_ISRE	0x8	/* path is a perl RE */

/* methods */
#define CONFIG_IMAGE_UNKNOWN	0x0
#define CONFIG_IMAGE_UCAST	0x1
#define CONFIG_IMAGE_MCAST	0x2
#define CONFIG_IMAGE_BCAST	0x4

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
	int (*config_init)(void);
	void (*config_deinit)(void);
	int (*config_read)(void);
	int (*config_get_host_authinfo)(struct in_addr *, char *,
					struct config_host_authinfo **,
					struct config_host_authinfo **);
	void (*config_free_host_authinfo)(struct config_host_authinfo *);
	int (*config_get_server_address)(struct config_host_authinfo *, int,
					 in_addr_t *, in_port_t *, int *);
	void *(*config_save)(void);
	int (*config_restore)(void *);
	void (*config_free)(void *);
	void (*config_dump)(FILE *);
};

extern int	config_init(int);
extern void	config_deinit(void);
extern int	config_read(void);
extern int	config_get_host_authinfo(struct in_addr *, char *,
					 struct config_host_authinfo **,
					 struct config_host_authinfo **);
extern void	config_free_host_authinfo(struct config_host_authinfo *);
extern int	config_auth_by_IP(struct in_addr *, char *,
				  struct config_host_authinfo **);
extern int	config_get_server_address(struct config_host_authinfo *, int,
					  in_addr_t *, in_port_t *, int *);
extern char *	config_perror(int);
extern void *	config_save(void);
extern int	config_restore(void *);
extern void	config_dump(FILE *);

#define CONFIG_ERR_HA_FAILED	1	/* internal host auth error */
#define CONFIG_ERR_HA_NOHOST	2	/* no such host */
#define CONFIG_ERR_HA_NOIMAGE	3	/* no such image */
#define CONFIG_ERR_HA_NOACCESS	4	/* access not allowed for host */
#define CONFIG_ERR_HA_NOMETHOD	5	/* not avail to host via method */
