/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Support for extracting a changed block list from the shadow/checkpoint
 * device driver.
 *
 * Open the indicated device and make repeated ioctl calls to extract the
 * changed block info.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/ioctl.h>

#include "shd.h"
#include "sliceinfo.h"
#include "global.h"
#include "imagehdr.h"

#define	ENTRIESPERCALL	1024
#define FAKEIT

#ifdef FAKEIT
#define SHDIOCTL	fake_ioctl
int fake_ioctl(int, int, void *);
#else
#define SHDIOCTL	ioctl
#endif

int
read_shd(char *shddev, char *infile, int infd, u_int32_t ssect)
{
	int shdfd;
	struct shd_modinfo sm;

	sm.bufsiz = ENTRIESPERCALL;
	sm.buf = malloc(sm.bufsiz * sizeof(struct shd_range));
	if (sm.buf == 0) {
		fprintf(stderr, "No memory for SHD ranges\n");
		return 1;
	}

	/*
	 * Open the shd device so we can ioctl
	 */
	shdfd = open(shddev, O_RDONLY, 0);
	if (shdfd < 0) {
		perror(shddev);
		return 1;
	}

	/*
	 * Initialize the iterator (and return the first set of entries)
	 */
	sm.command = 1;
	if (SHDIOCTL(shdfd, SHDGETMODIFIEDRANGES, &sm) < 0) {
		perror(shddev);
		close(shdfd);
		return 1;
	}

	/*
	 * Loop extracting changed block ranges and creating imagezip
	 * block ranges.
	 */
	while (sm.retsiz > 0) {
		struct shd_range *sr = sm.buf;
		
		if (debug > 1)
			fprintf(stderr, "GETRANGES returns %ld ranges:\n",
				sm.retsiz);
		for (sr = sm.buf; sm.retsiz > 0; sr++, sm.retsiz--) {
			if (debug > 2)
				fprintf(stderr, "  %12d    %9d\n",
					sr->start, sr->size);
			addvalid(sr->start + ssect, sr->size);
		}

		sm.command = 2;
		if (SHDIOCTL(shdfd, SHDGETMODIFIEDRANGES, &sm) < 0) {
			perror(shddev);
			close(shdfd);
			/* XXX should flush the valid table */
			return 1;
		}

	}

	sm.command = 3;
	(void) SHDIOCTL(shdfd, SHDGETMODIFIEDRANGES, &sm);

	close(shdfd);
	return 0;
}

#ifdef FAKEIT

struct shd_range foo[] = {
        { 110427, 4 },
       { 2410487, 8 },
       { 2564819, 12 },
       { 2862827, 16 },
       { 2774027, 20 },
       { 4277895, 24 },
       { 3853567, 28 },
       { 2372447, 32 },
      { 12352615, 40 },
       { 5428223, 44 },
       { 2436383, 48 },
       { 2602463, 52 },
      { 11039151, 56 },
       { 4774459, 68 },
      { 10265207, 88 },
       { 4409343, 120 },
       { 4387795, 204 },
      { 11033303, 408 },
       { 9786815, 1040 },
       { 2361715, 1056 },
       { 8139631, 1752 },
       { 9795103, 11808 },
       { 4499231, 104368 },
      { 11832015, 112032 },
       { 9076719, 115560 },
       { 9307719, 123600 },
       { 5598591, 130240 },
	{ 0, 0 }
};

int
fake_ioctl(int fd, int cmd, void *data)
{
	struct shd_modinfo *sm = data;
	static struct shd_range *fooptr, *out;
	int i;

	switch (sm->command) {
	case 1:
		fooptr = foo;
	case 2:
		out = sm->buf;
		for (i = 0; i < sm->bufsiz; i++) {
			if (fooptr->start == 0 && fooptr->size == 0)
				break;
			*out++ = *fooptr++;
		}
		sm->retsiz = i;
		break;
	case 3:
		break;
	}

	return 0;
}
#endif
