/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Usage: imagedump <input file>
 *
 * Prints out information about an image.
 */

#ifdef WITH_HASH
/*
 * This enables a very specific command line option (-H) for printing out
 * an MD5 hash for every chunk of an image.  I put this in just to get a
 * sense of how common shared chunks between images are (i.e., could a
 * frisbee server that serves multiple images take advantage of this to
 * significant effect).
 */
#define WITH_HASHCMD
#endif

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <zlib.h>
#include <sys/stat.h>
#ifdef WITH_HASHCMD
#include <openssl/sha.h>
#include <openssl/md5.h>
#endif
#include "imagehdr.h"
#include "checksum.h"

static int detail = 0;
static int dumpmap = 0;
static int ignorev1 = 0;
static int checksums = 0; // On by default?
static int infd = -1;
static char *chkpointdev;
static int dumphash = 0;

static unsigned long long wasted;
static uint32_t sectinuse;
static uint32_t sectfree;
static uint32_t relocs;
static unsigned long long relocbytes;

static void usage(void);
static int dumpfile(char *name, int fd);
static int dumpchunk(char *name, char *buf, int chunkno, int checkindex);
#ifdef WITH_HASHCMD
static void dumpchunkhash(char *name, char *buf, int chunkno, int checkindex);
#endif

#define SECTOBYTES(s)	((unsigned long long)(s) * SECSIZE)

int
main(int argc, char **argv)
{
	int ch, version = 0;
	extern char build_info[];
	int errors = 0;

	while ((ch = getopt(argc, argv, "C:dimvHc")) != -1)
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

	for (; argc > 0; argc--, argv++) {
		int isstdin = !strcmp(argv[0], "-");

		if (!isstdin) {
			if ((infd = open(argv[0], O_RDONLY, 0666)) < 0) {
				perror(argv[0]);
				continue;
			}
		} else
			infd = fileno(stdin);

#ifdef WITH_CRYPTO
#ifdef SIGN_CHECKSUM
		if (checksums > 0) {
			char *keyfile = checksum_keyfile(argv[0]);

			if (!init_checksum(keyfile)) {
				fprintf(stderr,
					"%s: Cannot validate checksum signing key\n",
					argv[0]);
				continue;
			}
		}
#endif
#endif

		errors = dumpfile(isstdin ? "<stdin>" : argv[0], infd);

#ifdef WITH_CRYPTO
#ifdef SIGN_CHECKSUM
		if (checksums > 0)
			cleanup_checksum();
#endif
#endif

		if (!isstdin)
			close(infd);
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
		" -c              Verify chunk checksums\n");
	exit(1);
}

