/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2011 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Some simple common utility functions.
 */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#if !defined(linux) && !defined(__CYGWIN__)
#include <sys/sysctl.h>
#elif defined(linux)
#include <time.h>
#endif
#include <assert.h>

#include "decls.h"
#include "utils.h"

/*
 * Return current time in a string printable format. Caller must absorb
 * the string. 
 */
char *
CurrentTimeString(void)
{
	static char	buf[32];
	time_t		curtime;
	static struct tm tm;
	
	time(&curtime);
	strftime(buf, sizeof(buf), "%T", localtime_r(&curtime, &tm));

	return buf;
}

/*
 * Determine a sleep time based on the resolution of the clock.
 * If doround is set, the provided value is rounded up to the next multiple
 * of the clock resolution.  If not, the value is returned untouched but
 * an optional warning is printed.
 *
 * Note that rounding is generally a bad idea.  Say the kernel is 1 usec
 * past system tick N (1 tick == 1000usec).  If we round a value of 1/2
 * tick up to 1 tick and then call sleep, the kernel will add our 1 tick
 * to the current value to get a time slightly past tick N+1.  It will then
 * round that up to N+2, so we effectively sleep almost two full ticks.
 * But if we don't round the tick, then adding that to the current time
 * might yield a value less than N+1, which will round up to N+1 and we
 * will at worst sleep one full tick.
 */
int
sleeptime(unsigned int usecs, char *str, int doround)
{
	static unsigned int clockres_us;
	int nusecs;

	if (clockres_us == 0) {
#if !defined(linux) && !defined(__CYGWIN__)
		struct clockinfo ci;
		size_t cisize = sizeof(ci);

		ci.hz = 0;
		if (sysctlbyname("kern.clockrate", &ci, &cisize, 0, 0) == 0 &&
		    ci.hz > 0)
			clockres_us = ci.tick;
		else
#elif defined(linux)
		struct timespec res;

		if (clock_getres(CLOCK_REALTIME, &res) == 0 && res.tv_sec == 0) {
			/* Assume min resolution of 1000 usec, round to nearest usec */
			if (res.tv_nsec < 1000000)
				clockres_us = 1000;
			else
				clockres_us = (res.tv_nsec / 1000) * 1000;
		}
		else
#endif
		{
			warning("cannot get clock resolution, assuming 100HZ");
			clockres_us = 10000;
		}

		if (debug)
			log("clock resolution: %d us", clockres_us);
	}
	nusecs = ((usecs + clockres_us - 1) / clockres_us) * clockres_us;
	if (doround) {
		if (nusecs != usecs && str != NULL)
			warning("%s: rounded to %d from %d "
				"due to clock resolution", str, nusecs, usecs);
	} else {
		if (nusecs != usecs && str != NULL) {
			warning("%s: may be up to %d (instead of %d) "
				"due to clock resolution", str, nusecs, usecs);
		}
		nusecs = usecs;
	}

	return nusecs;
}

/*
 * Sleep. Basically wraps nanosleep, but since the threads package uses
 * signals, it keeps ending early!
 */
int
fsleep(unsigned int usecs)
{
	struct timespec time_to_sleep, time_not_slept;
	int	foo;

	time_to_sleep.tv_nsec  = (usecs % 1000000) * 1000;
	time_to_sleep.tv_sec   = usecs / 1000000;
	time_not_slept.tv_nsec = 0;
	time_not_slept.tv_sec  = 0;

	while ((foo = nanosleep(&time_to_sleep, &time_not_slept)) != 0) {
		if (foo < 0 && errno != EINTR) {
			return -1;
		}
		time_to_sleep.tv_nsec  = time_not_slept.tv_nsec;
		time_to_sleep.tv_sec   = time_not_slept.tv_sec;
		time_not_slept.tv_nsec = 0;
		time_not_slept.tv_sec  = 0;
	}
	return 0;
}

/*
 * Sleep til indicated time.
 * Returns 0 if it did not sleep.
 */
