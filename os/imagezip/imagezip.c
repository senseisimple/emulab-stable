/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * An image zipper.
 *
 * TODO:
 *	Multithread so that we can be reading ahead on the input device
 *	and overlapping IO with compression.  Maybe a third thread for
 *	doing output.
 *
 *	Split out the FS-specific code into subdirectories.  Clean it up.
 */
#include <err.h>
#include <fcntl.h>
#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <zlib.h>
#define DKTYPENAMES
#ifndef linux
#include <sys/disklabel.h>
#include <ufs/ffs/fs.h>
#endif
#include "ext2_fs.h"
#include "imagehdr.h"
#ifdef WITH_NTFS
#include <linux/fs.h>
#include "volume.h"
#include "inode.h"
#include "support.h" 
#include "attrib.h"
#include "runlist.h"
#include "dir.h"
#endif

#define min(a,b) ((a) <= (b) ? (a) : (b))

/* Why is this not defined in a public header file? */
#define ACTIVE		0x80
#define BOOT_MAGIC	0xAA55
#define DOSPTYP_UNUSED	0	/* Unused */
#ifndef DOSPTYP_LINSWP
#define	DOSPTYP_LINSWP	0x82	/* Linux swap partition */
#define	DOSPTYP_LINUX	0x83	/* Linux partition */
#define	DOSPTYP_EXT	5	/* DOS extended partition */
#endif
#ifndef DOSPTYPE_NTFS
#define DOSPTYP_NTFS    7       /* Windows NTFS partition */
#endif
#ifndef DOSPTYP_OPENBSD
#define DOSPTYP_OPENBSD 0xa6
#endif

char	*infilename;
int	infd, outfd, outcanseek;
int	secsize	  = 512;	/* XXX bytes. */
int	debug	  = 0;
int	dots	  = 0;
int     info      = 0;
int     version   = 0;
int     slicemode = 0;
int     maxmode   = 0;
int     slice     = 0;
int	level	  = 4;
long	dev_bsize = 1;
int	ignore[NDOSPART], forceraw[NDOSPART];
int	oldstyle  = 0;
int	frangesize= 64;	/* 32k */
int	safewrites= 1;
off_t	datawritten;

#define sectobytes(s)	((off_t)(s) * secsize)
#define bytestosec(b)	(uint32_t)((b) / secsize)

#define HDRUSED(reg, rel) \
    (sizeof(blockhdr_t) + \
    (reg) * sizeof(struct region) + (rel) * sizeof(struct blockreloc))

/*
 * We want to be able to compress slices by themselves, so we need
 * to know where the slice starts when reading the input file for
 * compression. 
 *
 * These numbers are in sectors.
 */
long	inputminsec	= 0;
long    inputmaxsec	= 0;	/* 0 means the entire input image */

/*
 * A list of data ranges. 
 */
struct range {
	uint32_t	start;		/* In sectors */
	uint32_t	size;		/* In sectors */
	void		*data;
	struct range	*next;
};
struct range	*ranges, *skips, *fixups;
int		numranges, numskips;
struct blockreloc	*relocs;
int			numregions, numrelocs;

void	addskip(uint32_t start, uint32_t size);
void	dumpskips(void);
void	sortrange(struct range *head, int domerge);
void    makeranges(void);
void	addfixup(off_t offset, off_t poffset, off_t size, void *data,
		 int reloctype);
void	addreloc(off_t offset, off_t size, int reloctype);

/* Forward decls */
#ifndef linux
int	read_image(void);
int	read_bsdslice(int slice, u_int32_t start, u_int32_t size, int type);
int	read_bsdpartition(struct disklabel *dlabel, int part);
int	read_bsdcg(struct fs *fsp, struct cg *cgp, unsigned int dboff);
#ifdef WITH_NTFS
int     read_ntfsslice(int slice, u_int32_t start, u_int32_t size,
		       char *openname);
#endif
#endif
int	read_linuxslice(int slice, u_int32_t start, u_int32_t size);
int	read_linuxswap(int slice, u_int32_t start, u_int32_t size);
int	read_linuxgroup(struct ext2_super_block *super,
			struct ext2_group_desc	*group,
			int index, u_int32_t diskoffset);
int	read_raw(void);
int	compress_image(void);
void	usage(void);

#ifdef linux
#define devlseek	lseek
#define devread		read
#else
static inline off_t devlseek(int fd, off_t off, int whence)
{
	off_t noff;
	assert((off & (DEV_BSIZE-1)) == 0);
	noff = lseek(fd, off, whence);
	assert(noff == (off_t)-1 || (noff & (DEV_BSIZE-1)) == 0);
	return noff;
}

static inline int devread(int fd, void *buf, size_t size)
{
	assert((size & (DEV_BSIZE-1)) == 0);
	return read(fd, buf, size);
}
#endif

/*
 * Wrap up write in a retry mechanism to protect against transient NFS
 * errors causing a fatal error. 
 */
ssize_t
mywrite(int fd, const void *buf, size_t nbytes)
{
	int		cc, i, count = 0;
	off_t		startoffset = 0;
	int		maxretries = 10;

	if (outcanseek &&
	    ((startoffset = lseek(fd, (off_t) 0, SEEK_CUR)) < 0)) {
		perror("mywrite: seeking to get output file ptr");
		exit(1);
	}

	for (i = 0; i < maxretries; i++) {
		while (nbytes) {
			cc = write(fd, buf, nbytes);

			if (cc > 0) {
				nbytes -= cc;
				buf    += cc;
				count  += cc;
				continue;
			}
			if (!safewrites) {
				if (cc < 0)
					perror("mywrite: writing");
				else 
					fprintf(stderr, "mywrite: writing\n");
				exit(1);
			}

			if (i == 0) 
				perror("write error: will retry");
	
			sleep(1);
			nbytes += count;
			buf    -= count;
			count   = 0;
			goto again;
		}
		if (safewrites && fsync(fd) < 0) {
			perror("fsync error: will retry");
			sleep(1);
			nbytes += count;
			buf    -= count;
			count   = 0;
			goto again;
		}
		datawritten += count;
		return count;
	again:
		if (lseek(fd, startoffset, SEEK_SET) < 0) {
			perror("mywrite: seeking to set file ptr");
			exit(1);
		}
	}
	perror("write error: busted for too long");
	fflush(stderr);
	exit(1);
}

/* Map partition number to letter */
#define BSDPARTNAME(i)       ("ABCDEFGHIJKLMNOP"[(i)])

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int	ch, rval;
	char	*outfilename = 0;
	int	linuxfs	  = 0;
	int	bsdfs     = 0;
	int	rawmode	  = 0;
	extern char build_info[];

	while ((ch = getopt(argc, argv, "vlbdihrs:c:z:oI:1F:DR:")) != -1)
		switch(ch) {
		case 'v':
			version++;
			break;
		case 'i':
			info++;
			break;
		case 'D':
			safewrites = 0;
			break;
		case 'd':
			debug++;
			break;
		case 'l':
			linuxfs++;
			break;
		case 'b':
			bsdfs++;
			break;
		case 'o':
			dots++;
			break;
		case 'r':
			rawmode++;
			break;
		case 's':
			slicemode = 1;
			slice = atoi(optarg);
			break;
		case 'z':
			level = atoi(optarg);
			if (level < 0 || level > 9)
				usage();
			break;
		case 'c':
			maxmode     = 1;
			inputmaxsec = atoi(optarg);
			break;
		case 'I':
			rval = atoi(optarg);
			if (rval < 1 || rval > 4)
				usage();
			ignore[rval-1] = 1;
			break;
		case 'R':
			rval = atoi(optarg);
			if (rval < 1 || rval > 4)
				usage();
			forceraw[rval-1] = 1;
			break;
		case '1':
			oldstyle = 1;
			break;
		case 'F':
			frangesize = atoi(optarg);
			if (frangesize < 0)
				usage();
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (version || info || debug) {
		fprintf(stderr, "%s\n", build_info);
		if (version)
			exit(0);
	}

	if (argc < 1 || argc > 2)
		usage();

	if (slicemode && (slice < 1 || slice > 4)) {
		fprintf(stderr, "Slice must be a DOS partition (1-4)\n\n");
		usage();
	}
	if (maxmode && slicemode) {
		fprintf(stderr, "Count option (-c) cannot be used with "
			"the slice (-s) option\n\n");
		usage();
	}
	if (linuxfs && bsdfs) {
		fprintf(stderr, "Cannot specify linux and bsd together!\n\n");
		usage();
	}
	if (!info && argc != 2) {
		fprintf(stderr, "Must specify an output filename!\n\n");
		usage();
	}
	else
		outfilename = argv[1];

	if (info && !debug)
		debug++;

	infilename = argv[0];
	if ((infd = open(infilename, O_RDONLY, 0)) < 0) {
		perror(infilename);
		exit(1);
	}

	if (linuxfs)
		rval = read_linuxslice(0, 0, 0);
#ifndef linux
	else if (bsdfs)
		rval = read_bsdslice(0, 0, 0, DOSPTYP_386BSD);
	else if (rawmode)
		rval = read_raw();
	else
		rval = read_image();
#endif
	sortrange(skips, 1);
	if (debug)
		dumpskips();
	makeranges();
	sortrange(fixups, 0);
	fflush(stderr);

	if (info) {
		close(infd);
		exit(0);
	}

	if (strcmp(outfilename, "-")) {
		if ((outfd = open(outfilename, O_RDWR|O_CREAT|O_TRUNC, 0666))
		    < 0) {
			perror("opening output file");
			exit(1);
		}
		outcanseek = 1;
	}
	else {
		outfd = fileno(stdout);
		outcanseek = 0;
		safewrites = 0;
	}
	compress_image();
	
	fflush(stderr);
	close(infd);
	if (outcanseek)
		close(outfd);
	exit(0);
}

