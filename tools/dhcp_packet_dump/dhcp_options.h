#ifndef __DHCP_OPTIONS_H__
#define __DHCP_OPTIONS_H__

/* RFC 2132 */
#define OPT_PAD                            0
#define OPT_SUBNET_MASK                    1
#define OPT_TIME_OFFSET                    2
#define OPT_ROUTER                         3
#define OPT_RFC868_SERVER                  4
#define OPT_IEN116_SERVER                  5
#define OPT_DNS_SERVER                     6
#define OPT_LOG_SERVER                     7
#define OPT_COOKIE_SERVER                  8
#define OPT_LPR_SERVER                     9
#define OPT_IMPRESS_SERVER                10
#define OPT_RFC887_SERVER                 11
#define OPT_HOST_NAME                     12
#define OPT_BOOT_FILE_SIZE                13
#define OPT_MERIT_DUMP_FILE               14
#define OPT_DOMAIN_NAME                   15
#define OPT_SWAP_SERVER                   16
#define OPT_ROOT_PATH                     17
#define OPT_EXTENSIONS_PATH               18
#define OPT_IP_FORWARDING                 19
#define OPT_NONLOCAL_SOURCE_ROUTING       20
#define OPT_POLICY_FILTER                 21
#define OPT_MAX_DATAGRAM_SIZE             22
#define OPT_DEFAULT_IP_TTL                23
#define OPT_PATH_MTU_AGING_TIMEOUT        24
#define OPT_PATH_MTU_PLATEAU_TABLE        25
#define OPT_INTERFACE_MTU                 26
#define OPT_ALL_SUBNETS_LOCAL             27
#define OPT_BROADCAST_ADDRESS             28
#define OPT_PERFORM_MASK_DISCOVERY        29
#define OPT_MASK_SUPPLIER                 30
#define OPT_PERFORM_ROUTER_DISCOVERY      31
#define OPT_ROUTER_SOLICITATION_ADDRESS   32
#define OPT_STATIC_ROUTE                  33
#define OPT_TRAILER_ENCAPSULATION         34
#define OPT_ARP_CACHE_TIMEOUT             35
#define OPT_ETHERNET_ENCAPSULATION        36
#define OPT_TCP_DEFAULT_TTL               37
#define OPT_TCP_KEEPALIVE_INTERVAL        38
#define OPT_TCP_KEEPALIVE_GARBAGE         39
#define OPT_NIS_DOMAIN                    40
#define OPT_NIS_SERVER                    41
#define OPT_NTP_SERVER                    42
#define OPT_VENDOR_SPECIFIC_INFO          43
#define OPT_NETBIOS_NAME_SERVER           44
#define OPT_NETBIOS_DATAGRAM_DIST_SERVER  45
#define OPT_NETBIOS_NODE_TYPE             46
#define OPT_NETBIOS_SCOPE                 47
#define OPT_XWIN_FONT_SERVER              48
#define OPT_XWIN_DISPLAY_MANAGER          49
#define OPT_REQUESTED_IP_ADDRESS          50
#define OPT_IP_ADDRESS_LEASE_TIME         51
#define OPT_OPTION_OVERLOAD               52
#define OPT_DHCP_MESSAGE_TYPE             53
#define OPT_SERVER_IDENTIFIER             54
#define OPT_PARAMETER_REQUEST_LIST        55
#define OPT_MESSAGE                       56
#define OPT_MAX_DHCP_MESSAGE_SIZE         57
#define OPT_RENEWAL_TIME_VALUE            58
#define OPT_REBINDING_TIME_VALUE          59
#define OPT_VENDOR_CLASS_ID               60
#define OPT_CLIENT_ID                     61

