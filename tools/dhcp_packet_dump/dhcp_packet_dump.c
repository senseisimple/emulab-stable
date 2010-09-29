#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

#include "dhcp_options.h"

#define STR(x) #x

#define MAGIC_COOKIE                  0x63825363

#define OPTION_TYPE_NONE                   0
#define OPTION_TYPE_IP_ADDRESS             1
#define OPTION_TYPE_IP_ADDRESS_PAIR        2
#define OPTION_TYPE_STRING                 3
#define OPTION_TYPE_DATA                   4
#define OPTION_TYPE_BOOL                   5
#define OPTION_TYPE_INT8                   6
#define OPTION_TYPE_INT16                  7
#define OPTION_TYPE_INT32                  8
#define OPTION_TYPE_INT64                  9

#define ERROR_NONE                         0
#define ERROR_MEM                          1
#define ERROR_INVALID_LENGTH               2
#define ERROR_INVALID_VALUE                3

struct option {
	uint8_t code;
	int length;
	struct option *next;
	struct option *child;
	uint8_t data[1];
};

struct dhcp_option_info {
	const char *name;
	int (*parser)(struct option *option, char **out);
	int list;
};

int parse_ip_address(struct option *option, char **out);
int parse_ip_address_pair(struct option *option, char **out);
int parse_string(struct option *option, char **out);
int parse_data(struct option *option, char **out);
int parse_int8(struct option *option, char **out);
int parse_int16(struct option *option, char **out);
int parse_int32(struct option *option, char **out);
int parse_int64(struct option *option, char **out);
int parse_bool(struct option *option, char **out);
int parse_encapsulated(struct option *option, char **out);

int _parse_data(uint8_t *data, size_t len, char **out);

int parse_values = 1;
int use_pretty_names = 1;