#ifndef linux
/*
 * Parse the DOS partition table and dispatch to the individual readers.
 */
int
read_image()
{
	int		i, cc, rval = 0;
	struct doslabel {
		char		align[sizeof(short)];	/* Force alignment */
		char		pad2[DOSPARTOFF];
		struct dos_partition parts[NDOSPART];
		unsigned short  magic;
	} doslabel;
#define DOSPARTSIZE \
	(DOSPARTOFF + sizeof(doslabel.parts) + sizeof(doslabel.magic))

	if ((cc = devread(infd, doslabel.pad2, DOSPARTSIZE)) < 0) {
		warn("Could not read DOS label");
		return 1;
	}
	if (cc != DOSPARTSIZE) {
		warnx("Could not get the entire DOS label");
 		return 1;
	}
	if (doslabel.magic != BOOT_MAGIC) {
		warnx("Wrong magic number in DOS partition table");
 		return 1;
	}

	if (debug) {
		fprintf(stderr, "DOS Partitions:\n");
		for (i = 0; i < NDOSPART; i++) {
			fprintf(stderr, "  P%d: ", i + 1 /* DOS Numbering */);
			switch (doslabel.parts[i].dp_typ) {
			case DOSPTYP_UNUSED:
				fprintf(stderr, "  UNUSED");
				break;
			case DOSPTYP_386BSD:
				fprintf(stderr, "  FBSD  ");
				break;
			case DOSPTYP_OPENBSD:
				fprintf(stderr, "  OBSD  ");
				break;
			case DOSPTYP_LINSWP:
				fprintf(stderr, "  LSWAP ");
				break;
			case DOSPTYP_LINUX:
				fprintf(stderr, "  LINUX ");
				break;
			case DOSPTYP_EXT:
				fprintf(stderr, "  DOS   ");
				break;
#ifdef WITH_NTFS
			case DOSPTYP_NTFS:
				fprintf(stderr, "  NTFS  ");
				break;
#endif
			default:
				fprintf(stderr, "  UNKNO ");
			}
			fprintf(stderr, "  start %9d, size %9d\n", 
			       doslabel.parts[i].dp_start,
			       doslabel.parts[i].dp_size);
		}
		fprintf(stderr, "\n");
	}

	/*
	 * In slicemode, we need to set the bounds of compression.
	 * Slice is a DOS partition number (1-4). If not in slicemode,
	 * we cannot set the bounds according to the doslabel since its
	 * possible that someone will create a disk with empty space
	 * before the first partition (typical, to start partition 1
	 * at the second cylinder) or after the last partition (Mike!).
	 * However, do not set the inputminsec since we usually want the
	 * stuff before the first partition, which is the boot stuff.
	 */
	if (slicemode) {
		inputminsec = doslabel.parts[slice-1].dp_start;
		inputmaxsec = doslabel.parts[slice-1].dp_start +
			      doslabel.parts[slice-1].dp_size;
	}

	/*
	 * Now operate on individual slices. 
	 */
	for (i = 0; i < NDOSPART; i++) {
		unsigned char	type  = doslabel.parts[i].dp_typ;
		u_int32_t	start = doslabel.parts[i].dp_start;
		u_int32_t	size  = doslabel.parts[i].dp_size;

		if (slicemode && i != (slice - 1) /* DOS Numbering */)
			continue;
		
		if (ignore[i])
			type = DOSPTYP_UNUSED;
		else if (forceraw[i]) {
			fprintf(stderr,
				"  Slice %d, Forcing raw compression\n", i+1);
			goto skipcheck;
		}

		switch (type) {
		case DOSPTYP_386BSD:
		case DOSPTYP_OPENBSD:
			rval = read_bsdslice(i, start, size, type);
			break;
		case DOSPTYP_LINSWP:
			rval = read_linuxswap(i, start, size);
			break;
		case DOSPTYP_LINUX:
			rval = read_linuxslice(i, start, size);
			break;
#ifdef WITH_NTFS
		case DOSPTYP_NTFS:
			rval = read_ntfsslice(i, start, size, infilename);
			break;
#endif
		case DOSPTYP_UNUSED:
			fprintf(stderr,
				"  Slice %d %s, NOT SAVING.\n", i + 1,
				ignore[i] ? "ignored" : "is unused");
			if (size > 0)
				addskip(start, size);
			break;
		default:
			fprintf(stderr,
				"  Slice %d is an unknown type (%x), "
				"performing raw compression.\n",
				i + 1 /* DOS Numbering */, type);
			break;
		}
		if (rval) {
			warnx("Stopping zip at Slice %d",
			      i + 1 /* DOS Number */);
			return rval;
		}
		
	skipcheck:
		if (!slicemode && !maxmode) {
			if (start + size > inputmaxsec)
				inputmaxsec = start + size;
		}
	}

	return rval;
}

/*
 * Operate on a BSD slice
 */
int
read_bsdslice(int slice, u_int32_t start, u_int32_t size, int bsdtype)
{
	int		cc, i, rval = 0, npart;
	union {
		struct disklabel	label;
		char			pad[BBSIZE];
	} dlabel;

	if (debug)
		fprintf(stderr, "  P%d (%sBSD Slice)\n", slice + 1,
			bsdtype == DOSPTYP_386BSD ? "Free" : "Open");
	
	if (devlseek(infd, sectobytes(start), SEEK_SET) < 0) {
		warn("Could not seek to beginning of BSD slice");
		return 1;
	}

	/*
	 * Then seek ahead to the disklabel.
	 */
	if (devlseek(infd, sectobytes(LABELSECTOR), SEEK_CUR) < 0) {
		warn("Could not seek to beginning of BSD disklabel");
		return 1;
	}

	if ((cc = devread(infd, &dlabel, sizeof(dlabel))) < 0) {
		warn("Could not read BSD disklabel");
		return 1;
	}
	if (cc != sizeof(dlabel)) {
		warnx("Could not get the entire BSD disklabel");
 		return 1;
	}

	/*
	 * Check the magic numbers.
	 */
	if (dlabel.label.d_magic  != DISKMAGIC ||
	    dlabel.label.d_magic2 != DISKMAGIC) {
#if 0 /* not needed, a fake disklabel is created by the kernel */
		/*
		 * If we were forced with the bsdfs option,
		 * assume this is a single partition disk like a
		 * memory or vnode disk.  We cons up a disklabel
		 * and let it rip.
		 */
		if (size == 0) {
			fprintf(stderr,
				"No disklabel, assuming single partition\n");
			dlabel.label.d_partitions[0].p_offset = 0;
			dlabel.label.d_partitions[0].p_size = 0;
			dlabel.label.d_partitions[0].p_fstype = FS_BSDFFS;
			return read_bsdpartition(&dlabel.label, 0);
		}
#endif
		warnx("Wrong magic number is BSD disklabel");
 		return 1;
	}

	/*
	 * Now scan partitions.
	 *
	 * XXX space not covered by a partition winds up being compressed,
	 * we could detect this.
	 */
	npart = dlabel.label.d_npartitions;
	assert(npart >= 0 && npart <= 16);
	if (debug)
		fprintf(stderr, "  P1: %d partitions\n", npart);
	for (i = 0; i < npart; i++) {
		if (! dlabel.label.d_partitions[i].p_size)
			continue;

		if (dlabel.label.d_partitions[i].p_fstype == FS_UNUSED)
			continue;

		/*
		 * OpenBSD maps the extended DOS partitions as slices 8-15,
		 * skip them.
		 */
		if (bsdtype == DOSPTYP_OPENBSD && i >= 8 && i < 16) {
			if (debug)
				fprintf(stderr, "    %c   skipping, "
					"OpenBSD mapping of DOS partition %d\n",
					BSDPARTNAME(i), i - 6);
			continue;
		}

		if (debug) {
			fprintf(stderr, "    %c ", BSDPARTNAME(i));

			fprintf(stderr, "  start %9d, size %9d\t(%s)\n",
			   dlabel.label.d_partitions[i].p_offset,
			   dlabel.label.d_partitions[i].p_size,
			   fstypenames[dlabel.label.d_partitions[i].p_fstype]);
		}

		rval = read_bsdpartition(&dlabel.label, i);
		if (rval)
			return rval;
	}
	
	/*
	 * Record a fixup for the partition table, adjusting the
	 * partition offsets to make them slice relative.
	 */
	if (slicemode &&
	    start != 0 && dlabel.label.d_partitions[0].p_offset == start) {
		for (i = 0; i < npart; i++) {
			if (dlabel.label.d_partitions[i].p_size == 0)
				continue;

			/*
			 * Don't mess with OpenBSD partitions 8-15 which map
			 * extended DOS partitions.  Also leave raw partition
			 * alone as it maps the entire disk (not just slice)
			 */
			if (bsdtype == DOSPTYP_OPENBSD &&
			    (i == 2 || (i >= 8 && i < 16)))
				continue;

			assert(dlabel.label.d_partitions[i].p_offset >= start);
			dlabel.label.d_partitions[i].p_offset -= start;
		}
		dlabel.label.d_checksum = 0;
		dlabel.label.d_checksum = dkcksum(&dlabel.label);
		addfixup(sectobytes(start+LABELSECTOR), sectobytes(start),
			 (off_t)sizeof(dlabel.label), &dlabel,
			 bsdtype == DOSPTYP_OPENBSD ?
			 RELOC_OBSDDISKLABEL : RELOC_FBSDDISKLABEL);
	}

	return 0;
}

