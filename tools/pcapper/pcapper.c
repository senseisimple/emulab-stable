/*
 * pcapper.c - Program to count the number of various types of IP packets
 * traveling accross a node's interfaces.
 *
 * TODO:
 *     Allow counting packets based upon tcpdump-style filter expressions
 *     More fine-grained locking
 */

/*
 * These seem to make things happier when compiling on Linux
 */
#ifdef __linux__
#define __PREFER_BSD
#define __USE_BSD
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <linux/sockios.h>
#else
#include <sys/sockio.h>
#endif

#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pcap.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 4242
#define MAX_INTERFACES 10
#define MAX_CLIENTS 10

/*
 * Program run to determine the control interface.
 */
#define CONTROL_IFACE "/etc/testbed/control_interface"

/*
 * Some struct definitions shamelessly stolen from tcpdump.org
 */

/* Ethernet header */
struct sniff_ethernet {
	u_char ether_dhost[ETHER_ADDR_LEN]; /* Destination host address */
	u_char ether_shost[ETHER_ADDR_LEN]; /* Source host address */
	u_short ether_type; /* IP? ARP? RARP? etc */
};

/* IP header */
struct sniff_ip {
#if BYTE_ORDER == LITTLE_ENDIAN
	u_int ip_hl:4, /* header length */
	ip_v:4; /* version */
#endif
#if BYTE_ORDER == BIG_ENDIAN
	u_int ip_v:4, /* version */
	ip_hl:4; /* header length */
#endif
	u_char ip_tos; /* type of service */
	u_short ip_len; /* total length */
	u_short ip_id; /* identification */
	u_short ip_off; /* fragment offset field */
	u_char ip_ttl; /* time to live */
	u_char ip_p; /* protocol */
	u_short ip_sum; /* checksum */
	struct in_addr ip_src,ip_dst; /* source and dest address */
};

/*
 * Arguments to the readpackets function - passed in a struct since
 * pthread-starting functions take a single void* as a parameter.
 */
struct readpackets_args {
	u_char index;
	char *devname;
};

/*
 * Arguments to the feedclient function.
 */
struct feedclient_args {
	int cli;
	int fd;
	int interval;
};

/*
 * Counts that we keep per-interface, per-interval.
 */
struct counter {
	unsigned int icmp_bytes;
	unsigned int tcp_bytes;
	unsigned int udp_bytes;
	unsigned int other_bytes;

	unsigned int icmp_packets;
	unsigned int tcp_packets;
	unsigned int udp_packets;
	unsigned int other_packets;
};

/*
 * Per-interface, per-interval structure, that keeps track of the next
 * interval as where, and when they begin and end.
 */
struct interface_counter {
	/*
	 * The start and end times for this interval, so that we can tell if
	 * a packet got noticed way too late, or if the client-feeding thread
	 * has missed its deadline, and we need to roll over into the next
	 * interval.
	 */
	struct timeval start_time;
	struct timeval rollover;

	/*
	 * Client-feeding threads report data from this_interval. But, if the
	 * interface-watching thread notices that the rollover time has passed,
	 * it counts the packets toward the next_interval. Rather than zeroing
	 * out this_interval after reporting it, the client-feeding threads
	 * copy in next_interval.
	 */
	struct counter this_interval;
	struct counter next_interval;
};

/*
 * Forward declarations
 */
void *readpackets(void *args);
void *feedclient(void *args);
void got_packet(u_char *args, const struct pcap_pkthdr *header,
	const u_char *packet);
int getaddr(char *dev, struct in_addr *addr);

/*
 * Number of interfaces we've detected, and their names.
 */
int interfaces = 0;
char interface_names[MAX_INTERFACES][1024];

/*
 * Number of packets that are discovered in the correct interval, and those
 * that come in "early" (ie. after the rollover time, but before the client-
 * feeding thread has woken up.) For statistical purposes.
 */
int ontime_packets = 0, early_packets = 0;

/*
 * For getopt()
 */
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

