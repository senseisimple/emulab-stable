/*
 * Disk framework from /dev/ccd/ccd.c
 *
 */
 
#include "shd.h"
#include "trie.h"
#include "block_alloc.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/disklabel.h>
#include <ufs/ffs/fs.h> 
#include <sys/devicestat.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>

#include <sys/shdvar.h>
#include <dev/shd/shdconf.h>

#include <vm/vm_zone.h>
#include <sys/time.h>

#ifdef SHDDEBUG
int shddebug = SHDB_FOLLOW|SHDB_INIT|SHDB_IO|SHDB_MAP|SHDB_MAPDETAIL|SHDB_ERROR;
SYSCTL_INT(_debug, OID_AUTO, shddebug, CTLFLAG_RW, &shddebug, 0, "");
#endif

#define	shdunit(x)	dkunit(x)
#define shdpart(x)	dkpart(x)

struct shdbuf {
	struct buf	cb_buf;		/* new I/O buf */
	daddr_t		cb_sbn;		/* source disk bn for I/O */
	struct buf	*cb_obp;	/* ptr. to original I/O buf */
	struct shdbuf	*cb_freenext;	/* free list link */
	int		cb_unit;	/* target unit */
};

#define SHDLABELDEV(dev)	\
	(makedev(major((dev)), dkmakeminor(shdunit((dev)), 0, RAW_PART)))

static d_open_t shdopen;
static d_close_t shdclose;
static d_strategy_t shdstrategy;
static d_ioctl_t shdioctl;
static d_dump_t shddump;
static d_psize_t shdsize;

/*
 * Map sync frequency in seconds
 */
#define SHD_TIMEOUT	30

#define NSHDFREEHIWAT	16

#define CDEV_MAJOR 174
#define BDEV_MAJOR 0

static struct cdevsw shd_cdevsw = {
	/* open */	shdopen,
	/* close */	shdclose,
	/* read */	physread,
	/* write */	physwrite,
	/* ioctl */	shdioctl,
	/* poll */	nopoll,
	/* mmap */	nommap,
	/* strategy */	shdstrategy,
	/* name */	"shd",
	/* maj */	CDEV_MAJOR,
	/* dump */	shddump,
	/* psize */	shdsize,
	/* flags */	D_DISK,
	/* bmaj */	BDEV_MAJOR
};

/* called during module initialization */
static	void shd_attach __P((void));
static	int shd_modevent __P((module_t, int, void *));

static	void shdio __P((struct shd_softc *, struct buf *, struct proc *));
static	int shd_init __P((struct shddevice *, struct proc *));
static	void shd_deinit __P((struct shddevice *, struct proc *));
static	int shd_lookup __P((char *, struct proc *p, int mode, struct vnode **));
static	void shdgetdisklabel __P((dev_t));
static	void shdmakedisklabel __P((struct shd_softc *));
static	int shdlock __P((struct shd_softc *));
static	void shdunlock __P((struct shd_softc *));

extern void (*shdinitp) (void);
void root_init (void);
extern void (*shdrebootp) (void); 
void reboot_to_checkpoint (void);

extern	struct shdmapops onetooneops, compactops;

/* Non-private for the benefit of libkvm. */
struct	shd_softc *shd_softc;
struct	shddevice *shddevs;
static	int numshd = 0;
static	int numshdactive = 0;
static	struct callout_handle shdtimo;

#define MAX_CHECKPOINTS 100
#define BLOCK_SIZE 512

int bCheckpoint;
long shadow_size; /* in BLOCK_SIZE byte blocks */

Trie * latest_trie;
int latest_version;
int reboot_version;
long block_copy (struct shd_softc *ss, struct proc *p, long src_block, long dest_block, long size, int direction);
struct TrieList
{
    Trie *trie;
    int version;
    char *name;
    char *time;
    struct TrieList *next;
} *head_trie;

int init_trie_list (void)
{
    head_trie = 0;
}

Trie * get_latest_trie (void)
{
   return latest_trie; 
}

Trie * get_trie_for_version (int version)
{
   struct TrieList *current = head_trie;
   while (current != 0)
   {
       if (current->version == version)
           return current->trie;
       current = current->next; 
   }
   printf ("Error ! Trie for version %d not found\n", version);
   return 0;
}

void set_trie_at_pos (int pos, Trie * trie)
{
   int pos_count = 0;
   struct TrieList *current = head_trie;
   while (current != 0)
   {
       pos_count++;
       if (pos_count == pos)
       {
           TrieCleanup (current->trie); 
           current->trie = trie;
           return;
       }
       current = current->next; 
   } 
   printf ("Error ! Position %d not found in list\n", pos);
}
 
Trie * create_new_trie (int version)
{
   Trie * new_trie = 0;
   struct TrieList *current;
   if (TrieInit (&new_trie, BlockAlloc, BlockFree, block_copy) == 0)
   {
       printf ("Error initializing new trie\n");
       return 0;
   }
   if (0 == head_trie)
   {
       head_trie = (struct TrieList *) malloc (sizeof (struct TrieList), M_DEVBUF, M_NOWAIT);
       head_trie->trie = new_trie;
       head_trie->version = version;
       head_trie->next = 0;
   } 
   else
   {
       current = head_trie;
       while (current->next != 0)
           current = current->next;
       current->next = (struct TrieList *) malloc (sizeof (struct TrieList), M_DEVBUF, M_NOWAIT);
       current = current->next;
       current->trie = new_trie;
       current->version = version;
       current->next = 0;
   }
   latest_trie = new_trie;
   return new_trie;
}

void delete_all_tries (void)
{
    struct TrieList *current, *prev;
    current = prev = head_trie;
    while (current != 0)
    {
        prev = current;
        current = current->next; 
        TrieCleanup (prev->trie);
        free (prev, M_DEVBUF);
        prev = 0;
    }    
    init_trie_list ();
}

void delete_version_trie (int version)
{
    struct TrieList *current, *prev;
    current = prev = head_trie;
    while (current->version != version)
    {
        prev = current;
        current = current->next; 
    }
    if (current->version == version)
    {
        if (current == head_trie)
        {
            if (current->next == 0)
            {
                TrieCleanup (current->trie);
                free (current, M_DEVBUF);
                head_trie = 0;
            }
            else
            {
                head_trie = current->next;
                TrieCleanup (current->trie);
                free (current, M_DEVBUF);
            } 
        } 
        else
        {
            prev->next = current->next;
            TrieCleanup (current->trie);
            free (current, M_DEVBUF);
        }
    } 
}

void reorder_trie_versions ()
{
    struct TrieList *current;
    int ix;
    current = head_trie;
    latest_version = ix = 1;
    if (0 == current)
    {
        create_new_trie (1);
        return;
    }
    while (current != 0)
    {
        current->version = latest_version = ix;
        current = current->next;
        ix++;
    }
}

set_checkpoint (int pos, char *name, char *time)
{
    int pos_count;
    struct TrieList *current = head_trie;
    while (current != 0)
    {
       pos_count++;
       if (pos_count == pos)
       {
           current->name = (char *) malloc (strlen (name) + 1, M_DEVBUF, M_NOWAIT);
           current->time = (char *) malloc (strlen (time) + 1, M_DEVBUF, M_NOWAIT);
           bcopy (name, current->name, strlen (name) + 1);
           bcopy (time, current->time, strlen (time) + 1);
           return;
       }
       current = current->next; 
    } 
}

print_checkpoint_time (int version)
{
    int pos_count;
    struct TrieList *current = head_trie;
    while (current != 0)
    {
       if (current->version == version)
       {
           printf ("Name = %s, Time = %s\n", current->name, current->time);
           return;
       }
       current = current->next;
    }
}

/*
 * Called by main() during pseudo-device attachment.  All we need
 * to do is allocate enough space for devices to be configured later, and
 * add devsw entries.
 */
