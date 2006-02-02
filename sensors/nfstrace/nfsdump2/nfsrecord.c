/*
 * $Id: nfsrecord.c,v 1.3 2006-02-02 16:16:17 stack Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <rpc/rpc.h>

#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <assert.h>

#include <rpcsvc/mount.h>

#include "interface.h"
#include "addrtoname.h"
#include "ethertype.h"

#include "ether.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"
/* #include "nfs.h" */

#include "nfsrecord.h"

#if !defined(NFS_PROGRAM)
#define	NFS_PROGRAM	100003
#endif
#define	RPC_VERSION	2

nfs_v3_stat_t	v3statsBlock;
nfs_v2_stat_t	v2statsBlock;
packetTable_t  *readTable;
packetTable_t  *writeTable;

typedef	struct	_hash_t	{
	u_int32_t	rpcXID;
	u_int32_t	pthash;
	u_int32_t	srcHost;		/* invoking host */
	u_int32_t	srcPort;		/* port on invoking host */
	u_int32_t	nfsVersion;
	u_int32_t	nfsProc;
	u_int32_t	nfsProg;
	u_int32_t	call_time;	/* For aging. */
	struct _hash_t	*next;
} hash_t;

void packetPrinter (u_char *user, struct pcap_pkthdr *h, u_char *pp);
int processPacket (struct pcap_pkthdr *h, u_char *pp, nfs_pkt_t *record);
void printRecord (nfs_pkt_t *record, void *xdr, u_int32_t payload_len,
		u_int32_t proto, u_int32_t actual_len, int print);
void printHostIp (u_int32_t ipa);
int getEtherHeader (u_int32_t packet_len,
		u_char *bp, unsigned int *proto, unsigned int *len);
int getIpHeader (struct ip *ip_b, unsigned int *proto, unsigned int *len,
		u_int32_t *src, u_int32_t *dst);
int getTcpHeader (struct tcphdr *tcp_b);
int getUdpHeader (struct udphdr *udp_b);
int getRpcHeader (struct rpc_msg *rpc_b, u_int32_t *dir, u_int32_t maxlen,
		u_int32_t *euid, u_int32_t *egid);
int getData (u_char *bp, u_char *buf, unsigned int maxlen);

hash_t *hashInsertXid (u_int32_t time, u_int32_t xid,
		u_int32_t host, u_int32_t port,
		u_int32_t version, u_int32_t proc, u_int32_t prog);

hash_t *hashLookupXid (u_int32_t now,
		u_int32_t xid, u_int32_t host, u_int32_t port);

static int get_auth (u_int32_t *ui, u_int32_t *euid, u_int32_t *egid);

#define	HASHSIZE	3301
#define	HASH(x,h,p)	(((x) % HASHSIZE + (h) % HASHSIZE + (p) % HASHSIZE) \
				% HASHSIZE)

static	hash_t	*xidHashTable [HASHSIZE];
	int	xidHashElemCnt	= 0;

void packetPrinter (u_char *user, struct pcap_pkthdr *h, u_char *pp)
{
	int rc;
	nfs_pkt_t record;

	rc = processPacket (h, pp, &record);

}

#define	MIN_ETHER_HDR_LEN	14		/* RFC 894 */
#define	MIN_IP_HDR_LEN		20		/* w/o options */
#define	MIN_TCP_HDR_LEN		20		/* w/o options */
#define	MIN_UDP_HDR_LEN		8

FILE	*OutFile	= NULL;

/*
 * Returns 1 if the packet is interesting, 0 if the packet was
 * uninteresting and should be discarded, and -1 if there was an
 * unexpected error while processing the packet.
 */