/*
 * BSD partition table offsets are relative to the start of the raw disk.
 * Very convenient.
 */
int
read_bsdpartition(struct disklabel *dlabel, int part)
{
	int		i, cc, rval = 0;
	union {
		struct fs fs;
		char pad[MAXBSIZE];
	} fs;
	union {
		struct cg cg;
		char pad[MAXBSIZE];
	} cg;
	u_int32_t	size, offset;

	offset = dlabel->d_partitions[part].p_offset;
	size   = dlabel->d_partitions[part].p_size;
	
	if (dlabel->d_partitions[part].p_fstype == FS_SWAP) {
		addskip(offset, size);
		return 0;
	}

	if (dlabel->d_partitions[part].p_fstype != FS_BSDFFS) {
		warnx("BSD Partition %c: Not a BSD Filesystem",
		      BSDPARTNAME(part));
		return 1;
	}

	if (devlseek(infd, sectobytes(offset) + SBOFF, SEEK_SET) < 0) {
		warnx("BSD Partition %c: Could not seek to superblock",
		      BSDPARTNAME(part));
		return 1;
	}

	if ((cc = devread(infd, &fs, SBSIZE)) < 0) {
		warn("BSD Partition %c: Could not read superblock",
		     BSDPARTNAME(part));
		return 1;
	}
	if (cc != SBSIZE) {
		warnx("BSD Partition %c: Truncated superblock",
		      BSDPARTNAME(part));
		return 1;
	}
 	if (fs.fs.fs_magic != FS_MAGIC) {
		warnx("BSD Partition %c: Bad magic number in superblock",
		      BSDPARTNAME(part));
 		return 1;
 	}

	if (debug) {
		fprintf(stderr, "        bfree %9d, size %9d\n",
		       fs.fs.fs_cstotal.cs_nbfree, fs.fs.fs_bsize);
	}

	for (i = 0; i < fs.fs.fs_ncg; i++) {
		unsigned long	cgoff, dboff;

		cgoff = fsbtodb(&fs.fs, cgtod(&fs.fs, i)) + offset;
		dboff = fsbtodb(&fs.fs, cgbase(&fs.fs, i)) + offset;

		if (devlseek(infd, sectobytes(cgoff), SEEK_SET) < 0) {
			warn("BSD Partition %c: Could not seek to cg %d",
			     BSDPARTNAME(part), i);
			return 1;
		}
		if ((cc = devread(infd, &cg, fs.fs.fs_bsize)) < 0) {
			warn("BSD Partition %c: Could not read cg %d",
			     BSDPARTNAME(part), i);
			return 1;
		}
		if (cc != fs.fs.fs_bsize) {
			warn("BSD Partition %c: Truncated cg %d",
			     BSDPARTNAME(part), i);
			return 1;
		}
		if (debug > 1) {
			fprintf(stderr,
				"        CG%d\t offset %9ld, bfree %6d\n",
				i, cgoff, cg.cg.cg_cs.cs_nbfree);
		}
		
		rval = read_bsdcg(&fs.fs, &cg.cg, dboff);
		if (rval)
			return rval;
	}

	return rval;
}

int
read_bsdcg(struct fs *fsp, struct cg *cgp, unsigned int dbstart)
{
	int  i, max;
	char *p;
	int count, j;

	max = fsp->fs_fpg;
	p   = cg_blksfree(cgp);

	/*
	 * XXX The bitmap is fragments, not FS blocks.
	 *
	 * The block bitmap lists blocks relative to the base (cgbase()) of
	 * the cylinder group. cgdmin() is the first actual datablock, but
	 * the bitmap includes all the blocks used for all the blocks
	 * comprising the cg. These include the superblock, cg, inodes,
	 * datablocks and the variable-sized padding before all of these
	 * (used to skew the offset of consecutive cgs).
	 * The "dbstart" parameter is thus the beginning of the cg, to which
	 * we add the bitmap offset. All blocks before cgdmin() will always
	 * be allocated, but we scan them anyway. 
	 */

	if (debug > 2)
		fprintf(stderr, "                 ");
	for (count = i = 0; i < max; i++)
		if (isset(p, i)) {
			unsigned long dboff, dbcount;

			j = i;
			while ((i+1)<max && isset(p, i+1))
				i++;

			dboff = dbstart + fsbtodb(fsp, j);
			dbcount = fsbtodb(fsp, (i-j) + 1);
					
			if (debug > 2) {
				if (count)
					fprintf(stderr, ",%s",
						count % 4 ?
						" " : "\n                 ");
				fprintf(stderr, "%lu:%ld", dboff, dbcount);
			}
			addskip(dboff, dbcount);
			count++;
		}
	if (debug > 2)
		fprintf(stderr, "\n");
	return 0;
}
#endif

/*
 * Operate on a linux slice. I actually don't have a clue what a linux
 * slice looks like. I just know that in our images, the linux slice
 * has the boot block in the first sector, part of the boot in the
 * second sector, and then the superblock for the one big filesystem
 * in the 3rd sector. Just start there and move on.
 *
 * Unlike BSD partitions, linux block offsets are from the start of the
 * slice, so we have to add the starting sector to everything. 
 */
int
read_linuxslice(int slice, u_int32_t start, u_int32_t size)
{
#define LINUX_SUPERBLOCK_OFFSET	1024
#define LINUX_SUPERBLOCK_SIZE 	1024
#define LINUX_GRPSPERBLK	\
	(LINUX_SUPERBLOCK_SIZE/sizeof(struct ext2_group_desc))

	int			cc, i, numgroups, rval = 0;
	struct ext2_super_block	fs;
	struct ext2_group_desc	groups[LINUX_GRPSPERBLK];
	int			dosslice = slice + 1; /* DOS Numbering */

	assert((sizeof(fs) & ~LINUX_SUPERBLOCK_SIZE) == 0);
	assert((sizeof(groups) & ~LINUX_SUPERBLOCK_SIZE) == 0);

	if (debug)
		fprintf(stderr, "  P%d (Linux Slice)\n", dosslice);
	
	/*
	 * Skip ahead to the superblock.
	 */
	if (devlseek(infd, sectobytes(start) + LINUX_SUPERBLOCK_OFFSET,
		     SEEK_SET) < 0) {
		warnx("Linux Slice %d: Could not seek to superblock",
		      dosslice);
		return 1;
	}

	if ((cc = devread(infd, &fs, LINUX_SUPERBLOCK_SIZE)) < 0) {
		warn("Linux Slice %d: Could not read superblock", dosslice);
		return 1;
	}
	if (cc != LINUX_SUPERBLOCK_SIZE) {
		warnx("Linux Slice %d: Truncated superblock", dosslice);
		return 1;
	}
 	if (fs.s_magic != EXT2_SUPER_MAGIC) {
		warnx("Linux Slice %d: Bad magic number in superblock",
		      dosslice);
 		return (1);
 	}

	if (debug) {
		fprintf(stderr, "        bfree %9u, size %9d\n",
		       fs.s_free_blocks_count, EXT2_BLOCK_SIZE(&fs));
	}
	numgroups = fs.s_blocks_count / fs.s_blocks_per_group;
	
	/*
	 * Read each group descriptor. It says where the free block bitmap
	 * lives. The absolute block numbers a group descriptor refers to
	 * is determined by its index * s_blocks_per_group. Once we know where
	 * the bitmap lives, we can go out to the bitmap and see what blocks
	 * are free.
	 */
	for (i = 0; i <= numgroups; i++) {
		int gix;
		off_t soff;

		gix = (i % LINUX_GRPSPERBLK);
		if (gix == 0) {
			soff = sectobytes(start) +
				((off_t)(EXT2_BLOCK_SIZE(&fs) +
					 (i * sizeof(struct ext2_group_desc))));
			if (devlseek(infd, soff, SEEK_SET) < 0) {
				warnx("Linux Slice %d: "
				      "Could not seek to Group %d",
				      dosslice, i);
				return 1;
			}

			if ((cc = devread(infd, groups, sizeof(groups))) < 0) {
				warn("Linux Slice %d: "
				     "Could not read Group %d",
				     dosslice, i);
				return 1;
			}
			if (cc != sizeof(groups)) {
				warnx("Linux Slice %d: "
				      "Truncated Group %d", dosslice, i);
				return 1;
			}
		}

		if (debug) {
			fprintf(stderr,
				"        Group:%-2d\tBitmap %9u, bfree %9d\n",
				i, groups[gix].bg_block_bitmap,
				groups[gix].bg_free_blocks_count);
		}

		rval = read_linuxgroup(&fs, &groups[gix], i, start);
	}
	
	return 0;
}