static char chunkbuf[CHUNKSIZE];
static unsigned int magic;
static unsigned long chunkcount;
static uint32_t nextsector;
static uint32_t fmax, fmin, franges, amax, amin, aranges;
static uint32_t adist[8]; /* <4k, <8k, <16k, <32k, <64k, <128k, <256k, >=256k */
static int regmax, regmin;
static uint32_t losect, hisect;
static uint8_t imageid[UUID_LENGTH];
static int sigtype, enctype;

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
	hisect = 0;
	losect = ~0;

	fmax = amax = 0;
	fmin = amin = ~0;
	franges = aranges = 0;
	regmin = regmax = 0;
	memset(adist, 0, sizeof(adist));

	if (!isstdin) {
		struct stat st;

		if (fstat(fd, &st) < 0) {
			perror(name);
			return 1;
		}
		if ((st.st_size % CHUNKSIZE) != 0)
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
			if ((filesize / CHUNKSIZE) != chunkcount) {
				if (chunkcount != 0) {
					if (isstdin)
						filesize = (off_t)chunkcount *
							CHUNKSIZE;
					else
						printf("%s: WARNING: file size "
						       "inconsistant with "
						       "chunk count "
						       "(%lu != %lu)\n",
						       name,
						       (unsigned long)
						       (filesize/CHUNKSIZE),
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

			if (!dumphash) {
				printf("%s: %llu bytes, %lu chunks, version %d",
				       name, (unsigned long long)filesize,
				       (unsigned long)(filesize / CHUNKSIZE),
				       magic - COMPRESSED_MAGIC_BASE + 1);
				if (magic >= COMPRESSED_V4) {
					sigtype = hdr->csum_type;
					if (sigtype != CSUM_NONE) {
						printf(", ");
						if (sigtype & CSUM_SIGNED)
							printf("signed ");
						printf("csum (0x%x)", sigtype);
					}
					enctype = hdr->enc_cipher;
					if (enctype != ENC_NONE)
						printf(", encrypted (%d)",
						       enctype);
					memcpy(imageid, hdr->imageid,
					       UUID_LENGTH);
					if (detail > 0) {
						char idbuf[UUID_LENGTH*2+1];
						mem_to_hexstr(idbuf,
							      hdr->imageid,
							      UUID_LENGTH);
						printf("\n  uuid: %s", idbuf);
					}
				}
				printf("\n");
			}
		} else if (chunkno == 1 && !ignorev1) {
			blockhdr_t *hdr = (blockhdr_t *)chunkbuf;

			/*
			 * If reading from stdin, we don't know til the
			 * second chunk whether blockindexes are being kept.
			 */
			if (isstdin && filesize == 0 && hdr->blockindex == 0)
				checkindex = 0;
		}

#ifdef WITH_HASHCMD
		if (dumphash)
			dumpchunkhash(name, chunkbuf, chunkno, checkindex);
		else
#endif
		if (dumpchunk(name, chunkbuf, chunkno, checkindex)) {
			errors++;
			break;
		}
	}
 done:

	if (filesize == 0)
		filesize = (off_t)(chunkno + 1) * CHUNKSIZE;

	cbytes = (unsigned long long)(filesize - wasted);
	dbytes = SECTOBYTES(sectinuse);
	tbytes = SECTOBYTES(sectinuse + sectfree);

	if (dumphash)
		return 0;

	if (detail > 0)
		printf("\n");

	printf("  %llu bytes of overhead/wasted space (%5.2f%% of image file)\n",
	       wasted, (double)wasted / filesize * 100);
	printf("  %d total regions: %.1f/%d/%d ave/min/max per chunk\n",
	       aranges, (double)aranges / (chunkno + 1), regmin, regmax);
	if (relocs)
		printf("  %d relocations covering %llu bytes\n",
		       relocs, relocbytes);
	if (hisect != ~0)
		printf("  covered sector range: [%u-%u]\n",
		       losect, hisect);
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
		uint32_t adistsum;

		printf("  %d allocated ranges: %llu/%llu/%llu ave/min/max size\n",
		       aranges, SECTOBYTES(sectinuse)/aranges,
		       SECTOBYTES(amin), SECTOBYTES(amax));
		printf("  size distribution:\n");
		adistsum = 0;
		maxsz = 4*SECSIZE;
		for (i = 0; i < 7; i++) {
			maxsz *= 2;
			if (adist[i]) {
				adistsum += adist[i];
				printf("    <  %3dk bytes: %6d %4.1f%% %4.1f%%\n",
				       maxsz/1024, adist[i],
				       (double)adist[i]/aranges*100,
				       (double)adistsum/aranges*100);
			}
		}
		if (adist[i]) {
			printf("    >= %3dk bytes: %6d %4.1f%%\n",
			       maxsz/1024, adist[i],
			       (double)adist[i]/aranges*100);
		}
	}

	return errors;
}

#ifdef WITH_HASHCMD
static char *
spewhash(unsigned char *h, int hlen)
{
	static char hbuf[16*2+1];
	static const char hex[] = "0123456789abcdef";
	int i;

	for (i = 0; i < hlen; i++) {
		hbuf[i*2] = hex[h[i] >> 4];
		hbuf[i*2+1] = hex[h[i] & 0xf];
	}
	hbuf[i*2] = '\0';
	return hbuf;
}