int processPacket (struct pcap_pkthdr *h,	/* Captured stuff */
		u_char *pp,			/* packet pointer */
		nfs_pkt_t *record
		)
{
	struct	ip	*ip_b		= NULL;
	struct	tcphdr	*tcp_b		= NULL;
	struct	udphdr	*udp_b		= NULL;
	struct  rpc_msg	*rpc_b		= NULL;
	u_int32_t	tot_len		= h->caplen;
	u_int32_t	consumed	= 0;
	u_int32_t	src_port	= 0;
	u_int32_t	dst_port	= 0;
	u_int32_t	payload_len	= 0;
	int	e_len, i_len, h_len, rh_len;
	u_int32_t	rpc_len;
	unsigned int	length;
	u_int32_t srcHost, dstHost;
	unsigned int proto;
	u_int32_t dir;
	unsigned int cnt;

	if (OutFile == NULL) {
		OutFile = stdout;
	}

	/*
	 * It doesn't make any sense to run this program with a small
	 * caplen (aka snap length) because the important stuff will
	 * get lost.
	 *
	 * Too-short packets *should* never happen, since the min
	 * packet length is longer than this, but it's always better
	 * to be safe than to be sucker-punched by a bug elsewhere...
	 */

	if (tot_len <= (MIN_ETHER_HDR_LEN + MIN_IP_HDR_LEN + MIN_UDP_HDR_LEN)) {
		return (0);
	}

	e_len = getEtherHeader (tot_len, pp, &proto, &length);
	if (e_len <= 0) {
		return (-1);
	}
	consumed += e_len;

	/*
	 * If the type of the packet isn't IP, then we're not
	 * interested in it-- chuck it now.
	 *
	 * Note-- ordinarily by the time we get here, we've already
	 * filtered out the packets using a pattern in the pcap
	 * library, so this shouldn't happen (unless we are running
	 * off a capture of the entire traffic on the wire).
	 */

	if (proto != 0x0800) {
		return (0);
	}

	ip_b = (struct ip *) (pp + e_len);

	i_len = getIpHeader (ip_b, &proto, &length, &srcHost, &dstHost);
	if (i_len <= 0) {
		return (-2);
	}

	consumed += i_len;

	/*
	 * Truncated packet-- what's up with that?  At this point,
	 * this can only happen if the packet is very short and the IP
	 * options are very long.  Still, must be cautious...
	 */

	if (consumed >= tot_len) {
		return (0);
	}

	if (proto == IPPROTO_TCP) {
		if (consumed + MIN_TCP_HDR_LEN >= tot_len) {
/* 			fprintf (OutFile, "XX 1: TCP pkt too short.\n"); */
			return (0);
		}

		tcp_b = (struct tcphdr *) (pp + consumed);
		h_len = getTcpHeader (tcp_b);
		if (h_len <= 0) {
/* 			fprintf (OutFile, "XX 2: TCP header error\n"); */
			return (-3);
		}

		consumed += h_len;
		if (consumed >= tot_len) {
/* 			fprintf (OutFile, */
/* 			"XX 3: Dropped (consumed = %d, tot_len = %d)\n", */
/* 					consumed, tot_len); */
			return (0);
		}

		h_len += sizeof (u_int32_t);

		src_port = ntohs (tcp_b->th_sport);
		dst_port = ntohs (tcp_b->th_dport);

	}
	else if (proto == IPPROTO_UDP) {
		if (consumed + MIN_UDP_HDR_LEN >= tot_len) {
			return (0);
		}

		udp_b = (struct udphdr *) (pp + consumed);
		h_len = getUdpHeader (udp_b);
		if (h_len <= 0) {
			return (-4);
		}

		consumed += h_len;
		if (consumed >= tot_len) {
			return (0);
		}

		src_port = ntohs (udp_b->uh_sport);
		dst_port = ntohs (udp_b->uh_dport);

		rpc_len = tot_len - consumed;

	}
	else {

		/* 
		 * If it's not TCP or UPD, then no matter what it is,
		 * we don't care about it, so just ignore it.
		 */

/* 		fprintf (OutFile, "XX 5: Not TCP or UDP.\n"); */
		return (0);
	}

	/*
	 * If we get to this point, there's a good chance this packet
	 * contains something interesting, so we start filling in the
	 * fields of the record immediately.
	 */

	record->secs	= h->ts.tv_sec;
	record->usecs	= h->ts.tv_usec;
	record->srcHost	= srcHost;
	record->dstHost	= dstHost;
	record->srcPort	= src_port;
	record->dstPort	= dst_port;

	/* 
	 * The tricky thing is that it might actually contain several
	 * encapsulated RPC messages.
	 */

/* 	fprintf (OutFile, "XX start loop\n"); */

	cnt = 0;
	while (consumed < tot_len) {
		u_int32_t euid, egid;
		int print_it, print_all_of_it = 1;
		hash_t *hp = NULL;

		cnt++;
		euid = egid = -1;

		/*
		 * If it's RPC over TCP, then the length of the rest
		 * of the RPC is given next.
		 *
		 * skip over the RPC length header, but save a copy
		 * for later use.
		 *
		 * &&& Note that the high-order bit is always set in
		 * the RPC length field.  This gives us a tiny extra
		 * sanity check, if we ever want it.
		 */

		if (proto == IPPROTO_TCP) {
			rpc_len = ntohl (*(u_int32_t *)(pp + consumed));
			rpc_len &= 0x7fffffff;

			consumed += sizeof (u_int32_t);
		}

		rpc_b = (struct rpc_msg *) (pp + consumed);
		rh_len = getRpcHeader (rpc_b, &dir, tot_len - consumed,
				&euid, &egid);
		if (rh_len == 0) {
			int *x = (int *) rpc_b;

			/*
			fprintf (OutFile, "XX Dropped 6 (rpc_len = %x %x %x)\n",
					rpc_len, x[0], x[1]);
			*/

			if (cnt > 1) {
				fprintf (OutFile, "XX rh_len == 0, cnt = %d\n", cnt);
			}
			return (0);
		}
		else if (rh_len < 0) {
/* 			fprintf (OutFile, "XX cnt = %d\n", cnt); */
			return (-5);
		}

		consumed += rh_len;

		if (consumed == tot_len && dir == CALL &&
		    ntohl (rpc_b->ru.RM_cmb.cb_proc) == NFSPROC_NULL) {

			/* It's a NULL RPC; do nothing */
			continue;
		}

		if (consumed >= tot_len) {
/* 			fprintf (OutFile, "XX truncated payload %d >= tot_len %d\n", */
/* 					consumed, tot_len); */
			goto end;
		}

/* 		fprintf (OutFile, "XX good (cnt = %d)\n", cnt); */

		print_it = 1;

		if (dir == CALL) {
			record->rpcCall		= CALL;
			record->rpcXID		= ntohl (rpc_b->rm_xid);
			record->nfsVersion	= ntohl (rpc_b->ru.RM_cmb.cb_vers);
			record->nfsProc		= ntohl (rpc_b->ru.RM_cmb.cb_proc);
			record->nfsProg		= ntohl (rpc_b->ru.RM_cmb.cb_prog);
			hp = hashInsertXid (record->secs, record->rpcXID,
					record->srcHost, record->srcPort,
					record->nfsVersion, record->nfsProc,
				       record->nfsProg);
		}
		else {
			u_int32_t accepted;
			u_int32_t acceptStatus;

			record->rpcCall		= REPLY;
			record->rpcXID		= ntohl (rpc_b->rm_xid);

			/*
			 * If the RPC was not accepted, dump it.
			 */

			accepted = ntohl (rpc_b->rm_reply.rp_stat);
			acceptStatus = ntohl (*(u_int32_t *) (pp + consumed));
			if (accepted != 0 || acceptStatus != 0) {
/* 				fprintf (OutFile, */
/* 						"XX not accepted (%x, %x)\n", */
/* 						accepted, acceptStatus); */
				print_it = 0;
			}

			/*
			 * If we see an XID we've never seen before,
			 * print an "unknown" record.  Note that we
			 * don't know the actual NFS version -- we
			 * just assume that it is v3.  Since we can't
			 * decode the payload anyway, it doesn't
			 * matter very much what version of the
			 * protocol it was.
			 *
			 * You need to take all of the "unknown"
			 * responses with a grain of salt, because
			 * they might be complete garbage.
			 *
			 * NOTE:  this is a new feature!  Before
			 * 3/23/03, unmatched responses would simply
			 * be dumped.
			 */

			hp = hashLookupXid (record->secs, record->rpcXID,
					record->dstHost, record->dstPort);
			if (hp == NULL) {
/* 				fprintf (OutFile, "XX response without call\n"); */
				print_it = 1;
				record->nfsVersion	= 3;
				record->nfsProc		= -1;
				record->nfsProg		= -1;
				record->rpcStatus	= ntohl (*(u_int32_t *)
					(pp + e_len + i_len + h_len + rh_len +
						sizeof (u_int32_t)));
			}
			else {
				record->nfsVersion	= hp->nfsVersion;
				record->nfsProc		= hp->nfsProc;
				record->nfsProg		= hp->nfsProg;
				record->pthash		= hp->pthash;
				record->rpcStatus	= ntohl (*(u_int32_t *)
					(pp + e_len + i_len + h_len + rh_len +
						sizeof (u_int32_t)));
				free (hp);
			}

			/*
			 * Treat the accept_status and call_status
			 * as part of the RPC header, for replies.
			 */

			rh_len += 2 * sizeof (u_int32_t);
			consumed += 2 * sizeof (u_int32_t);
		}

		if (record->nfsVersion == NFS_VERSION) {
		    switch (record->nfsProc) {
		    case NFSPROC_NULL:
		    case NFSPROC_GETATTR:
		    case NFSPROC_SETATTR:
		    case NFSPROC_ROOT:
		    case NFSPROC_WRITECACHE:
		    case NFSPROC_READDIR:
		    case NFSPROC_STATFS:
			print_it = 0;
			break;
		    case NFSPROC_READ:
			if (dir == REPLY) {
			    print_it = 0;
			}
			else {
			    print_all_of_it = 0;
			}
			break;
		    case NFSPROC_WRITE:
			print_all_of_it = 0;
			break;
		    default:
			break;
		    }
		}
		else if (record->nfsVersion == NFS_V3) {
		    switch (record->nfsProc) {
		    case NFSPROC3_NULL:
		    case NFSPROC3_GETATTR:
		    case NFSPROC3_SETATTR:
		    case NFSPROC3_ACCESS:
		    case NFSPROC3_READDIR:
		    case NFSPROC3_READDIRPLUS:
		    case NFSPROC3_FSSTAT:
		    case NFSPROC3_FSINFO:
		    case NFSPROC3_PATHCONF:
		    case NFSPROC3_COMMIT:
			print_it = 0;
			break;
		    case NFSPROC3_READ:
			if (dir == REPLY) {
			    print_it = 0;
			}
			else {
			    print_all_of_it = 0;
			}
			break;
		    case NFSPROC3_WRITE:
			print_all_of_it = 0;
			break;
		    default:
			break;
		    }
		}
		else {
		    // printf ("unknown version %d\n", record->nfsVersion);
		}
		
		/*
		 * The payload is everything left in the packet except
		 * the rpc header.
		 */

		payload_len = rpc_len - rh_len;

		/*
		if (payload_len + consumed > tot_len) {
			fprintf (OutFile, "XX too long, must ignore! %d > %d\n",
					payload_len + consumed, tot_len);
		}
		*/

		if (record->nfsProg == MOUNTPROG) {
			char *payload_data = (pp + consumed);
			
			if (record->nfsProc != MOUNTPROC_MNT) {
			}
			else if (dir == CALL) {
				int dplen;

				if (payload_len < 4) {
					printf("short packet\n");
					goto bail;
				}
				dplen = ntohl(*((u_int32_t *)payload_data));
				if (payload_len < dplen) {
					printf("short packet 2\n");
					goto bail;
				}
				payload_data += 4;
				payload_data[dplen] = '\0';
				
				fprintf (OutFile, "%u.%.6u ", record->secs, record->usecs);
				printHostIp (record->srcHost);
				fprintf (OutFile, ".%.4x ", 0xffff & record->srcPort);
				printHostIp (record->dstHost);
				fprintf (OutFile, ".%.4x ", 0xffff & record->dstPort);
				
				fprintf (OutFile, "%c ", proto == IPPROTO_TCP ? 'T' : 'U');
				fprintf (OutFile, "C%d %d 1 mnt fn \"%s\" ", record->nfsVersion, ntohl (rpc_b->rm_xid), payload_data);
				if (euid != -1 && egid != -1) {
					fprintf (OutFile, "euid %d egid %d ",
						 euid, egid);
				}
				fprintf (OutFile, "con = %d len = %d",
					 consumed,
					 payload_len + consumed > tot_len ?
					 tot_len : payload_len + consumed);
				fprintf(OutFile, "\n");
			}
			else {
				fprintf (OutFile, "%u.%.6u ", record->secs, record->usecs);
				printHostIp (record->srcHost);
				fprintf (OutFile, ".%.4x ", 0xffff & record->srcPort);
				printHostIp (record->dstHost);
				fprintf (OutFile, ".%.4x ", 0xffff & record->dstPort);
				
				fprintf (OutFile, "%c ", proto == IPPROTO_TCP ? 'T' : 'U');
				fprintf (OutFile, "R%d %d ", record->nfsVersion, ntohl (rpc_b->rm_xid));
				if (record->rpcStatus == 0) {
					fprintf(OutFile, "1 mnt OK ");
					if (record->nfsVersion == 1 || record->nfsVersion == 2) {
						print_fh2((u_int32_t *)payload_data, (u_int32_t *)(payload_data + (tot_len - consumed)), 1, "fh");
					}
					else {
						fprintf(OutFile, "fh ");
						print_fh3((u_int32_t *)payload_data, (u_int32_t *)(payload_data + (tot_len - consumed)), 1);
					}
				}
				else {
					fprintf(OutFile, "1 mnt %d ", record->rpcStatus);
				}
				fprintf (OutFile, "status=%d ", record->rpcStatus);
				
				fprintf (OutFile, "pl = %d ", payload_len);
				fprintf (OutFile, "con = %d len = %d",
					 consumed,
					 payload_len + consumed > tot_len ?
					 tot_len : payload_len + consumed);
				fprintf(OutFile, "\n");
			}
		}
		else if (print_it) {
			printRecord (record, (void *) (pp + consumed),
					payload_len, proto,
					tot_len - consumed,
				     print_all_of_it);

			if (dir == CALL && hp != NULL)
			    hp->pthash = record->pthash;

			if (!print_all_of_it)
			    goto bail;
			
			if (euid != -1 && egid != -1) {
				fprintf (OutFile, "euid %d egid %d ",
						euid, egid);
			}

			fprintf (OutFile, "con = %d len = %d",
					consumed,
					payload_len + consumed > tot_len ?
						tot_len : payload_len + consumed);

			/* For debugging purposes: */
			if (tot_len > payload_len + consumed) {
				fprintf (OutFile, " LONGPKT-%d ",
						tot_len - (payload_len + consumed));
			}

			/*
			 * Used to check here to see if the
			 * payload_len + consumed was greater than
			 * tot_len, but this is just wrong for TCP,
			 * where the payload might be spread out over
			 * several IP datagrams.
			 */
			
#ifdef	COMMENT
			if (payload_len + consumed > tot_len) {
			    fprintf (OutFile, " +++");
			}
#endif	/* COMMENT */
			
			fprintf (OutFile, "\n");
		}
		
	bail:
		consumed += payload_len;

		/* 		fprintf (OutFile, "XX consumed = %d, tot_len = %d\n", consumed, tot_len); */

	}

	// fflush(OutFile);

/* 	fprintf (OutFile, "XX end loop\n"); */
end:
	/* fprintf (OutFile, "END %d\n", cnt); */

	return (1);
}