/*
 * A group descriptor says where on the disk the block bitmap is. Its
 * a 1bit per block map, where each bit is a FS block (instead of a
 * fragment like in BSD). Since linux offsets are relative to the start
 * of the slice, need to adjust the numbers using the slice offset.
 */
int
read_linuxgroup(struct ext2_super_block *super,
		struct ext2_group_desc	*group,
		int index,
		u_int32_t sliceoffset /* Sector offset of slice */)
{
	char	*p, bitmap[0x1000];
	int	i, cc, max;
	int	count, j;
	off_t	offset;

	offset  = sectobytes(sliceoffset);
	offset += (off_t)EXT2_BLOCK_SIZE(super) * group->bg_block_bitmap;
	if (devlseek(infd, offset, SEEK_SET) < 0) {
		warn("Linux Group %d: "
		     "Could not seek to Group bitmap %ld",
		     index, group->bg_block_bitmap);
		return 1;
	}

	/*
	 * The bitmap is how big? Just make sure that its what we expect
	 * since I don't know what the rules are.
	 */
	if (EXT2_BLOCK_SIZE(super) != 0x1000 ||
	    super->s_blocks_per_group  != 0x8000) {
		warnx("Linux Group %d: "
		      "Bitmap size not what I expect it to be!", index);
		return 1;
	}

	if ((cc = devread(infd, bitmap, sizeof(bitmap))) < 0) {
		warn("Linux Group %d: "
		     "Could not read Group bitmap", index);
		return 1;
	}
	if (cc != sizeof(bitmap)) {
		warnx("Linux Group %d: Truncated Group bitmap", index);
		return 1;
	}

	max = super->s_blocks_per_group;
	p   = bitmap;

	/*
	 * XXX The bitmap is FS blocks.
	 *     The bitmap is an "inuse" map, not a free map.
	 */
#define LINUX_FSBTODB(count) \
	((EXT2_BLOCK_SIZE(super) / secsize) * (count))

	if (debug > 2)
		fprintf(stderr, "                 ");
	for (count = i = 0; i < max; i++)
		if (!isset(p, i)) {
			unsigned long dboff;
			int dbcount;

			j = i;
			while ((i+1)<max && !isset(p, i+1))
				i++;

			/*
			 * The offset of this free range, relative
			 * to the start of the disk, is the slice
			 * offset, plus the offset of the group itself,
			 * plus the index of the first free block in
			 * the current range.
			 */
			dboff = sliceoffset +
				LINUX_FSBTODB(index * max) +
				LINUX_FSBTODB(j);

			dbcount = LINUX_FSBTODB((i-j) + 1);
					
			if (debug > 2) {
				if (count)
					fprintf(stderr, ",%s",
						count % 4 ?
						" " : "\n                 ");
				fprintf(stderr, "%lu:%d %d:%d",
					dboff, dbcount, j, i);
			}
			addskip(dboff, dbcount);
			count++;
		}
	if (debug > 2)
		fprintf(stderr, "\n");

	return 0;
}

/*
 * For a linux swap partition, all that matters is the first little
 * bit of it. The rest of it does not need to be written to disk.
 */
int
read_linuxswap(int slice, u_int32_t start, u_int32_t size)
{
	if (debug) {
		fprintf(stderr,
			"  P%d (Linux Swap)\n", slice + 1 /* DOS Numbering */);
		fprintf(stderr,
			"        start %12d, size %9d\n",
			start, size);
	}

	start += bytestosec(0x8000);
	size  -= bytestosec(0x8000);
	
	addskip(start, size);
	return 0;
}

#ifdef WITH_NTFS
/********Code to deal with NTFS file system*************/
/* Written by: Russ Christensen <rchriste@cs.utah.edu> */

/**@bug If the Windows partition is only created in a part of a
 * primary partition created with FreeBSD's fdisk then the sectors
 * after the end of the NTFS partition and before the end of the raw
 * primary partition will not be marked as free space. */


struct ntfs_cluster;
struct ntfs_cluster {
	unsigned long start;
	unsigned long length;
	struct ntfs_cluster *next;
};

static __inline__ int
ntfs_isAllocated(char *map, __s64 pos)
{
	int result;
	char unmasked;
	char byte;
	short shift;
	byte = *(map+(pos/8));
	shift = pos % 8;
	unmasked = byte >> shift;
	result = unmasked & 1;
	assert((result == 0 || result == 1) &&
	       "Programming error in statement above");
	return result;
}

static void
ntfs_addskips(ntfs_volume *vol,struct ntfs_cluster *free,u_int32_t offset)
{
	u_int8_t sectors_per_cluster;
	struct ntfs_cluster *cur;
	int count = 0;
	sectors_per_cluster = vol->cluster_size / vol->sector_size;
	if(debug) {
		fprintf(stderr,"sectors per cluster: %d\n",
			sectors_per_cluster);
		fprintf(stderr,"offset: %d\n", offset);
	}
	for(count = 0, cur = free; cur != NULL; cur = cur->next, count++) {
		if(debug > 1) {
			fprintf(stderr, "\tGroup:%-10dCluster%8li, size%8li\n",
				count, cur->start,cur->length);
		}
		addskip(cur->start*sectors_per_cluster + offset,
			cur->length*sectors_per_cluster);
	}
}

static int
ntfs_freeclusters(struct ntfs_cluster *free)
{
	int total;
	struct ntfs_cluster *cur;
	for(total = 0, cur = free; cur != NULL; cur = cur->next)
		total += cur->length;
	return total;
}

