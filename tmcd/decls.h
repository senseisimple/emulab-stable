/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#define TBSERVER_PORT		7777
#define TBSERVER_PORT2		14447
#define MYBUFSIZE		2048
#define BOSSNODE_FILENAME	"bossnode"
#define MAXTMCDPACKET		0x4000	/* Allow for console logs */

/*
 * As the tmcd changes, incompatable changes with older version of
 * the software cause problems. Starting with version 3, the client
 * will tell tmcd what version they are. If no version is included,
 * assume its DEFAULT_VERSION.
 *
 * Be sure to update the versions as the TMCD changes. Both the
 * tmcc and tmcd have CURRENT_VERSION compiled in, so be sure to
 * install new versions of each binary when the current version
 * changes. libsetup.pm module also encodes a current version, so be
 * sure to change it there too!
 *
 * Note, this is assumed to be an integer. No need for 3.23.479 ...
 * NB: See ron/libsetup.pm. That is version 4! I'll merge that in. 
 */
#define DEFAULT_VERSION		2
#define CURRENT_VERSION		33