/*
 * payload_len is the length that the RPC header claimed was the
 * total length of the message, and actual_len is the number of bytes
 * actually snarfed.  Sometimes it's OK if the actual_len is less
 * than the payload len (for example, if it's a read response, or
 * a write request; we don't care about the data) but it's definately
 * a sign that there might be trouble.  This needs to be caught
 * much later, down in the routines to actually carve up the packets.
 */

void printRecord (nfs_pkt_t *record, void *xdr, u_int32_t payload_len,
		  u_int32_t proto, u_int32_t actual_len, int print)
{
	u_int32_t *dp = xdr;

	if (print) {
		fprintf (OutFile, "%u.%.6u ", record->secs, record->usecs);
		printHostIp (record->srcHost);
		fprintf (OutFile, ".%.4x ", 0xffff & record->srcPort);
		printHostIp (record->dstHost);
		fprintf (OutFile, ".%.4x ", 0xffff & record->dstPort);

		fprintf (OutFile, "%c ", proto == IPPROTO_TCP ? 'T' : 'U');
}

	if (record->rpcCall == CALL) {
		if (record->nfsVersion == 3) {
			nfs_v3_print_call (record, record->nfsProc, record->rpcXID,
					dp, payload_len, actual_len,
					&v3statsBlock);
		}
		else if (record->nfsVersion == 2) {
			nfs_v2_print_call (record, record->nfsProc, record->rpcXID,
					dp, payload_len, actual_len,
					&v2statsBlock);
		}
		else {
			fprintf (OutFile, "CU%d\n", record->nfsVersion);
		}
	}
	else {
		if (record->nfsVersion == 3) {
			nfs_v3_print_resp (record, record->nfsProc, record->rpcXID,
					dp, payload_len, actual_len,
					record->rpcStatus,
					&v3statsBlock);
		}
		else if (record->nfsVersion == 2) {
			nfs_v2_print_resp (record, record->nfsProc, record->rpcXID,
					dp, payload_len, actual_len,
					record->rpcStatus,
					&v2statsBlock);
		}
		else {
			fprintf (OutFile, "RU%d\n", record->nfsVersion);
		}

		if (print) {
		fprintf (OutFile, "status=%d ", record->rpcStatus);

		fprintf (OutFile, "pl = %d ", payload_len);
		}
	}

	return ;
}

