/*
 * An image zipper.
 */
#include <err.h>
#include <fcntl.h>
#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
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

#define min(a,b) ((a) <= (b) ? (a) : (b))

/* Why is this not defined in a public header file? */
#define ACTIVE		0x80
#define BOOT_MAGIC	0xAA55
#ifndef DOSPTYP_LINSWP
#define	DOSPTYP_LINSWP	0x82	/* Linux swap partition */
#define	DOSPTYP_LINUX	0x83	/* Linux partition */
#define	DOSPTYP_EXT	5	/* DOS extended partition */
#endif

int	infd, outfd;
int	secsize	  = 512;	/* XXX bytes. */
int	debug	  = 0;
int     info      = 0;
int     slicemode = 0;
int     maxmode   = 0;
int     slice     = 0;
long	dev_bsize = 1;

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
	unsigned long	start;		/* In sectors */
	unsigned long	size;		/* In sectors */
	struct range	*next;
};
struct range	*ranges, *skips;
int		numranges, numskips;
void	addskip(unsigned long start, unsigned long size);
void	dumpskips(void);
void	sortskips(void);
void    makeranges(void);

/* Forward decls */
#ifndef linux
int	read_image(void);
int	read_bsdslice(int slice, u_int32_t start, u_int32_t size);
int	read_bsdpartition(struct disklabel *dlabel, int part);
int	read_bsdcg(struct fs *fsp, struct cg *cgp, unsigned int dboff);
#endif
int	read_linuxslice(int slice, u_int32_t start, u_int32_t size);
int	read_linuxswap(int slice, u_int32_t start, u_int32_t size);
int	read_linuxgroup(struct ext2_super_block *super,
			struct ext2_group_desc	*group,
			int index, u_int32_t diskoffset);
int	compress_image(void);
void	usage(void);