static void
shd_attach()
{
	int i;
	int num = NSHD;
        struct shd_softc *ss;
        struct shddevice shd;
        int error;

	/* XXX */
	if (num == 1)
		num = 16;

	/*printf("shd[0-%d]: Shadow disk devices\n", num-1);*/

	shd_softc = (struct shd_softc *)malloc(num * sizeof(struct shd_softc),
	    M_DEVBUF, M_NOWAIT);
	shddevs = (struct shddevice *)malloc(num * sizeof(struct shddevice),
	    M_DEVBUF, M_NOWAIT);
	if ((shd_softc == NULL) || (shddevs == NULL)) {
		printf("WARNING: no memory for shadow disks\n");
		if (shd_softc != NULL)
			free(shd_softc, M_DEVBUF);
		if (shddevs != NULL)
			free(shddevs, M_DEVBUF);
		return;
	}
	numshd = num;
	bzero(shd_softc, num * sizeof(struct shd_softc));
	bzero(shddevs, num * sizeof(struct shddevice));

	cdevsw_add(&shd_cdevsw);

	/* XXX: is this necessary? */
	for (i = 0; i < numshd; ++i)
		shddevs[i].shd_dk = -1;
}

static int
shd_modevent(mod, type, data)
	module_t mod;
	int type;
	void *data;
{
	int error = 0;

	switch (type) {
	case MOD_LOAD:
                shdrebootp = reboot_to_checkpoint;
                shdinitp = root_init; 
		shd_attach();
		break;

	case MOD_UNLOAD:
		printf("shd0: Unload not supported!\n");
		error = EOPNOTSUPP;
		break;

	default:	/* MOD_SHUTDOWN etc */
		break;
	}
	return (error);
}

DEV_MODULE(shd, shd_modevent, NULL);

void shd_refresh_devices (struct shddevice *shd, struct proc *p)
{
    struct shd_softc *ss = &shd_softc[shd->shd_unit];
    size_t size;
    ss->sc_srcdisk.ci_dev = makedev (116, 131074);
    ss->sc_copydisk.ci_dev = makedev (116, 327682);
    if (devsw(ss->sc_copydisk.ci_dev)->d_psize != NULL)
                size = (*devsw(ss->sc_copydisk.ci_dev)->d_psize)(ss->sc_copydisk.ci_dev);
    printf ("shadow size returned = %ld\n", size); 
    printf ("src dev = %x, copy dev = %x\n", ss->sc_srcdisk.ci_dev, ss->sc_copydisk.ci_dev);
}

static int
shd_getcinfo(struct shddevice *shd, struct proc *p, int issrc)
{
	struct shd_softc *ss = &shd_softc[shd->shd_unit];
	struct shdcinfo *ci = NULL;	/* XXX */
	char *pp;
	size_t size, secsize, reqsize;
	struct vnode *vp;
	char tmppath[MAXPATHLEN];
	int error = 0;

#ifdef SHDDEBUG
	/*if (shddebug & (SHDB_FOLLOW|SHDB_INIT))
		printf("shd%d: getcinfo: %s disk\n",
		       shd->shd_unit, issrc ? "source" : "copy");*/
#endif

	ss->sc_size = 0;

	if (issrc) {
		pp = shd->shd_srcpath;
		vp = shd->shd_srcvp;
		ci = &ss->sc_srcdisk;
		reqsize = shd->shd_srcsize;
	} else {
		pp = shd->shd_copypath;
		vp = shd->shd_copyvp;
		ci = &ss->sc_copydisk;
		reqsize = shd->shd_copysize;
	}
	ci->ci_vp = vp;

	ci->ci_path = malloc(11, M_DEVBUF, M_WAITOK);
        ci->ci_pathlen = 11;
        if (issrc)
        { 
            ci->ci_dev = makedev (116, 131074);
            bcopy ("/dev/ad0s1", ci->ci_path, 11);
        }
        else
        {
            ci->ci_dev = makedev (116, 327682); 	
            bcopy ("/dev/ad0s4", ci->ci_path, 11);
        }

	/*
	 * Determine the size of the disk.
	 * If it is really a disk, we use the cdevsw routine otherwise
	 * we just GETATTR.
	 */
	size = 0;
	secsize = DEV_BSIZE;
		if (devsw(ci->ci_dev)->d_psize != NULL)
			size = (*devsw(ci->ci_dev)->d_psize)(ci->ci_dev);
		if (reqsize > 0 && reqsize < size)
			size = reqsize;
		secsize = ci->ci_dev->si_bsize_phys; 

	if (size == 0) {
		free(ci->ci_path, M_DEVBUF);
		ci->ci_path = 0;
		return (ENODEV);
	}
	if (reqsize > 0 && reqsize > size) {
		free(ci->ci_path, M_DEVBUF);
		ci->ci_path = 0;
		return (ENOSPC);
	}
	ci->ci_secsize = secsize;
	ci->ci_size = size;
        
        if (!issrc)
            shadow_size = size * (secsize / BLOCK_SIZE); 

/*#ifdef SHDDEBUG
	if (shddebug & SHDB_INIT)
		printf(" %s: reqsize=%d, size=%d, secsize=%d\n",
		       ci->ci_path, reqsize, ci->ci_size, ci->ci_secsize);
#endif*/
	return (0);
}

static void
shd_timeout(void *notused)
{
	int i;

	for (i = 0; i < numshd; i++) {
		struct shd_softc *ss = &shd_softc[i];

		if ((ss->sc_flags & (SHDF_INITED|SHDF_SYNC)) == SHDF_INITED)
			(*ss->sc_mapops->sync)(ss, NULL);
	}
	timeout((timeout_t *)shd_timeout, 0, SHD_TIMEOUT * hz);
}

int shd_iocset()
{
        int error;
        int unit = 0;
        struct shd_softc *ss;
        struct shddevice shd;

        bcopy(&shddevs[unit], &shd, sizeof(shd));
        latest_version = 1;
        bCheckpoint = 0;
        init_trie_list ();
        if (0 == create_new_trie (1))
        {
            printf ("Error creating trie!!!\n");
            return (EINVAL);
        } 

        InitBlockAllocator (EXPLICIT_CKPT_DELETE, 3, shadow_size);
                /*
                 * The shd has been successfully initialized, so
                 * we can place it into the array and read2 the disklabel.
                 */
        bCheckpoint = 1;
        bcopy(&shd, &shddevs[unit], sizeof(shd));
}

void root_init (void)
{
        int error;
        int unit = 0;
        struct shd_softc *ss;
        struct shddevice shd;
        static char *srcpath = "/dev/ad0s1";
        static char *copypath = "/dev/ad0s4";
         
        ss = &shd_softc[unit];
        bzero(&shd, sizeof(shd));
        shd.shd_unit = unit;

        if ((error = shdlock(ss)) != 0)
        {
            printf ("Error shdlock(ss)\n");
            return;
        }

        shd.shd_srcpath = srcpath;

        shd.shd_copypath = copypath;

        error = shd_init(&shd, curproc);
        if (error) {
                printf ("shd_init failed. error = %d\n", error);
                bzero(&shd_softc[unit], sizeof(struct shd_softc));
                shdunlock(ss);
                return (error);
        }
        bcopy(&shd, &shddevs[unit], sizeof(shd));
        shd_iocset();
        shdunlock(ss);
}

static int
shd_init(shd, p)
	struct shddevice *shd;
	struct proc *p;
{
	struct shd_softc *ss = &shd_softc[shd->shd_unit];
	int error = 0;

/*#ifdef SHDDEBUG
	if (shddebug & (SHDB_FOLLOW|SHDB_INIT))
		printf("shd%d: init: flags 0x%x\n",
		       shd->shd_unit, shd->shd_flags);
#endif*/