void printHostIp (u_int32_t ipa)
{

	if (vflag) {
		fprintf (OutFile, "%u.%u.%u.%u",
				0xff & (ipa >> 24), 0xff & (ipa >> 16),
				0xff & (ipa >> 8), 0xff & (ipa >> 0));
	}
	else {
		fprintf (OutFile, "%.8x", ipa);
	}
}

/*
 * bp is assumed to point to the start of an ethernet header.  If
 * sanity checks pass, fills in eth_b with the contents of the header,
 * returns the number of bytes consumed by the header (which can
 * depend on the precise ethernet protocol), and fills in *len with
 * the number of bytes in the REST of the packet (not counting the
 * header).
 */

#define	OLD_ETHERMTU	1500	/* somewhat archaic, because jumbo frames */

int getEtherHeader (u_int32_t packet_len,
		u_char *bp, unsigned int *proto, unsigned int *len)
{
	int length = (bp [12] << 8) | bp [13];

	/*
	 * Look for either 802.3 or RFC 894:
	 *
	 * if "length" <= OLD_ETHERMTU).  Note that we let the pcap
	 * library make the decision for jumbo frames-- pcap has
	 * already calculated the apparent packet length, and if it
	 * agrees with the 802.3 length field, and the next few fields
	 * make sense according to the 802.3 spec, then we assume that
	 * this is an 802.3 packet.  This is not riskless, but I have
	 * no better way to tell the difference.
	 */

	if ((length <= ETHERMTU) || 
			(length == (packet_len - 22) &&	/* jumbo? */
			(bp [14] == 0xAA) &&		/* DSAP */
			(bp [15] == 0xAA) &&		/* SSAP */
			(bp [16] == 0x03))) {		/* CNTL */
		*proto = (bp [20] << 8) | bp [21];
		*len = length;
		return (22);
	}
	else {
		*len = packet_len - 14;
		*proto = length;
		return (14);
	}
}

