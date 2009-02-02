/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */
struct boot_what;
struct boot_info;

int		open_bootinfo_db(void);
int		close_bootinfo_db(void);
int		bootinfo_init(void);
int		bootinfo(struct in_addr ipaddr,
			 struct boot_info *info, void *opaque);
int		query_bootinfo_db(struct in_addr ipaddr, int version,
				  struct boot_what *info, char *key);
int		elabinelab_hackcheck(struct sockaddr_in *target);

extern int debug;

#ifdef EVENTSYS
int		bievent_init(void);
int		bievent_shutdown(void);
int		bievent_send(struct in_addr ipaddr, void *opaque, char *event);
#endif