/* Map partition number to letter */
#define BSDPARTNAME(i)       ("ABCDEFGHIJK"[(i)])

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

	while ((ch = getopt(argc, argv, "lbdihrs:c:")) != -1)
		switch(ch) {
		case 'i':
			info++;
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
		case 'r':
			rawmode++;
			break;
		case 's':
			slicemode = 1;
			slice = atoi(optarg);
			break;
		case 'c':
			maxmode     = 1;
			inputmaxsec = atoi(optarg);
			break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

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

	if ((infd = open(argv[0], O_RDONLY, 0)) < 0) {
		perror("opening input file");
		exit(1);
	}

	if (linuxfs)
		rval = read_linuxslice(0, 0, 0);
#ifndef linux
	else if (bsdfs)
		rval = read_bsdslice(0, 0, 0);
	else if (rawmode)
		rval = read_raw();
	else
		rval = read_image();
#endif
	sortskips();
	if (debug)
		dumpskips();
	makeranges();

	if (info) {
		close(infd);
		exit(0);
	}

	if ((outfd = open(outfilename, O_RDWR|O_CREAT|O_TRUNC, 0666)) < 0) {
		perror("opening output file");
		exit(1);
	}
	compress_image();
	
	close(infd);
	close(outfd);
	exit(rval);
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

	if ((cc = read(infd, doslabel.pad2, DOSPARTSIZE)) < 0) {
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
		printf("DOS Partitions:\n");
		for (i = 0; i < NDOSPART; i++) {
			printf("  P%d: ", i + 1 /* DOS Numbering */);
			switch (doslabel.parts[i].dp_typ) {
			case DOSPTYP_386BSD:
				printf("  BSD   ");
				break;
			case DOSPTYP_LINSWP:
				printf("  LSWAP ");
				break;
			case DOSPTYP_LINUX:
				printf("  LINUX ");
				break;
			case DOSPTYP_EXT:
				printf("  DOS   ");
				break;
			default:
				printf("  UNKNO ");
			}
			printf("  start %9d, size %9d\n", 
			       doslabel.parts[i].dp_start,
			       doslabel.parts[i].dp_size);
		}
		printf("\n");
	}

	/*
	 * In slicemode, we need to set the bounds of compression.
	 * Slice is a DOS partition number (1-4). If not in slicemode,
	 * we have to set the bounds according to the doslabel since its
	 * possible that someone (Mike!) will create a disk with empty
	 * space before the first partition or after the last partition!
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
		
		switch (type) {
		case DOSPTYP_386BSD:
			rval = read_bsdslice(i, start, size);
			break;
		case DOSPTYP_LINSWP:
			rval = read_linuxswap(i, start, size);
			break;
		case DOSPTYP_LINUX:
			rval = read_linuxslice(i, start, size);
			break;
		}
		if (rval)
			return rval;
		
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
read_bsdslice(int slice, u_int32_t start, u_int32_t size)
{
	int		cc, i, rval = 0;
	union {
		struct disklabel	label;
		char			pad[BBSIZE];
	} dlabel;

	if (debug)
		printf("  P%d (BSD Slice)\n", slice + 1 /* DOS Numbering */);
	
	if (lseek(infd, ((off_t)start) * secsize, SEEK_SET) < 0) {
		warn("Could not seek to beginning of BSD slice");
		return 1;
	}

	/*
	 * Then seek ahead to the disklabel.
	 */
	if (lseek(infd, (off_t)(LABELSECTOR * secsize), SEEK_CUR) < 0) {
		warn("Could not seek to beginning of BSD disklabel");
		return 1;
	}

	if ((cc = read(infd, &dlabel, sizeof(dlabel))) < 0) {
		warn("Could not read BSD disklabel");
		return 1;
	}
	if (cc != sizeof(dlabel)) {
		warnx("Could not get the entire BSD disklabel");
 		return 1;
	}

	/*
	 * Be sure both magic numbers are correct.
	 */
	if (dlabel.label.d_magic  != DISKMAGIC ||
	    dlabel.label.d_magic2 != DISKMAGIC) {
		warnx("Wrong magic number is BSD disklabel");
 		return 1;
	}

	/*
	 * Now scan partitions.
	 */
	for (i = 0; i < MAXPARTITIONS; i++) {
		if (! dlabel.label.d_partitions[i].p_size)
			continue;

		if (dlabel.label.d_partitions[i].p_fstype == FS_UNUSED)
			continue;

		if (debug) {
			printf("    %c ", BSDPARTNAME(i));

			printf("  start %9d, size %9d\t(%s)\n",
			   dlabel.label.d_partitions[i].p_offset,
			   dlabel.label.d_partitions[i].p_size,
			   fstypenames[dlabel.label.d_partitions[i].p_fstype]);
		}

		rval = read_bsdpartition(&dlabel.label, i);
		if (rval)
			return rval;
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
		warnx("BSD Partition %d: Not a BSD Filesystem", part);
		return 1;
	}

	if (lseek(infd, ((off_t)offset * (off_t) secsize) + SBOFF,
		  SEEK_SET) < 0) {
		warnx("BSD Partition %d: Could not seek to superblock", part);
		return 1;
	}

	if ((cc = read(infd, &fs, SBSIZE)) < 0) {
		warn("BSD Partition %d: Could not read superblock", part);
		return 1;
	}
	if (cc != SBSIZE) {
		warnx("BSD Partition %d: Truncated superblock", part);
		return 1;
	}
 	if (fs.fs.fs_magic != FS_MAGIC) {
		warnx("BSD Partition %d: Bad magic number in superblock",part);
 		return (1);
 	}

	if (debug) {
		printf("        bfree %9d, size %9d\n",
		       fs.fs.fs_cstotal.cs_nbfree, fs.fs.fs_bsize);
	}

	for (i = 0; i < fs.fs.fs_ncg; i++) {
		unsigned long	cgoff, dboff;

		cgoff = fsbtodb(&fs.fs, cgtod(&fs.fs, i)) + offset;
		dboff = fsbtodb(&fs.fs, cgstart(&fs.fs, i)) + offset;

		if (lseek(infd, (off_t)cgoff * (off_t)secsize, SEEK_SET) < 0) {
			warn("BSD Partition %d: Could not seek to cg %d",
			     part, i);
			return 1;
		}
		if ((cc = read(infd, &cg, fs.fs.fs_bsize)) < 0) {
			warn("BSD Partition %d: Could not read cg %d",
			     part, i);
			return 1;
		}
		if (cc != fs.fs.fs_bsize) {
			warn("BSD Partition %d: Truncated cg %d", part, i);
			return 1;
		}
		if (debug > 1) {
			printf("        CG%d\t offset %9d, bfree %6d\n",
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
	 * The block bitmap lists blocks relative to the start of the
	 * cylinder group. cgdmin() is the first actual datablock, but
	 * the bitmap includes all the blocks used for all the blocks
	 * comprising the cg. These include superblock/cg/inodes/datablocks.
	 * The "dbstart" parameter is thus the beginning of the cg, to which
	 * we add the bitmap offset. All blocks before cgdmin() will always
	 * be allocated, but we scan them anyway. 
	 */

	if (debug > 2)
		printf("                 ");
	for (count = i = 0; i < max; i++)
		if (isset(p, i)) {
			j = i;
			while ((i+1)<max && isset(p, i+1))
				i++;
#if 1
			if (i != j && i-j >= 31) {
#else
			if (i-j >= 0) {
#endif
				unsigned long	dboff =
					dbstart + fsbtodb(fsp, j);

				unsigned long	dbcount =
					fsbtodb(fsp, (i-j) + 1);
					
				if (debug > 2) {
					if (count)
						printf(",%s",
						       count % 4 ? " " :
						       "\n                 ");
					printf("%u:%d", dboff, dbcount);
				}
				addskip(dboff, dbcount);
				count++;
			}
		}
	if (debug > 2)
		printf("\n");
	return 0;
}
#endif

/*
 * Operate on a linux slice. I actually don't have a clue what a linux
 * slice looks like. I just now that in our images, the linux slice
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

	int		cc, i, numgroups, rval = 0;
	union {
		struct ext2_super_block super;
		char pad[LINUX_SUPERBLOCK_SIZE];
	} fs;
	union {
		struct ext2_group_desc	groupdesc;
		char pad[LINUX_SUPERBLOCK_SIZE];
	} group;

	if (debug)
		printf("  P%d (Linux Slice)\n", slice + 1 /* DOS Numbering */);
	
	/*
	 * Skip ahead to the superblock.
	 */
	if (lseek(infd, (((off_t)start) * secsize) +
		  LINUX_SUPERBLOCK_OFFSET, SEEK_SET) < 0) {
		warnx("Linux Slice %d: Could not seek to superblock", slice);
		return 1;
	}

	if ((cc = read(infd, &fs, LINUX_SUPERBLOCK_SIZE)) < 0) {
		warn("Linux Slice %d: Could not read superblock", slice);
		return 1;
	}
	if (cc != LINUX_SUPERBLOCK_SIZE) {
		warnx("Linux Slice %d: Truncated superblock", slice);
		return 1;
	}
 	if (fs.super.s_magic != EXT2_SUPER_MAGIC) {
		warnx("Linux Slice %d: Bad magic number in superblock", slice);
 		return (1);
 	}

	if (debug) {
		printf("        bfree %9d, size %9d\n",
		       fs.super.s_free_blocks_count,
		       EXT2_BLOCK_SIZE(&fs.super));
	}
	numgroups = fs.super.s_blocks_count / fs.super.s_blocks_per_group;
	
	/*
	 * Read each group descriptor. It says where the free block bitmap
	 * lives. The absolute block numbers a group descriptor refers to
	 * is determined by its index * s_blocks_per_group. Once we know where
	 * the bitmap lives, we can go out to the bitmap and see what blocks
	 * are free.
	 */
	for (i = 0; i <= numgroups; i++) {
		if (lseek(infd, (((off_t)start) * secsize) +
			        ((off_t) (EXT2_BLOCK_SIZE(&fs.super) +
					  (i * sizeof(group.groupdesc)))),
			  SEEK_SET) < 0) {
			warnx("Linux Slice %d: "
			      "Could not seek to Group %d", slice, i);
			return 1;
		}

		if ((cc = read(infd, &group, sizeof(group))) < 0) {
			warn("Linux Slice %d: "
			     "Could not read Group %d", slice, i);
			return 1;
		}
		if (cc != sizeof(group)) {
			warnx("Linux Slice %d: Truncated Group %d", slice, i);
			return 1;
		}
		
		printf("        Group:%-2d\tBitmap %9d, bfree %9d\n", i,
		       group.groupdesc.bg_block_bitmap,
		       group.groupdesc.bg_free_blocks_count);

		rval = read_linuxgroup(&fs.super, &group.groupdesc, i, start);
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

	offset  = (off_t)sliceoffset * secsize;
	offset += (off_t)EXT2_BLOCK_SIZE(super) * group->bg_block_bitmap;
	if (lseek(infd, offset, SEEK_SET) < 0) {
		warn("Linux Group %d: "
		     "Could not seek to Group bitmap %d", i);
		return 1;
	}

	/*
	 * The bitmap is how big? Just make sure that its what we expect
	 * since I don't know what the rules are.
	 */
	if (EXT2_BLOCK_SIZE(super) != 0x1000 ||
	    super->s_blocks_per_group  != 0x8000) {
		warnx("Linux Group %d: "
		      "Bitmap size not what I expect it to be!", i);
		return 1;
	}

	if ((cc = read(infd, bitmap, sizeof(bitmap))) < 0) {
		warn("Linux Group %d: "
		     "Could not read Group bitmap", i);
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
		printf("                 ");
	for (count = i = 0; i < max; i++)
		if (!isset(p, i)) {
			j = i;
			while ((i+1)<max && !isset(p, i+1))
				i++;
			if (i != j && i-j >= 7) {
				unsigned long	dboff;
				int		dbcount;

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
						printf(",%s",
						       count % 4 ? " " :
						       "\n                 ");
					printf("%u:%d %d:%d",
					       dboff, dbcount, j, i);
				}
				addskip(dboff, dbcount);
				count++;
			}
		}
	if (debug > 2)
		printf("\n");

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
		printf("  P%d (Linux Swap)\n", slice + 1 /* DOS Numbering */);
		printf("        start %12d, size %9d\n",
		       start, size);
	}

	start += 0x8000 / secsize;
	size  -= 0x8000 / secsize;
	
	addskip(start, size);
	return 0;
}

/*
 * For a raw image (something we know nothing about), we report the size
 * and compress the entire thing (that is, there are no skip ranges).
 */
int
read_raw(void)
{
	off_t	size;

	if ((size = lseek(infd, (off_t) 0, SEEK_END)) < 0) {
		warn("lseeking to end of raw image");
		return 1;
	}

	if (debug) {
		printf("  Raw Image\n");
		printf("        start %12d, size %12qd\n", 0, size);
	}
	return 0;
}

char *usagestr = 
 "usage: imagezip [-idlbhr] [-s #] [-c #] <image | device> [outputfilename]\n"
 " -i              Info mode only. Do not write an output file\n"
 " -d              Turn on debugging. Multiple -d options increase output\n"
 " -r              A `raw' image. No FS compression is attempted\n"
 " -c count	   Compress <count> number of sectors (not with slice mode)\n"
 " -s slice        Compress a particular slice (DOS numbering 1-4)\n"
 " -h              Print this help message\n"
 " image | device  The input image or a device special file (ie: /dev/rad2)\n"
 " outputfilename  The output file, when -i is not specified\n"
 "\n"
 " Debugging options (not to be used by mere mortals!)\n"
 " -l              Linux slice only. Input must be a Linux slice image\n"
 " -b              FreeBSD slice only. Input must be a FreeBSD slice image\n";

void
usage()
{
	fprintf(stderr, usagestr);
	exit(1);
}

void
addskip(unsigned long start, unsigned long size)
{
	struct range	   *skip;

	if ((skip = (struct range *) malloc(sizeof(*skip))) == NULL) {
		fprintf(stderr, "Out of memory!\n");
		exit(1);
	}
	
	skip->start = start;
	skip->size  = size;
	skip->next  = skips;
	skips       = skip;
	numskips++;
}

/*
 * A very dumb bubblesort!
 */
void
sortskips(void)
{
	struct range	*pskip, tmp, *ptmp;
	int		changed = 1;

	if (!skips)
		return;
	
	while (changed) {
		changed = 0;

		pskip = skips;
		while (pskip) {
			if (pskip->next &&
			    pskip->start > pskip->next->start) {
				tmp.start = pskip->start;
				tmp.size  = pskip->size;

				pskip->start = pskip->next->start;
				pskip->size  = pskip->next->size;
				pskip->next->start = tmp.start;
				pskip->next->size  = tmp.size;

				changed = 1;
			}
			pskip  = pskip->next;
		}
	}

	/*
	 * No look for contiguous free regions and combine them.
	 */
	pskip = skips;
	while (pskip) {
	again:
		if (pskip->next &&
		    pskip->start + pskip->size == pskip->next->start) {
			pskip->size += pskip->next->size;
			
			ptmp        = pskip->next;
			pskip->next = pskip->next->next;
			free(ptmp);
			goto again;
		}
		pskip  = pskip->next;
	}
}

void
dumpskips(void)
{
	struct range	*pskip;
	unsigned long	total = 0;

	if (!skips)
		return;
	
	printf("Skip ranges (start/size) in sectors:\n");
	
	pskip = skips;
	while (pskip) {
		printf("  %12d    %9d\n", pskip->start, pskip->size);
		total += pskip->size;
		pskip  = pskip->next;
	}
	
	printf("\nTotal Number of Free Sectors: %d (bytes %qd)\n",
	       total, (off_t)total * (off_t)secsize);
}

/*
 * Life is easier if I think in terms of the valid ranges instead of
 * the free ranges. So, convert them.
 */
void
makeranges(void)
{
	struct range	*pskip, *ptmp, *range, *lastrange = 0;
	unsigned long	offset;
	
	if (!skips)
		return;

	if (inputminsec)
		offset = inputminsec;
	else
		offset = 0;

	pskip = skips;
	while (pskip) {
		if ((range = (struct range *)
		             malloc(sizeof(*range))) == NULL) {
			fprintf(stderr, "Out of memory!\n");
			exit(1);
		}
		if (! ranges)
			ranges = range;
			
		range->start = offset;
		range->size  = pskip->start - offset;
		range->next  = 0;
		offset       = pskip->start + pskip->size;
		
		if (lastrange)
			lastrange->next = range;
		lastrange = range;
		numranges++;

		ptmp  = pskip;
		pskip = pskip->next;
		free(ptmp);
	}
	/*
	 * Last piece.
	 */
	if ((range = (struct range *) malloc(sizeof(*range))) == NULL) {
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
	 * Mark the last range with 0.
	 */
	if (inputmaxsec)
		range->size = inputmaxsec - offset;
	else
		range->size = 0;
	lastrange->next = range;
	lastrange = range;
	numranges++;

	if (debug) {
		range = ranges;
		while (range) {
			printf("  %12d    %9d\n", range->start, range->size);
			range  = range->next;
		}
	}
}

/*
 * Compress the image.
 */
int
compress_image(void)
{
	int		cc, partial, i, count;
	off_t		fileregoff, inputoffset, size, outputoffset;
	off_t		tmpoffset, rangesize, totalinput = 0;
	struct range	*prange = ranges;
	struct blockhdr *blkhdr;
	struct region	*curregion, *regions;
	off_t		compress_chunk(off_t, int *, unsigned long *);
	int		compress_finish(unsigned long *subblksize);
	struct timeval  stamp, estamp;
	char		buf[DEFAULTREGIONSIZE];

	gettimeofday(&stamp, 0);

	bzero(buf, sizeof(buf));
	blkhdr    = (struct blockhdr *) buf;
	regions   = (struct region *) (blkhdr + 1);
	curregion = regions;

	/*
	 * No free blocks? Compress the entire input file. 
	 */
	if (!prange) {
		if (inputminsec)
			inputoffset = (off_t) inputminsec * (off_t) secsize;
		else
			inputoffset = (off_t) 0;

		if (lseek(infd, inputoffset, SEEK_SET) < 0) {
			warn("Could not seek to start of image");
			exit(1);
		}
		
		/*
		 * We keep a seperate output offset counter, which is always
		 * relative to zero since images are generally layed down in a
		 * partition or slice, and where it came from in the input
		 * image is irrelevant.
		 */
		outputoffset = (off_t) 0;
	again:
		/*
		 * Reserve room for the subblock hdr and the region pairs.
		 * We go back and fill it it later after the subblock is
		 * written and we know much input data was compressed into
		 * the block. We remember the offset so we can come back
		 * to it later.
		 */
		if ((fileregoff = lseek(outfd, (off_t) 0, SEEK_CUR)) < 0) {
			perror("remembering where block header goes");
			exit(1);
		}
		if (lseek(outfd, DEFAULTREGIONSIZE, SEEK_CUR) < 0) {
			perror("lseeking past block header and region pairs");
			exit(1);
		}
		
		if (debug) {
			printf("Compressing range: %14qd --> ", inputoffset);
			fflush(stdout);
		}

		/*
		 * A bug in FreeBSD causes lseek on a device special file to
		 * return 0 all the time! Well we want to be able to read
		 * directly out of a raw disk (/dev/rad0), so we need to
		 * use the compressor to figure out the actual size when it
		 * isn't known beforehand.
		 */
		if (inputmaxsec)
			size = ((inputmaxsec - inputminsec) * (off_t) secsize)
				- totalinput;
		else
			size = 0;

		size = compress_chunk(size, &partial, &blkhdr->size);
	
		if (debug)
			printf("%12qd\n", inputoffset + size);

		/*
		 * The last subblock is special if inputmaxsec is nonzero.
		 * In that case, we have to finish off the compression block
		 * since the compressor will not have known to do that.
		 */
		if (! partial)
			compress_finish(&blkhdr->size);

		/*
		 * Go back and stick in the block header and the region
		 * information. Since there are no skip ranges, there
		 * is but a single region and it covers the entire block.
		 */
		blkhdr->magic       = COMPRESSED_MAGIC;
		blkhdr->regionsize  = DEFAULTREGIONSIZE;
		blkhdr->regioncount = 1;
		curregion->start    = outputoffset / secsize;
		curregion->size     = size / secsize;
		curregion++;

		/*
		 * Remember where in the output file we are.
		 */
		if ((tmpoffset = lseek(outfd, (off_t) 0, SEEK_CUR)) < 0) {
			perror("seeking to remember output offset");
			exit(1);
		}
		if (lseek(outfd, (off_t) fileregoff, SEEK_SET) < 0) {
			perror("seeking back to block header offset");
			exit(1);
		}
		if ((cc = write(outfd, buf, sizeof(buf))) < 0) {
			perror("writing subblock header and regions");
			exit(1);
		}
		assert(cc == sizeof(buf));

		totalinput += size;
		if (partial) {
			if (lseek(outfd, (off_t) tmpoffset, SEEK_SET) < 0) {
				perror("seeking back to where we where!");
				exit(1);
			}
			
			outputoffset += size;
			inputoffset  += size;
			curregion     = regions;
			goto again;
		}

		/*
		 * We cannot allow the image to end on a non-sector boundry!
		 */
		if (totalinput & (secsize - 1)) {
			fprintf(stderr,
				"  Input image size (%qd) is not a multiple "
				"of the sector size (%d)\n",
				inputoffset, secsize);
			return 1;
		}
		goto done;
	}

	/*
	 * Loop through the image, compressing the stuff between the
	 * the free block ranges.
	 */

	/*
	 * Reserve room for the subblock hdr and the region pairs.
	 * We go back and fill it it later after the subblock is
	 * written and we know much input data was compressed into
	 * the block. We remember the offset so we can come back
	 * to it later.
	 */
	if ((fileregoff = lseek(outfd, (off_t) 0, SEEK_CUR)) < 0) {
		perror("remembering where block header goes");
		exit(1);
	}
	if (lseek(outfd, DEFAULTREGIONSIZE, SEEK_CUR) < 0) {
		perror("lseeking past block header and region pairs");
		exit(1);
	}
		
	while (prange) {
		inputoffset = (off_t) prange->start * (off_t) secsize;

		/*
		 * Seek to the beginning of the data range to compress.
		 */
		lseek(infd, (off_t) inputoffset, SEEK_SET);

		/*
		 * The amount to compress is the size of the range, which
		 * might be zero if its the last one (size unknown).
		 */
		rangesize = (off_t) prange->size * (off_t) secsize;

		/*
		 * Compress the chunk.
		 */
		if (debug) {
			printf("Compressing range: %14qd --> ", inputoffset);
			fflush(stdout);
		}

		size = compress_chunk(rangesize, &partial, &blkhdr->size);
	
		if (debug) {
			gettimeofday(&estamp, 0);
			estamp.tv_sec -= stamp.tv_sec;
			printf("%12qd in %ld seconds\n",
			       inputoffset + size, estamp.tv_sec);
		}

		/*
		 * This should never happen!
		 */
		if (size & (secsize - 1)) {
			fprintf(stderr, "  Not on a sector boundry at %qd\n",
				inputoffset);
			return 1;
		}

		/*
		 * If we managed to compress the entire range, we want
		 * to go to the next range.
		 */
		if (! partial) {
			assert(rangesize == 0 || size == rangesize);

			curregion->start = prange->start - inputminsec;
			curregion->size  = size / secsize;
			curregion++;

			prange = prange->next;
			continue;
		}

		/*
		 * A partial range. Well, maybe a partial range.
		 *
		 * Go back and stick in the block header and the region
		 * information. Since there are no skip ranges, there
		 * is but a single region and it covers the entire block.
		 */
		curregion->start = prange->start - inputminsec;
		curregion->size  = size / secsize;
		curregion++;

		blkhdr->magic       = COMPRESSED_MAGIC;
		blkhdr->regionsize  = DEFAULTREGIONSIZE;
		blkhdr->regioncount = (curregion - regions);

		/*
		 * Remember where in the output file we are.
		 */
		if ((tmpoffset = lseek(outfd, (off_t) 0, SEEK_CUR)) < 0) {
			perror("seeking to remember output offset");
			exit(1);
		}
		if (lseek(outfd, (off_t) fileregoff, SEEK_SET) < 0) {
			perror("seeking back to block header offset");
			exit(1);
		}
		if ((cc = write(outfd, buf, sizeof(buf))) < 0) {
			perror("writing subblock header and regions");
			exit(1);
		}
		assert(cc == sizeof(buf));

		if (lseek(outfd, (off_t) tmpoffset, SEEK_SET) < 0) {
			perror("seeking back to where we where!");
			exit(1);
		}
			
		/*
		 * Moving to the next block. Reserve the header area,
		 * remembering where we are now so we can come back
		 * later and write it.
		 */
		fileregoff = tmpoffset;
		if (lseek(outfd, DEFAULTREGIONSIZE, SEEK_CUR) < 0) {
			perror("lseeking past block header and region pairs");
			exit(1);
		}
		curregion = regions;

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
			unsigned long sectors = size / secsize;
			
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
		
		blkhdr->magic       = COMPRESSED_MAGIC;
		blkhdr->regionsize  = DEFAULTREGIONSIZE;
		blkhdr->regioncount = (curregion - regions);

		if (lseek(outfd, (off_t) fileregoff, SEEK_SET) < 0) {
			perror("seeking back to block header offset");
			exit(1);
		}
		if ((cc = write(outfd, buf, sizeof(buf))) < 0) {
			perror("writing subblock header and regions");
			exit(1);
		}
		assert(cc == sizeof(buf));
	}

	if (debug) {
		gettimeofday(&estamp, 0);
		estamp.tv_sec -= stamp.tv_sec;
		printf("Done in %ld seconds!\n", estamp.tv_sec);
	}

	/*
	 * Get the total filesize, and then number the subblocks.
	 * Useful, for netdisk.
	 */
	if ((tmpoffset = lseek(outfd, (off_t) 0, SEEK_END)) < 0) {
		perror("seeking to get output file size");
		exit(1);
	}
	count = tmpoffset / SUBBLOCKSIZE;
	printf("%qd %d\n", tmpoffset, count);
	for (i = 0, outputoffset = 0; i < count;
	     i++, outputoffset += SUBBLOCKSIZE) {
		
		if (lseek(outfd, (off_t) outputoffset, SEEK_SET) < 0) {
			perror("seeking to read block header");
			exit(1);
		}
		if ((cc = read(outfd, buf, sizeof(struct blockhdr))) < 0) {
			perror("reading subblock header");
			exit(1);
		}
		assert(cc == sizeof(struct blockhdr));
		if (lseek(outfd, (off_t) outputoffset, SEEK_SET) < 0) {
			perror("seeking to write new block header");
			exit(1);
		}
		blkhdr = (struct blockhdr *) buf;
		blkhdr->blockindex = i;
		blkhdr->blocktotal = count;
		
		if ((cc = write(outfd, buf, sizeof(struct blockhdr))) < 0) {
			perror("writing new subblock header");
			exit(1);
		}
		assert(cc == sizeof(struct blockhdr));
	}
	return 0;
}

/*
 * Compress a chunk. The next bit of input stream is read in and compressed
 * into the output file. 
 */
#define BSIZE		0x20000
static char		inbuf[BSIZE], outbuf[BSIZE];
static			int subblockleft = SUBBLOCKMAX;
static z_stream		d_stream;	/* Compression stream */

#define CHECK_ZLIB_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}

off_t
compress_chunk(off_t size, int *partial, unsigned long *subblksize)
{
	int		cc, count, err, eof, finish;
	off_t		total = 0;

	/*
	 * Whenever subblockleft equals SUBBLOCKMAX, it means that a new
	 * compression subblock needs to be started.
	 */
	if (subblockleft == SUBBLOCKMAX) {
		d_stream.zalloc = (alloc_func)0;
		d_stream.zfree  = (free_func)0;
		d_stream.opaque = (voidpf)0;

		err = deflateInit(&d_stream, 4);
		CHECK_ZLIB_ERR(err, "deflateInit");
	}
	*partial = 0;
	finish   = 0;

	/*
	 * If no size, then we want to compress until the end of file
	 * (and report back how much).
	 */
	if (!size) {
		eof  = 1;
		size = QUAD_MAX;
	}

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
		if (count > subblockleft) {
			count = subblockleft - secsize;

			/*
			 * But of course, we always want to be sector aligned.
			 */
			count = count & ~(secsize - 1);
		}

		cc = read(infd, inbuf, count);
		if (cc < 0) {
			perror("reading input file");
			exit(1);
		}
		size  -= cc;
		total += cc;
		
		if (cc == 0) {
			/*
			 * If hit the end of the file, then finish off
			 * the compression.
			 */
			finish = 1;
			break;
		}

		if (cc != count && !eof) {
			fprintf(stderr, "Bad count in read!\n");
			exit(1);
		}

		d_stream.next_in   = inbuf;
		d_stream.avail_in  = cc;
		d_stream.next_out  = outbuf;
		d_stream.avail_out = BSIZE;

		err = deflate(&d_stream, Z_SYNC_FLUSH);
		CHECK_ZLIB_ERR(err, "deflate");

		if (d_stream.avail_in) {
			fprintf(stderr, "Something went wrong!\n");
			exit(1);
		}
		count = BSIZE - d_stream.avail_out;

		if ((cc = write(outfd, outbuf, count)) < 0) {
			perror("writing output file");
			exit(1);
		}
		assert(cc == count);

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
			finish   = 1;
			*partial = 1;
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
int
compress_finish(unsigned long *subblksize)
{
	int		err, count, cc;

	if (subblockleft == SUBBLOCKMAX)
		return 0;
	
	d_stream.next_in   = 0;
	d_stream.avail_in  = 0;
	d_stream.next_out  = outbuf;
	d_stream.avail_out = BSIZE;
	err = deflate(&d_stream, Z_FINISH);
	if (err != Z_STREAM_END)
		CHECK_ZLIB_ERR(err, "deflate");

	/*
	 * There can be some left even though we use Z_SYNC_FLUSH!
	 */
	count = BSIZE - d_stream.avail_out;
	if (count) {
		if ((cc = write(outfd, outbuf, count)) < 0) {
			perror("writing output file");
			exit(1);
		}
		assert(cc == count);
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
	 * Pad the subblock out. Silly. 
	 */
	bzero(outbuf, sizeof(outbuf));
	while (subblockleft) {
		if (subblockleft > sizeof(outbuf))
			count = sizeof(outbuf);
		else
			count = subblockleft;
			
		if ((cc = write(outfd, outbuf, count)) < 0) {
			perror("writing output file");
			exit(1);
		}
		assert(cc == count);
		subblockleft -= count;
	}

	subblockleft = SUBBLOCKMAX;
	return 1;
}