/*
 * ip_b is assumed to point to the start of an IP header.  If sanity
 * checks pass, returns the number of bytes consumed by the header
 * (including options).
 */

int getIpHeader (struct ip *ip_b, unsigned int *proto, unsigned int *len,
		u_int32_t *src, u_int32_t *dst)
{
	u_int32_t *ip = (u_int32_t *) ip_b;
	u_int32_t off;

	/*
	 * If it's not IPv4, then we're completely lost.
	 */

	if (IP_V (ip_b) != 4) {
		return (-1);
	}

	/*
	 * If this isn't fragment zero of an higher-level "packet",
	 * then it's not something that we're interested in, so dump
	 * it.
	 */

	off = ntohs(ip_b->ip_off);
	if ((off & 0x1fff) != 0) {
		return (0);
	}

	*proto = ip_b->ip_p;
	*len = ntohs (ip_b->ip_len);
	*src = ntohl (ip [3]);
	*dst = ntohl (ip [4]);

	return (IP_HL (ip_b) * 4);
}

/*
 * Right now, all we want out of the tcp header is the length,
 * so we can skip over it.  If all goes well, we pluck out the
 * ports elsewhere.
 */

int getTcpHeader (struct tcphdr *tcp_b)
{

	return (4 * TH_OFF (tcp_b));
}