int
sleeptil(struct timeval *nexttime)
{
	struct timeval curtime;
	struct timespec todo, left;

	gettimeofday(&curtime, 0);
	if (!pasttime(&curtime, nexttime)) {
		subtime(&curtime, nexttime, &curtime);
		todo.tv_sec = curtime.tv_sec;
		todo.tv_nsec = curtime.tv_usec * 1000;
		left.tv_sec = left.tv_nsec = 0;
		while (nanosleep(&todo, &left) != 0) {
			if (errno != EINTR) {
				log("nanosleep failed\n");
				exit(1);
			}
			todo = left;
			left.tv_sec = left.tv_nsec = 0;
		}
		return 1;
	}
	return 0;
}

/*
 * Deal with variable chunk/block sizes
 */
static int _ChunkSize, _BlockSize;
static int _TotalChunks, _TotalBlocks;
static int _LastChunkSize, _LastBlockSize;

void
InitSizes(int32_t chunksize, int32_t blocksize, int64_t bytes)
{
	/* XXX no support for multiple chunk/block sizes yet */
	assert(chunksize == MAXCHUNKSIZE);
	assert(blocksize == MAXBLOCKSIZE);

	_ChunkSize = chunksize;
	_BlockSize = blocksize;
	_TotalBlocks = (bytes + _BlockSize-1) / _BlockSize;
	_TotalChunks = (_TotalBlocks + _ChunkSize-1) / _ChunkSize;

	/* how many bytes in last block (zero means full) */
	_LastBlockSize = bytes % _BlockSize;
	/* how many blocks in last chunk (zero means full) */
	_LastChunkSize = _TotalBlocks % _ChunkSize;
}
 
int
TotalChunks(void)
{
	return _TotalChunks;
}

/*
 * Return the size of a specific chunk.
 * Always the image chunk size (_ChunkSize) except possibly for a
 * final partial chunk.
 * If called with -1, return the image chunk size.
 */
int
ChunkSize(int chunkno)
{
	assert(chunkno < _TotalChunks);

	if (chunkno < _TotalChunks-1 || _LastChunkSize == 0)
		return _ChunkSize;
	return _LastChunkSize;
}

/*
 * Return the number of bytes in the indicated chunk.
 * Always the image chunk size in bytes (_ChunkSize * _BlockSize) except
 * possibly for a final partial chunk.
 */
int
ChunkBytes(int chunkno)
{
	assert(chunkno < _TotalChunks);

	if (chunkno < _TotalChunks-1 || _LastChunkSize == 0)
		return (_ChunkSize * _BlockSize);
	return ((_LastChunkSize-1) * _BlockSize + _LastBlockSize);
}

int
TotalBlocks(void)
{
	return _TotalBlocks;
}

/*
 * Return the size of a specific block.
 * Always the image block size (_BlockSize) except possibly for a
 * final partial block in the final chunk.
 * If called with -1, return the image block size.
 */
int
BlockSize(int chunkno, int blockno)
{
	int imageblock;

	assert(chunkno < _TotalChunks && blockno < _BlockSize);

	imageblock = chunkno * _ChunkSize + blockno;
	if (chunkno < 0 || imageblock < _TotalBlocks-1 || _LastBlockSize == 0)
		return _BlockSize;
	return _LastBlockSize;
}

void
BlockMapInit(BlockMap_t *blockmap, int block, int count)
{
	assert(block >= 0);
	assert(count > 0);
	assert(block < MAXCHUNKSIZE);
	assert(block + count <= MAXCHUNKSIZE);

	if (count == MAXCHUNKSIZE) {
		memset(blockmap->map, ~0, sizeof(blockmap->map));
		return;
	}
	memset(blockmap->map, 0, sizeof(blockmap->map));
	BlockMapAdd(blockmap, block, count);
}

void
BlockMapAdd(BlockMap_t *blockmap, int block, int count)
{
	int i, off;

	assert(block >= 0);
	assert(count > 0);
	assert(block < MAXCHUNKSIZE);
	assert(block + count <= MAXCHUNKSIZE);

	i = block / CHAR_BIT;
	off = block % CHAR_BIT;
	while (count > 0) {
		if (off == 0 && count >= CHAR_BIT) {
			blockmap->map[i++] = ~0;
			count -= CHAR_BIT;
		} else {
			blockmap->map[i] |= (1 << off);
			if (++off == CHAR_BIT) {
				i++;
				off = 0;
			}
			count--;
		}
	}
}

