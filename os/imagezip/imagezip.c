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

/* Compression stuff, */
#define BSIZE	0x20000
char		inbuf[BSIZE], outbuf[BSIZE];
z_stream	d_stream;	/* Compression stream */

#define CHECK_ZLIB_ERR(err, msg) { \
    if (err != Z_OK) { \
        fprintf(stderr, "%s error: %d\n", msg, err); \
        exit(1); \
    } \
}
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
 * A list of zero ranges. We build them up as we go and then spit them
 * out (to what, I don't know yet) at the end.
 */
struct zero {
	unsigned long	start;		/* In sectors */
	unsigned long	size;		/* In sectors */
	struct zero	*next;
};
struct zero	*zeros;
int		numzeros;
void	addzero(unsigned long start, unsigned long size);
void	dumpzeros(void);
void	sortzeros(void);

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
	if (inputmaxsec && !rawmode) {
		fprintf(stderr, "Count option (-c) can only be used with "
			"the raw (-r) option\n\n");
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
	sortzeros();
	if (debug)
		dumpzeros();

	if (info) {
		close(infd);
		exit(0);
	}

	if ((outfd = open(outfilename, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0) {
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
	 * Slice is a DOS partition number (1-4).
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
		addzero(offset, size);
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
				addzero(dboff, dbcount);
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
	struct ext2_group_desc	groupdesc;

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
			  (EXT2_BLOCK_SIZE(&fs.super)+(i * sizeof(groupdesc))),
			  SEEK_SET) < 0) {
			warnx("Linux Slice %d: "
			      "Could not seek to Group %d", slice, i);
			return 1;
		}

		if ((cc = read(infd, &groupdesc, sizeof(groupdesc))) < 0) {
			warn("Linux Slice %d: "
			     "Could not read Group %d", slice, i);
			return 1;
		}
		if (cc != sizeof(groupdesc)) {
			warnx("Linux Slice %d: Truncated Group %d", slice, i);
			return 1;
		}
		
		printf("        Group:%-2d\tBitmap %9d, bfree %9d\n", i,
		       groupdesc.bg_block_bitmap,
		       groupdesc.bg_free_blocks_count);

		rval = read_linuxgroup(&fs.super, &groupdesc, i, start);
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
		warnx("Linux Group %d: Truncated Group bitmap", i);
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
				addzero(dboff, dbcount);
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
	
	addzero(start, size);
	return 0;
}

/*
 * For a raw image (something we know nothing about), we report the size
 * and compress the entire thing (that is, there are no zero ranges).
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
 " -l              Linux slice only. Input must be a Linux slice image\n"
 " -b              FreeBSD slice only. Input must be a FreeBSD slice image\n"
 " -r              A `raw' image. No FS compression is attempted\n"
 " -c count	   Compress <count> number of sectors (raw mode only)\n"
 " -s slice        Compress a particular slice (DOS numbering 1-4)\n"
 " -h              Print this help message\n"
 " image | device  The input image or a device special file (ie: /dev/rad2)\n"
 " outputfilename  The output file, when -i is not specified\n";

void
usage()
{
	fprintf(stderr, usagestr);
	exit(1);
}

void
addzero(unsigned long start, unsigned long size)
{
	struct zero	   *zero;

	if ((zero = (struct zero *) malloc(sizeof(*zero))) == NULL) {
		fprintf(stderr, "Out of memory!\n");
		exit(1);
	}
	
	zero->start = start;
	zero->size  = size;
	zero->next  = zeros;
	zeros       = zero;
	numzeros++;
}

/*
 * A very dumb bubblesort!
 */
void
sortzeros(void)
{
	struct zero	*pzero, tmp, *ptmp;
	int		changed = 1;

	if (!zeros)
		return;
	
	while (changed) {
		changed = 0;

		pzero = zeros;
		while (pzero) {
			if (pzero->next &&
			    pzero->start > pzero->next->start) {
				tmp.start = pzero->start;
				tmp.size  = pzero->size;

				pzero->start = pzero->next->start;
				pzero->size  = pzero->next->size;
				pzero->next->start = tmp.start;
				pzero->next->size  = tmp.size;

				changed = 1;
			}
			pzero  = pzero->next;
		}
	}

	/*
	 * No look for contiguous free regions and combine them.
	 */
	pzero = zeros;
	while (pzero) {
	again:
		if (pzero->next &&
		    pzero->start + pzero->size == pzero->next->start) {
			pzero->size += pzero->next->size;
			
			ptmp        = pzero->next;
			pzero->next = pzero->next->next;
			free(ptmp);
			goto again;
		}
		pzero  = pzero->next;
	}
}

void
dumpzeros(void)
{
	struct zero	*pzero;
	unsigned long	total = 0;

	if (!zeros)
		return;
	
	printf("Zero ranges (start/size) in sectors:\n");
	
	pzero = zeros;
	while (pzero) {
		printf("  %12d    %9d\n", pzero->start, pzero->size);
		total += pzero->size;
		pzero  = pzero->next;
	}
	
	printf("\nTotal Number of Free Sectors: %d (bytes %qd)\n",
	       total, (off_t)total * (off_t)secsize);
}

/*
 * Compress the image.
 */
int
compress_image(void)
{
	int		regsize, cc, err;
	long long	filesize = 0;
	off_t		offset, size, regoff, regmin;
	struct zero	*pzero = zeros;
	struct imagehdr hdr;
	struct region	*regions, *curregion;
	off_t		compress_chunk(off_t size);
	struct timeval  stamp, estamp;

	gettimeofday(&stamp, 0);

	/*
	 * Now set up to do the compression.
	 */
	d_stream.zalloc = (alloc_func)0;
	d_stream.zfree = (free_func)0;
	d_stream.opaque = (voidpf)0;

	err = deflateInit(&d_stream, 4);
	CHECK_ZLIB_ERR(err, "deflateInit");

	/*
	 * Prepend room for the magic header. Padded to a 1K boundry.
	 */
	if ((cc = write(outfd, outbuf, IMAGEHDRMINSIZE)) < 0) {
		perror("writing initial output file");
		exit(1);
	}
	assert(cc == IMAGEHDRMINSIZE);

	/*
	 * Prepend room for the region pairs. Padded to a 1K boundry.
	 * We will go back and fill in the actual contents later.
	 */
	regsize   = roundup(sizeof(struct region)*(numzeros+2), REGIONMINSIZE);
	regions   = (struct region *) calloc(regsize, 1);
	curregion = regions;

	if (lseek(outfd, (off_t) regsize, SEEK_CUR) < 0) {
		perror("lseeking past region pairs");
		exit(1);
	}

	/*
	 * No free blocks? Compress the entire input file. Generate a single
	 * region to cover the entire image, for the unzippers.
	 */
	if (!pzero) {
		if (inputminsec)
			offset = (off_t) inputminsec * (off_t) secsize;
		else
			offset = (off_t) 0;
		
		if (lseek(infd, offset, SEEK_SET) < 0) {
			warn("Could not seek to start of image");
			exit(1);
		}
		
		if (debug) {
			printf("Compressing range: %d --> ", offset);
			fflush(stdout);
		}

		/*
		 * A bug in FreeBSD causes lseek on a device special file to
		 * return 0 all the time! Well we want to be able to read
		 * directly out of a raw disk (/dev/rad0), so we need to
		 * use the compressor to figure out the actual size when it
		 * isn't known beforehand.
		 */
		if (inputmaxsec) {
			size = (inputmaxsec - inputminsec) * (off_t) secsize;
			compress_chunk(size);
		}
		else
			size = compress_chunk(0);
	
		if (debug)
			printf("%12qd\n", offset + size);

		/*
		 * We cannot allow the image to end on a non-sector boundry!
		 */
		if (size & (secsize - 1)) {
			warnx("ABORTING!\n"
			      "  Input image size (%qd) is not a multiple of "
			      "the sector size (%d)\n", size, secsize);
			return 1;
		}

		/*
		 * One region covering the entire image. Note that the
		 * region is always relative to zero since images are
		 * generally layed down in a partition or slice, and where
		 * it came from in the input image is irrelavant.
		 */
		curregion->start = 0;
		curregion->size  = size / secsize;
		curregion++;
	
		goto done;
	}

	/*
	 * Loop through the image, compressing the stuff between the
	 * the free block ranges.
	 */
	if (inputminsec)
		offset = (off_t) inputminsec * (off_t) secsize;
	else
		offset = (off_t) 0;

	/*
	 * We keep a seperate region offset counter, which is always
	 * relative to zero since images are generally layed down in a
	 * partition or slice, and where it came from in the input
	 * image is irrelavant.
	 */
	regoff = 0;

	while (pzero) {
		/*
		 * Seek to the beginning of the data range to compress.
		 */
		lseek(infd, (off_t) offset, SEEK_SET);

		/*
		 * The amount to compress is the difference between the current
		 * input offset, and the start of the free range.
		 */
		size = (((off_t) pzero->start) * ((off_t) secsize)) - offset;
		assert(size);

		/*
		 * Compress the chunk.
		 */
		if (debug) {
			printf("Compressing range: %14qd --> %12qd ",
			       offset, offset + size);
			fflush(stdout);
		}
		compress_chunk(size);

		if (debug) {
			gettimeofday(&estamp, 0);
			estamp.tv_sec -= stamp.tv_sec;
			printf("  in %ld seconds\n", estamp.tv_sec);
		}

		/*
		 * And set the region info.
		 */
		curregion->start = regoff / secsize;
		curregion->size  = size   / secsize;
		curregion++;

		/*
		 * Move the input file pointer past the free range to
		 * the beginning of the next real data range.
		 */
		regmin  = offset;
		offset  = (off_t) pzero->start * (off_t) secsize;
		offset += (off_t) pzero->size  * (off_t) secsize;

		/*
		 * The region pointer (where the data gets layed down)
		 * is relative to where we started in the input image,
		 * so the difference between the start of the previous
		 * range and the start of this new range.
		 */
		regoff += offset - regmin;

		pzero  = pzero->next;
	}

	/*
	 * Lets not forget the last part of the image, after the last
	 * free range!
	 */
	if (lseek(infd, offset, SEEK_SET) < 0) {
		warn("Could not seek to last part of image");
		exit(1);
	}
	
	if (debug) {
		printf("Compressing range: %14qd --> ", offset);
		fflush(stdout);
	}

	/*
	 * A bug in FreeBSD causes lseek on a device special file to
	 * return 0 all the time! Well we want to be able to read
	 * directly out of a raw disk (/dev/rad0), so we need to
	 * use the compressor to figure out the actual size when it
	 * isn't known beforehand.
	 */
	if (inputmaxsec) {
		size = (inputmaxsec * (off_t) secsize) - offset;
		if (size)
			compress_chunk(size);
	}
	else
		size = compress_chunk(0);
	
	if (debug) {
		gettimeofday(&estamp, 0);
		estamp.tv_sec -= stamp.tv_sec;
		printf("%12qd ", offset + size);
		printf("  in %ld seconds\n", estamp.tv_sec);
	}

	/*
	 * We cannot allow the image to end on a non-sector boundry!
	 */
	if (size & (secsize - 1)) {
		warnx("ABORTING!\n"
		      "  Input image size (%qd) is not a multiple of the "
		      "sector size (%d)\n", size, secsize);
		return 1;
	}

	/*
	 * If there was nothing to compress in the last part, don't
	 * tack on a zero sized region.
	 */
	if (size) {
		curregion->start = regoff / secsize;
		curregion->size  = size   / secsize;
		curregion++;
	}

 done:
	/* Finish it off */
	d_stream.next_in   = 0;
	d_stream.avail_in  = 0;
	d_stream.next_out  = outbuf;
	d_stream.avail_out = BSIZE;
	err = deflate(&d_stream, Z_FINISH);
	if (err != Z_STREAM_END)
		CHECK_ZLIB_ERR(err, "deflate");
	
	err = deflateEnd(&d_stream);
	CHECK_ZLIB_ERR(err, "deflateEnd");

	/*
	 * Seek to the end to get the file size
	 */
	if ((offset = lseek(outfd, 0L, SEEK_END)) < 0) {
		perror("seeking to end");
		exit(1);
	}
	filesize = offset - (IMAGEHDRMINSIZE + regsize);

	/*
	 * And back to the beggining to write the header
	 */
	if ((err = lseek(outfd, 0L, SEEK_SET)) < 0) {
		perror("seeking to end");
		exit(1);
	}

	hdr.filesize    = filesize;
	hdr.magic       = COMPRESSED_MAGIC;
	hdr.regionsize  = regsize;
	hdr.regioncount = (curregion - regions);

	if ((cc = write(outfd, &hdr, sizeof(hdr))) < 0) {
		perror("writing output file magic header");
		exit(1);
	}
	assert(cc == sizeof(hdr));

	/*
	 * Now skip ahead and write the region info.
	 */
	if ((err = lseek(outfd, (off_t) IMAGEHDRMINSIZE, SEEK_SET)) < 0) {
		perror("seeking to write region info");
		exit(1);
	}
	if ((cc = write(outfd, regions, regsize)) < 0) {
		perror("writing output file region information");
		exit(1);
	}
	assert(cc == regsize);

	if (debug) {
		printf("Done! %d bytes written ", d_stream.total_out);
		gettimeofday(&estamp, 0);
		estamp.tv_sec -= stamp.tv_sec;
		printf("in %ld seconds\n", estamp.tv_sec);
	}

	return 0;
}

/*
 * Compress a chunk. The next bit of input stream is read in and compressed
 * into the output file. 
 */
off_t
compress_chunk(off_t size)
{
	int		cc, count, err, eof;
	off_t		total = 0;

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

		cc = read(infd, inbuf, count);
		if (cc < 0) {
			perror("reading input file");
			exit(1);
		}
		size  -= cc;
		total += cc;
		
		if (cc == 0)
			break;

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

		if ((cc = write(outfd, outbuf, BSIZE - d_stream.avail_out))
		    < 0) {
			perror("writing output file");
			exit(1);
		}
		assert(cc == (BSIZE - d_stream.avail_out));
	}
	return total;
}