	/*
	 * Verify the source disk and destination disks
	 */
	error = shd_getcinfo(shd, p, 1);
	if (error)
		goto bad;
	error = shd_getcinfo(shd, p, 0);
	if (error)
		goto bad;

	/*
	 * Source sector size must be at least as large as the copy
	 */
	if (ss->sc_srcdisk.ci_secsize < ss->sc_copydisk.ci_secsize) {
		printf("shd%d: source bsize < copy bsize (%d < %d)\n",
		       ss->sc_unit, ss->sc_srcdisk.ci_secsize,
		       ss->sc_copydisk.ci_secsize);
		error = EINVAL;
		goto bad;
	}

	/*
	 * Set essential fields
	 */
	if (shd->shd_flags & SHDF_ONETOONE)
		ss->sc_mapops = &onetooneops;
	else
		ss->sc_mapops = &compactops;
	ss->sc_cflags = shd->shd_flags;
	ss->sc_unit = shd->shd_unit;
	ss->sc_size = ss->sc_srcdisk.ci_size;
	ss->sc_cred = crdup(p->p_ucred);
	ss->sc_secsize = ss->sc_srcdisk.ci_secsize;


	/*
	 * Add an devstat entry for this device.
	 */
	devstat_add_entry(&ss->device_stats, "shd", shd->shd_unit,
			  ss->sc_secsize, DEVSTAT_ALL_SUPPORTED,
			  DEVSTAT_TYPE_STORARRAY|DEVSTAT_TYPE_IF_OTHER,
			  DEVSTAT_PRIORITY_ARRAY);

	ss->sc_flags |= SHDF_INITED;
	if (++numshdactive == 1)
		shdtimo = timeout((timeout_t *)shd_timeout, 0,
				  SHD_TIMEOUT * hz);
	return (0);
 bad:
	shd_deinit(shd, p);
	return (error);
}

static void
shd_deinit(struct shddevice *shd, struct proc *p)
{
	struct shd_softc *ss = &shd_softc[shd->shd_unit];
	struct ucred *cred = ss->sc_cred ? ss->sc_cred : p->p_ucred;
	struct shdcinfo *ci;

	if (--numshdactive == 0)
		untimeout((timeout_t *)shd_timeout, 0, shdtimo);
	(*ss->sc_mapops->deinit)(ss, p);

	ci = &ss->sc_copydisk;
	if (ci->ci_vp)
		(void)vn_close(ci->ci_vp, FREAD|FWRITE, cred, p);
	if (ci->ci_path)
		free(ci->ci_path, M_DEVBUF);

	ci = &ss->sc_srcdisk;
	if (ci->ci_vp)
		(void)vn_close(ci->ci_vp, FREAD, cred, p);
	if (ci->ci_path)
		free(ci->ci_path, M_DEVBUF);

	if (ss->sc_cred)
		crfree(ss->sc_cred);
	ss->sc_flags &= ~SHDF_INITED;
}


/* ARGSUSED */
static int
shdopen(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	int unit = shdunit(dev);
	struct shd_softc *ss;
	struct disklabel *lp;
	int error = 0, part, pmask;

/*#ifdef SHDDEBUG
	if (shddebug & SHDB_FOLLOW)
		printf("shd%d: open: flags 0x%x\n", unit, flags);
#endif*/
	if (unit >= numshd)
		return (ENXIO);
	ss = &shd_softc[unit];

	if ((error = shdlock(ss)) != 0)
		return (error);

	lp = &ss->sc_label;

	part = shdpart(dev);
	pmask = (1 << part);

	/*
	 * If we're initialized, check to see if there are any other
	 * open partitions.  If not, then it's safe to update
	 * the in-core disklabel.
	 */
	if ((ss->sc_flags & SHDF_INITED) && (ss->sc_openmask == 0))
		shdgetdisklabel(dev);

	/* Check that the partition exists. */
	if (part != RAW_PART && ((part >= lp->d_npartitions) ||
	    (lp->d_partitions[part].p_fstype == FS_UNUSED))) {
		error = ENXIO;
		goto done;
	}

	ss->sc_openmask |= pmask;

 done:
	shdunlock(ss);
	return (0);
}

/* ARGSUSED */
static int
shdclose(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	int unit = shdunit(dev);
	struct shd_softc *ss;
	int error = 0, part;

/*#ifdef SHDDEBUG
	if (shddebug & SHDB_FOLLOW)
		printf("shd%d: close: flags 0x%x\n", unit, flags);
#endif*/

	if (unit >= numshd)
		return (ENXIO);
	ss = &shd_softc[unit];

	if ((error = shdlock(ss)) != 0)
		return (error);

	part = shdpart(dev);

	/* ...that much closer to allowing unconfiguration... */
	ss->sc_openmask &= ~(1 << part);
	shdunlock(ss);
	return (0);
}

static void
shdstrategy(bp)
	struct buf *bp;
{
	int unit = shdunit(bp->b_dev);
	struct shd_softc *ss = &shd_softc[unit];
	int wlabel;
	struct disklabel *lp;
        int error;


/*#ifdef SHDDEBUG
	if (shddebug & SHDB_FOLLOW)
		printf("shd%d: strategy: %s bp %p, bn %d, size %lu\n",
		       unit, (bp->b_flags & B_READ) ? "read" : "write",
		       bp, bp->b_blkno, (u_long)bp->b_bcount);
#endif*/
	if ((ss->sc_flags & SHDF_INITED) == 0) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}

	/*
	 * Check for required alignment.  Transfers must be a valid
	 * multiple of the sector size.
	 */
	if (bp->b_bcount % ss->sc_secsize != 0 ||
	    bp->b_blkno % (ss->sc_secsize / DEV_BSIZE) != 0) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR | B_INVAL;
		biodone(bp);
                printf ("Error! Transfer is not a valid multiple of sector size\n");
		return;
	}

	/* If it's a nil transfer, wake up the top half now. */
	if (bp->b_bcount == 0) {
		biodone(bp);
                printf ("Error! nil transfer\n");
		return;
	}

	lp = &ss->sc_label;

	/*
	 * Do bounds checking and adjust transfer.  If there's an
	 * error, the bounds check will flag that for us.
	 */
	wlabel = ss->sc_flags & (SHDF_WLABEL|SHDF_LABELLING);
	if (shdpart(bp->b_dev) != RAW_PART) {
		if (bounds_check_with_label(bp, lp, wlabel) <= 0) {
			biodone(bp);
                        printf ("Error! Bounds checking and transfer adjust failed\n");
			return;
		}
	} else {
		int pbn;        /* in sc_secsize chunks */
		long sz;        /* in sc_secsize chunks */

		pbn = bp->b_blkno / (ss->sc_secsize / DEV_BSIZE);
		sz = howmany(bp->b_bcount, ss->sc_secsize);

		/*
		 * If out of bounds return an error. If at the EOF point,
		 * simply read or write less.
		 */

		if (pbn < 0 || pbn >= ss->sc_size) {
			bp->b_resid = bp->b_bcount;
			if (pbn != ss->sc_size) {
				bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR | B_INVAL;
			}
			biodone(bp);
                        printf ("Error! Out of bounds\n");
			return;
		}

		/*
		 * If the request crosses EOF, truncate the request.
		 */
		if (pbn + sz > ss->sc_size) {
			bp->b_bcount = (ss->sc_size - pbn) * 
			    ss->sc_secsize;
		}
	}

	bp->b_resid = bp->b_bcount;
	shdio(ss, bp, curproc);
}