struct dhcp_option_info rfc2132_option_info[] = {
	{STR(OPT_PAD),                          NULL,                   0},
	{STR(OPT_SUBNET_MASK),                  parse_ip_address,       0},
	{STR(OPT_TIME_OFFSET),                  parse_int32,            0},
	{STR(OPT_ROUTER),                       parse_ip_address,       1},
	{STR(OPT_RFC868_SERVER),                parse_ip_address,       1},
	{STR(OPT_IEN116_SERVER),                parse_ip_address,       1},
	{STR(OPT_DNS_SERVER),                   parse_ip_address,       1},
	{STR(OPT_LOG_SERVER),                   parse_ip_address,       1},
	{STR(OPT_COOKIE_SERVER),                parse_ip_address,       1},
	{STR(OPT_LPR_SERVER),                   parse_ip_address,       1},
	{STR(OPT_IMPRESS_SERVER),               parse_ip_address,       1},
	{STR(OPT_RFC887_SERVER),                parse_ip_address,       1},
	{STR(OPT_HOST_NAME),                    parse_string,           0},
	{STR(OPT_BOOT_FILE_SIZE),               parse_int16,            0},
	{STR(OPT_MERIT_DUMP_FILE),              parse_string,           0},
	{STR(OPT_DOMAIN_NAME),                  parse_string,           0},
	{STR(OPT_SWAP_SERVER),                  parse_ip_address,       0},
	{STR(OPT_ROOT_PATH),                    parse_string,           0},
	{STR(OPT_EXTENSIONS_PATH),              parse_string,           0},
	{STR(OPT_IP_FORWARDING),                parse_bool,             0},
	{STR(OPT_NONLOCAL_SOURCE_ROUTING),      parse_bool,             0},
	{STR(OPT_POLICY_FILTER),                parse_ip_address_pair,  1},
	{STR(OPT_MAX_DATAGRAM_SIZE),            parse_int16,            0},
	{STR(OPT_DEFAULT_IP_TTL),               parse_int8,             0},
	{STR(OPT_PnATH_MTU_AGING_TIMEOUT),      parse_int32,            0},
	{STR(OPT_PATH_MTU_PLATEAU_TABLE),       parse_int16,            1},
	{STR(OPT_INTERFACE_MTU),                parse_int16,            0},
	{STR(OPT_ALL_SUBNETS_LOCAL),            parse_bool,             0},
	{STR(OPT_BROADCAST_ADDRESS),            parse_ip_address,       0},
	{STR(OPT_PERFORM_MASK_DISCOVERY),       parse_bool,             0},
	{STR(OPT_MASK_SUPPLIER),                parse_bool,             0},
	{STR(OPT_PERFORM_ROUTER_DISCOVERY),     parse_bool,             0},
	{STR(OPT_ROUTER_SOLICITATION_ADDRESS),  parse_ip_address,       0},
	{STR(OPT_STATIC_ROUTE),                 parse_ip_address_pair,  1},
	{STR(OPT_TRAILER_ENCAPSULATION),        parse_bool,             0},
	{STR(OPT_ARP_CACHE_TIMEOUT),            parse_int32,            0},
	{STR(OPT_ETHERNET_ENCAPSULATION),       parse_bool,             0},
	{STR(OPT_TCP_DEFAULT_TTL),              parse_int8,             0},
	{STR(OPT_TCP_KEEPALIVE_INTERVAL),       parse_int32,            0},
	{STR(OPT_TCP_KEEPALIVE_GARBAGE),        parse_bool,             0},
	{STR(OPT_NIS_DOMAIN),                   parse_string,           0},
	{STR(OPT_NIS_SERVER),                   parse_ip_address,       1},
	{STR(OPT_NTP_SERVER),                   parse_ip_address,       1},
	{STR(OPT_VENDOR_SPECIFIC_INFO),         parse_encapsulated,             0},
	{STR(OPT_NETBIOS_NAME_SERVER),          parse_ip_address,       1},
	{STR(OPT_NETBIOS_DATAGRAM_DIST_SERVER), parse_ip_address,       1},
	{STR(OPT_NETBIOS_NODE_TYPE),            parse_int8,             0},
	{STR(OPT_NETBIOS_SCOPE),                parse_string,           0},
	{STR(OPT_XWIN_FONT_SERVER),             parse_ip_address,       1},
	{STR(OPT_XWIN_DISPLAY_MANAGER),         parse_ip_address,       1},
	{STR(OPT_REQUESTED_IP_ADDRESS),         parse_ip_address,       0},
	{STR(OPT_IP_ADDRESS_LEASE_TIME),        parse_int32,            0},
	{STR(OPT_OPTION_OVERLOAD),              parse_int8,             0},
	{STR(OPT_DHCP_MESSAGE_TYPE),            parse_int8,             0},
	{STR(OPT_SERVER_IDENTIFIER),            parse_ip_address,       0},
	{STR(OPT_PARAMETER_REQUEST_LIST),       parse_data,             0},
	{STR(OPT_MESSAGE),                      parse_string,           0},
	{STR(OPT_MAX_DHCP_MESSAGE_SIZE),        parse_int16,            0},
	{STR(OPT_RENEWAL_TIME_VALUE),           parse_int32,            0},
	{STR(OPT_REBINDING_TIME_VALUE),         parse_int32,            0},
	{STR(OPT_VENDOR_CLASS_ID),              parse_string,           0},
	{STR(OPT_CLIENT_ID),                    parse_data,             0},
	{"",                                    NULL,             0},
	{"",                                    NULL,             0},
	{STR(OPT_NISPLUS_DOMAIN),               parse_string,           0},
	{STR(OPT_NISPLUS_SERVER),               parse_ip_address,       1},
	{STR(OPT_TFTP_SERVER),                  parse_string,           0},
	{STR(OPT_BOOTFILE_NAME),                parse_string,           0},
	{STR(OPT_MOBILE_IP_HOME_AGENT),         parse_ip_address,       1},
	{STR(OPT_SMTP_SERVER),                  parse_ip_address,       1},
	{STR(OPT_POP3_SERVER),                  parse_ip_address,       1},
	{STR(OPT_NNTP_SERVER),                  parse_ip_address,       1},
	{STR(OPT_DEFAULT_WWW_SERVER),           parse_ip_address,       1},
	{STR(OPT_DEFAULT_FINGER_SERVER),        parse_ip_address,       1},
	{STR(OPT_DEFAULT_IRC_SERVER),           parse_ip_address,       1},
	{STR(OPT_STREETTALK_SERVER),            parse_ip_address,       1},
	{STR(OPT_STREETTALK_STDA_SERVER),       parse_ip_address,       1},
	{STR(OPT_USER_CLASS),                   parse_data,             0},
	{STR(OPT_SLP_DIRECTORY_AGENT),          parse_data,             0},
	{STR(OPT_SLP_SERVICE_SCOPE),            parse_data,             0},
	{STR(OPT_RAPID_COMMIT),                 parse_bool,             0},
	{STR(OPT_CLIENT_FQDN),                  parse_data,             0},
	{STR(OPT_RELAY_AGENT_INFO),             parse_data, /* encap */ 0},
	{STR(OPT_ISNS),                         parse_data,             0},
	{STR(OPT_NDS_SERVERS),                  parse_ip_address,       1},
	{STR(OPT_NDS_TREE_NAME),                parse_string,           0},
	{STR(OPT_NDS_CONTEXT),                  parse_string,           0},
	{STR(OPT_BCMCS_CONTROLLER),             parse_string,           0},
	{"", /*89*/                             NULL,             0},
	{STR(OPT_DHCP_AUTHENTICATION),          parse_data,             0},
	{"", /*91*/                             NULL,             0},
	{"", /*92*/                             NULL,             0},
	{STR(OPT_PXE_CLIENT_ARCH),              parse_int16,            0},
	{STR(OPT_PXE_CLIENT_IFACE_ID),          parse_data,             0},
	{"", /*95*/                             NULL,             0},
	{"", /*96*/                             NULL,             0},
	{STR(OPT_PXE_CLIENT_MACHINE_ID),        parse_data,             0},
	{STR(OPT_USER_AUTHENTICATION),          parse_string,           0},
	{STR(OPT_CIVIC_LOCATION),               parse_data,             0},
	{STR(OPT_TIMEZONE_POSIX_STRING),        parse_string,           0},
	{STR(OPT_TIMEZONE_DATABASE_STRING),     parse_string,           0},
	{"", /*102*/                            NULL,             0},
	{"", /*103*/                            NULL,             0},
	{"", /*104*/                            NULL,             0},
	{"", /*105*/                            NULL,             0},
	{"", /*106*/                            NULL,             0},
	{"", /*106*/                            NULL,             0},
	{"", /*108*/                            NULL,             0},
	{"", /*109*/                            NULL,             0},
	{"", /*110*/                            NULL,             0},
	{"", /*111*/                            NULL,             0},
	{"", /*112*/                            NULL,             0},
	{"", /*113*/                            NULL,             0},
	{"", /*114*/                            NULL,             0},
	{"", /*115*/                            NULL,             0},
	{STR(OPT_AUTOCONFIGURE),                parse_bool,             0},
	{STR(OPT_NAME_SERVICE_SEARCH),          parse_int16,            1},
	{STR(OPT_SUBNET_SELECTION),             parse_ip_address,       0},
	{STR(OPT_DOMAIN_SEARCH),                parse_data, /* XXX */   0},
	{STR(OPT_SIP_SERVER),                   parse_data, /* XXX */   0},
	{STR(OPT_CLASSLESS_ROUTE),              parse_data,             0},
	{STR(OPT_CABLELABS_CLIENT_CONFIG),      parse_data, /* encap */ 0},
	{STR(OPT_LOCATION_CONFIG_INFO),         parse_data,             0},
	{STR(OPT_VENDOR_IDENT_CLASS),           parse_data,             0},
	{STR(OPT_VENDOR_IDENT_SPECIFIC_INFO),   parse_data,             0},
	{"", /*126*/                            NULL,             0},
	{"", /*127*/                            NULL,             0},
	{"", /*128*/                            NULL,             0},
	{"", /*129*/                            NULL,             0},
	{"", /*130*/                            NULL,             0},
	{"", /*131*/                            NULL,             0},
	{"", /*132*/                            NULL,             0},
	{"", /*133*/                            NULL,             0},
	{"", /*134*/                            NULL,             0},
	{"", /*135*/                            NULL,             0},
	{STR(OPT_PANA_AGENT),                   parse_ip_address,       1},
	{STR(OPT_LOST_SERVER),                  parse_string, /* XXX */ 0},
	{STR(OPT_CAPWAP_AC),                    parse_ip_address,       1},
	{"", /*139*/                            NULL,             0},
	{"", /*140*/                            NULL,             0},
	{"", /*141*/                            NULL,             0},
	{"", /*142*/                            NULL,             0},
	{"", /*143*/                            NULL,             0},
	{"", /*144*/                            NULL,             0},
	{"", /*145*/                            NULL,             0},
	{"", /*146*/                            NULL,             0},
	{"", /*147*/                            NULL,             0},
	{"", /*148*/                            NULL,             0},
	{"", /*149*/                            NULL,             0},
	{STR(OPT_TFTP_SERVER_LIST),             parse_ip_address,       1},
	{NULL,                                  NULL,                           0},
};