void
BlockMapClear(BlockMap_t *blockmap, int block, int count)
{
	int i, off;

	assert(block >= 0);
	assert(count > 0);
	assert(block < MAXCHUNKSIZE);
	assert(block + count <= MAXCHUNKSIZE);

	i = block / CHAR_BIT;
	off = block % CHAR_BIT;
	while (count > 0) {
		if (off == 0 && count >= CHAR_BIT) {
			blockmap->map[i++] = 0;
			count -= CHAR_BIT;
		} else {
			blockmap->map[i] &= ~(1 << off);
			if (++off == CHAR_BIT) {
				i++;
				off = 0;
			}
			count--;
		}
	}
}

/*
 * Mark the specified block as allocated and return the old value
 */
int
BlockMapAlloc(BlockMap_t *blockmap, int block)
{
	int i, off;

	assert(block >= 0);
	assert(block < MAXCHUNKSIZE);

	i = block / CHAR_BIT;
	off = block % CHAR_BIT;
	if ((blockmap->map[i] & (1 << off)) == 0) {
		blockmap->map[i] |= (1 << off);
		return 0;
	}
	return 1;
}

/*
 * Extract the next contiguous range of blocks, removing them from the map.
 * Returns the number of blocks extracted.
 */
int
BlockMapExtract(BlockMap_t *blockmap, int *blockp)
{
	int block, count, mask;
	int i, bit;

	assert(blockp != 0);

	/*
	 * Skip empty space at the front quickly
	 */
	for (i = 0; i < sizeof(blockmap->map); i++)
		if (blockmap->map[i] != 0)
			break;

	for (block = count = 0; i < sizeof(blockmap->map); i++) {
		for (bit = 0; bit < CHAR_BIT; bit++) {
			mask = 1 << bit;
			if ((blockmap->map[i] & mask) != 0) {
				blockmap->map[i] &= ~mask;
				if (count == 0)
					block = (i * CHAR_BIT) + bit;
				count++;
			} else {
				if (count > 0) {
					*blockp = block;
					return count;
				}
				if (blockmap->map[i] == 0)
					break;
			}
		}
	}
	if (count > 0)
		*blockp = block;

	return count;
}

/*
 * Return the number of blocks allocated in the range specified
 */
int
BlockMapIsAlloc(BlockMap_t *blockmap, int block, int count)
{
	int i, off, did = 0;
	char val;

	assert(block >= 0);
	assert(count > 0);
	assert(block < MAXCHUNKSIZE);
	assert(block + count <= MAXCHUNKSIZE);

	i = block / CHAR_BIT;
	off = block % CHAR_BIT;
	val = blockmap->map[i];
	while (count > 0) {
		/*
		 * Handle common aggregate cases
		 */
		if (off == 0 && count >= CHAR_BIT && (val == 0 || val == ~0)) {
			if (val)
				did += CHAR_BIT;
			count -= CHAR_BIT;
			off += CHAR_BIT;
		} else {
			if (val & (1 << off))
				did++;
			count--;
			off++;
		}
		if (off == CHAR_BIT && count > 0) {
			val = blockmap->map[++i];
			off = 0;
		}
	}
	return did;
}

void
BlockMapInvert(BlockMap_t *oldmap, BlockMap_t *newmap)
{
	int i;

	for (i = 0; i < sizeof(oldmap->map); i++)
		newmap->map[i] = ~(oldmap->map[i]);
}

/*
 * Merge any blocks from request frommap into tomap.
 */
int
BlockMapMerge(BlockMap_t *frommap, BlockMap_t *tomap)
{
	int i, bit, mask, did = 0;

	for (i = 0; i < sizeof(frommap->map); i++) {
		if (frommap->map[i] == 0 || tomap->map[i] == ~0 ||
		    frommap->map[i] == tomap->map[i])
			continue;
		for (bit = 0; bit < CHAR_BIT; bit++) {
			mask = 1 << bit;
			if ((frommap->map[i] & mask) != 0 &&
			    (tomap->map[i] & mask) == 0) {
				tomap->map[i] |= mask;
				did++;
			}
		}
	}

	return did;
}

/*
 * Return the block number of the first block allocated in the map.
 * Returns MAXCHUNKSIZE if no block is set.
 */