int delete_checkpoints (int start, int end)
{
    int ix;
    Trie * dest_trie, * temp;
    if (1 >= start)
    {
        TrieInit (&dest_trie, BlockAlloc, BlockFree, block_copy);
        for (ix = start; ix <= end; ix++)
        {
            temp = get_trie_for_version (ix);
            merge (dest_trie, temp, FREE_OVERLAP);
            if (ix != 1)
                delete_version_trie (ix);
        }
        set_trie_at_pos (1, dest_trie);
    }
    else
    {
        dest_trie = get_trie_for_version (start - 1);
        for (ix = start; ix <= end; ix++)
        {
            temp = get_trie_for_version (ix);
            merge (dest_trie, temp, FREE_OVERLAP);
            delete_version_trie (ix);
        }
    }
    reorder_trie_versions ();
}

void swapout_modified_blocks (struct shd_softc *ss, struct proc *p)
{
    Trie * merged_trie;
    int ix;
    int ok;
    TrieIterator pos;
    TrieInit (&merged_trie, BlockAlloc, BlockFree, block_copy);

    for (ix = latest_version; ix >= 1; ix--)
    {
        merge (merged_trie, get_trie_for_version (ix), DEFAULT_OVERLAP);
    }
    ok = TrieIteratorInit(merged_trie, &pos);
    if (ok)
    {
        for ( ; TrieIteratorIsValid(pos); TrieIteratorAdvance(&pos))
        {
             /* network_transfer (pos->key, depthToSize (pos->maxDepth)); */
        }
        TrieIteratorCleanup(&pos);
    }      


}

void swapout_checkpoint (int version, struct shd_softc *ss, struct proc *p)
{
    Trie * merged_trie; 
    TrieIterator pos;
    int ix;
    int ok;
    TrieInit (&merged_trie, BlockAlloc, BlockFree, block_copy);

    for (ix = latest_version; ix >= version; ix--)
    {
        merge (merged_trie, get_trie_for_version (ix), DEFAULT_OVERLAP);
    }
    ok = TrieIteratorInit(merged_trie, &pos);
    if (ok)
    {   
        for ( ; TrieIteratorIsValid(pos); TrieIteratorAdvance(&pos))
        {
             /* network_transfer (pos->key, pos->value, depthToSize (pos->maxDepth); */    
        }
        TrieIteratorCleanup(&pos);
    }       
}

/* Rolls back to checkpoint "version" */

void rollback_to_checkpoint (int version, struct shd_softc *ss, struct proc *p)
{
   
    Trie * merged_trie;
    TrieIterator pos;
    int ix;
    int ok; 
    /*printf ("Initializing merged trie\n");*/
    TrieInit (&merged_trie, BlockAlloc, BlockFree, block_copy);

    for (ix = latest_version; ix >= version; ix--)
    {
        merge (merged_trie, get_trie_for_version (ix), DEFAULT_OVERLAP);
        /*printf ("Merged trie for version %d\n", ix);   */
    }
    /*printf ("Initializing trie iterator\n");*/
    ok = TrieIteratorInit(merged_trie, &pos);
    if (ok) 
    {
        for ( ; TrieIteratorIsValid(pos); TrieIteratorAdvance(&pos))
        {
             printf ("Copying %d blocks from %ld to %ld\n", depthToSize(pos->maxDepth), pos->value, pos->key);
             block_copy (ss, p, pos->value, pos->key, depthToSize(pos->maxDepth), SHADOW_TO_SRC);    
        }
        /*printf ("Cleaning up iterator\n");*/
        TrieIteratorCleanup(&pos);
    }
    /*printf ("Cleaning up merged trie\n");
    TrieCleanup(merged_trie);
    for (ix = latest_version; ix >= version; ix--)
    {
        delete_version_trie (ix);
        printf ("Deleted trie for version %d\n", ix);
    }
    reorder_trie_versions ();
    printf ("Reordered trie versions\n");
    if (1 != latest_version)
    {
        if (0 != create_new_trie (latest_version + 1))
            latest_version++;
        printf ("Created new trie for version 1\n"); 
    }
    printf ("Returning from rollback_to_checkpoint\n");*/
}

void print_checkpoint_map (int version)
{
   int ok;
   TrieIterator pos;
   Trie * trie = get_trie_for_version (version);
   ok = TrieIteratorInit(trie, &pos);
   if (ok)
   {
       for ( ; TrieIteratorIsValid(pos); TrieIteratorAdvance(&pos))
       {
           printf ("\nbn in source = %ld, bn in shadow = %ld, size = %ld", pos->key, pos->value, depthToSize(pos->maxDepth)); 
       }
       TrieIteratorCleanup (&pos);
   } 
   /*logTrieStructure (trie);*/
}

void load_checkpoint_map (struct shd_softc *ss, struct proc *p)
{
    long int temp_addr [128];
    int ix;
    long metadata_block;
    long blocks_used;
    long map [128];
    int array_ix;
    int i, j, k;
    int write_start;
    int current_block;
    int read_start;
    int num_blocks;

    metadata_block = 2;
    latest_version = 0;

    delete_all_tries ();

    for (i = 0; i < 512 / sizeof(long int); i++)
    {
       map[i] = 0;  
       temp_addr[i] = 0;
    }   
    
    read_block (ss, p, (char *) temp_addr, metadata_block);        
    i = 0;
    while (temp_addr[i] != 0)
    {
        read_start = temp_addr[i];
        num_blocks = temp_addr[i+1]; 
      /*printf ("Reading %ld blocks starting %ld\n", num_blocks, read_start);*/ 
        i+=2;
        if (0 == create_new_trie (latest_version + 1))
        {   
            printf ("Error creating trie for version %d\n", latest_version);
            return;
        }
        else
            latest_version++;

        for (j=0; j<num_blocks; j++)
        {
            read_block (ss, p, (char *) map, read_start);
            for (k=0; k<126; k+=3)
            {
                if (map[k] == 0)
                    break;
/*                printf ("Inserting (%ld, %ld, %ld)\n", map[k], map[k+2], map[k+1]);     */ 
                TrieInsertStrong (get_trie_for_version (latest_version), map[k], map[k+2], map[k+1], DEFAULT_OVERLAP);
                /* use non-allocating version of insertweak */
            }
            read_start++;
        } 
    }
}                    

void save_checkpoint_map (struct shd_softc *ss, struct proc *p)
{
    long int temp_addr [128];
    int ix;
    long metadata_block;
    long blocks_used;
    long map [128];
    int array_ix;
    int i;
    int write_start;
    int current_block;

    metadata_block = 2;
 
    for (i = 0; i < 512 / sizeof(long int); i++)
    {
       map[i] = 0; 
       temp_addr[i] = 0;
    }

    for (ix = 1; ix < latest_version; ix++)    
    {
        Trie * trie = get_trie_for_version (ix);
        TrieIterator pos;
        int ok = TrieIteratorInit(trie, &pos);
        array_ix = 0;
        if (ok)
        {
          blocks_used = 0;
          for ( ; TrieIteratorIsValid(pos); TrieIteratorAdvance(&pos))
          {
            map [array_ix++] = pos->key; 
            map [array_ix++] = pos->value;
            map [array_ix++] = depthToSize(pos->maxDepth);
      /*      printf ("Saving (%ld, %ld, %ld)\n", pos->key, pos->value, depthToSize(pos->maxDepth)); */
            if (array_ix >= 125)
            {
                current_block = BlockAlloc (1);
                if (0 == blocks_used)
                    write_start = current_block;
                array_ix = 0;
                blocks_used++;
                if (write_block (ss, p, (char *) map, current_block))
                    return;
                 for (i = 0; i < 128; i++)
                     map[i] = 0;
            } /* end of if */
          } /* end of for */
        } /* end of if */        
        if (array_ix > 0)
        {
            current_block = BlockAlloc (1);
            if (0 == blocks_used)
                write_start = current_block;
            blocks_used++;
            if (write_block (ss, p, (char *) map, current_block))
                return;
             for (i = 0; i < 128; i++)
                 map[i] = 0;
        }
        temp_addr [2*ix - 2] = (long int) write_start;
        temp_addr [2*ix - 1] = (long int) blocks_used;
      /*  printf ("Saving version %d, %d blocks starting at %d\n", ix, blocks_used, write_start); */ 
    }
    if (write_block (ss, p, (char *) temp_addr, metadata_block))
        return;
}