struct dhcp_option_info pxe_option_info[] = {
	{"",                                    NULL,                           0},
	{STR(OPT_PXE_MTFTP_IP),                 parse_ip_address,       0},
	{STR(OPT_PXE_MTFTP_CPORT),              parse_int16,            0},
	{STR(OPT_PXE_MTFTP_SPORT),              parse_int16,            0},
	{STR(OPT_PXE_MTFTP_TMOUT),              parse_int8,             0},
	{STR(OPT_PXE_MTFTP_DELAY),              parse_int8,             0},
	{STR(OPT_PXE_DISCOVERY_CONTROL),        parse_int8,             0},
	{STR(OPT_PXE_DISCOVER_MCAST_ADDR),      parse_ip_address,       0},
	{STR(OPT_PXE_BOOT_SERVERS),             parse_data,             0},
	{STR(OPT_PXE_BOOT_MENU),                parse_data,             0},
	{STR(OPT_PXE_MENU_PROMPT),              parse_data,             0},
	{STR(OPT_PXE_MCAST_ADDRS_ALLOC),        parse_int64,            0},
	{STR(OPT_PXE_CREDENTIAL_TYPES),         parse_int32,            1},
	{STR(OPT_PXE_BOOT_ITEM),                parse_int32,            0},
	{NULL,                                  NULL,                           0},
};

