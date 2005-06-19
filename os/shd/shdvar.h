/*
 * A concatenated disk is described at initialization time by this structure.
 */
struct shddevice {
	int		shd_unit;	/* logical unit of this shd */
	int		shd_flags;	/* misc. information */
	int		shd_dk;		/* disk number */
	struct vnode	*shd_srcvp;	/* source vnode */
	struct vnode	*shd_copyvp;	/* copy vnode */
	char		*shd_srcpath;	/* source path */
	size_t		shd_srcsize;	/* source size (DEV_BSIZE blocks) */
	char		*shd_copypath;	/* copy path */
	size_t		shd_copysize;	/* copy size (DEV_BSIZE blocks) */
};

/*
 * This structure is used to configure a shd via ioctl(2).
 */
struct shd_ioctl {
	char	*shio_srcdisk;		/* source disk path */
	size_t	shio_srcsize;		/* DEV_BSIZE blocks to map */
	char	*shio_copydisk;		/* copy disk path */
	size_t	shio_copysize;		/* DEV_BSIZE blocks to provide */
	int	shio_flags;		/* misc. information */
	int	shio_unit;		/* unit number: use varies */
	size_t	shio_size;		/* (returned) size of shd */
        int     version;
        long    delete_start;
        long    delete_end;
};

struct shd_modinfo {
        int command;
        long *buf;
        long bufsiz;
        long retsiz;
};

struct shd_used_block {
    long int key;
    long int size;
};
 
struct shd_readbuf {
        long block_num;
        char *buf;
};

struct shd_ckpt {
        char *time;
        char *name;
};
 
/* shd_flags */
#define	SHDF_ONETOONE	0x01	/* 1-to-1 mapping with source disk */
#define SHDF_COMPACT	0x02	/* compact mapping */
#define SHDF_COR	0x04	/* copy on reference */
#define SHDF_COW	0x08	/* copy on write */
#define SHDF_INITIALIZE	0x10	/* first time, don't expect existing map */
#define SHDF_TRANSIENT	0x20	/* shadow is transient (don't sync map) */
#define SHDF_VALIDATE	0x40	/* validate the shadowed object */

/* Mask of user-settable shd flags. */
#define SHDF_USERMASK	0x7F

/*
 * Component info table.
 * Describes a single component of a shadow disk.  Each shadow disk has two
 * components: the source disk being shadowed, and the write copy.
 */
struct shdcinfo {
	struct vnode	*ci_vp;			/* device's vnode */
	dev_t		ci_dev;			/* XXX: device's dev_t */
	size_t		ci_secsize;		/* sector size */
	size_t		ci_size; 		/* size */
	char		*ci_path;		/* path to component */
	size_t		ci_pathlen;		/* length of component path */
};

struct shd_softc;
struct shdmapops {
	int	(*init)(struct shd_softc *, struct proc *);
	void	(*deinit)(struct shd_softc *, struct proc *);
	int	(*lookup)(struct shd_softc *, daddr_t, size_t, int,
			  daddr_t *, daddr_t *, size_t *);
	void	(*update)(struct shd_softc *, daddr_t, size_t);
	void	(*sync)(struct shd_softc *, struct proc *);
};

/*
 * Info for "uniquely" identifying the source being shadowed.
 * Stored on the disk along with the copy map.
 */
struct shd_id {
	u_int32_t	type;
	union {
		u_int32_t crc;		/* simple CRC */
		u_int8_t  digest[16];	/* MD5 digest */
		u_int8_t  string[252];	/* string */
	} data;
};
#define SHDID_NONE	0
#define SHDID_CRC32	1
#define SHDID_MD5	3
#define SHDID_STRING	4

/*
 * A shadow disk is described after initialization by this structure.
 */
struct shd_softc {
	int		 sc_unit;		/* logical unit number */
	int		 sc_flags;		/* flags */
	int		 sc_cflags;		/* configuration flags */
	size_t		 sc_secsize;		/* sector size */
	size_t		 sc_size;		/* size of shd */
	struct shdcinfo	 sc_srcdisk;		/* source disk info */
	struct shdcinfo	 sc_copydisk;		/* copy disk info */
	struct shdmapops *sc_mapops;		/* copy map operations */
	void		 *sc_map;		/* copy map */
	struct ucred	 *sc_cred;		/* creds of creator */
	struct devstat	 device_stats;		/* device statistics */
	struct disklabel sc_label;		/* generic disk device info */
	int		 sc_openmask;
};

/* sc_flags */
#define SHDF_INITED	0x01	/* unit has been initialized */
#define SHDF_WLABEL	0x02	/* label area is writable */
#define SHDF_LABELLING	0x04	/* unit is currently being labelled */
#define SHDF_SYNC	0x08	/* map must be updated synchronously */
#define SHDF_WANTED	0x40	/* someone is waiting for the lock */
#define SHDF_LOCKED	0x80	/* unit is locked */

/*
 * Before you can use a unit, it must be configured with SHDIOCSET.
 * The configuration persists across opens and closes of the device;
 * a SHDIOCCLR must be used to reset a configuration.  An attempt to
 * SHDIOCSET an already active unit will return EBUSY.  Attempts to
 * SHDIOCCLR an inactive unit will return ENXIO.
 */
#define SHDIOCSET	_IOWR('S', 16, struct shd_ioctl)   /* enable shd */
#define SHDIOCCLR	_IOW('S', 17, struct shd_ioctl)    /* disable shd */
#define SHDCHECKPOINT       _IOWR('S', 18, struct shd_ckpt)  
#define SHDGETCHECKPOINTS    _IOWR('S', 19, struct shd_ioctl)  
#define SHDENABLECHECKPOINTING       _IOWR('S', 20, struct shd_ioctl)
#define SHDDISABLECHECKPOINTING    _IOWR('S', 21, struct shd_ioctl)
#define SHDAPPLYCHECKPOINTS _IOWR('S', 22, int)
#define SHDROLLBACK    _IOWR('S', 23, struct shd_ioctl)
#define SHDSAVECHECKPOINTMAP _IOWR('S', 24, struct shd_ioctl)
#define SHDLOADCHECKPOINTMAP _IOWR('S', 25, struct shd_ioctl)
#define SHDDELETECHECKPOINTS _IOWR('S', 26, struct shd_ioctl)
#define SHDREADBLOCK _IOWR('S', 27, struct shd_readbuf)
#define SHDSETREBOOTVERSION _IOWR('S', 28, struct shd_ioctl)
#define SHDGETMODIFIEDRANGES _IOWR('S', 29, struct shd_modinfo)
#define SHDCRASH _IOWR('S', 30, int)
#define SHDADDUSEDBLOCK _IOWR('S', 31, struct shd_used_block)
#define SHDGETLOGSIZEUSED _IOWR('S', 32, int) 