int read_block (struct shd_softc *ss, struct proc *p, char block[512], long int block_num)
{
    struct uio auio;
    struct iovec aiov;
    struct vnode *vp;
    int error;
    vp = ss->sc_copydisk.ci_vp;
    bzero(&auio, sizeof(auio));
    aiov.iov_base = &block[0];
    aiov.iov_len = 512;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_offset = (vm_ooffset_t) block_num * ss->sc_secsize;
    auio.uio_segflg = UIO_SYSSPACE;
    auio.uio_rw = UIO_READ;
    auio.uio_resid = 512;
    auio.uio_procp = p;

    error = (*devsw(ss->sc_copydisk.ci_dev)->d_read) (ss->sc_copydisk.ci_dev, &auio, IO_DIRECT);
    /*vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
    error = VOP_READ(vp, &auio, IO_DIRECT, ss->sc_cred);
    VOP_UNLOCK(vp, 0, p);*/
    if (error) {
       printf("shd%d: error %d \n",
       ss->sc_unit, error);
       return 1;
    }
    return 0;
}

int write_block (struct shd_softc *ss, struct proc *p, char block[512], long int block_num)
{
    struct uio auio;
    struct iovec aiov;
    struct vnode *vp;
    int error;
    vp = ss->sc_copydisk.ci_vp;
    bzero(&auio, sizeof(auio));
    aiov.iov_base = &block[0];
    aiov.iov_len = 512;
    auio.uio_iov = &aiov;
    auio.uio_iovcnt = 1;
    auio.uio_offset = (vm_ooffset_t) block_num * ss->sc_secsize;
    auio.uio_segflg = UIO_SYSSPACE;
    auio.uio_rw = UIO_WRITE;
    auio.uio_resid = 512;
    auio.uio_procp = p;

    error = (*devsw(ss->sc_copydisk.ci_dev)->d_write) (ss->sc_copydisk.ci_dev, &auio, IO_NOWDRAIN);

/*    vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
    error = VOP_WRITE(vp, &auio, IO_NOWDRAIN, ss->sc_cred);
    VOP_UNLOCK(vp, 0, p); */
    if (error) {
       printf("shd%d: error %d \n",
       ss->sc_unit, error);
       return 1;
    }
    return 0;
}

/* Copies "size" bytes starting at block src_block to block dest_block */
 
long block_copy (struct shd_softc *ss, struct proc *p, long src_block, long dest_block, long num_blocks, int direction)
{
        int error = 0;
        struct uio auio;
        struct iovec aiov;
        struct vnode *vp = NULL;
        caddr_t temp_addr;
        long size;

        size = num_blocks * 512; /* convert number of blocks to number of bytes */
/*        printf ("Copying %ld bytes from %ld to %ld\n", size, src_block, dest_block);*/
        temp_addr = (char *) malloc (size + 1, M_DEVBUF, M_NOWAIT);
        if (SRC_TO_SHADOW == direction) {
            vp = ss->sc_srcdisk.ci_vp;
        }
        else {
            vp =  ss->sc_copydisk.ci_vp;
        }
        bzero(&auio, sizeof(auio));
        aiov.iov_base = temp_addr;
        aiov.iov_len = size;
        auio.uio_iov = &aiov;
        auio.uio_iovcnt = 1;
        auio.uio_offset = (vm_ooffset_t)src_block * ss->sc_secsize;
        auio.uio_segflg = UIO_SYSSPACE;
        auio.uio_rw = UIO_READ;
        auio.uio_resid = size;
        auio.uio_procp = p;

        if (SRC_TO_SHADOW == direction)
        {
            error = (*devsw(ss->sc_srcdisk.ci_dev)->d_read) (ss->sc_srcdisk.ci_dev, &auio, IO_DIRECT);
        }
        else
        {
            error = (*devsw(ss->sc_copydisk.ci_dev)->d_read) (ss->sc_copydisk.ci_dev, &auio, IO_DIRECT);
        }
        /*vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
        error = VOP_READ(vp, &auio, IO_DIRECT, ss->sc_cred);
        VOP_UNLOCK(vp, 0, p); */
        if (error) {
            printf("shd%d: error %d on %s block %d \n",
                    ss->sc_unit, error,
                    "source",
                    src_block);
            free (temp_addr, M_DEVBUF);
            return -1;
         }


         if (SRC_TO_SHADOW == direction) {
             vp = ss->sc_copydisk.ci_vp;
         }
         else {
             vp =  ss->sc_srcdisk.ci_vp;
         }

         bzero(&auio, sizeof(auio));
         aiov.iov_base = temp_addr;
         aiov.iov_len = size;
         auio.uio_iov = &aiov;
         auio.uio_iovcnt = 1;
         auio.uio_offset = (vm_ooffset_t)dest_block * ss->sc_secsize;
         auio.uio_segflg = UIO_SYSSPACE;
         auio.uio_rw = UIO_WRITE;
         auio.uio_resid = size;
         auio.uio_procp = p;

        if (SRC_TO_SHADOW == direction)
        {
         error = (*devsw(ss->sc_copydisk.ci_dev)->d_write) (ss->sc_copydisk.ci_dev, &auio, IO_NOWDRAIN);
         /*printf ("Copied %ld bytes from bn %ld device %x to bn %ld device %x \n", size, src_block, ss->sc_srcdisk.ci_dev, dest_block, ss->sc_copydisk.ci_dev);*/
        }
        else
        {
         error = (*devsw(ss->sc_srcdisk.ci_dev)->d_write) (ss->sc_srcdisk.ci_dev, &auio, IO_NOWDRAIN);
         /*printf ("Copied %ld bytes from bn %ld device %x to bn %ld device %x \n", size, src_block, ss->sc_copydisk.ci_dev, dest_block, ss->sc_srcdisk.ci_dev);*/
        }
/*       vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
         error = VOP_WRITE(vp, &auio, IO_NOWDRAIN, ss->sc_cred);
         VOP_UNLOCK(vp, 0, p); */
         if (error) {
             printf("shd%d: error %d on %s block %d \n",
                               ss->sc_unit, error,
                               "copy",
                               dest_block);
             free (temp_addr, M_DEVBUF);
             return -1;
         }
         free (temp_addr, M_DEVBUF);
         return 0;
}

