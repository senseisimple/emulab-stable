
#include <sys/types.h>

/*
 * This little sucker goes on the beginning of the outputfile. It is blocked
 * in the first 1K of the file. It tells the inflator how many 1K blocks
 * come after it, which contain the start/size pairs of the good stuff that
 * needs to be written to disk.
 */
struct imagehdr {
	off_t	filesize;
	int	magic;
	int	regionsize;
	int	regioncount;
} *hdr;

/*
 * This number is defined to be the same as the NFS read size in netdisk.
 * The above structure is padded to this when it is written to the
 * output file.
 */
#define IMAGEHDRMINSIZE		1024

/*
 * Magic number when image is compresses
 */
#define COMPRESSED_MAGIC	0x69696969

/*
 * This little struct defines the pair. Each number is in sectors. An array
 * of these come after the header above, and is padded to a 1K boundry.
 */
struct region {
	unsigned long	start;
	unsigned long	size;
};

/*
 * This number is defined to be the same as the NFS read size in
 * netdisk.  The array of regions is padded to this when it is written
 * to the output file.
 */
#define REGIONMINSIZE		1024