struct bootp_packet {
	uint8_t op;
	uint8_t htype;
	uint8_t hlen;
	uint8_t hops;
	uint32_t xid;
	uint16_t secs;
	uint16_t flags;
	in_addr_t ciaddr;
	in_addr_t yiaddr;
	in_addr_t siaddr;
	in_addr_t giaddr;
	uint8_t chaddr[16];
	uint8_t sname[64];
	uint8_t file[128];
	uint8_t vend[64];
} __attribute__((__packed__));

int print_bootp_fields(struct bootp_packet *packet, int overload);
int print_options(struct option *option_list, char *prefix,
                  struct dhcp_option_info *option_info);
void free_option_list(struct option *option_list);
int add_option(struct option **list, uint8_t code, uint8_t *data, size_t length);
struct option *find_option(struct option *list, uint8_t code);
int parse_options(uint8_t *data, size_t size, struct option **listptr);


int main(int argc, char **argv)
{
	FILE *response;
	char *filename;
	struct bootp_packet *packet;
	size_t size;
	char *buffer;
	uint32_t cookie;
	char c;

	opterr = 0;

	while ((c = getopt (argc, argv, "nr")) != -1) {
		switch(c) {
		case 'n': use_pretty_names = 0; break;
		case 'r': parse_values = 0; break;
		case '?':
			if (isprint(optopt))
				fprintf(stderr, "Unknown option '-%c'.\n",
				        optopt);
			else
				fprintf(stderr, "Unknown option character '\\x%x'.\n",
				        optopt);

			return 1;
			break;
		default:
			abort();
		}
	}

	if (optind == argc) {
		fprintf(stderr, "Usage: dhcp_packet_dump [-nr] <filename>\n");
		return 1;
	}
	
	filename = argv[optind];

	response = fopen(filename, "r");
	if (!response) {
		fprintf(stderr, "failed to open file \"%s\": %s\n",
		        filename, strerror(errno));
		return 1;
	}

	fseek(response, 0, SEEK_END);
	size = ftell(response);
	fseek(response, 0, SEEK_SET);

	packet = malloc(size);
	if (!packet) {
		fprintf(stderr, "failed to allocate memory for packet\n");
		fclose(response);
		return 1;
	}

	if (fread(packet, size, 1, response) < 1) {
		fprintf(stderr, "failed to read response\n");
		free(packet);
		fclose(response);
		return 1;
	}

	fclose(response);

	cookie = ntohl(*(uint32_t *)packet->vend);
	if (cookie != MAGIC_COOKIE) {
		_parse_data(packet->vend, sizeof(packet->vend), &buffer);
		print_bootp_fields(packet, 0);
		printf("VEND=%s\n", buffer);
		free(buffer);
		return 0;
	} else {
		size_t option_size;
		struct option *option_list;
		struct option *overload;
		int ol;

		option_list = NULL;
		option_size = size - (&packet->vend[4] - (uint8_t *)packet);
		parse_options(&packet->vend[4], option_size, &option_list);

		overload = find_option(option_list, OPT_OPTION_OVERLOAD);

		if (overload && overload->length >= 1) {
			ol = overload->data[0];
			if (ol & 1)
				parse_options(packet->file,
				              sizeof(packet->file),
				              &option_list);

			if (ol & 2)
				parse_options(packet->sname,
				              sizeof(packet->sname),
				              &option_list);
		}

		print_options(option_list, "", rfc2132_option_info);

		print_bootp_fields(packet, ol);

		free_option_list(option_list);
	}
	
	free(packet);
	
	return 0;
}