int getUdpHeader (struct udphdr *udp_b)
{

	return (8);
}

int getRpcHeader (struct rpc_msg *bp, u_int32_t *dir_p, u_int32_t maxlen,
		u_int32_t *euid, u_int32_t *egid)
{
	int cred_len, verf_len;
	u_int32_t *ui, *auth_base;
	u_int32_t rh_len;
	u_int32_t dir;
	u_int32_t *max_ptr;

	max_ptr = (u_int32_t *) bp;
	max_ptr += (maxlen / sizeof (u_int32_t));

	*dir_p = dir = ntohl (bp->rm_direction);

	if (dir == CALL) {
		if ((ntohl (bp->ru.RM_cmb.cb_rpcvers) != RPC_VERSION) ||
		    ((ntohl (bp->ru.RM_cmb.cb_prog) != NFS_PROGRAM) &&
		     (ntohl (bp->ru.RM_cmb.cb_prog) != MOUNTPROG))) {
			return (-1);
		}

		/*
		 * Need to skip over all the credentials and
		 * verification fields.  The nuisance is figuring out
		 * how long these fields are, since they do not have a
		 * fixed size.
		 *
		 * Note that we must depart from using the rpc_msg
		 * structure here, because it doesn't know how big
		 * these structs are either.  It just uses an opaque
		 * reference to get at them, after they've been
		 * parsed.  Since I see things in an unparsed state, I
		 * need to actually do the calculation.
		 *
		 * Note that the credentials and verifier are
		 * word-aligned, so we can't just add up cred_len and
		 * verf_len and always get the right answer.  Instead,
		 * we need to round them both up.  It's easier to just
		 * do some pointer arithmetic at the end.
		 */

		ui = (u_int32_t *) &bp->rm_call.cb_cred;
		auth_base = ui;

		/*
		 * &&& Can also add sanity checks; there are limits on
		 * the possible lengths for the cred and the verifier!
		 */

		if ((ui + 1) >= max_ptr) {
/* 			fprintf (OutFile, "XX cut-off RPC header (cred)\n"); */
			return (-1);
		}

		cred_len = ntohl (ui [1]);

		auth_base = ui;

		ui += (cred_len + (2 * sizeof (*ui) + 3)) / sizeof (*ui);

		if ((ui + 1) >= max_ptr) {
/* 			fprintf (OutFile, "XX cut-off RPC header (verif)\n"); */
			return (-1);
		}

		get_auth (auth_base, euid, egid);

		verf_len = ntohl (ui [1]);

		ui += (verf_len + (2 * sizeof (*ui) + 3)) / sizeof (*ui);

		rh_len = (4 * (ui - (u_int32_t *) bp));

	}
	else if (dir == REPLY) {

		/*
		 * Like the CALL case, but uglier because of potential
		 * alignment problems; can't use very much of the
		 * reply struct as-is.
		 */

		ui = ((u_int32_t *) &bp->rm_reply) + 1;

		if ((ui + 1) >= max_ptr) {
/* 			fprintf (OutFile, */
/* 				"XX cut-off RPC reply header (verif)\n"); */
			return (-1);
		}

		verf_len = ntohl (ui [1]);

		ui += (verf_len + (2 * sizeof (*ui) + 3)) / sizeof (*ui);

		rh_len = 4 * (ui - (u_int32_t *) bp);

	}
	else {
		/* Doesn't look like an RPC header: punt. */
		return (-1);
	}

	return (rh_len);
}