/* The calling function owns the pointer returned.*/
static void *
ntfs_read_data_attr(ntfs_attr *na)
{
	void  *result;
	int64_t pos;
	int64_t tmp;
	int64_t amount_needed;
	int   count;

	/**ntfs_attr_pread might actually read in more data than we
	 * ask for.  It will round up to the nearest DEV_BSIZE boundry
	 * so make sure we allocate enough memory.*/
	amount_needed = na->data_size - (na->data_size % DEV_BSIZE) + DEV_BSIZE;
	assert(amount_needed > na->data_size && amount_needed % DEV_BSIZE == 0
	       && "amount_needed is rounded up to DEV_BSIZE multiple");
	if(!(result = malloc(amount_needed))) {
		perror("Out of memory!\n");
		exit(1);
	}
	pos = 0;
	count = 0;
	while(pos < na->data_size) {
		tmp = ntfs_attr_pread(na,pos,na->data_size - pos,result+pos);
		if(tmp < 0) {
			perror("ntfs_attr_pread failed");
			exit(1);
		}
		assert(tmp != 0 && "Not supposed to happen error!  "
		       "Either na->data_size is wrong or there is another "
		       "problem");
		assert(tmp % DEV_BSIZE == 0 && "Not supposed to happen");
		pos += tmp;
	}
#if 0 /*Turn on if you want to look at the free list directly*/
	{
		int fd;

		fprintf(stderr, "Writing ntfs_free_bitmap.bin\n");
		if((fd = open("ntfs_free_bitmap.bin",
			      O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
			perror("open ntfs_free_bitmap.bin failed\n");
			exit(1);
		}
		if(write(fd, result, na->data_size) != na->data_size) {
			perror("writing free space bitmap.bin failed\n");
			exit(1);
		}
		close(fd);
		fprintf(stderr, "Done\n");
	}
#endif
	return result;
}

static struct ntfs_cluster *
ntfs_compute_freeblocks(ntfs_attr *na, void *cluster_map, __s64 num_clusters)
{
	struct ntfs_cluster *result;
	struct ntfs_cluster *curr;
	struct ntfs_cluster *tmp;
	__s64 pos = 1;
	int total_free = 0;
	result = curr = NULL;
	assert(num_clusters <= na->data_size * 8 && "If there are more "
	       "clusters than bits in the free space file then we have a "
	       "problem.  Fewer clusters than bits is okay.");
	if(debug)
		fprintf(stderr,"num_clusters==%qd\n",num_clusters);
	while(pos < num_clusters) {
		if(!ntfs_isAllocated(cluster_map,pos++)) {
			curr->length++;
			total_free++;
		}
		else {
			while(ntfs_isAllocated(cluster_map,pos)
			      && pos < num_clusters) {
				++pos;
			}
			if(pos >= num_clusters) break;
			if(!(tmp = malloc(sizeof(struct ntfs_cluster)))) {
				perror("clusters_free: Out of memory");
				exit(1);
			}
			if(curr) {
				curr->next = tmp;
				curr = curr->next;
			} else
				result = curr = tmp;
			curr->start = pos;
			curr->length = 0;
			curr->next = NULL;
		}
	}
	if(debug)
		fprintf(stderr, "total_free==%d\n",total_free);
	return result;
}

/*Add the blocks used by filename to the free list*/
void
ntfs_skipfile(ntfs_volume *vol, char *filename, u_int32_t offset)
{
	u_int8_t sectors_per_cluster;
	ntfs_inode *ni, *ni_root;
	ntfs_attr *na;
	MFT_REF File;
	runlist_element *rl;
	int ulen;
	uchar_t *ufilename;
	int i;
	int amount_skipped;

	/*Goal: Get MFT_REF for filename before we can call ntfs_inode_open
	        on the file.*/
	if(!(ni_root = ntfs_inode_open(vol, FILE_root))) {
		perror("Opening file $ROOT failed\n");
		ntfs_umount(vol,TRUE);
		exit(1);
	}
	/* Subgoal: get the uchar_t name for filename */
	ufilename = malloc(sizeof(uchar_t)*(strlen(filename)+1));
	if(!ufilename) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	bzero(ufilename,sizeof(uchar_t)*strlen(filename)+1);
	ulen = ntfs_mbstoucs(filename, &ufilename, strlen(filename)+1);
	if(ulen == -1) {
		perror("ntfs_mbstoucs failed");
		exit(1);
	}
	File = ntfs_inode_lookup_by_name(ni_root, ufilename, ulen);
	if(IS_ERR_MREF(File)) {
		if (debug > 1) {
			fprintf(stderr, "%s does not exist so there is no need "
				"to skip the file.\n", filename);
		}
		return;
	}
  	free(ufilename);
	ufilename = NULL;
	if(debug > 1 ) fprintf(stderr,"vol->nr_mft_records==%lld\n",
			       vol->nr_mft_records);
	/*Goal: Skip the file*/
	if(!(ni = ntfs_inode_open(vol, File))) {
	  perror("calling ntfs_inode_open (0)");
	  ntfs_umount(vol,TRUE);
	  exit(1);
	}
	if(!(na = ntfs_attr_open(ni, AT_DATA, NULL, 0))) {
		perror("Opening attribute $DATA failed\n");
		ntfs_umount(vol,TRUE);
		exit(1);
	}
	assert(NAttrNonResident(na) && "You are trying to skip a file that is "
	       "small enough to be resident inside the Master File Table. "
	       "This is a bit silly.");
	/*Goal: Find out what clusters on the disk are being used by filename*/
	sectors_per_cluster = vol->cluster_size / vol->sector_size;
	if(!(rl = ntfs_attr_find_vcn(na, 0))) {
	    perror("Error calling ntfs_attr_find_vcn");
	    exit(1);
	}
	amount_skipped = 0;
	for(i=0; rl[i].length != 0; i++) {
		if (rl[i].lcn == LCN_HOLE) {
		    if (debug > 1) {
			fprintf(stderr, "LCN_HOLE\n");
		    }
		    continue;
		}
		if (rl[i].lcn == LCN_RL_NOT_MAPPED) {
		    /* Pull in more of the runlist because the NTFS library
		       might not pull in the entire runlist when you ask
		       for it.  When I asked the NTFS library folks why they
		       do this they said it was for performance reasons. */
		    if (debug > 1) {
			fprintf(stderr, "LCN_RL_NOT_MAPPED\n");
		    }
		    if (ntfs_attr_map_runlist(na, rl[i].vcn) == -1) {
			perror("ntfs_attr_map_runlist failed\n");
			exit(1);
		    } else {
			rl = ntfs_attr_find_vcn(na, 0);
			/* There *might* be a memory leak here.  I don't
			   know if rl needs to be freed by us or not. */
			if(!rl) {
			    perror("Error calling ntfs_attr_find_vcn");
			    exit(1);
			}
			/*retry*/
			--i;
			continue;
		    }
		}
		if (debug > 1) {
		    fprintf(stderr, "For file %s skipping:%lld length:%lld\n",
			    filename,
			    (long long int)rl[i].lcn*sectors_per_cluster +
			    offset,
			    (long long int)rl[i].length*sectors_per_cluster);
		}
		assert(rl[i].length > 0 && "Programming error");
		assert(rl[i].lcn > 0 &&
		       "Programming error: Not catching NTFS Lib error value");
		amount_skipped += rl[i].length*sectors_per_cluster;
		addskip(rl[i].lcn*sectors_per_cluster + offset,
			rl[i].length*sectors_per_cluster);
	}
	if (debug) {
	    fprintf(stderr, "For NTFS file %s skipped %d bytes\n", filename,
		    amount_skipped*512);
	}
}

/*
 * Primary function to call to operate on an NTFS slice.
 */
int
read_ntfsslice(int slice, u_int32_t start, u_int32_t size, char *openname)
{
	ntfs_inode     *ni_bitmap;
	ntfs_attr      *na_bitmap;
	void           *buf;
	struct ntfs_cluster *cfree;
  	struct ntfs_cluster *tmp;
	char           *name;
	ntfs_volume    *vol;
	int            length;

	/* Check to make sure the types the NTFS lib defines are what they
	   claim*/
	assert(sizeof(s64) == 8);
	assert(sizeof(s32) == 4);
	assert(sizeof(u64) == 8);
	assert(sizeof(u32) == 4);
	length = strlen(openname);
	name = malloc(length+3);
	/*Our NTFS Library code needs the /dev name of the partion to
	 * examine.  The following line is FreeBSD specific code.*/
	sprintf(name,"%ss%d",openname,slice + 1);
	/*The volume must be mounted to find out what clusters are free*/
	if(!(vol = ntfs_mount(name, MS_RDONLY))) {
		fprintf(stderr,"When mounting %s\n",name);
		perror("Failed to read superblock information.  Not a valid "
		       "NTFS partition\n");
		return -1;
	}
	/*A bitmap of free clusters is in the $DATA attribute of the
	 *  $BITMAP file*/
	if(!(ni_bitmap = ntfs_inode_open(vol, FILE_Bitmap))) {
		perror("Opening file $BITMAP failed\n");
		ntfs_umount(vol,TRUE);
		exit(1);
	}
	if(!(na_bitmap = ntfs_attr_open(ni_bitmap, AT_DATA, NULL, 0))) {
		perror("Opening attribute $DATA failed\n");
		exit(1);
	}
	buf = ntfs_read_data_attr(na_bitmap);
	cfree = ntfs_compute_freeblocks(na_bitmap,buf,vol->nr_clusters);
	ntfs_addskips(vol,cfree,start);
	if(debug > 1) {
		fprintf(stderr, "  P%d (NTFS v%u.%u)\n",
			slice + 1 /* DOS Numbering */,
			vol->major_ver,vol->minor_ver);
		fprintf(stderr, "        %s",name);
		fprintf(stderr, "      start %10d, size %10d\n", start, size);
		fprintf(stderr, "        Sector size: %u, Cluster size: %u\n",
			vol->sector_size, vol->cluster_size);
		fprintf(stderr, "        Volume size in clusters: %qd\n",
			vol->nr_clusters);
		fprintf(stderr, "        Free clusters:\t\t %u\n",
			ntfs_freeclusters(cfree));
	}

      	ntfs_skipfile(vol, "pagefile.sys", start);
      	ntfs_skipfile(vol, "hiberfil.sys", start);

	/*We have the information we need so unmount everything*/
	ntfs_attr_close(na_bitmap);
	if(ntfs_inode_close(ni_bitmap)) {
		perror("ntfs_close_inode ni_bitmap failed");
		exit(1);
	}
	if(ntfs_umount(vol,FALSE)) {
		perror("ntfs_umount failed");
		exit(1);
	}
	/*Free NTFS malloc'd memory*/
	assert(name && "Programming Error, name should be freed here");
	free(name);
	assert(buf && "Programming Error, buf should be freed here");
	free(buf);
	assert(cfree && "Programming Error, "
	       "'struct cfree' should be freed here");
	while(cfree) {
		tmp = cfree->next;
		free(cfree);
		cfree = tmp;
	}
	return 0; /*Success*/
}
#endif

/*
 * For a raw image (something we know nothing about), we report the size
 * and compress the entire thing (that is, there are no skip ranges).
 */
int
read_raw(void)
{
	off_t	size;

	if ((size = devlseek(infd, (off_t) 0, SEEK_END)) < 0) {
		warn("lseeking to end of raw image");
		return 1;
	}

	if (debug) {
		fprintf(stderr, "  Raw Image\n");
		fprintf(stderr, "        start %12d, size %12qd\n", 0, size);
	}
	return 0;
}

char *usagestr = 
 "usage: imagezip [-vihor] [-s #] <image | device> [outputfilename]\n"
 " -v             Print version info and exit\n"
 " -i             Info mode only.  Do not write an output file\n"
 " -h             Print this help message\n"
 " -o             Print progress indicating dots\n"
 " -r             Generate a `raw' image.  No FS compression is attempted\n"
 " -s slice       Compress a particular slice (DOS numbering 1-4)\n"
 " image | device The input image or a device special file (ie: /dev/ad0)\n"
 " outputfilename The output file ('-' for stdout)\n"
 "\n"
 " Advanced options\n"
 " -z level       Set the compression level.  Range 0-9 (0==none, default==4)\n"
 " -I slice       Ignore (skip) the indicated slice (not with slice mode)\n"
 " -R slice       Force raw compression of the indicated slice (not with slice mode)\n"
 " -c count       Compress <count> number of sectors (not with slice mode)\n"
 " -D             Do `dangerous' writes (don't check for async errors)\n"
 " -1             Output a version one image file\n"
 "\n"
 " Debugging options (not to be used by mere mortals!)\n"
 " -d             Turn on debugging.  Multiple -d options increase output\n"
 " -l             Linux slice only.  Input must be a Linux slice image\n"
 " -b             FreeBSD slice only.  Input must be a FreeBSD slice image\n";

void
usage()
{
	fprintf(stderr, usagestr);
	exit(1);
}

void
addskip(uint32_t start, uint32_t size)
{
	struct range	   *skip;

	if (size < frangesize)
		return;

	if ((skip = (struct range *) malloc(sizeof(*skip))) == NULL) {
		fprintf(stderr, "No memory for skip range, "
			"try again with '-F <numsect>'\n"
			"where <numsect> is greater than the current %d\n",
			frangesize);
		exit(1);
	}
	
	skip->start = start;
	skip->size  = size;
	skip->next  = skips;
	skips       = skip;
	numskips++;
}

void
dumpskips(void)
{
	struct range	*pskip;
	uint32_t	offset = 0, total = 0;

	if (!skips)
		return;

	if (debug > 2)
		fprintf(stderr, "Skip ranges (start/size) in sectors:\n");
	
	pskip = skips;
	while (pskip) {
		assert(pskip->start >= offset);
		if (debug > 2)
			fprintf(stderr,
				"  %12d    %9d\n", pskip->start, pskip->size);
		offset = pskip->start + pskip->size;
		total += pskip->size;
		pskip  = pskip->next;
	}
	if (debug > 2)
		fprintf(stderr, "\n");
	
	fprintf(stderr, "Total Number of Free Sectors: %d (bytes %qd)\n",
		total, sectobytes(total));
}

/*
 * A very dumb bubblesort!
 */
void
sortrange(struct range *head, int domerge)
{
	struct range	*prange, tmp, *ptmp;
	int		changed = 1;

	if (head == NULL)
		return;
	
	while (changed) {
		changed = 0;

		prange = head;
		while (prange) {
			if (prange->next &&
			    prange->start > prange->next->start) {
				tmp.start = prange->start;
				tmp.size  = prange->size;

				prange->start = prange->next->start;
				prange->size  = prange->next->size;
				prange->next->start = tmp.start;
				prange->next->size  = tmp.size;

				changed = 1;
			}
			prange  = prange->next;
		}
	}

	if (!domerge)
		return;

	/*
	 * Now look for contiguous free regions and combine them.
	 */
	prange = head;
	while (prange) {
	again:
		if (prange->next &&
		    prange->start + prange->size == prange->next->start) {
			prange->size += prange->next->size;
			
			ptmp        = prange->next;
			prange->next = prange->next->next;
			free(ptmp);
			goto again;
		}
		prange  = prange->next;
	}
}

/*
 * Life is easier if I think in terms of the valid ranges instead of
 * the free ranges. So, convert them.  Note that if there were no skips,
 * we create a single range covering the entire partition.
 */
void
makeranges(void)
{
	struct range	*pskip, *ptmp, *range, **lastrange;
	uint32_t	offset;
	uint32_t	total = 0;
	
	offset = inputminsec;
	lastrange = &ranges;

	pskip = skips;
	while (pskip) {
		if ((range = (struct range *)
		             malloc(sizeof(*range))) == NULL) {
			fprintf(stderr, "Out of memory!\n");
			exit(1);
		}
		range->start = offset;
		range->size  = pskip->start - offset;
		range->next  = 0;
		offset       = pskip->start + pskip->size;
		total	     += range->size;
		
		*lastrange = range;
		lastrange = &range->next;
		numranges++;

		ptmp  = pskip;
		pskip = pskip->next;
		free(ptmp);
	}
	/*
	 * Last piece, but only if there is something to compress.
	 */
	if (inputmaxsec == 0 || (inputmaxsec - offset) != 0) {
		assert(inputmaxsec == 0 || inputmaxsec > offset);
		if ((range = (struct range *)malloc(sizeof(*range))) == NULL) {
			fprintf(stderr, "Out of memory!\n");
			exit(1);
		}
		range->start = offset;
	
		/*
		 * A bug in FreeBSD causes lseek on a device special file to
		 * return 0 all the time! Well we want to be able to read
		 * directly out of a raw disk (/dev/rad0), so we need to
		 * use the compressor to figure out the actual size when it
		 * isn't known beforehand.
		 *
		 * Mark the last range with 0 so compression goes to end
		 * if we don't know where it is.
		 */
		if (inputmaxsec)
			range->size = inputmaxsec - offset;
		else
			range->size = 0;
		range->next = 0;
		total += range->size;

		*lastrange = range;
		numranges++;
	}

	if (debug) {
		if (debug > 2) {
			range = ranges;
			while (range) {
				fprintf(stderr, "  %12d    %9d\n",
					range->start, range->size);
				range  = range->next;
			}
			fprintf(stderr, "\n");
		}
		fprintf(stderr,
			"Total Number of Valid Sectors: %d (bytes %qd)\n",
			total, sectobytes(total));
	}
}

/*
 * Fixup descriptor handling.
 *
 * Fixups are modifications that need to be made to file data prior
 * to compressing.  The only use right now is to fixup BSD disklabel
 * partition tables so they don't contain absolute offsets.
 */
struct fixup {
	off_t offset;	/* disk offset */
	off_t poffset;	/* partition offset */
	off_t size;
	int reloctype;
	char data[0];
};

void
addfixup(off_t offset, off_t poffset, off_t size, void *data, int reloctype)
{
	struct range *entry;
	struct fixup *fixup;

	if (oldstyle) {
		static int warned;

		if (!warned) {
			fprintf(stderr, "WARNING: no fixups in V1 images\n");
			warned = 1;
		}
		return;
	}

	if ((entry = malloc(sizeof(*entry))) == NULL ||
	    (fixup = malloc(sizeof(*fixup) + (int)size)) == NULL) {
		fprintf(stderr, "Out of memory!\n");
		exit(1);
	}
	
	entry->start = bytestosec(offset);
	entry->size  = bytestosec(size + secsize - 1);
	entry->data  = fixup;
	
	fixup->offset    = offset;
	fixup->poffset   = poffset;
	fixup->size      = size;
	fixup->reloctype = reloctype;
	memcpy(fixup->data, data, size);

	entry->next  = fixups;
	fixups       = entry;
}

void
applyfixups(off_t offset, off_t size, void *data)
{
	struct range **prev, *entry;
	struct fixup *fp;
	uint32_t coff, clen;

	for (prev = &fixups; (entry = *prev) != NULL; prev = &entry->next) {
		fp = entry->data;

		if (offset < fp->offset+fp->size && offset+size > fp->offset) {
			/* XXX lazy: fixup must be totally contained */
			assert(offset <= fp->offset);
			assert(fp->offset+fp->size <= offset+size);

			coff = (u_int32_t)(fp->offset - offset);
			clen = (u_int32_t)fp->size;
			if (debug > 1)
				fprintf(stderr,
					"Applying fixup [%qu-%qu] "
					"to [%qu-%qu]\n",
					fp->offset, fp->offset+fp->size,
					offset, offset+size);
			memcpy(data+coff, fp->data, clen);

			/* create a reloc if necessary */
			if (fp->reloctype != RELOC_NONE)
				addreloc(fp->offset - fp->poffset,
					 fp->size, fp->reloctype);

			*prev = entry->next;
			free(fp);
			free(entry);

			/* XXX only one per customer */
			break;
		}
	}
}

void
addreloc(off_t offset, off_t size, int reloctype)
{
	struct blockreloc *reloc;

	assert(!oldstyle);

	numrelocs++;
	if (HDRUSED(numregions, numrelocs) > DEFAULTREGIONSIZE) {
		fprintf(stderr, "Over filled region/reloc table (%d/%d)\n",
			numregions, numrelocs);
		exit(1);
	}

	relocs = realloc(relocs, numrelocs * sizeof(struct blockreloc));
	if (relocs == NULL) {
		fprintf(stderr, "Out of memory!\n");
		exit(1);
	}

	reloc = &relocs[numrelocs-1];
	reloc->type = reloctype;
	reloc->sector = bytestosec(offset);
	reloc->sectoff = offset - sectobytes(reloc->sector);
	reloc->size = size;
}

void
freerelocs(void)
{
	numrelocs = 0;
	free(relocs);
	relocs = NULL;
}

/*
 * Compress the image.
 */
static u_char   output_buffer[SUBBLOCKSIZE];
static int	buffer_offset;
static off_t	inputoffset;
static struct timeval cstamp;
static long long bytescompressed;

static off_t	compress_chunk(off_t, off_t, int *, uint32_t *);
static int	compress_finish(uint32_t *subblksize);
static void	compress_status(int sig);

/*
 * Loop through the image, compressing the allocated ranges.
 */
int
compress_image(void)
{
	int		cc, full, i, count, chunkno;
	off_t		size = 0, outputoffset;
	off_t		tmpoffset, rangesize;
	struct range	*prange;
	blockhdr_t	*blkhdr;
	struct region	*curregion, *regions;
	struct timeval  estamp;
	char		*buf;
	uint32_t	cursect = 0;
	struct region	*lreg;

	gettimeofday(&cstamp, 0);
	inputoffset = 0;
#ifdef SIGINFO
	signal(SIGINFO, compress_status);
#endif

	buf = output_buffer;
	memset(buf, 0, DEFAULTREGIONSIZE);
	blkhdr = (blockhdr_t *) buf;
	if (oldstyle)
		regions = (struct region *)((struct blockhdr_V1 *)blkhdr + 1);
	else
		regions = (struct region *)(blkhdr + 1);
	curregion = regions;
	numregions = 0;
	chunkno = 0;

	/*
	 * Reserve room for the subblock hdr and the region pairs.
	 * We go back and fill it it later after the subblock is
	 * done and we know much input data was compressed into
	 * the block.
	 */
	buffer_offset = DEFAULTREGIONSIZE;
	
	prange = ranges;
	while (prange) {
		inputoffset = sectobytes(prange->start);

		/*
		 * Seek to the beginning of the data range to compress.
		 */
		devlseek(infd, (off_t) inputoffset, SEEK_SET);

		/*
		 * The amount to compress is the size of the range, which
		 * might be zero if its the last one (size unknown).
		 */
		rangesize = sectobytes(prange->size);

		/*
		 * Compress the chunk.
		 */
		if (debug > 0 && debug < 3) {
			fprintf(stderr,
				"Compressing range: %14qd --> ", inputoffset);
			fflush(stderr);
		}

		size = compress_chunk(inputoffset, rangesize,
				      &full, &blkhdr->size);
	
		if (debug >= 3) {
			fprintf(stderr, "%14qd -> %12qd %10ld %10u %10d %d\n",
				inputoffset, inputoffset + size,
				prange->start - inputminsec,
				bytestosec(size),
				blkhdr->size, full);
		}
		else if (debug) {
			gettimeofday(&estamp, 0);
			estamp.tv_sec -= cstamp.tv_sec;
			fprintf(stderr, "%12qd in %ld seconds.\n",
				inputoffset + size, estamp.tv_sec);
		}
		else if (dots && full) {
			static int pos;

			putc('.', stderr);
			if (pos++ >= 60) {
				gettimeofday(&estamp, 0);
				estamp.tv_sec -= cstamp.tv_sec;
				fprintf(stderr, " %12qd %4ld\n",
					inputoffset+size, estamp.tv_sec);
				pos = 0;
			}
			fflush(stderr);
		}

		if (size == 0)
			goto done;

		/*
		 * This should never happen!
		 */
		if (size & (secsize - 1)) {
			fprintf(stderr, "  Not on a sector boundry at %qd\n",
				inputoffset);
			return 1;
		}

		/*
		 * We have completed a region.  We have either:
		 *
		 * 1. compressed the entire current input range
		 * 2. run out of room in the 1MB chunk
		 * 3. hit the end of the input file
		 *
		 * For #1 we want to continue filling the current chunk.
		 * For 2 and 3 we are done with the current chunk.
		 */
		curregion->start = prange->start - inputminsec;
		curregion->size  = bytestosec(size);
		curregion++;
		numregions++;

		/*
		 * Check to see if the region/reloc table is full.
		 * If this is the last region that will fit in the available
		 * space (i.e., one more would not), finish off any
		 * compression we are in the middle of and declare the
		 * region full.
		 */
		if (HDRUSED(numregions+1, numrelocs) > DEFAULTREGIONSIZE) {
			assert(HDRUSED(numregions, numrelocs) <=
			       DEFAULTREGIONSIZE);
			if (!full) {
				compress_finish(&blkhdr->size);
				full = 1;
			}
		}

		/*
		 * 1. We managed to compress the entire range,
		 *    go to the next range continuing to fill the
		 *    current chunk.
		 */
		if (!full) {
			assert(rangesize == 0 || size == rangesize);

			prange = prange->next;
			continue;
		}

		/*
		 * A partial range. Well, maybe a partial range.
		 *
		 * Go back and stick in the block header and the region
		 * information.
		 */
		blkhdr->magic = oldstyle ?
			COMPRESSED_V1 : COMPRESSED_MAGIC_CURRENT;
		blkhdr->blockindex  = chunkno;
		blkhdr->regionsize  = DEFAULTREGIONSIZE;
		blkhdr->regioncount = (curregion - regions);
		if (!oldstyle) {
			blkhdr->firstsect   = cursect;
			if (size == rangesize) {
				/*
				 * Finished subblock at the end of a range.
				 * Find the beginning of the next range so that
				 * we include any free space between the ranges
				 * here.  If this was the last range, we use
				 * inputmaxsec.  If inputmaxsec is zero, we know
				 * that we did not end with a skip range.
				 */
				if (prange->next)
					blkhdr->lastsect = prange->next->start -
						inputminsec;
				else if (inputmaxsec > 0)
					blkhdr->lastsect = inputmaxsec -
						inputminsec;
				else {
					lreg = curregion - 1;
					blkhdr->lastsect =
						lreg->start + lreg->size;
				}
			} else {
				lreg = curregion - 1;
				blkhdr->lastsect = lreg->start + lreg->size;
			}
			cursect = blkhdr->lastsect;

			blkhdr->reloccount = numrelocs;
		}

		/*
		 * Dump relocation info
		 */
		if (numrelocs) {
			assert(!oldstyle);
			assert(relocs != NULL);
			memcpy(curregion, relocs,
			       numrelocs * sizeof(struct blockreloc));
			freerelocs();
		}

		/*
		 * Write out the finished chunk to disk.
		 */
		mywrite(outfd, output_buffer, sizeof(output_buffer));

		/*
		 * Moving to the next block. Reserve the header area.
		 */
		buffer_offset = DEFAULTREGIONSIZE;
		curregion     = regions;
		numregions    = 0;
		chunkno++;

		/*
		 * Okay, so its possible that we ended the region at the
		 * end of the subblock. I guess "partial" is a bad name.
		 * Anyway, most of the time we ended a subblock in the
		 * middle of a range, and we have to keeping going on it.
		 *
		 * Ah, the last range is a possible special case. It might
		 * have a 0 size if we were reading from a device special
		 * file that does not return the size from lseek (Freebsd).
		 * Zero indicated that we just read until EOF cause we have
		 * no idea how big it really is.
		 */
		if (size == rangesize) 
			prange = prange->next;
		else {
			uint32_t sectors = bytestosec(size);
			
			prange->start += sectors;
			if (prange->size)
				prange->size -= sectors;
		}
	}

 done:
	/*
	 * Have to finish up by writing out the last batch of region info.
	 */
	if (curregion != regions) {
		compress_finish(&blkhdr->size);
		
		blkhdr->magic =
			oldstyle ? COMPRESSED_V1 : COMPRESSED_MAGIC_CURRENT;
		blkhdr->blockindex  = chunkno;
		blkhdr->regionsize  = DEFAULTREGIONSIZE;
		blkhdr->regioncount = (curregion - regions);
		if (!oldstyle) {
			blkhdr->firstsect = cursect;
			if (inputmaxsec > 0)
				blkhdr->lastsect = inputmaxsec - inputminsec;
			else {
				lreg = curregion - 1;
				blkhdr->lastsect = lreg->start + lreg->size;
			}
			blkhdr->reloccount = numrelocs;
		}

		/*
		 * Check to see if the region/reloc table is full.
		 * XXX handle this gracefully sometime.
		 */
		if (HDRUSED(numregions, numrelocs) > DEFAULTREGIONSIZE) {
			fprintf(stderr, "Over filled region table (%d/%d)\n",
				numregions, numrelocs);
			exit(1);
		}

		/*
		 * Dump relocation info
		 */
		if (numrelocs) {
			assert(!oldstyle);
			assert(relocs != NULL);
			memcpy(curregion, relocs,
			       numrelocs * sizeof(struct blockreloc));
			freerelocs();
		}

		/*
		 * Write out the finished chunk to disk, and
		 * start over from the beginning of the buffer.
		 */
		mywrite(outfd, output_buffer, sizeof(output_buffer));
		buffer_offset = 0;
	}

	inputoffset += size;
	if (debug || dots)
		fprintf(stderr, "\n");
	compress_status(0);
	fflush(stderr);

	/*
	 * For version 2 we don't bother to go back and fill in the
	 * blockcount.  Imageunzip and frisbee don't use it.  We still
	 * do it if creating V1 images and we can seek on the output.
	 */
	if (!oldstyle || !outcanseek)
		return 0;
	
	/*
	 * Get the total filesize, and then number the subblocks.
	 * Useful, for netdisk.
	 */
	if ((tmpoffset = lseek(outfd, (off_t) 0, SEEK_END)) < 0) {
		perror("seeking to get output file size");
		exit(1);
	}
	count = tmpoffset / SUBBLOCKSIZE;

	for (i = 0, outputoffset = 0; i < count;
	     i++, outputoffset += SUBBLOCKSIZE) {
		
		if (lseek(outfd, (off_t) outputoffset, SEEK_SET) < 0) {
			perror("seeking to read block header");
			exit(1);
		}
		if ((cc = read(outfd, buf, sizeof(blockhdr_t))) < 0) {
			perror("reading subblock header");
			exit(1);
		}
		assert(cc == sizeof(blockhdr_t));
		if (lseek(outfd, (off_t) outputoffset, SEEK_SET) < 0) {
			perror("seeking to write new block header");
			exit(1);
		}
		blkhdr = (blockhdr_t *) buf;
		assert(blkhdr->blockindex == i);
		blkhdr->blocktotal = count;
		
		if ((cc = mywrite(outfd, buf, sizeof(blockhdr_t))) < 0) {
			perror("writing new subblock header");
			exit(1);
		}
		assert(cc == sizeof(blockhdr_t));
	}
	return 0;
}

static void
compress_status(int sig)
{
	struct timeval stamp;
	int oerrno = errno;
	unsigned int ms, bps;

	gettimeofday(&stamp, 0);
	if (stamp.tv_usec < cstamp.tv_usec) {
		stamp.tv_usec += 1000000;
		stamp.tv_sec--;
	}
	ms = (stamp.tv_sec - cstamp.tv_sec) * 1000 +
		(stamp.tv_usec - cstamp.tv_usec) / 1000;
	fprintf(stderr,
		"%qu input (%qu compressed) bytes in %u.%03u seconds\n",
		inputoffset, bytescompressed, ms / 1000, ms % 1000);
	if (sig == 0) {
		fprintf(stderr, "Image size: %qu bytes\n", datawritten);
		bps = (bytescompressed * 1000) / ms;
		fprintf(stderr, "%.3fMB/second compressed\n",
			(double)bps / (1024*1024));
	}
	errno = oerrno;
}

/*
 * Compress a chunk. The next bit of input stream is read in and compressed
 * into the output file. 
 */
#define BSIZE		(128 * 1024)
static char		inbuf[BSIZE];
static			int subblockleft = SUBBLOCKMAX;
static z_stream		d_stream;	/* Compression stream */

#define CHECK_ZLIB_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}

static off_t
compress_chunk(off_t off, off_t size, int *full, uint32_t *subblksize)
{
	int		cc, count, err, eof, finish, outsize;
	off_t		total = 0;

	/*
	 * Whenever subblockleft equals SUBBLOCKMAX, it means that a new
	 * compression subblock needs to be started.
	 */
	if (subblockleft == SUBBLOCKMAX) {
		d_stream.zalloc = (alloc_func)0;
		d_stream.zfree  = (free_func)0;
		d_stream.opaque = (voidpf)0;

		err = deflateInit(&d_stream, level);
		CHECK_ZLIB_ERR(err, "deflateInit");
	}
	*full  = 0;
	finish = 0;

	/*
	 * If no size, then we want to compress until the end of file
	 * (and report back how much).
	 */
	if (!size) {
		eof  = 1;
		size = QUAD_MAX;
	} else
		eof  = 0;

	while (size > 0) {
		if (size > BSIZE)
			count = BSIZE;
		else
			count = (int) size;

		/*
		 * As we get near the end of the subblock, reduce the amount
		 * of input to make sure we can fit without producing a
		 * partial output block. Easier. See explanation below.
		 * Also, subtract out a little bit as we get near the end since
		 * as the blocks get smaller, it gets more likely that the
		 * data won't be compressable (maybe its already compressed),
		 * and the output size will be *bigger* than the input size.
		 */
		if (count > (subblockleft - 1024)) {
			count = subblockleft - 1024;

			/*
			 * But of course, we always want to be sector aligned.
			 */
			count = count & ~(secsize - 1);
		}

		cc = devread(infd, inbuf, count);
		if (cc < 0) {
			perror("reading input file");
			exit(1);
		}
		
		if (cc == 0) {
			/*
			 * If hit the end of the file, then finish off
			 * the compression.
			 */
			finish = 1;
			break;
		}

		if (cc != count && !eof) {
			fprintf(stderr, "Bad count in read, %d != %d at %qu\n",
				cc, count, off+total);
			exit(1);
		}

		/*
		 * Apply fixups.  This may produce a relocation record.
		 */
		if (fixups != NULL)
			applyfixups(off+total, count, inbuf);

		size  -= cc;
		total += cc;

		outsize = SUBBLOCKSIZE - buffer_offset;

		/* XXX match behavior of original compressor */
		if (oldstyle && outsize > 0x20000)
			outsize = 0x20000;

		d_stream.next_in   = inbuf;
		d_stream.avail_in  = cc;
		d_stream.next_out  = &output_buffer[buffer_offset];
		d_stream.avail_out = outsize;
		assert(d_stream.avail_out > 0);

		err = deflate(&d_stream, Z_SYNC_FLUSH);
		CHECK_ZLIB_ERR(err, "deflate");

		if (d_stream.avail_in != 0 ||
		    (!oldstyle && d_stream.avail_out == 0)) {
			fprintf(stderr, "Something went wrong, ");
			if (d_stream.avail_in)
				fprintf(stderr, "not all input deflated!\n");
			else
				fprintf(stderr, "too much data for chunk!\n");
			exit(1);
		}
		count = outsize - d_stream.avail_out;
		buffer_offset += count;
		assert(buffer_offset <= SUBBLOCKSIZE);
		bytescompressed += cc - d_stream.avail_in;

		/*
		 * If we have reached the subblock maximum, then need
		 * to start a new compression block. In order to make
		 * this simpler, I do not allow a partial output
		 * buffer to be written to the file. No carryover to the
		 * next block, and thats nice. I also avoid anything
		 * being left in the input buffer. 
		 * 
		 * The downside of course is wasted space, since I have to
		 * quit early to avoid not having enough output space to
		 * compress all the input. How much wasted space is kinda
		 * arbitrary since I can just make the input size smaller and
		 * smaller as you get near the end, but there are diminishing
		 * returns as your write calls get smaller and smaller.
		 * See above where I compare count to subblockleft.
		 */
		subblockleft -= count;
		assert(subblockleft >= 0);
		
		if (subblockleft < 0x2000) {
			finish = 1;
			*full  = 1;
			break;
		}
	}
	if (finish) {
		compress_finish(subblksize);
		return total;
	}
	*subblksize = SUBBLOCKMAX - subblockleft;
	return total;
}

/*
 * Need a hook to finish off the last part and write the pending data.
 */
static int
compress_finish(uint32_t *subblksize)
{
	int		err, count;

	if (subblockleft == SUBBLOCKMAX)
		return 0;
	
	d_stream.next_in   = 0;
	d_stream.avail_in  = 0;
	d_stream.next_out  = &output_buffer[buffer_offset];
	d_stream.avail_out = SUBBLOCKSIZE - buffer_offset;

	err = deflate(&d_stream, Z_FINISH);
	if (err != Z_STREAM_END)
		CHECK_ZLIB_ERR(err, "deflate");

	assert(d_stream.avail_out > 0);

	/*
	 * There can be some left even though we use Z_SYNC_FLUSH!
	 */
	count = (SUBBLOCKSIZE - buffer_offset) - d_stream.avail_out;
	if (count) {
		buffer_offset += count;
		assert(buffer_offset <= SUBBLOCKSIZE);
		subblockleft -= count;
		assert(subblockleft >= 0);
	}

	err = deflateEnd(&d_stream);
	CHECK_ZLIB_ERR(err, "deflateEnd");

	/*
	 * The caller needs to know how big the actual data is.
	 */
	*subblksize  = SUBBLOCKMAX - subblockleft;
		
	/*
	 * Pad the subblock out.
	 */
	assert(buffer_offset + subblockleft <= SUBBLOCKSIZE);
	memset(&output_buffer[buffer_offset], 0, subblockleft);
	buffer_offset += subblockleft;
	subblockleft = SUBBLOCKMAX;
	return 1;
}