/*
 * Lock used to make sure that the threads don't clobber the pakcet counts.
 * We just use one big fat lock for all of them.
 */
pthread_mutex_t lock;

/*
 * Pakcet counts for the three types of packets that we care about. 
 */
struct interface_counter counters[MAX_CLIENTS][MAX_INTERFACES];

/*
 * Keep track of the pcap devices, since we need them for things like
 * getting counts of packets dropped by the kernel.
 */
pcap_t *pcap_devs[MAX_INTERFACES];

/*
 * An array of 'bools', indicating if any client is using that number.
 */
int client_connected[MAX_CLIENTS];

int MAX(int a, int b) { if ((a) > (b)) return (a); else return (b); }

/*
 * XXX: Shorten
 */
int main (int argc, char **argv) {
	pthread_t thread;
	int sock;
	struct protoent *proto;
	struct sockaddr_in address;
	struct in_addr ifaddr;
	struct hostent *host;
	FILE *control_interface;
	int i;
	struct sigaction action;
	int limit_interfaces;
	struct ifconf ifc;
	void  *ptr;
	char lastname[IFNAMSIZ];
	char *filename;
	int filetime;
	char ch;

#ifdef EMULAB
	char control[1024];
#endif
	/*
	 * Defaults for command-line arugments
	 */

	filename = NULL;
	filetime = 1000;

	/*
	 * Process command-line arguments
	 */
	while ((ch = getopt(argc, argv, "f:i:")) != -1)
		switch(ch) {
		case 'f':
			filename = optarg;
			break;
		case 'i':
			filetime = atoi(optarg);
			break;
		default:
			fprintf(stderr,"Bad argument\n");
		}
	argc -= optind;
	argv += optind;

	/*
	 * If they give any more arguments, we take those as interface/hostname
	 * names to match against, and skip all others.
	 */
	if (argc > 1) {
	    	limit_interfaces = 1;
	} else {
	    	limit_interfaces = 0;
	}

	/*
	 * Zero out all packet counts and other arrays.
	 */
	bzero(counters,sizeof(counters));
	bzero(pcap_devs,sizeof(pcap_devs));
	bzero(client_connected,sizeof(client_connected));

	pthread_mutex_init(&lock,NULL);

#ifdef EMULAB
	/*
	 * Find out which interface is the control net
	 */
	if ((control_interface = popen(CONTROL_IFACE,"r")) >= 0) {
		if (!fgets(control,1024,control_interface)) {
			fprintf(stderr,"Unable to determine control "
					"interface\n");
			control[0] = '\0';
		}

		pclose(control_interface);

		/*
		 * Chomp off the newline
		 */
		if (control[strlen(control) -1] == '\n') {
			control[strlen(control) -1] = '\0';
		}
		
	} else {
		fprintf(stderr,"Unable to determine control interface\n");
		control[0] = '\0';
	}

#endif

	/*
	 * Open a socket to do our ioctl()s on
	 */

	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return (1);
	}

	/*
	 * Use the SIOCGIFCONF ioctl to get a list of all interfaces on
	 * this system.
	 * XXX: The size used is totally arbitrary - this should be done in
	 * a loop with increasing sizes, as in Stevens' book.
	 * NOTE: Linux (oh so helpfully) only gives back interfaces that
	 * are up in response to SIOCGIFCONF
	 */
	ifc.ifc_buf = malloc(MAX_INTERFACES * 100 * sizeof (struct ifreq));
	ifc.ifc_len = MAX_INTERFACES * 100;
	if (ioctl(sock,SIOCGIFCONF,&ifc) < 0) {
		perror("SIOCGIFCONF");
		exit(1);
	}

	strcpy(lastname,"");

	/*
	 * Go through the list of interfaces, and fork off packet-reading
	 * threads, etc. as appropriate.
	 */
	for (ptr = ifc.ifc_buf; ptr < (void*)(ifc.ifc_buf + ifc.ifc_len); ) {
		char hostname[1024];
		char *name;
		struct ifreq *ifr, flag_ifr;

		/*
		 * Save the current position, and move ptr along
		 */
		ifr = (struct ifreq *)ptr;

		/*
		 * Great, no sa_len in Linux. This will be broken for things
		 * other than IPv4
		 */
#ifdef __linux__
		ptr = ptr + sizeof(ifr->ifr_name) +
			MAX(sizeof(struct sockaddr),sizeof(ifr->ifr_addr));
#else
		ptr = ptr + sizeof(ifr->ifr_name) +
			MAX(sizeof(struct sockaddr),ifr->ifr_addr.sa_len);
#endif

		name = ifr->ifr_name;
		if (!strcmp(name,lastname)) {
			/*
			 * If we get duplicates, skip them
			 */
			continue;
		} else {
			strcpy(lastname,name);
		}

		/*
		 * Get interface flags
		 */
		bzero(&flag_ifr,sizeof(flag_ifr));
		strcpy(flag_ifr.ifr_name, name);
		if (ioctl(sock,SIOCGIFFLAGS, (caddr_t)&flag_ifr) < 0) {
			perror("Getting interface flags");
			exit(1);
		}

		printf("Interface %s is ... ",name);

#ifdef EMULAB
		if (control && !strcmp(control,name)) {
			/* Skip control net */
			printf("control\n");
		} else
#endif
		if (flag_ifr.ifr_flags & IFF_LOOPBACK) {
			/* Skip loopback */
			printf("loopback\n");
		} else if (flag_ifr.ifr_flags & IFF_UP) {
			struct readpackets_args *args;

			/*
			 * Get theIP address for the interface.
			 */
			if (getaddr(name,&ifaddr)) {
				/* Has carrier, but no IP */
				printf("down (with carrier)\n");
				continue;
			}

			/*
			 * Grab the 'hostname' for the interface, but fallback
			 * to IP if we can't get it.
			 */
			host = gethostbyaddr((char*)&ifaddr,
					     sizeof(ifaddr),AF_INET);

			if (host) {
				strcpy(hostname,host->h_name);
			} else {
				strcpy(hostname,inet_ntoa(ifaddr));
			}

			/*
			 * If we're limiting the interfaces we look at,
			 * skip this one unless it matches one of the
			 * arguments given by the user
			 */
			if (limit_interfaces) {
				int j;
				for (j = 1; j < argc; j++) {
					if ((strstr(name,argv[j])) ||
					    (strstr(hostname,argv[j]))) {
					    break;
					}
				}
				if (j == argc) {
					printf("skipped\n");
					continue;
				}
			}

			printf("up\n");

			/*
			 * Copy it into the global interfaces list
			 */
			strcpy(interface_names[interfaces],hostname);
			strcat(interface_names[interfaces],"\n");

			/*
			 * Start up a new thread to read packets from this
			 * interface.
			 */
			args = (struct readpackets_args*)
				malloc(sizeof(struct  readpackets_args));
			args->devname = (char*)malloc(strlen(name) + 1);
			strcpy(args->devname,name);
			args->index = interfaces;
			if (pthread_create(&thread,NULL,readpackets,args)) {
				fprintf(stderr,"Unable to start thread!\n");
				exit(1);
			}

			interfaces++;

		} else {
			/* Skip interfaces that don't have carrier */
			printf("down\n");
		}
	}

	close(sock);

	/*
	 * If they gave us a filename to log to, open it up.
	 */
	if (filename) {
		int fd;
		struct feedclient_args *args;

		fd = open(filename,O_WRONLY | O_CREAT | O_TRUNC);
		if (fd < 0) {
			perror("Opening savefile");
			exit(1);
		}

		/*
		 * Start a new thread to feed the file
		 */
		args = (struct feedclient_args*)
			malloc(sizeof(struct feedclient_args));
		args->fd = fd;
		args->cli = 0;
		args->interval = filetime;
		client_connected[0] = 1;
		pthread_create(&thread,NULL,feedclient,args);
	}

	/*
	 * Create a socket to listen on, so that we can tell remote
	 * applications about the counts we're getting.
	 */
	proto = getprotobyname("TCP");
	sock = socket(AF_INET,SOCK_STREAM,proto->p_proto);
	if (sock < 0) {
		perror("Creating socket");
		exit(1);
	}

	i = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0)
		perror("SO_REUSEADDR");

	address.sin_family = AF_INET;
	address.sin_port = htons(PORT);
	address.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock,(struct sockaddr*)(&address),sizeof(address))) {
		perror("Binding socket");
		exit(1);
	}

	if (listen(sock,-1)) {
		perror("Listening on socket");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE - we'll detect the remote end closing the connection
	 * by a failed write() .
	 */
	action.sa_handler = SIG_IGN;
	sigaction(SIGPIPE,&action,NULL);

	/*
	 * Loop forever, so that we can handle multiple clients connecting
	 */
	while (1) {
		int fd;
		struct feedclient_args *args;
		struct sockaddr client_addr;
		size_t client_addrlen;

		fd = accept(sock,&client_addr,&client_addrlen);

		if (fd < 0) {
			perror("accept");
			continue;
		}
		/*
		 * Pick a client number
		 */
		pthread_mutex_lock(&lock);
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (!client_connected[i]) {
				/* printf("New client is %i\n",i); */
				client_connected[i] = 1;
				break;
			}
		}
		pthread_mutex_unlock(&lock);
		
		if (i == MAX_CLIENTS) {
			/*
			 * Didn't find a free slot
			 */
			fprintf(stderr,"Too many clients\n");
			close(fd);
		} else {
			args = (struct feedclient_args*)
				malloc(sizeof(struct feedclient_args));
			args->fd = fd;
			args->cli = i;
			args->interval = 0;
			pthread_create(&thread,NULL,feedclient,args);
		}
	}

}