static void
shdio(struct shd_softc *ss, struct buf *bp, struct proc *p)
{
        int frag1_start, frag2_start;
        long frag1_size, frag2_size;
	long bcount, rcount;
	caddr_t addr;
        caddr_t temp_addr;
	daddr_t bn;
	daddr_t cbn, sbn;
	size_t cbsize, bsize;
	int error = 0;
	struct uio auio;
	struct iovec aiov;
	struct vnode *vp = NULL;
        daddr_t free_block;
/*#ifdef SHDDEBUG
	if (shddebug & SHDB_FOLLOW)
		printf("shd%d: shdio\n", ss->sc_unit);
#endif*/

	/*
	 * Translate the partition-relative block number to an absolute.
	 */

	bn = bp->b_blkno;
	if (shdpart(bp->b_dev) != RAW_PART)
		bn += ss->sc_label.d_partitions[shdpart(bp->b_dev)].p_offset;
	/* Record the transaction start  */

	devstat_start_transaction(&ss->device_stats);

	/*
	 * Loop performing contiguous IOs from source and copy.
	 */
	addr = bp->b_data;
	bp->b_resid = bp->b_bcount;
        if (!(bp->b_flags & B_READ) && bCheckpoint != 0)
        {
            /* Search in Trie and do a copy on write */
            int failed;

            failed = TrieInsertWeak (get_trie_for_version (latest_version), bn, (bp->b_bcount)/512, ss, bp, p); 
            if (failed < 0)
            {
                printf ("No more space on disk! Copy on write failed\n");
                bp->b_error = EACCES;
                bp->b_flags |= B_ERROR;
                bp->b_resid = 0;
                devstat_end_transaction_buf(&ss->device_stats, bp);
                biodone(bp);
                return;
            }

        } /* end of if flags & B_BREAD */

        vp = ss->sc_srcdisk.ci_vp;  
        rcount = bp->b_bcount;
	/*
	 * VNODE I/O
	 *
	 * If an error occurs, we set B_ERROR but we do not set 
	 * B_INVAL because (for a write anyway), the buffer is 
	 * still valid.
	 */
	bzero(&auio, sizeof(auio));
	aiov.iov_base = addr;
	aiov.iov_len = rcount;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = (vm_ooffset_t)bn * ss->sc_secsize;
	auio.uio_segflg = UIO_SYSSPACE;
	if (bp->b_flags & B_READ)
		auio.uio_rw = UIO_READ;
	else
		auio.uio_rw = UIO_WRITE;
	auio.uio_resid = rcount;
	auio.uio_procp = p;

	/*vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);*/
	if (bp->b_flags & B_READ)
        {
    	    /*error = VOP_READ(vp, &auio, IO_DIRECT, ss->sc_cred);*/
            error = (*devsw(ss->sc_srcdisk.ci_dev)->d_read) (ss->sc_srcdisk.ci_dev, &auio, IO_DIRECT);
        }
	else
        {
  	    /*error = VOP_WRITE(vp, &auio, IO_NOWDRAIN, ss->sc_cred); */
            error = (*devsw(ss->sc_srcdisk.ci_dev)->d_write) (ss->sc_srcdisk.ci_dev, &auio, IO_NOWDRAIN);
        }

	/*VOP_UNLOCK(vp, 0, p); */
        if (auio.uio_resid) {
                        rcount -= auio.uio_resid;
                        cbsize -= auio.uio_resid / ss->sc_secsize;
        }
        bn += cbsize;
        addr += rcount;
        bp->b_resid -= rcount;

	if (error) {
		    bp->b_error = error;
		    bp->b_flags |= B_ERROR;
		    if (vp == NULL)
			printf("shd%d: error %d looking up block %d\n",
			       ss->sc_unit, bp->b_error, bn);
		    else
			printf("shd%d: error %d on %s block %d \n",
			       ss->sc_unit, bp->b_error,
			       vp==ss->sc_srcdisk.ci_vp ? "source" : "copy",
			       bn); 
	}
        cbsize = rcount / ss->sc_secsize;

	devstat_end_transaction_buf(&ss->device_stats, bp);
	biodone(bp);
}

int reboot_machine = 0;
void reboot_to_checkpoint (void)
{
     struct shd_softc *ss;
     struct vnode *vp;
     int unit = 0;
     ss = &shd_softc[unit];
     if (reboot_version > latest_version)
     { 
        printf ("Error! Cannot rollback to version %d because you have only %d previous checkpoints so far\n", reboot_version, latest_version);
        return (EINVAL);
     }
     if (reboot_version != 0 && latest_version != 0) 
     {
         printf ("Rolling back disk state from checkpoint version %d to version %d\n\n", latest_version, reboot_version); 
         bCheckpoint = 0;
         /*reboot_machine = 1;*/
         rollback_to_checkpoint (reboot_version, ss, curproc);
         sync(curproc, 0);
         /*save_checkpoint_map (ss, curproc);*/
         printf ("Disk state completely restored\n\n");
     }
}

extern void sync_before_checkpoint (void);

Trie *mod_trie = 0;
TrieIterator mod_pos = 0;
int mod_ok = 0;
 
int get_mod_blocks (int command, long *buf, long bufsiz)
{
    int ix;
    int retsiz = 0;

    switch (command)
    {
        case 1:
            if (0 == mod_trie || 0 == mod_pos) 
            {
                TrieInit (&mod_trie, BlockAlloc, BlockFree, block_copy);
                for (ix = latest_version; ix >= 1; ix--)
                {
                    merge (mod_trie, get_trie_for_version (ix), DEFAULT_OVERLAP);
                }
                mod_ok = TrieIteratorInit(mod_trie, &mod_pos);
            }
            if (mod_ok)
            {
                retsiz = 0;
                for ( ; TrieIteratorIsValid(mod_pos); TrieIteratorAdvance(&mod_pos))
                {
                    buf[retsiz] = mod_pos->key;
                    buf[retsiz+1] = mod_pos->key + depthToSize(mod_pos->maxDepth);
                    retsiz+=2;
                    if (retsiz >= bufsiz)
                        return retsiz/2;
                }
                TrieIteratorCleanup(&mod_pos);
                TrieCleanup(mod_trie);
                mod_pos = 0;
                mod_trie = 0;
                return retsiz/2;
            }
            else
            {
                printf ("Error getting modified blocks. Data structures not initialized\n");
                return -1;
            }
            break;
        case 2:
            if (mod_ok)
            {
                retsiz = 0;
                for ( ; TrieIteratorIsValid(mod_pos); TrieIteratorAdvance(&mod_pos))
                {
                    buf[retsiz] = mod_pos->key;
                    buf[retsiz+1] = mod_pos->key + depthToSize(mod_pos->maxDepth);
                    retsiz+=2;
                    if (retsiz >= bufsiz)
                        return retsiz/2;
                }
                TrieIteratorCleanup(&mod_pos);
                TrieCleanup(mod_trie);
                mod_pos = 0;
                mod_trie = 0;
                return retsiz/2;
            }
            else
            {
                printf ("Error getting modified blocks. Data structures not initialized\n");
                return -1;
            }
            break; 
        case 3:
            if (mod_pos)
            { 
                TrieIteratorCleanup(&mod_pos);
            }
            if (mod_trie)
            {
                TrieCleanup(mod_trie);
            }
            break;
    } 
}

static int
shdioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
        int version = 0;
	int unit = shdunit(dev);
	int error = 0;
	int part, pmask, s;
	struct shd_softc *ss;
	struct shd_ioctl *shio = (struct shd_ioctl *)data;
        struct shd_readbuf *shread; 
	struct shddevice shd;
        struct shd_modinfo *shmod;
        struct shd_ckpt *ckpt;

        struct vnode * vp;        