#define OPT_NISPLUS_DOMAIN                64
#define OPT_NISPLUS_SERVER                65
#define OPT_TFTP_SERVER                   66
#define OPT_BOOTFILE_NAME                 67
#define OPT_MOBILE_IP_HOME_AGENT          68
#define OPT_SMTP_SERVER                   69
#define OPT_POP3_SERVER                   70
#define OPT_NNTP_SERVER                   71
#define OPT_DEFAULT_WWW_SERVER            72
#define OPT_DEFAULT_FINGER_SERVER         73
#define OPT_DEFAULT_IRC_SERVER            74
#define OPT_STREETTALK_SERVER             75
#define OPT_STREETTALK_STDA_SERVER        76

/* RFC 3004 */
#define OPT_USER_CLASS                    77 /* data */

/* RFC 2610 */
#define OPT_SLP_DIRECTORY_AGENT           78 /* 1 byte + ip list */
#define OPT_SLP_SERVICE_SCOPE             79 /* 1 byte + UTF-8 string */

/* RFC 4039 */
#define OPT_RAPID_COMMIT                  80 /* no data (option presence
                                                indicates boolean true) */

/* RFC 4702 */
#define OPT_CLIENT_FQDN                   81 /* structured data */

/* RFC 3046 */
#define OPT_RELAY_AGENT_INFO              82 /* encapsulated options */

/* RFC 4174 */
#define OPT_ISNS                          83 /* structured data */

/* RFC 2241 */
#define OPT_NDS_SERVERS                   85 /* ip list */
#define OPT_NDS_TREE_NAME                 86 /* string */
#define OPT_NDS_CONTEXT                   87 /* string */

/* RFC 4280 */
#define OPT_BCMCS_CONTROLLER              88 /* string */

/* RFC 3118 */
#define OPT_DHCP_AUTHENTICATION           90 /* structured data */

/* RFC 4578 */
#define OPT_PXE_CLIENT_ARCH               93 /* 16-bit int */
#define OPT_PXE_CLIENT_IFACE_ID           94 /* data (3 bytes) */

/* RFC 4578 */
#define OPT_PXE_CLIENT_MACHINE_ID         97 /* data */

/* RFC 2485 */
#define OPT_USER_AUTHENTICATION           98 /* string */

/* RFC 4676 */
#define OPT_CIVIC_LOCATION                99 /* data */

/* RFC 4833 */
#define OPT_TIMEZONE_POSIX_STRING        100 /* string */
#define OPT_TIMEZONE_DATABASE_STRING     101 /* string */

/* RFC 2563 */
#define OPT_AUTOCONFIGURE                116 /* bool */

/* RFC 2937 */
#define OPT_NAME_SERVICE_SEARCH          117 /* list of 16-bit ints */

/* RFC 3011 */
#define OPT_SUBNET_SELECTION             118 /* single ip address */

/* RFC 3397 */
#define OPT_DOMAIN_SEARCH                119 /* string of RFC1035 and RFC3396 */

/* RFC 3361 */
#define OPT_SIP_SERVER                   120 /* byte + string of RFC1035
                                                and RFC3396 or
                                                list of ip addresses */

/* RFC 3442 */
#define OPT_CLASSLESS_ROUTE              121 /* structured data XXX */

/* RFC 3495 */
#define OPT_CABLELABS_CLIENT_CONFIG      122 /* encapsulated options */

/* RFC 3825 */
#define OPT_LOCATION_CONFIG_INFO         123 /* structured data */

/* RFC 3925 */ /* XXX bad names */
#define OPT_VENDOR_IDENT_CLASS           124 /* structured data */
#define OPT_VENDOR_IDENT_SPECIFC_INFO    125 /* structured data */

/* RFC 5192 */
#define OPT_PANA_AGENT                   136 /* ip list */

/* RFC 5223 */
#define OPT_LOST_SERVER                  137 /* fqdn */

/* RFC 5417 */
#define OPT_CAPWAP_AC                    138 /* ip list */

/* XXX used by etherboot (old) */
/* RFC 5859 */
#define OPT_TFTP_SERVER_LIST             150 /* ip list */

/* RFC 2132 */
#define OPT_END                          255

#endif /* __DHCP_OPTIONS_H__ */