/*
 * Feed data back to a client - since this is done through a file
 * descriptor, the client can be a socket, a file, etc.
 */
void *feedclient(void *args) {
	struct feedclient_args *s_args;
	int cli;
	int fd;
	char inbuf[1024];
	char outbuf[1024];
	int interval;
	int i;
	struct timeval next_time, now, interval_tv;
	int dropped;

	s_args = (struct feedclient_args*)args;
	cli = s_args->cli;
	fd = s_args->fd;
	interval = s_args->interval;

	free(args);

	dropped = 0;
	
	/*
	 * The first thing we send is the number of interfaces,
	 * followed by the name of each one (1 per line)
	 */
	sprintf(outbuf,"%i\n",interfaces);
	write(fd,outbuf,strlen(outbuf));

	for (i  = 0; i < interfaces; i++) {
		write(fd,interface_names[i],strlen(interface_names[i]));
	}

	/*
	 * Now, we wait for the client to tell us how long, in ms, they
	 * want to wait in between reports. If we were given an interval in
	 * our arguments (presumably because the output fd is write-only),
	 * forgo this step.
	 */
	if (!interval) {
		read(fd,inbuf,1024);
		interval = atoi(inbuf);
	}

	/*
	 * Convert to microseconds.
	 */
	interval *= 1000;

	interval_tv.tv_usec = interval % 1000000;
	interval_tv.tv_sec = interval / 1000000;

	/*
	 * Zero out the packet counts before the first report
	 */
	pthread_mutex_lock(&lock);
	bzero(counters[cli], sizeof(counters[cli]));
	pthread_mutex_unlock(&lock);

	gettimeofday(&now,NULL);
	timeradd(&now,&interval_tv,&next_time);

	while (1) { /* Repeat until client disconnects */
		int used = 0;
		int writerv;
		struct timeval sleepintr;
		struct timespec sleepintr_ts;
		struct pcap_stat ps;

		/*
		 * Zero out the packet counts, and set up the times in the
		 *
		 */
		pthread_mutex_lock(&lock);
		gettimeofday(&now,NULL);
		for (i  = 0; i < interfaces; i++) {
			bcopy(&(counters[cli][i].next_interval),
			      &(counters[cli][i].this_interval),
			      sizeof(counters[cli][i].next_interval));
			bzero(&(counters[cli][i].next_interval),
			      sizeof(counters[cli][i].next_interval));
			bcopy(&(next_time),&(counters[cli][i].start_time),
			      sizeof(now));
			bcopy(&(next_time),&(counters[cli][i].rollover),
			      sizeof(next_time));
		}
		pthread_mutex_unlock(&lock);

		/*
		 * Figure out how long to sleep for
		 */
		gettimeofday(&now,NULL);
		timersub(&next_time,&now,&sleepintr);

		/*
		 * Have to convert to a timespec, since nanosleep takes a
		 * timespec, but all the other functions I want to use deal
		 * in timeval's
		 */
		sleepintr_ts.tv_sec = sleepintr.tv_sec;
		sleepintr_ts.tv_nsec = sleepintr.tv_usec * 1000;

		nanosleep(&sleepintr_ts,NULL);
		gettimeofday(&now,NULL);

		/*
		 * Figure out the next time we'll be waking up
		 */
		timeradd(&next_time,&interval_tv,&next_time);

		/*
		 * Put a timestamp on the beginning of each line
		 */
		used += sprintf((outbuf +used),"%li.%06li ",now.tv_sec,
				now.tv_usec);

		/*
		 * Gather up the counts for each interface into
		 * a string.
		 */
		pthread_mutex_lock(&lock);
		for (i  = 0; i < interfaces; i++) {
			used += sprintf((outbuf +used),
				"(icmp:%i,%i tcp:%i,%i udp:%i,%i other:%i,%i) ",
				counters[cli][i].this_interval.icmp_bytes,
				counters[cli][i].this_interval.icmp_packets,
				counters[cli][i].this_interval.tcp_bytes,
				counters[cli][i].this_interval.tcp_packets,
				counters[cli][i].this_interval.udp_bytes,
				counters[cli][i].this_interval.udp_packets,
				counters[cli][i].this_interval.other_bytes,
				counters[cli][i].this_interval.other_packets);
		}
		pthread_mutex_unlock(&lock);

		/*
		 * Drop information is not kept per-interface, so we just
		 * grab it from one of them.
		 */
		if (pcap_stats(pcap_devs[0],&ps)) {
			printf("Unable to get stats\n");
			exit(1);
		}

		/*
		 * Put the difference between the last and current kernel drop
		 * count on the end of the line.
		 */
		used += sprintf((outbuf + used),"(dropped:%i)",
				ps.ps_drop - dropped);
		dropped = ps.ps_drop;

		/*
		 * Make it more human-readable
		 */
		strcat(outbuf,"\n");

		writerv = write(fd,outbuf,strlen(outbuf));
		if (writerv < 0) {
			if (errno != EPIPE) {
				perror("write");
			}
			/* printf("Client %i disconnected\n",cli);*/
			client_connected[cli] = 0;
			/*
			 * Client disconnected - exit the loop
			 */
			break;
		}
	}

	return NULL;
}