static void
dumpchunkhash(char *name, char *buf, int chunkno, int checkindex)
{
	unsigned char hash[16];
	MD5((unsigned char *)buf, CHUNKSIZE, hash);
	printf("%s %s %d\n", spewhash(hash, 16), name, chunkno);
}
#endif

static int
dumpchunk(char *name, char *buf, int chunkno, int checkindex)
{
	blockhdr_t *hdr;
	struct region *reg;
	uint32_t count;
	int i;

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
		if (chunkno > 0) {
			if (sigtype != hdr->csum_type) {
				printf("%s: wrong checksum type in chunk %d\n",
				       name, chunkno);
				return 1;
			}
			if (enctype != hdr->enc_cipher) {
				printf("%s: wrong cipher type in chunk %d\n",
				       name, chunkno);
				return 1;
			}
			if (memcmp(imageid, hdr->imageid, UUID_LENGTH)) {
				printf("%s: wrong image ID in chunk %d\n",
				       name, chunkno);
				return 1;
			}
		}
		if (checksums && hdr->csum_type != CSUM_NONE) {
			if ((hdr->csum_type & CSUM_TYPE) != CSUM_SHA1) {
				printf("%s: unsupported checksum type %d in "
				       "chunk %d", name,
				       (hdr->csum_type & CSUM_TYPE),
				       chunkno);
				return 1;
			}
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
	if (hdr->size > (CHUNKSIZE - hdr->regionsize)) {
		printf("%s: bad chunksize (%d > %d) in chunk %d\n",
		       name, hdr->size, CHUNKSIZE-hdr->regionsize, chunkno);
		return 1;
	}
#if 1
	/* include header overhead */
	wasted += CHUNKSIZE - hdr->size;
#else
	wasted += ((CHUNKSIZE - hdr->regionsize) - hdr->size);
#endif
	if (regmin == 0 || hdr->regioncount < regmin)
		regmin = hdr->regioncount;
	if (regmax == 0 || hdr->regioncount > regmax)
		regmax = hdr->regioncount;

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
		if (hdr->magic >= COMPRESSED_V4) {
			int len;

			if (hdr->csum_type != CSUM_NONE) {
				len = 0;
				switch (hdr->csum_type) {
				case CSUM_SIGNED|CSUM_SHA1:
					len = CSUM_MAX_LEN;
					break;
				case CSUM_SHA1:
					len = CSUM_SHA1_LEN;
					break;
				}
				if (len) {
					char csumstr[CSUM_MAX_LEN*2+1];

					mem_to_hexstr(csumstr,
						      hdr->checksum, len);
					printf("    Checksum: 0x%s", csumstr);
				}
				printf("\n");
			}

			if (hdr->enc_cipher != ENC_NONE) {
				len = 0;
				switch (hdr->enc_cipher) {
				case ENC_BLOWFISH_CBC:
					len = ENC_MAX_KEYLEN;
					break;
				}
				if (len) {
					char ivstr[ENC_MAX_KEYLEN*2+1];

					mem_to_hexstr(ivstr,
						      hdr->enc_iv, len);
					printf("    CipherIV: 0x%s", ivstr);
				}
				printf("\n");

			}
		}
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
			if (hdr->firstsect < losect)
				losect = hdr->firstsect;
			if (hdr->lastsect > hisect)
				hisect = hdr->lastsect;
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
			case RELOC_SHORTSECTOR:
				relocstr = "SHORTSECTOR";
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

#ifdef WITH_CRYPTO
	/*
	 * Checksum this image.  Assumes SHA1, because we check for this above.
	 */
	if (checksums && hdr->csum_type != CSUM_NONE) {
		if (!verify_checksum(hdr, (unsigned char *)buf,
				     hdr->csum_type)) {
			printf("ERROR: chunk %d fails checksum!\n", chunkno);
			return 1;
		}
	}
#endif

	return 0;
}