void free_option_list(struct option *option_list)
{
	struct option *current, *next;

	for (current = option_list; current; current = next) {
		if (current->child)
			free_option_list(current->child);
		next = current->next;
		free(current);
	}
}

int add_option(struct option **list, uint8_t code, uint8_t *data, size_t length)
{
	struct option *current, *last;

	current = *list;

	last = NULL;
	while (current) {
		if (current->code == code)
			break;
		last = current;
		current = current->next;
	}

	if (!current) {
		/* Option wasn't found; create it */

		current = malloc(sizeof(*current) + length);
		if (!current)
			return ERROR_MEM;

		current->code = code;
		current->length = length;
		memcpy(current->data, data, length);
		current->next = NULL;
		current->child = NULL;
	} else {
		void *tmp = realloc(current, current->length + length);
		if (!tmp)
			return ERROR_MEM;

		current = tmp;
		memcpy(current->data + current->length, data, length);
		current->length += length;
	}
		
	if (last)
		last->next = current;
	else
		*list = current;

	return ERROR_NONE;
}

struct option *find_option(struct option *list, uint8_t code)
{
	struct option *current = list;

	while (current && current->code != code)
		current = current->next;

	return current;
}

int parse_options(uint8_t *data, size_t size, struct option **listptr)
{
	uint8_t *p;
	size_t len;
	int rc;

	p = data;
	
	while (p < data + size) {
		if (*p == OPT_PAD) {
			p++;
			continue;

		} else if (*p == OPT_END) {
			break;
		}
		
		len = *(p + 1);

		if (p + len > data + size)
			return ERROR_INVALID_LENGTH;

		rc = add_option(listptr, *p, p + 2, len);
		if (rc)
			return rc;

		p += len + 2;
	}

	return ERROR_NONE;
}

int print_options(struct option *option_list, char *prefix,
                  struct dhcp_option_info *option_info)
{
	char tmp[128];
	struct option *current;
	int rc;
	char *buffer;
	int option_count;

	option_count = 0;
	while (option_info && option_info[option_count].name != NULL) {
		option_count++;
	}
	option_count++;