/*
 * Start the pcap_loop for an interface
 */
void *readpackets(void *args) {
	char ebuf[1024];
	pcap_t *dev;
	struct readpackets_args *sargs;
	int size;
#ifdef __linux__
	bpf_u_int32 mask, net;
	struct bpf_program filter;
#endif

	/*
	 * How much of the packet we want to grab. For some reason 
	 */
	size = sizeof(struct sniff_ethernet) + sizeof(struct sniff_ip);
	sargs = (struct readpackets_args*)args;

	/*
	 * NOTE: We set the timeout to a full second - if we set it lower, we
	 * don't get to see packets until a certain number have been buffered
	 * up, for some reason.
	 */
	dev = pcap_open_live(sargs->devname,size,1,1000,ebuf);

	if (!dev) {
		fprintf(stderr,"Failed to open %s: %s\n",sargs->devname,ebuf);
		exit(1);
	}

	/*
	 * Record our pcap device into the global array, so that it can be
	 * found by other processes.
	 */
	pcap_devs[sargs->index] = dev;

	/*
	 * Put an empty filter on the device, or we get nothing (but only
	 * in Linux) - how frustrating!
	 */
#ifdef __linux__
	pcap_lookupnet(sargs->devname, &net, &mask, ebuf);
	pcap_compile(dev, &filter, "", 0, net);
	pcap_setfilter(dev, &filter);
#endif

	if (pcap_loop((pcap_t *)dev,-1,got_packet,&(sargs->index))) {
		printf("Failed to start pcap_loop for %s\n",sargs->devname);
		exit(1);
	}

	return NULL;
}