/*#ifdef SHDDEBUG
	if (shddebug & SHDB_FOLLOW)
		printf("shd%d: shioctl\n", unit);
#endif*/

	if (unit >= numshd)
		return (ENXIO);
	ss = &shd_softc[unit];

	bzero(&shd, sizeof(shd));
	shd.shd_unit = unit;

	switch (cmd) {
        case SHDGETMODIFIEDRANGES:
                shmod = (struct shd_modinfo *) data;
                shmod->retsiz = get_mod_blocks (shmod->command, shmod->buf, shmod->bufsiz);
                break;
        case SHDREADBLOCK:
                shread = (struct shd_readbuf *) data;
                read_block (ss, p, shread->buf, shread->block_num);
                break;
        case SHDSETREBOOTVERSION:
                reboot_version = shio->version;
                break;
        case SHDROLLBACK:
                if (shio->version > latest_version)
                {
                    printf ("Error! Cannot rollback to version %d because you have only %d previous checkpoints so far\n", (int) shio->version, latest_version);
                    return (EINVAL);                  
                }
                rollback_to_checkpoint ((int )shio->version, ss, p);
                break;
        case SHDSAVECHECKPOINTMAP:
                save_checkpoint_map (ss, p);
                break;
        case SHDLOADCHECKPOINTMAP:
                load_checkpoint_map (ss, p);
                break;
        case SHDENABLECHECKPOINTING:
                bCheckpoint = 1;
                break;
        case SHDDISABLECHECKPOINTING:
                bCheckpoint = 0;
                break;
        case SHDCHECKPOINT:
                ckpt = (struct shd_ckpt *) data; 
                printf ("[SHDCHECKPOINT] Received checkpoint command!\n");
                sync_before_checkpoint (); /*Uncomment this*/
                if (MAX_CHECKPOINTS <= latest_version)
                {
                    printf ("Error! Cannot take more checkpoints because you have reached the limit of %d checkpoints\n", latest_version);
                    return (EINVAL);
                }            
                if (0 != create_new_trie (latest_version + 1))
                    latest_version++;
                else
                {
                    return (EINVAL);
                } 
                printf ("Setting checkpoint name %s time %s\n", ckpt->name, ckpt->time);
                /*set_checkpoint (latest_version, ckpt->name, ckpt->time);*/
                break;
        case SHDGETCHECKPOINTS:
                if (latest_version < 1)
                    return;
                for (version = 1; version <= latest_version; ++version)
                {
                    printf ("Checkpoint version %d is ===>\n", version);
                    /*print_checkpoint_time (version);*/
                    print_checkpoint_map (version);
                    printf ("\n");
                }
                break;
        case SHDDELETECHECKPOINTS:
                if (shio->delete_start < 1 || shio->delete_end > latest_version)
                {
                    printf ("Please enter valid checkpoint numbers only! The valid range is 1 to %d\n", latest_version); 
                    return (EINVAL);
                } 
                delete_checkpoints (shio->delete_start, shio->delete_end); 
                break;
	case SHDIOCSET:
                bcopy(&shddevs[unit], &shd, sizeof(shd));
                latest_version = 1;
                bCheckpoint = 0;
                init_trie_list ();
                if (0 == create_new_trie (1))
                    return (EINVAL);
 
		if ((flag & FWRITE) == 0)
			return (EBADF);

		if ((error = shdlock(ss)) != 0)
			return (error);

		if (shd.shd_srcsize < 0 || shd.shd_copysize < 0) {
			shdunlock(ss);
			return (EINVAL);
		}

		shd.shd_srcpath = shio->shio_srcdisk;
		shd.shd_srcsize = shio->shio_srcsize;

		shd.shd_copypath = shio->shio_copydisk;
		shd.shd_copysize = shio->shio_copysize;

                InitBlockAllocator (EXPLICIT_CKPT_DELETE, 3, shadow_size);
		/*
		 * The shd has been successfully initialized, so
		 * we can place it into the array and read2 the disklabel.
		 */
		bcopy(&shd, &shddevs[unit], sizeof(shd));
		shio->shio_unit = unit;
		shio->shio_size = ss->sc_size;
		shdgetdisklabel(dev);
		shdunlock(ss);

		break;

	case SHDIOCCLR:
		if ((ss->sc_flags & SHDF_INITED) == 0)
			return (ENXIO);

		if ((flag & FWRITE) == 0)
			return (EBADF);

		if ((error = shdlock(ss)) != 0)
			return (error);

                latest_version = 1;
                bCheckpoint = 0;

		/* Don't unconfigure if any other partitions are open */
		part = shdpart(dev);
		pmask = (1 << part);
		if ((ss->sc_openmask & ~pmask)) {
			shdunlock(ss);
			return (EBUSY);
		}

		/*
		 * Free shd_softc information and clear entry.
		 */

		/* Close the components and free their pathnames. */
#ifdef SHDDEBUG
		if (shddebug & SHDB_VNODE) {
			vprint("SHDIOCCLR: src vnode info",
			       ss->sc_srcdisk.ci_vp);
			vprint("SHDIOCCLR: copy vnode info",
			       ss->sc_copydisk.ci_vp);
		}
#endif
		shd_deinit(&shd, p);

		/*
		 * Free shddevice information and clear entry.
		 */
		shd.shd_dk = -1;
		bcopy(&shd, &shddevs[unit], sizeof(shd));

		/*
		 * And remove the devstat entry.
		 */
		devstat_remove_entry(&ss->device_stats);

		/* This must be atomic. */
		s = splhigh();
		shdunlock(ss);
		bzero(ss, sizeof(struct shd_softc));
		splx(s);

		break;

	case DIOCGDINFO:
		if ((ss->sc_flags & SHDF_INITED) == 0)
			return (ENXIO);

		*(struct disklabel *)data = ss->sc_label;
		break;

	case DIOCGPART:
		if ((ss->sc_flags & SHDF_INITED) == 0)
			return (ENXIO);

		((struct partinfo *)data)->disklab = &ss->sc_label;
		((struct partinfo *)data)->part =
			&ss->sc_label.d_partitions[shdpart(dev)];
		break;

	case DIOCWDINFO:
	case DIOCSDINFO:
		if ((ss->sc_flags & SHDF_INITED) == 0)
			return (ENXIO);

		if ((flag & FWRITE) == 0)
			return (EBADF);

		if ((error = shdlock(ss)) != 0)
			return (error);

		ss->sc_flags |= SHDF_LABELLING;

		error = setdisklabel(&ss->sc_label,
				     (struct disklabel *)data, 0);
		if (error == 0) {
			if (cmd == DIOCWDINFO)
				error = writedisklabel(SHDLABELDEV(dev),
						       &ss->sc_label);
		}

		ss->sc_flags &= ~SHDF_LABELLING;

		shdunlock(ss);

		if (error)
			return (error);
		break;

	case DIOCWLABEL:
		if ((ss->sc_flags & SHDF_INITED) == 0)
			return (ENXIO);

		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (*(int *)data != 0)
			ss->sc_flags |= SHDF_WLABEL;
		else
			ss->sc_flags &= ~SHDF_WLABEL;
		break;

	default:
		return (ENOTTY);
	}

	return (0);
}

static int
shdsize(dev)
	dev_t dev;
{
	struct shd_softc *ss;
	int part, size;

	if (shdopen(dev, 0, S_IFCHR, curproc))
		return (-1);

	ss = &shd_softc[shdunit(dev)];
	part = shdpart(dev);

	if ((ss->sc_flags & SHDF_INITED) == 0)
		return (-1);

	if (ss->sc_label.d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = ss->sc_label.d_partitions[part].p_size;

	if (shdclose(dev, 0, S_IFCHR, curproc))
		return (-1);

	return (size);
}

static int
shddump(dev)
	dev_t dev;
{

	/* Not implemented. */
	return ENXIO;
}

/*
 * Lookup the provided name in the filesystem.  If the file exists,
 * is a valid block device, and isn't being used by anyone else,
 * set *vpp to the file's vnode.
 */
static int
shd_lookup(path, p, mode, vpp)
	char *path;
	struct proc *p;
	int mode;
	struct vnode **vpp;	/* result */
{
	struct nameidata nd;
	struct vnode *vp;
	int error;

        printf ("Before NDINIT\n");
	NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, path, p);
        printf ("vn_open on %s\n", path);
	if ((error = vn_open(&nd, mode, 0)) != 0) {
#ifdef SHDDEBUG
		if (shddebug & SHDB_INIT)
			printf("shd_lookup: vn_open error = %d\n", error);
#endif
		return (error);
	}
        printf ("After vn_open\n");  
	vp = nd.ni_vp;

	/*
	 * There should be no writers to the source disk and
	 * only us using the copy disk.
	 */
	if (((mode & FWRITE) && vp->v_usecount > 1) ||
	    (!(mode & FWRITE) && vp->v_writecount > 0)) {
		error = EBUSY;
		goto bad;
	}

#ifdef SHDDEBUG
	if (shddebug & SHDB_VNODE)
		vprint("shd_lookup: vnode info", vp);
#endif

	VOP_UNLOCK(vp, 0, p);
	NDFREE(&nd, NDF_ONLY_PNBUF);
	*vpp = vp;
	return (0);
bad:
	VOP_UNLOCK(vp, 0, p);
	NDFREE(&nd, NDF_ONLY_PNBUF);
	/* vn_close does vrele() for vp */
	(void)vn_close(vp, mode, p->p_ucred, p);
	return (error);
}