	for (current = option_list; current; current = current->next) {
		buffer = NULL;
		if (parse_values && option_info &&
		    option_info[current->code].parser) {
			rc = option_info[current->code].parser(current,
			                                       &buffer);
			/* Option didn't meet the parser's requirements
			   (min/max length, etc).  Print it as hexdump
			   instead).
			*/
			if (rc) {
				if (buffer)
					free(buffer);
				rc = parse_data(current, &buffer);
			}
		} else {
			rc = parse_data(current, &buffer);
		}

		if (current->child) {
			struct dhcp_option_info *child_info = NULL;

			if (current->code == OPT_VENDOR_SPECIFIC_INFO) {
				child_info = &pxe_option_info[0];
			}

			memset(tmp, 0, sizeof(tmp));
			if (use_pretty_names && option_info &&
			    option_info[current->code].name[0]) {
				snprintf(tmp, sizeof(tmp) - 1, "%s%s_SUB",
				         prefix,
				         option_info[current->code].name);
			} else {
				snprintf(tmp, sizeof(tmp) - 1, "%sOPT%u_SUB",
				         prefix, current->code);
			}
			print_options(current->child, tmp, child_info);
		} else {
			if (use_pretty_names && current->code < option_count &&
			    option_info[current->code].name[0])
				printf("%s%s=%s\n", prefix,
				       option_info[current->code].name, buffer);
			else
				printf("%sOPT_%u=%s\n", prefix, current->code,
				       buffer);
			free(buffer);
		}
	}
	
	return ERROR_NONE;
}

int print_bootp_fields(struct bootp_packet *packet, int overload)
{
	struct in_addr addr;
	char buffer[129];
	int i;
	
	printf("OP=%u\n", packet->op);
	printf("HTYPE=%u\n", packet->htype);
	printf("HLEN=%u\n", packet->hlen);
	printf("HOPS=%u\n", packet->hops);
	printf("XID=0x%08x\n", ntohl(packet->xid));
	printf("SECS=%u\n", ntohs(packet->secs));
	printf("FLAGS=0x%04x\n", ntohs(packet->flags));

	addr.s_addr = packet->ciaddr;
	inet_ntop(AF_INET, &addr, buffer, sizeof(buffer));
	printf("CIADDR=%s\n", buffer);

	addr.s_addr = packet->yiaddr;
	inet_ntop(AF_INET, &addr, buffer, sizeof(buffer));
	printf("YIADDR=%s\n", buffer);

	addr.s_addr = packet->siaddr;
	inet_ntop(AF_INET, &addr, buffer, sizeof(buffer));
	printf("SIADDR=%s\n", buffer);

	addr.s_addr = packet->giaddr;
	inet_ntop(AF_INET, &addr, buffer, sizeof(buffer));
	printf("GIADDR=%s\n", buffer);

	printf("CHADDR=");
	for (i = 0; i < packet->hlen; i++) {
		printf("%02x", packet->chaddr[i]);
		if (i < packet->hlen - 1)
			printf(":");
	}
	printf("\n");

	if (!(overload & 1)) {
		memcpy(buffer, packet->file, sizeof(packet->file));
		buffer[128] = '\0';
		printf("FILE=%s\n", buffer);
	}

	if (!(overload & 2)) {
		memcpy(buffer, packet->sname, sizeof(packet->sname));
		buffer[64] = '\0';
		printf("SNAME=%s\n", buffer);
	}

	return ERROR_NONE;
}


int _parse_ip_address(uint8_t *data, size_t len, int pair, char **out)
{
	struct in_addr ip;
	size_t buffer_len;
	char *buffer, *b;
	uint8_t *p;
	int first_half;

	if (!len || (pair && len % 8) || len % 4)
		return ERROR_INVALID_LENGTH;

	/*
	 * The buffer must be long enough to hold all
	 * addresses with a space between each one
	 * (and a terminating null at the end).
	 */
	buffer_len = (len / 4) * (INET_ADDRSTRLEN);
	buffer = malloc(buffer_len);
	if (!buffer)
		return ERROR_MEM;

	p = data;
	b = buffer;
	first_half = 1;
	while (len > 0) {
		const char *addr_str;
		ip.s_addr = *(in_addr_t *)p;
		addr_str = inet_ntop(AF_INET, &ip, b,
					INET_ADDRSTRLEN);
		if (addr_str == NULL) {
			perror("parse_ip_address");
			free(buffer);
			return ERROR_INVALID_VALUE;
		}

		b += strlen(b);

		p += 4;
		len -= 4;

		/* Change '\0' to ' ' and point to
		 * next byte in buffer if not last
		 * ip address.
		 */
		if (len > 0) {
			if (pair && first_half)
				*b++ = ':';
			else
				*b++ = ' ';

			if (pair)
				first_half = !first_half;
		}
	}

	*out = buffer;

	return ERROR_NONE;
}