int
BlockMapFirst(BlockMap_t *blockmap)
{
	int block, i;

	assert(sizeof(blockmap->map) * CHAR_BIT == MAXCHUNKSIZE);

	/*
	 * Skip empty space at the front quickly
	 */
	for (i = 0; i < sizeof(blockmap->map); i++)
		if (blockmap->map[i] != 0)
			break;

	block = i * CHAR_BIT;
	if (i < sizeof(blockmap->map)) {
		int bit, mask;
		for (bit = 0; bit < CHAR_BIT; bit++) {
			mask = 1 << bit;
			if ((blockmap->map[i] & mask) != 0)
				return (block + bit);
		}
	}
	return block;
}

/*
 * Repeatedly apply the given function to each contiguous allocated range.
 * If the function returns non-zero, we stop processing at that point and
 * return.  This allows a "short-circuit" ability.
 *
 * Returns number of allocated blocks processed.
 */
int
BlockMapApply(BlockMap_t *blockmap, int chunk,
	      int (*func)(int, int, int, void *), void *arg)
{
	int block, count, mask;
	int i, bit, val;
	int did = 0;

	block = count = 0;
	for (i = 0; i < sizeof(blockmap->map); i++) {
		val = blockmap->map[i];
		for (bit = 0; bit < CHAR_BIT; bit++) {
			mask = 1 << bit;
			if ((val & mask) != 0) {
				val &= ~mask;
				if (count == 0)
					block = (i * CHAR_BIT) + bit;
				count++;
			} else {
				if (count > 0) {
					if (func &&
					    func(chunk, block, count, arg))
						return(did+count);
					did += count;
					count = 0;
				}
				if (val == 0)
					break;
			}
		}
	}
	if (count > 0) {
		if (func && func(chunk, block, count, arg))
			return (did+count);
		did += count;
	}

	return did;
}

#ifdef STATS
void
ClientStatsDump(unsigned int id, ClientStats_t *stats)
{
	int seconds;

	switch (stats->version) {
	case 1:
		/* compensate for start delay */
		stats->u.v1.runmsec -= stats->u.v1.delayms;
		while (stats->u.v1.runmsec < 0) {
			stats->u.v1.runsec--;
			stats->u.v1.runmsec += 1000;
		}

		log("Client %u Performance:", id);
		log("  runtime:                %d.%03d sec",
		    stats->u.v1.runsec, stats->u.v1.runmsec);
		log("  start delay:            %d.%03d sec",
		    stats->u.v1.delayms/1000, stats->u.v1.delayms%1000);
		seconds = stats->u.v1.runsec;
		if (seconds == 0)
			seconds = 1;
		log("  real data written:      %qu (%qu Bps)",
		    stats->u.v1.rbyteswritten,
		    stats->u.v1.rbyteswritten/seconds);
		log("  effective data written: %qu (%qu Bps)",
		    stats->u.v1.ebyteswritten,
		    stats->u.v1.ebyteswritten/seconds);
		log("Client %u Params:", id);
		log("  chunk/block size:     %d/%d",
		    CHUNKSIZE, BLOCKSIZE);
		log("  chunk buffers:        %d",
		    stats->u.v1.chunkbufs);
		if (stats->u.v1.writebufmem)
			log("  disk buffering:       %dMB",
			    stats->u.v1.writebufmem);
		log("  readahead/inprogress: %d/%d",
		    stats->u.v1.maxreadahead, stats->u.v1.maxinprogress);
		log("  recv timo/count:      %d/%d",
		    stats->u.v1.pkttimeout, stats->u.v1.idletimer);
		log("  re-request delay:     %d",
		    stats->u.v1.redodelay);
		log("  writer idle delay:    %d",
		    stats->u.v1.idledelay);
		log("  randomize requests:   %d",
		    stats->u.v1.randomize);
		log("Client %u Stats:", id);
		log("  net thread idle/blocked:        %d/%d",
		    stats->u.v1.recvidles, stats->u.v1.nofreechunks);
		log("  decompress thread idle/blocked: %d/%d",
		    stats->u.v1.nochunksready, stats->u.v1.decompblocks);
		log("  disk thread idle:        %d",
		    stats->u.v1.writeridles);
		log("  join/request msgs:       %d/%d",
		    stats->u.v1.joinattempts, stats->u.v1.requests);
		log("  dupblocks(chunk done):   %d",
		    stats->u.v1.dupchunk);
		log("  dupblocks(in progress):  %d",
		    stats->u.v1.dupblock);
		log("  partial requests/blocks: %d/%d",
		    stats->u.v1.prequests, stats->u.v1.lostblocks);
		log("  re-requests:             %d",
		    stats->u.v1.rerequests);
		break;

	default:
		log("Unknown stats version %d", stats->version);
		break;
	}
}
#endif