static int get_auth (u_int32_t *ui, u_int32_t *euid, u_int32_t *egid)
{
	unsigned int *pi;
	unsigned int len;

	/*
	 * If the auth type isn't UNIX, then just give up.
	 *
	 * &&& Could also check to make sure that the length fields
	 * are something sensible, as an extra sanity check.
	 */

	if (ntohl (ui [0]) != AUTH_UNIX) {
	    // printf ("XX Not Auth_Unix (%d)??\n", ntohl (ui [0]));
		return (-1);
	}

	/*
	 * For UNIX, ui[1] is the total length of the auth and ui[2]
	 * is some kind of cookie.  ui[3] is the length of the next
	 * field, which doesn't seem to be well documented anywhere. 
	 * Set pi to point past this opaque field.  Then pi[0] is the
	 * euid, pi[1] is the egid, and pi[2] is the total number of
	 * groups.  At some point we might be interested in the total
	 * number of groups, and what they all are, but not today.
	 */

	len = ntohl (ui [3]);
	if (len % 4) {
		len /= 4;
		len++;
	}
	else {
		len /= 4;
	}

	pi = &ui [4 + len];

	if (euid != NULL)
		*euid = ntohl (pi [0]);

	if (egid != NULL)
		*egid = ntohl (pi [1]);

	return (0);
}