int parse_ip_address(struct option *option, char **out) {
	return _parse_ip_address(option->data, option->length, 0, out);
}

int parse_ip_address_pair(struct option *option, char **out) {
	return _parse_ip_address(option->data, option->length, 1, out);
}

int _parse_string(uint8_t *data, size_t len, char **out)
{
	char *buffer;

	if (!len)
		return ERROR_INVALID_LENGTH;

	buffer = malloc(len + 1);
	if (!buffer)
		return ERROR_MEM;

	memcpy(buffer, data, len);
	buffer[len] = '\0';

	*out = buffer;

	return ERROR_NONE;
}

int parse_string(struct option *option, char **out)
{
	return _parse_string(option->data, option->length, out);
}

int _parse_data(uint8_t *data, size_t len, char **out)
{
	uint8_t *p;
	char *buffer, *b;
	
	if (!len)
		return ERROR_INVALID_LENGTH;

	/* Allocate two characters for each
	 * data byte + 1 byte for ':' separator
	 * or terminating null.
	 */
	buffer = malloc(3 * sizeof(char) * len);
	if (!buffer)
		return ERROR_MEM;

	p = data;
	b = buffer;
	while (len > 0) {
		sprintf(b, "%02hhx", *p);

		b += 2;
		p++;
		len--;

		if (len > 0)
			*b++ = ':';
	}

	*out = buffer;

	return ERROR_NONE;
}

int parse_data(struct option *option, char **out)
{
	return _parse_data(option->data, option->length, out);
}

int _parse_bool(uint8_t *data, size_t len, char **out)
{
	char *buffer;

	if (len != 1)
		return ERROR_INVALID_LENGTH;

	if (*data & 0xfe)
		return ERROR_INVALID_VALUE;

	buffer = malloc(6);
	if (buffer == NULL)
		return ERROR_MEM;

	snprintf(buffer, 6, *data ? "true" : "false");

	*out = buffer;

	return ERROR_NONE;
}

int parse_bool(struct option *option, char **out)
{
	return _parse_bool(option->data, option->length, out);
}

int _parse_int(uint8_t *data, size_t len, int intsize, char **out)
{
	uint8_t *p;
	char *buffer;
	size_t buffer_len, buffer_used;

	if (!len || len % intsize)
		return ERROR_INVALID_LENGTH;

	buffer = NULL;
	buffer_len = 0;
	buffer_used = 0;
	p = data;
	while (len > 0) {
		void *tmp;
		uint64_t val;
		int string_len;

		if (intsize == 1)
			val = *p;
		else if (intsize == 2)
			val = ntohs(*(uint16_t *)p);
		else if (intsize == 4)
			val = ntohl(*(uint32_t *)p);
		else if (intsize == 8) {
			val = ((uint64_t)ntohl(*(uint32_t *)p) << 32 |
			       ntohl(*(uint32_t *)(p + 4)));
		} else {
			return ERROR_INVALID_LENGTH;
		}

		string_len = snprintf(buffer, 0, "%"PRIu64, val);
		tmp = realloc(buffer, buffer_len + string_len + 1);
		if (tmp == NULL) {
			if (buffer)
				free(buffer);
			return ERROR_MEM;
		}

		buffer = tmp;
		buffer_len += string_len + 1;

		snprintf(buffer + buffer_used, buffer_len - buffer_used,
		         "%"PRIu64, val);
		buffer_used += string_len + 1;

		p += intsize;
		len -= intsize;

		if (len > 0)
			*(buffer + buffer_used - 1) = ' ';
	}

	*out = buffer;

	return ERROR_NONE;
}

int parse_int8(struct option *option, char **out) {
	return _parse_int(option->data, option->length, 1, out);
}

int parse_int16(struct option *option, char **out) {
	return _parse_int(option->data, option->length, 2, out);
}

int parse_int32(struct option *option, char **out) {
	return _parse_int(option->data, option->length, 4, out);
}

int parse_int64(struct option *option, char **out) {
	return _parse_int(option->data, option->length, 8, out);
}

int parse_encapsulated(struct option *option, char **out) {
	return parse_options(option->data, option->length, &option->child);
}