#ifdef MASTER_SERVER
#include "configdefs.h"

char *
GetMSError(int error)
{
	char *err;

	switch (error) {
	case 0:
		err = "no error";
		break;
	case MS_ERROR_FAILED:
		err = "server authentication error";
		break;
	case MS_ERROR_NOHOST:
		err = "unknown host";
		break;
	case MS_ERROR_NOIMAGE:
		err = "unknown image";
		break;
	case MS_ERROR_NOACCESS:
		err = "access not allowed";
		break;
	case MS_ERROR_NOMETHOD:
		err = "not available via specified method";
		break;
	case MS_ERROR_INVALID:
		err = "invalid argument";
		break;
	case MS_ERROR_TRYAGAIN:
		err = "image busy, try again later";
		break;
	case MS_ERROR_TOOBIG:
		err = "upload too large for target image";
		break;
	case MS_ERROR_BADMTIME:
		err = "bad image modification time";
		break;
	case MS_ERROR_NOTIMPL:
		err = "operation not implemented";
		break;
	default:
		err = "unknown error";
		break;
	}

	return err;
}

char *
GetMSMethods(int methods)
{
	static char mbuf[256];

	mbuf[0] = '\0';
	if (methods & MS_METHOD_UNICAST) {
		if (mbuf[0] != '\0')
			strcat(mbuf, "/");
		strcat(mbuf, "unicast");
	}
	if (methods & MS_METHOD_MULTICAST) {
		if (mbuf[0] != '\0')
			strcat(mbuf, "/");
		strcat(mbuf, "multicast");
	}
	if (methods & MS_METHOD_BROADCAST) {
		if (mbuf[0] != '\0')
			strcat(mbuf, "/");
		strcat(mbuf, "broadcast");
	}
	if (mbuf[0] == '\0')
		strcat(mbuf, "UNKNOWN");

	return mbuf;
}

/*
 * Print the contents of a GET reply to stdout.
 * The reply struct fields should be in HOST order; i.e., as returned
 * by ClientNetFindServer.
 */
void
PrintGetInfo(char *imageid, GetReply *reply, int raw)
{
	struct in_addr in;
	uint64_t isize;
	int len;

	if (raw) {
		printf("imageid=%s\n", imageid);
		printf("error=%d\n", reply->error);
		if (reply->error)
			return;

		printf("method=0x%x\n", reply->method);
		isize = ((uint64_t)reply->hisize << 32) | reply->losize;
		printf("size=%llu\n", (unsigned long long)isize);
		printf("sigtype=0x%x\n", reply->sigtype);
		switch (reply->sigtype) {
		case MS_SIGTYPE_MTIME:
			printf("sig=0x%x\n", *(uint32_t *)reply->signature);
			len = 0;
			break;
		case MS_SIGTYPE_MD5:
			len = 16;
			break;
		case MS_SIGTYPE_SHA1:
			len = 20;
			break;
		default:
			len = 0;
			break;
		}
		if (len > 0) {
			char sigbuf[MS_MAXSIGLEN*2+1], *sbp;
			int i;

			sbp = sigbuf;
			for (i = 0; i < len; i++) {
				sprintf(sbp, "%02x", reply->signature[i]);
				sbp += 2;
			}
			*sbp = '\0';
			printf("sig=0x%s\n", sigbuf);
		}
		printf("running=%d\n", reply->isrunning);
		if (reply->isrunning) {
			in.s_addr = htonl(reply->servaddr);
			printf("servaddr=%s\n", inet_ntoa(in));
			in.s_addr = htonl(reply->addr);
			printf("addr=%s\n", inet_ntoa(in));
			printf("port=%d\n", reply->port);
		}
		return;
	}

	if (reply->error) {
		printf("%s: server denied access: %s\n",
		    imageid, GetMSError(reply->error));
		return;
	}

	if (reply->isrunning) {
		in.s_addr = htonl(reply->addr);
		printf("%s: access OK, server running at %s:%d using %s\n",
		    imageid, inet_ntoa(in), reply->port,
		    GetMSMethods(reply->method));
	} else
		printf("%s: access OK, available methods=%s\n",
		    imageid, GetMSMethods(reply->method));

	isize = ((uint64_t)reply->hisize << 32) | reply->losize;
	printf("  size=%llu\n", (unsigned long long)isize);

	switch (reply->sigtype) {
	case MS_SIGTYPE_MTIME:
	{
		time_t mt = *(time_t *)reply->signature;
		printf("  modtime=%s\n", ctime(&mt));
	}
	default:
		break;
	}
}

