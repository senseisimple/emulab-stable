/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Usage: imagedump <input file>
 *
 * Prints out information about an image.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <zlib.h>
#include <sha.h>
#include <sys/stat.h>
#include "imagehdr.h"

static int detail = 0;
static int dumpmap = 0;
static int ignorev1 = 0;
static int checksums = 0; // On by default?
static int infd = -1;
static char *chkpointdev;

static unsigned long long wasted;
static uint32_t sectinuse;
static uint32_t sectfree;
static uint32_t relocs;
static unsigned long long relocbytes;

static void usage(void);
static int dumpfile(char *name, int fd);
static int dumpchunk(char *name, char *buf, int chunkno, int checkindex);

#ifdef WITH_SHD
void add_shdrange(u_int32_t start, u_int32_t size);
int write_shd(char *shddev);
int debug = 0;
#endif

#define SECTOBYTES(s)	((unsigned long long)(s) * SECSIZE)

int
main(int argc, char **argv)
{
	int ch, version = 0;
	extern char build_info[];
        int errors = 0;

	while ((ch = getopt(argc, argv, "C:dimvc")) != -1)
		switch(ch) {
		case 'd':
			detail++;
			break;
		case 'i':
			ignorev1++;
			break;
		case 'm':
			dumpmap++;
			detail = 0;
			break;
		case 'C':
			chkpointdev = optarg;
			break;
		case 'v':
			version++;
			break;
                case 'c':
                        checksums++;
                        break;
		case 'h':
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (version || detail) {
		fprintf(stderr, "%s\n", build_info);
		if (version)
			exit(0);
	}

	if (argc < 1)
		usage();

	while (argc > 0) {
		int isstdin = !strcmp(argv[0], "-");

		if (!isstdin) {
			if ((infd = open(argv[0], O_RDONLY, 0666)) < 0) {
				perror("opening input file");
				exit(1);
			}
		} else
			infd = fileno(stdin);

		errors = dumpfile(isstdin ? "<stdin>" : argv[0], infd);

		if (!isstdin)
			close(infd);
		argc--;
		argv++;
	}
	exit(errors);
}

static void
usage(void)
{
	fprintf(stderr, "usage: "
		"imagedump options <image filename> ...\n"
		" -v              Print version info and exit\n"
		" -d              Turn on progressive levels of detail\n"
		" -c              Once: check checksums, Twice: print too\n");
	exit(1);
}	

static char chunkbuf[SUBBLOCKSIZE];
static unsigned int magic;
static unsigned long chunkcount;
static uint32_t nextsector;
static uint32_t fmax, fmin, franges, amax, amin, aranges;
static uint32_t adist[8]; /* <4k, <8k, <16k, <32k, <64k, <128k, <256k, >=256k */

static int
dumpfile(char *name, int fd)
{
	unsigned long long tbytes, dbytes, cbytes;
	int count, chunkno, checkindex = 1;
	off_t filesize;
	int isstdin;
	char *bp;
        int errors = 0;

	isstdin = (fd == fileno(stdin));
	wasted = sectinuse = sectfree = 0;
	nextsector = 0;
	relocs = 0;
	relocbytes = 0;

	fmax = amax = 0;
	fmin = amin = ~0;
	franges = aranges = 0;
	memset(adist, 0, sizeof(adist));

	if (!isstdin) {
		struct stat st;

		if (fstat(fd, &st) < 0) {
			perror(name);
			return 1;
		}
		if ((st.st_size % SUBBLOCKSIZE) != 0)
			printf("%s: WARNING: "
			       "file size not a multiple of chunk size\n",
			       name);
		filesize = st.st_size;
	} else
		filesize = 0;

	for (chunkno = 0; ; chunkno++) {
		bp = chunkbuf;

		if (isstdin || checksums)
			count = sizeof(chunkbuf);
		else {
			count = DEFAULTREGIONSIZE;
			if (lseek(infd, (off_t)chunkno*sizeof(chunkbuf),
				  SEEK_SET) < 0) {
				perror("seeking on zipped image");
				return 1;
			}
		}

		/*
		 * Parse the file one chunk at a time.  We read the entire
		 * chunk and hand it off.  Since we might be reading from
		 * stdin, we have to make sure we get the entire amount.
		 */
		while (count) {
			int cc;
			
			if ((cc = read(infd, bp, count)) <= 0) {
				if (cc == 0)
					goto done;
				perror("reading zipped image");
				return 1;
			}
			count -= cc;
			bp += cc;
		}
		if (chunkno == 0) {
			blockhdr_t *hdr = (blockhdr_t *)chunkbuf;

			magic = hdr->magic;
			if (magic < COMPRESSED_MAGIC_BASE ||
			    magic > COMPRESSED_MAGIC_CURRENT) {
				printf("%s: bad version %x\n", name, magic);
				return 1;
			}

                        if (checksums && magic < COMPRESSED_V4) {
                            printf("%s: WARNING: -c given, but file version "
                                    "doesn't support checksums!\n",name);
                            checksums = 0;
                        }

			if (ignorev1) {
				chunkcount = 0;
				checkindex = 0;
			} else
				chunkcount = hdr->blocktotal;
			if ((filesize / SUBBLOCKSIZE) != chunkcount) {
				if (chunkcount != 0) {
					if (isstdin)
						filesize = (off_t)chunkcount *
							SUBBLOCKSIZE;
					else
						printf("%s: WARNING: file size "
						       "inconsistant with "
						       "chunk count "
						       "(%lu != %lu)\n",
						       name,
						       (unsigned long)
						       (filesize/SUBBLOCKSIZE),
						       chunkcount);
				} else if (magic == COMPRESSED_V1) {
					if (!ignorev1)
						printf("%s: WARNING: "
						       "zero chunk count, "
						       "ignoring block fields\n",
						       name);
					checkindex = 0;
				}
			}

			printf("%s: %qu bytes, %lu chunks, version %d\n",
			       name, filesize,
			       (unsigned long)(filesize / SUBBLOCKSIZE),
			       hdr->magic - COMPRESSED_MAGIC_BASE + 1);
		} else if (chunkno == 1 && !ignorev1) {
			blockhdr_t *hdr = (blockhdr_t *)chunkbuf;

			/*
			 * If reading from stdin, we don't know til the
			 * second chunk whether blockindexes are being kept.
			 */
			if (isstdin && filesize == 0 && hdr->blockindex == 0)
				checkindex = 0;
		}

		if (dumpchunk(name, chunkbuf, chunkno, checkindex)) {
                        errors++;
			break;
                }
	}
 done:

#ifdef WITH_SHD
	if (chkpointdev && write_shd(chkpointdev)) {
		fprintf(stderr, "Could not record SHD alloc block info\n");
		exit(1);
	}
#endif

	if (filesize == 0)
		filesize = (off_t)(chunkno + 1) * SUBBLOCKSIZE;

	cbytes = (unsigned long long)(filesize - wasted);
	dbytes = SECTOBYTES(sectinuse);
	tbytes = SECTOBYTES(sectinuse + sectfree);

	if (detail > 0)
		printf("\n");

	printf("  %llu bytes of overhead/wasted space (%5.2f%% of image file)\n",
	       wasted, (double)wasted / filesize * 100);
	if (relocs)
		printf("  %d relocations covering %llu bytes\n",
		       relocs, relocbytes);
	printf("  %llu bytes of compressed data\n",
	       cbytes);
	printf("  %5.2fx compression of allocated data (%llu bytes)\n",
	       (double)dbytes / cbytes, dbytes);
	printf("  %5.2fx compression of total known disk size (%llu bytes)\n",
	       (double)tbytes / cbytes, tbytes);

	if (franges)
		printf("  %d free ranges: %llu/%llu/%llu ave/min/max size\n",
		       franges, SECTOBYTES(sectfree)/franges,
		       SECTOBYTES(fmin), SECTOBYTES(fmax));
	if (aranges) {
		int maxsz, i;

		printf("  %d allocated ranges: %llu/%llu/%llu ave/min/max size\n",
		       aranges, SECTOBYTES(sectinuse)/aranges,
		       SECTOBYTES(amin), SECTOBYTES(amax));
		printf("  size distribution:\n");
		maxsz = 4*SECSIZE;
		for (i = 0; i < 7; i++) {
			maxsz *= 2;
			if (adist[i])
				printf("    < %dk bytes: %d\n",
				       maxsz/1024, adist[i]);
		}
		if (adist[i])
			printf("    >= %dk bytes: %d\n", maxsz/1024, adist[i]);
	}

        return errors;
}

static int
dumpchunk(char *name, char *buf, int chunkno, int checkindex)
{
	blockhdr_t *hdr;
	struct region *reg;
        uint8_t *stored_digest = NULL;
        uint8_t *calc_digest = NULL;
        uint32_t digest_len = 0;
	uint32_t count;
	int i;
        SHA_CTX sha_ctx;

	hdr = (blockhdr_t *)buf;

	switch (hdr->magic) {
	case COMPRESSED_V1:
		reg = (struct region *)((struct blockhdr_V1 *)hdr + 1);
		break;
	case COMPRESSED_V2:
	case COMPRESSED_V3:
		reg = (struct region *)((struct blockhdr_V2 *)hdr + 1);
		break;
	case COMPRESSED_V4:
		reg = (struct region *)((struct blockhdr_V4 *)hdr + 1);
                if (checksums) {
                         if (hdr->checksumtype
                                           != CHECKSUM_SHA1) {
                                  printf("%s: unsupported checksum type %d in "
                                         "chunk %d", name,
                                         hdr->checksumtype,
                                         chunkno);
                        return 1;
                    }
                    digest_len = CHECKSUM_LEN_SHA1;
                    /*
                     * We save the digest (checksum) away, because we have to
                     * zero it out in the chunk to check
                     */
                    stored_digest = malloc(digest_len);
                    calc_digest = malloc(digest_len);
                    memcpy(stored_digest,hdr->checksum,
                           digest_len);
                }
		break;
	default:
		printf("%s: bad magic (%x!=%x) in chunk %d\n",
		       name, hdr->magic, magic, chunkno);
		return 1;
	}
	if (checkindex && hdr->blockindex != chunkno) {
		printf("%s: bad chunk index (%d) in chunk %d\n",
		       name, hdr->blockindex, chunkno);
		return 1;
	}
	if (chunkcount && hdr->blocktotal != chunkcount) {
		printf("%s: bad chunkcount (%d!=%lu) in chunk %d\n",
		       name, hdr->blocktotal, chunkcount, chunkno);
		return 1;
	}
	if (hdr->size > (SUBBLOCKSIZE - hdr->regionsize)) {
		printf("%s: bad chunksize (%d > %d) in chunk %d\n",
		       name, hdr->size, SUBBLOCKSIZE-hdr->regionsize, chunkno);
		return 1;
	}
#if 1
	/* include header overhead */
	wasted += SUBBLOCKSIZE - hdr->size;
#else
	wasted += ((SUBBLOCKSIZE - hdr->regionsize) - hdr->size);
#endif

	if (detail > 0) {
		printf("  Chunk %d: %u compressed bytes, ",
		       chunkno, hdr->size);
		if (hdr->magic > COMPRESSED_V1) {
			printf("sector range [%u-%u], ",
			       hdr->firstsect, hdr->lastsect-1);
			if (hdr->reloccount > 0)
				printf("%d relocs, ", hdr->reloccount);
		}
		printf("%d regions\n", hdr->regioncount);
	}
        if (checksums >= 2) {
                 printf("  Chunk %d: checksum 0x",chunkno);
                 for (i = 0; i < digest_len; i++) {
                          printf("%02x",stored_digest[i]);
                 }
                 printf("\n");
        }
	if (hdr->regionsize != DEFAULTREGIONSIZE)
		printf("  WARNING: "
		       "unexpected region size (%d!=%d) in chunk %d\n",
		       hdr->regionsize, DEFAULTREGIONSIZE, chunkno);

	for (i = 0; i < hdr->regioncount; i++) {
		if (detail > 1)
			printf("    Region %d: %u sectors [%u-%u]\n",
			       i, reg->size, reg->start,
			       reg->start + reg->size - 1);
		if (reg->start < nextsector)
			printf("    WARNING: chunk %d region %d "
			       "may overlap others\n", chunkno, i);
		if (reg->size == 0)
			printf("    WARNING: chunk %d region %d "
			       "zero-length region\n", chunkno, i);
		count = 0;
		if (hdr->magic > COMPRESSED_V1) {
			if (i == 0) {
				if (hdr->firstsect > reg->start)
					printf("    WARNING: chunk %d bad "
					       "firstsect value (%u>%u)\n",
					       chunkno, hdr->firstsect,
					       reg->start);
				else
					count = reg->start - hdr->firstsect;
			} else
				count = reg->start - nextsector;
			if (i == hdr->regioncount-1) {
				if (hdr->lastsect < reg->start + reg->size)
					printf("    WARNING: chunk %d bad "
					       "lastsect value (%u<%u)\n",
					       chunkno, hdr->lastsect,
					       reg->start + reg->size);
				else {
					if (count > 0) {
						sectfree += count;
						if (count < fmin)
							fmin = count;
						if (count > fmax)
							fmax = count;
						franges++;
					}
					count = hdr->lastsect -
						(reg->start+reg->size);
				}
			}
		} else
			count = reg->start - nextsector;
		if (count > 0) {
			sectfree += count;
			if (count < fmin)
				fmin = count;
			if (count > fmax)
				fmax = count;
			franges++;
		}

#ifdef WITH_SHD
		/*
		 * Accumulate SHD allocated list info
		 */
		if (chkpointdev)
			add_shdrange(reg->start, reg->size);
#endif

		count = reg->size;
		sectinuse += count;
		if (count < amin)
			amin = count;
		if (count > amax)
			amax = count;
		if (count < 8)
			adist[0]++;
		else if (count < 16)
			adist[1]++;
		else if (count < 32)
			adist[2]++;
		else if (count < 64)
			adist[3]++;
		else if (count < 128)
			adist[4]++;
		else if (count < 256)
			adist[5]++;
		else if (count < 512)
			adist[6]++;
		else
			adist[7]++;
		aranges++;

		if (dumpmap) {
			switch (hdr->magic) {
			case COMPRESSED_V1:
				if (reg->start - nextsector != 0)
					printf("F: [%08x-%08x]\n",
					       nextsector, reg->start-1);
				printf("A: [%08x-%08x]\n",
				       reg->start, reg->start + reg->size - 1);
				break;
			case COMPRESSED_V2:
			case COMPRESSED_V3:
				if (i == 0 && hdr->firstsect < reg->start)
					printf("F: [%08x-%08x]\n",
					       hdr->firstsect, reg->start-1);
				if (i != 0 && reg->start - nextsector != 0)
					printf("F: [%08x-%08x]\n",
					       nextsector, reg->start-1);
				printf("A: [%08x-%08x]\n",
				       reg->start, reg->start + reg->size - 1);
				if (i == hdr->regioncount-1 &&
				    reg->start+reg->size < hdr->lastsect)
					printf("F: [%08x-%08x]\n",
					       reg->start+reg->size,
					       hdr->lastsect-1);
				break;
			}
		}

		nextsector = reg->start + reg->size;
		reg++;
	}

	if (hdr->magic == COMPRESSED_V1)
		return 0;

	for (i = 0; i < hdr->reloccount; i++) {
		struct blockreloc *reloc = &((struct blockreloc *)reg)[i];

		relocs++;
		relocbytes += reloc->size;

		if (reloc->sector < hdr->firstsect ||
		    reloc->sector >= hdr->lastsect)
			printf("    WARNING: "
			       "Reloc %d at %u not in chunk [%u-%u]\n", i,
			       reloc->sector, hdr->firstsect, hdr->lastsect-1);
		if (detail > 1) {
			char *relocstr;

			switch (reloc->type) {
			case RELOC_FBSDDISKLABEL:
				relocstr = "FBSDDISKLABEL";
				break;
			case RELOC_OBSDDISKLABEL:
				relocstr = "OBSDDISKLABEL";
				break;
			case RELOC_LILOSADDR:
				relocstr = "LILOSADDR";
				break;
			case RELOC_LILOMAPSECT:
				relocstr = "LILOMAPSECT";
				break;
			case RELOC_LILOCKSUM:
				relocstr = "LILOCKSUM";
				break;
			default:
				relocstr = "??";
				break;
			}
			printf("    Reloc %d: %s sector %d, offset %u-%u\n", i,
			       relocstr, reloc->sector, reloc->sectoff,
			       reloc->sectoff + reloc->size);
		}
	}

        if (checksums) {
                 /*
                  * Checksum this image. Note: We first zero-out the checksum
                  * field of the header, because this is what it looked like
                  * when the checksum was taken
                  * Assumes SHA1, because we check for this above
                  */
                 memset(hdr->checksum,0,sizeof(hdr->checksum));
                 SHA1_Init(&sha_ctx);
                 SHA1_Update(&sha_ctx,buf,SUBBLOCKSIZE);
                 SHA1_Final(calc_digest,&sha_ctx);
                 for (i = 0; i < digest_len; i++) {
                          if (calc_digest[i] != stored_digest[i]) {
                                   printf("ERROR: chunk %d fails checksum!\n",
                                          chunkno);
                                   return 1;
                          }
                 }
                 free(calc_digest);
                 free(stored_digest);
        }

	return 0;
}