int getData (u_char *bp, u_char *buf, unsigned int maxlen)
{
	return (0);
}

/*
 * Returns a pointer to the hash_t structure for the given xid (or
 * NULL if any) and removes it from the hash table.  Doesn't free the
 * pointer-- it is the responsibility of the caller to free it after
 * use.
 */

#define	MAX_AGE		30

hash_t *hashLookupXid (u_int32_t now,
		u_int32_t xid, u_int32_t host, u_int32_t port)
{
	u_int32_t hashval;
	hash_t *curr, **prev_p;

	/*
	 * Heuristic-- as long as the current time is less
	 * than the GRACE_PERIOD (typically a few moments after
	 * the start of tracing, but it could also be set to
	 * the end of time...), accept anything without even looking.
	 *
	 * The reason is that XIDs pairs can easily get split over
	 * trace boundaries and we don't want to lose any replies just
	 * because we might have put the request into another file.
	 *
	 * NOT IMPLEMENTED YET!
	 */


	/*
	 * Otherwise, search the table.  If the XID is found, remove
	 * it; there should only be one entry corresponding to each
	 * XID so we won't be seeing it again.
	 */

	hashval = HASH (xid, host, port);
	prev_p = &xidHashTable [hashval];
	for (curr = xidHashTable [hashval]; curr != NULL; curr = curr->next) {


		if ((curr->rpcXID == xid) && (curr->srcHost == host) &&
				(curr->srcPort == port)) {
			*prev_p = curr->next;
			xidHashElemCnt--;
			return (curr);
		}
		prev_p = &curr->next;
	}


	return (NULL);
}

/*
 * There are a lot more fields, and they're important, but for now I'm
 * ignoring them.
 */

hash_t *hashInsertXid (u_int32_t now, u_int32_t xid,
		u_int32_t host, u_int32_t port,
		u_int32_t version, u_int32_t proc, u_int32_t prog)
{
	static int CullIndex = 0;
	u_int32_t then;
	u_int32_t hashval = HASH (xid, host, port);
	hash_t *new = (hash_t *) malloc (sizeof (hash_t));
	hash_t *curr, *next, *old;

	new->rpcXID = xid;
	new->srcHost = host;
	new->srcPort = port;
	new->nfsVersion = version;
	new->nfsProc = proc;
	new->nfsProg = prog;
	new->call_time = now;

	new->next = xidHashTable [hashval];
	xidHashTable [hashval] = new;

	xidHashElemCnt++;

	/*
	 * HACK ALERT!
	 *
	 * Due to dropped packets, some requests never get responses. 
	 * This causes the table to leak.  To make sure that
	 * eventually all the garbage has a chance to be collected,
	 * every time we do a lookup, we pick a hash bucket to cull. 
	 * In order to prevent garbage from hiding, we cull a
	 * different bucket each time, working our way around the
	 * whole table.
	 */

	then = now - MAX_AGE;
	for (curr = xidHashTable [CullIndex]; curr != NULL; curr = next) {
		next = curr->next;

		if (curr->call_time < then) {
			old = hashLookupXid (now, curr->rpcXID, curr->srcHost,
					curr->srcPort);
			assert (old != NULL);
			free (old);
		}
	}
	CullIndex = (CullIndex + 1) % HASHSIZE;

	return new;
}

/*
 * end of nfsrecord.c
 */