/*
 * Read the disklabel from the shd.  If one is not present, fake one up.
 */
static void
shdgetdisklabel(dev)
	dev_t dev;
{
	int unit = shdunit(dev);
	struct shd_softc *ss = &shd_softc[unit];
	char *errstring;
	struct disklabel *lp = &ss->sc_label;

	bzero(lp, sizeof(*lp));

	/*
	 * Create pseudo-geometry based on 1MB cylinders.
	 */
	lp->d_secperunit = ss->sc_size;
	lp->d_secsize = ss->sc_secsize;
	lp->d_nsectors = 1024 * 1024 / ss->sc_secsize;
	lp->d_ntracks = 1;
	lp->d_ncylinders = ss->sc_size / ss->sc_secsize;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	strncpy(lp->d_typename, "shd", sizeof(lp->d_typename));
	lp->d_type = DTYPE_CCD;
	strncpy(lp->d_packname, "fictitious", sizeof(lp->d_packname));
	lp->d_rpm = 3600;
	lp->d_interleave = 1;
	lp->d_flags = 0;

	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size = ss->sc_size;
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_bbsize = BBSIZE;				/* XXX */
	lp->d_sbsize = SBSIZE;				/* XXX */

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(&ss->sc_label);

	/*
	 * Call the generic disklabel extraction routine.
	 */
	errstring = readdisklabel(SHDLABELDEV(dev), &ss->sc_label);
	if (errstring != NULL)
		shdmakedisklabel(ss);

#ifdef SHDDEBUG
	/* It's actually extremely common to have unlabeled shds. */
	if (shddebug & SHDB_LABEL)
		if (errstring != NULL)
			printf("shd%d: %s\n", unit, errstring);
#endif
}

/*
 * Take care of things one might want to take care of in the event
 * that a disklabel isn't present.
 */
static void
shdmakedisklabel(ss)
	struct shd_softc *ss;
{
	struct disklabel *lp = &ss->sc_label;

	/*
	 * For historical reasons, if there's no disklabel present
	 * the raw partition must be marked FS_BSDFFS.
	 */
	lp->d_partitions[RAW_PART].p_fstype = FS_BSDFFS;

	strncpy(lp->d_packname, "default label", sizeof(lp->d_packname));
}

/*
 * Wait interruptibly for an exclusive lock.
 *
 * XXX
 * Several drivers do this; it should be abstracted and made MP-safe.
 */
static int
shdlock(ss)
	struct shd_softc *ss;
{
	int error;

	while ((ss->sc_flags & SHDF_LOCKED) != 0) {
		ss->sc_flags |= SHDF_WANTED;
		if ((error = tsleep(ss, PRIBIO | PCATCH, "shdlck", 0)) != 0)
			return (error);
	}
	ss->sc_flags |= SHDF_LOCKED;
	return (0);
}

/*
 * Unlock and wake up any waiters.
 */
static void
shdunlock(ss)
	struct shd_softc *ss;
{

	ss->sc_flags &= ~SHDF_LOCKED;
	if ((ss->sc_flags & SHDF_WANTED) != 0) {
		ss->sc_flags &= ~SHDF_WANTED;
		wakeup(ss);
	}
}

/*
 * CRC32 code.
 * XXX could just fix the libkern code
 */
typedef struct crc32_ctx {
	u_int32_t crc;
} CRC32_CTX;

static void
CRC32Init(CRC32_CTX *ctx)
{
	ctx->crc = ~0U;
}

static void
CRC32Update(CRC32_CTX *ctx, void *buf, int len)
{
	const u_int8_t *p = buf;
	u_int32_t crc = ctx->crc;

	while (len--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	ctx->crc = crc;
}

static void
CRC32Final(u_int32_t *crcp, CRC32_CTX *ctx)
{
	*crcp = ctx->crc ^ ~0U;
	ctx->crc = 0;
}

#include <sys/md5.h>

#define VALIDATEBSIZE	(64*1024)

/*
 * Initialize or validate the ID for the shadowed object.
 */
int
shd_validate(struct shd_softc *ss, struct shd_id *id, int initialize,
	     struct proc *p)
{
	struct vnode *vp;
	void *buf;
	struct uio auio;
	struct iovec aiov;
	off_t bleft, voff;
	size_t isize;
	int error;
	union {
		CRC32_CTX crc32ctx;
		MD5_CTX md5ctx;
	} context;
	union {
		u_int32_t crc32;
		u_char digest[16];
	} result;

#ifdef SHDDEBUG
	if (shddebug & SHDB_FOLLOW)
		printf("shd%d: validate: type=%d, init=%d\n",
		       ss->sc_unit, id->type, initialize);
#endif

	switch (id->type) {
	case SHDID_NONE:
		if (!initialize)
			printf("shd%d: WARNING no ID info\n", ss->sc_unit);
		return 0;

	case SHDID_STRING:
		if (ss->sc_srcdisk.ci_pathlen > sizeof(id->data.string))
			return EINVAL;

		if (initialize) {
			memcpy(id->data.string, ss->sc_srcdisk.ci_path,
			       ss->sc_srcdisk.ci_pathlen);
			return 0;
		}
		if (!memcmp(ss->sc_srcdisk.ci_path, id->data.string,
			    ss->sc_srcdisk.ci_pathlen))
			return 0;
		return EPERM;

	case SHDID_CRC32:
		CRC32Init(&context.crc32ctx);
		break;

	case SHDID_MD5:
		MD5Init(&context.md5ctx);
		break;

	default:
		printf("shd%d: unsupported ID info type %d\n",
		       ss->sc_unit, id->type);
		return EINVAL;
	}

	buf = malloc(VALIDATEBSIZE, M_DEVBUF, M_WAITOK);

	vp = ss->sc_srcdisk.ci_vp;
	voff = 0;
	bleft = (off_t)ss->sc_srcdisk.ci_size * ss->sc_srcdisk.ci_secsize;
	while (bleft > 0) {
		isize = (bleft > VALIDATEBSIZE) ? VALIDATEBSIZE : bleft;
			
		bzero(&auio, sizeof(auio));
		aiov.iov_base = buf;
		aiov.iov_len = isize;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = voff;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_rw = UIO_READ;
		auio.uio_resid = isize;
		auio.uio_procp = p;

		/*vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		error = VOP_READ(vp, &auio, IO_DIRECT, ss->sc_cred);
		VOP_UNLOCK(vp, 0, p); */
                error = (*devsw(ss->sc_srcdisk.ci_dev)->d_read) (ss->sc_srcdisk.ci_dev, &auio, IO_DIRECT);

		if (error)
			break;
		if (auio.uio_resid > 0) {
			error = EIO; /* XXX */
			break;
		}

		switch (id->type) {
		case SHDID_CRC32:
			CRC32Update(&context.crc32ctx, buf, isize);
			break;

		case SHDID_MD5:
			MD5Update(&context.md5ctx, buf, isize);
			break;
		}

		bleft -= isize;
		voff += isize;
	}

	error = 0;
	switch (id->type) {
	case SHDID_CRC32:
		CRC32Final(&result.crc32, &context.crc32ctx);
		if (initialize)
			id->data.crc = result.crc32;
		else if (id->data.crc != result.crc32)
			error = EPERM;
		break;

	case SHDID_MD5:
		MD5Final(result.digest, &context.md5ctx);
		if (initialize)
			memcpy(id->data.digest, result.digest, 16);
		else if (memcmp(id->data.digest, result.digest, 16))
			error = EPERM;
		break;
	}

	free(buf, M_DEVBUF);
	return error;
}
