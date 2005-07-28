/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "mezz.h"

static void usage(void)
{
    fprintf(stderr,
	    "Usage: mezzdump [-hbo] mezzfile\n"
	    "\n"
	    "Dump information about a mezzanine IPC file.\n"
	    "\n"
	    "Options:\n"
	    "  -h\t\tPrint this message\n"
	    "  -b\t\tPrint the blob list\n"
	    "  -o\t\tPrint the object list\n");
}

int main(int argc, char *argv[])
{
    int dump_blobs = 0, dump_objects = 0;
    int c, fd, retval = EXIT_SUCCESS;
    mezz_mmap_t *mm;

    while ((c = getopt(argc, argv, "hbo")) != -1) {
	switch (c) {
	case 'b':
	    dump_blobs = 1;
	    break;
	case 'o':
	    dump_objects = 1;
	    break;
	case 'h':
	case '?':
	default:
	    usage();
	    exit(0);
	    break;
	}
    }

    argv += optind;
    argc -= optind;
    
    if (argc == 0) {
        fprintf(stderr, "error: missing mezzanine file argument\n");
        usage();
        exit(1);
    }

    if ((fd = open(argv[0], O_RDONLY)) == -1) {
	perror("open");
    }
    else if ((mm = mmap(NULL,
			sizeof(mezz_mmap_t),
			PROT_READ,
			MAP_SHARED,
			fd,
			0)) == NULL) {
	perror("mmap");
    }
    else {
	printf("Time: %f\n"
	       "Frame count: %d\n"
	       "Calibrate: %d\n"
	       "Save: %d\n"
	       "Dimensions: %dx%d\n"
	       "Pixel area: %d\n"
	       "Depth: %d\n",
	       mm->time,
	       mm->count,
	       mm->calibrate,
	       mm->save,
	       mm->width, mm->height,
	       mm->area,
	       mm->depth);

	if (dump_blobs) {
	    int lpc;

	    for (lpc = 0; lpc < mm->bloblist.count; lpc++) {
		mezz_blob_t *mb = &mm->bloblist.blobs[lpc];

		printf("Blob[%d]:\n"
		       "  Class: %d\n"
		       "  Dimensions: %d,%d x %d,%d\n"
		       "  Pixel area: %d\n"
		       "  Image coords: %.2f %.2f\n"
		       "  World coords: %.2f %.2f\n"
		       "  Object: %d\n",
		       lpc,
		       mb->class,
		       mb->min_x, mb->min_y, mb->max_x, mb->max_y,
		       mb->area,
		       mb->ox, mb->oy,
		       mb->wox, mb->woy,
		       mb->object);
	    }
	}
	
	if (dump_objects) {
	    int lpc;

	    for (lpc = 0; lpc < mm->objectlist.count; lpc++) {
		mezz_object_t *mo = &mm->objectlist.objects[lpc];

		if (mo->valid) {
		    printf("Object[%d]:\n"
			   "  Classes: %d %d\n"
			   "  Max displacement: %.2f\n"
			   "  Max separation: %.2f\n"
			   "  Max missed: %d\n"
			   "  Missed: %d\n"
			   "  Pose: %.2f,%.2f  %.2f\n",
			   lpc,
			   mo->class[0], mo->class[1],
			   mo->max_disp,
			   mo->max_sep,
			   mo->max_missed,
			   mo->missed,
			   mo->px, mo->py, mo->pa);
		}
	    }
	}
    }
    
    return retval;
}