/*
 * Callback for pcap_loop - gets called every time a packet is recieved, and
 * adds its byte count to the total.
 */
void got_packet(u_char *args, const struct pcap_pkthdr *header,
	const u_char *packet) {

	struct sniff_ethernet *ether_pkt;
	struct sniff_ip *ip_pkt; 
	int length;
	int type;
	int index;
	int i;

	/*
	 * We only care about IP packets
	 */
	ether_pkt = (struct sniff_ethernet*)packet;
	if (ether_pkt->ether_type != 8) {
		return;
	}

	ip_pkt = (struct sniff_ip*)(packet + sizeof(struct sniff_ethernet));

	/*
	 * Add this packet length to the appropriate total
	 */
	length = ntohs(ip_pkt->ip_len);
	type = ip_pkt->ip_p;

	/* printf("got type %d, len=%d(0x%x)\n", type, length, length); */

	index = *args;

	pthread_mutex_lock(&lock);
	/*
	 * Since multiple clients may be monitoring this interface, we have
	 * to update counts for all of them.
	 */
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (client_connected[i] &&
				timerisset(&(counters[i][index].rollover))) {
			struct counter *count;

			/*
			 * Figure out if this packet is on time, or early
			 */
			if (timercmp(&(header->ts),
				     &(counters[i][index].rollover), <)) {
				count = &(counters[i][index].this_interval);
				ontime_packets += 1;
			} else if (timercmp(&(header->ts),
				        &(counters[i][index].start_time), <)) {
				fprintf(stderr,"Got a REALLY late packet!\n");
			} else {
				count = &(counters[i][index].next_interval);
				early_packets += 1;
			}

#ifdef NO_TS_CORRECTION
			count = &(counters[i][index].this_interval);
#endif

			/*
			printf("OTP: %f (%i,%i)\n",
				ontime_packets*1.0/
					(ontime_packets + early_packets),
				ontime_packets,early_packets);
			*/

			/*
			 * Now, just  add it into the approprate counts
			 */
			switch (type) {
				case 1:  count->icmp_bytes  += length;
					 count->icmp_packets++;
					 break;
				case 6:  count->tcp_bytes   += length;
					 count->tcp_packets++;
					 break;
				case 17: count->udp_bytes   += length;
					 count->udp_packets++;
					 break;
				default: count->other_bytes += length;
					 count->other_packets++;
					 break;
			}
		}
	}
	pthread_mutex_unlock(&lock);

}

/*
 * Get the IP address for an interface. Returns the IP in the addr parameter,
 * and returns non-zero if problems were encountered.
 */
int getaddr(char *dev, struct in_addr *addr) {
	struct ifreq ifr;
	int sock;
	struct sockaddr_in *sin;

	/*
	 * Open a socket to do the ioctl() on
	 */
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return (1);
	}

	strcpy(ifr.ifr_name,dev);

	/*
	 * Do the ioctl
	 */
	if (ioctl(sock, SIOCGIFADDR, (char *) &ifr) < 0) {
		close(sock);
		/*
		 * Don't print this, because it can occur during 'normal'
		 * operation
		 */
		/* perror("SIOCGIFADDR"); */
		return 1;
	} else {
		sin = (struct sockaddr_in *) & ifr.ifr_addr;
		/* printf("Got one: %s\n", inet_ntoa(sin->sin_addr)); */
		*addr = sin->sin_addr;
		close(sock);
		return 0;
	}
}