/*
 * Print the contents of a PUT reply to stdout.
 * The reply struct fields should be in HOST order.
 */
void
PrintPutInfo(char *imageid, PutReply *reply, int raw)
{
	struct in_addr in;
	uint64_t isize;
	int len;

	if (raw) {
		printf("imageid=%s\n", imageid);
		printf("error=%d\n", reply->error);
		if (reply->error) {
			if (reply->error == MS_ERROR_TOOBIG) {
				isize = ((uint64_t)reply->himaxsize << 32) |
					reply->lomaxsize;
				printf("maxsize=%llu\n",
				       (unsigned long long)isize);
			}
			return;
		}

		printf("exists=%d\n", reply->exists);
		if (reply->exists) {
			isize = ((uint64_t)reply->hisize << 32) |
				reply->losize;
			printf("size=%llu\n", (unsigned long long)isize);
			printf("sigtype=0x%x\n", reply->sigtype);
			switch (reply->sigtype) {
			case MS_SIGTYPE_MTIME:
				printf("sig=0x%x\n",
				       *(uint32_t *)reply->signature);
				len = 0;
				break;
			case MS_SIGTYPE_MD5:
				len = 16;
				break;
			case MS_SIGTYPE_SHA1:
				len = 20;
				break;
			default:
				len = 0;
				break;
			}
			if (len > 0) {
				char sigbuf[MS_MAXSIGLEN*2+1], *sbp;
				int i;

				sbp = sigbuf;
				for (i = 0; i < len; i++) {
					sprintf(sbp, "%02x",
						reply->signature[i]);
					sbp += 2;
				}
				*sbp = '\0';
				printf("sig=0x%s\n", sigbuf);
			}
		}
		isize = ((uint64_t)reply->himaxsize << 32) | reply->lomaxsize;
		printf("maxsize=%llu\n", (unsigned long long)isize);
		in.s_addr = htonl(reply->addr);
		printf("servaddr=%s\n", inet_ntoa(in));
		in.s_addr = htonl(reply->addr);
		printf("addr=%s\n", inet_ntoa(in));
		printf("port=%d\n", reply->port);
		return;
	}

	if (reply->error) {
		printf("%s: server denied access: %s\n",
		    imageid, GetMSError(reply->error));
		if (reply->error == MS_ERROR_TOOBIG) {
			isize = ((uint64_t)reply->himaxsize << 32) |
				reply->lomaxsize;
			printf("  max allowed size=%llu\n",
			       (unsigned long long)isize);
		}
		return;
	}

	if (reply->exists) {
		printf("%s: currently exists\n", imageid);
		isize = ((uint64_t)reply->hisize << 32) | reply->losize;
		printf("  size=%llu\n", (unsigned long long)isize);
		switch (reply->sigtype) {
		case MS_SIGTYPE_MTIME:
		{
			time_t mt = *(time_t *)reply->signature;
			printf("  modtime=%s\n", ctime(&mt));
		}
		default:
			break;
		}
	}
	isize = ((uint64_t)reply->himaxsize << 32) | reply->lomaxsize;
	printf("  max allowed size=%llu\n", (unsigned long long)isize);
	in.s_addr = htonl(reply->addr);
	printf("%s: access OK, upload via %s:%d, maxsize=%llu\n",
	       imageid, inet_ntoa(in), reply->port,
	       (unsigned long long)isize);
}
#endif
