/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __RMCP_H__
#define __RMCP_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>



#define RMCP_COMPAT_PORT   0x26f  /* 623 -- insecure ASF 1.0 */
#define RMCP_SECURE_PORT   0x298  /* 664 -- secure ASF 2.0 */

#define RMCP_IANA_ENT_ID   0x000011be  /* 4542 -- ASF IANA num */
#define RMCP_VERSION_1_0   0x06   /* 0-5 == legacy RMCP; 6 == RMCP 1.0; 7-255 
				  * reserved */
#define RMCP_NOACK_SEQNO   0xff
#define RMCP_ACK_CLASS_BIT 0x80

#define RMCP_RSP_NXT_HDR   0x06  /* from the spec... */

#define RMCP_ROLE_OP       0x00  /* operator privilege*/
#define RMCP_ROLE_ADM      0x01  /* administrator privilege */

#define RMCP_SID_BYPASS    0x0   /* the bypass RMCP session id for unprotected
				    msgs */

typedef enum {
    RMCP_SUCCESS               = 0x00,
    RMCP_ERR_ALREADY_CONNECTED = 0x01,
    RMCP_ERR_SOCKET            = 0x02,
    RMCP_ERR_BAD_CLIENT_NAME   = 0x03,
    RMCP_ERR_NO_DATA           = 0x04,
    RMCP_ERR_UNKNOWN_CLASS     = 0x05,
    RMCP_ERR_NO_ACK_NEEDED     = 0x06,
    RMCP_ERR_NO_ACK_INVALID    = 0x07,
    RMCP_ERR_TIMEOUT           = 0x08,
    RMCP_ERR_MAX_RETRIES       = 0x09,
    RMCP_ERR_PROTOCOL          = 0x0a,
    RMCP_ERR_NO_SECURE_CREDS   = 0x0b,
} rmcp_error_t;


typedef enum {
    RMCP_FLAG_SECURE_B = 0,

    RMCP_FLAG_MAX_B,
} rmcp_flag_b_t;

typedef enum {
    RMCP_FLAG_SECURE = (1L << RMCP_FLAG_SECURE_B),
    
    RMCP_FLAG_MAX = (1L << RMCP_FLAG_MAX_B),
} rmcp_flag_t;


#ifdef SWIG
typedef unsigned char u_int8_t;
//typedef char int8_t;
typedef unsigned short int u_int16_t;
//typedef short int int16_t;
typedef unsigned int u_int32_t;
//typedef int int32_t;
#endif


#define RMCP_CLASS_ASF 0x06
#define RMCP_CLASS_IPMI 0x07

typedef struct rmcp_hdr {
    u_int8_t version;
    u_int8_t reserved;
    u_int8_t seqno;
    u_int8_t class;
} rmcp_hdr_t;

#define RMCP_ASF_TYPE_RESET                   0x10
#define RMCP_ASF_TYPE_POWER_UP                0x11
#define RMCP_ASF_TYPE_POWER_DOWN              0x12
#define RMCP_ASF_TYPE_POWER_CYCLE_RESET       0x13
#define RMCP_ASF_TYPE_PRESENCE_PONG           0x40
#define RMCP_ASF_TYPE_CAPABILITIES_RESPONSE   0x41
#define RMCP_ASF_TYPE_SYSSTATE_RESPONSE       0x42
#define RMCP_ASF_TYPE_OPEN_SESSION_RESPONSE   0x43
#define RMCP_ASF_TYPE_CLOSE_SESSION_RESPONSE  0x44
#define RMCP_ASF_TYPE_PRESENCE_PING           0x80
#define RMCP_ASF_TYPE_CAPABILITIES_REQUEST    0x81
#define RMCP_ASF_TYPE_SYSSTATE_REQUEST       0x82
#define RMCP_ASF_TYPE_OPEN_SESSION_REQUEST    0x83
#define RMCP_ASF_TYPE_CLOSE_SESSION_REQUEST   0x84
#define RMCP_ASF_TYPE_RAKP_MSG_1              0xc0
#define RMCP_ASF_TYPE_RAKP_MSG_2              0xc1
#define RMCP_ASF_TYPE_RAKP_MSG_3              0xc2

typedef struct rmcp_asf_data {
    u_int32_t iana_ent_id;
    u_int8_t type;
    u_int8_t tag;
    u_int8_t reserved;
    u_int8_t data_len;
    /** should be a byte array of length datalen */
    u_int8_t *data;
} rmcp_asf_data_t;

typedef struct rmcp_rsp_hdr {
    u_int32_t sid;
    u_int32_t seqno;
} rmcp_rsp_hdr_t;

typedef struct rmcp_rsp_tlr {
    u_int8_t pad_len;
    u_int8_t next_hdr;
    u_int8_t integrity[12];
} rmcp_rsp_tlr_t;
    
typedef struct rmcp_msg {
    rmcp_rsp_hdr_t *shdr;
    rmcp_hdr_t *hdr;
    rmcp_asf_data_t *data;
    rmcp_rsp_tlr_t *stlr;
    /* seems silly to put this here, but it's easier to compute validity
     * on the first pass over the msg to make this whole struct, rather than
     * re-marshalling the byte buffer from it in a later function.
     */
    int tlr_was_valid;
} rmcp_msg_t;

/** 
 * this struct provides state info for high-level functions...
 */
typedef struct rmcp_ctx {
    int sockfd;
    struct sockaddr_in client_sa;
    int timeout;
    int retries;

    /** "session" state */
    u_int8_t rmcp_seqno;
    u_int8_t rmcp_rsp_seqno;
    u_int8_t rsp_init;
    u_int8_t secure;

    /** user-supplied secure data */
    u_int8_t role;
    u_int8_t role_key[20];
    int role_key_len;
    u_int8_t gen_key[20];
    int gen_key_len;
    int uid_len;
    char *uid;

    /** console's chosen session id... */
    u_int32_t msid;
    /** client's chosen session id from OPEN_SESSION_RESPONSE */
    u_int32_t csid;
    /** a rand we choose */
    u_int8_t mrand[16];
    /** client rand */
    u_int8_t crand[16];
    /** client GUID */
    u_int8_t cGUID[16];
    /** 
     * session integrity key (derived from an HMAC on Rm|Rc|Role|uidlen|<uid>,
     * which is 96 bits (or trunc'd to 96 bits)
     *
     * actually, I now think the spec says the key is NOT trunc'd, so it's 20 
     * bytes.
     */
    u_int8_t sik[20];
} rmcp_ctx_t;

/** "Supported Entities" byte */
#define RMCP_SUPPORTED_IPMI   (1L << 7)
#define RMCP_SUPPORTED_ASF_1  (1L << 0)
/** "Supported Interactions" byte */
#define RMCP_SUPPORTED_SECURE (1L << 7)

typedef struct rmcp_asf_supported {
    u_int32_t iana_id;
    u_int32_t oem;
    
    u_int8_t ipmi;
    u_int8_t asf_1;
    u_int8_t secure_rmcp;
} rmcp_asf_supported_t;

/**
 * These are all for the commands second of two bytes.
 */
#define RMCP_ASF_COM_CD       (1L << 4)
#define RMCP_ASF_COM_DIAG     (1L << 3)
#define RMCP_ASF_COM_HDD_SAFE (1L << 2)
#define RMCP_ASF_COM_HDD      (1L << 1)
#define RMCP_ASF_COM_PXE      (1L << 0)

/**
 * These are for the system capabilities byte.
 */
#define RMCP_ASF_CAP_RESET_BOTH         (1L << 7)
#define RMCP_ASF_CAP_POWER_UP_BOTH      (1L << 6)
#define RMCP_ASF_CAP_POWER_DOWN_BOTH    (1L << 5)
#define RMCP_ASF_CAP_POWER_CYCLE_BOTH   (1L << 4)
#define RMCP_ASF_CAP_RESET_SECURE       (1L << 3)
#define RMCP_ASF_CAP_POWER_UP_SECURE    (1L << 2)
#define RMCP_ASF_CAP_POWER_DOWN_SECURE  (1L << 1)
#define RMCP_ASF_CAP_POWER_CYCLE_SECURE (1L << 0)

/** 
 * These are for the system firmware four bytes (i.e., *_Bn_*)
 */
#define RMCP_ASF_FCAP_B1_SLEEP_BUTTON_LOCK    (1L << 6)
#define RMCP_ASF_FCAP_B1_LOCK_KEYBOARD        (1L << 5)
#define RMCP_ASF_FCAP_B1_RESET_BUTTON_LOCK    (1L << 2)
#define RMCP_ASF_FCAP_B1_POWER_BUTTON_LOCK    (1L << 1)
#define RMCP_ASF_FCAP_B1_FW_VERB_SCREEN_BLANK (1L << 0)

#define RMCP_ASF_FCAP_B2_CONFIG_DATA_RESET  (1L << 7)
#define RMCP_ASF_FCAP_B2_FW_VERB_QUIET      (1L << 6)
#define RMCP_ASF_FCAP_B2_FW_VERB_VERB       (1L << 5)
#define RMCP_ASF_FCAP_B2_FORCED_PROG_EVENTS (1L << 4)
#define RMCP_ASF_FCAP_B2_USER_PASSWD_BYPASS (1L << 3)

typedef struct rmcp_asf_capabilities {
    u_int32_t iana_id;
    u_int32_t oem;
    struct {
	u_int8_t force_cd_boot;
	u_int8_t force_diag_boot;
	u_int8_t force_hdd_safe_boot;
	u_int8_t force_hdd_boot;
	u_int8_t force_pxe_boot;
    } commands;
    struct {
	u_int8_t reset_both;
	u_int8_t power_up_both;
	u_int8_t power_down_both;
	u_int8_t power_cycle_both;
	u_int8_t reset_secure;
	u_int8_t power_up_secure;
	u_int8_t power_down_secure;
	u_int8_t power_cycle_secure;
    } capabilities;
    struct {
	u_int8_t sleep_button_lock;
	u_int8_t lock_keyboard;
	u_int8_t reset_button_lock;
	u_int8_t power_button_lock;
	u_int8_t firmware_screen_blank;

	u_int8_t config_data_reset;
	u_int8_t firmware_quiet;
	u_int8_t firmware_verbose;
	u_int8_t forced_progress_events;
	u_int8_t user_passwd_bypass;
    } firmware_capabilities;
} rmcp_asf_capabilities_t;

#define RMCP_ASF_SYSSTATEMASK         0x0f /* last four bits */

#define RMCP_ASF_SYSSTATE_S0_G0       0x00
#define RMCP_ASF_SYSSTATE_S1          0x01
#define RMCP_ASF_SYSSTATE_S2          0x02
#define RMCP_ASF_SYSSTATE_S3          0x03
#define RMCP_ASF_SYSSTATE_S4          0x04
#define RMCP_ASF_SYSSTATE_S5_G2       0x05
#define RMCP_ASF_SYSSTATE_S4_S5       0x06
#define RMCP_ASF_SYSSTATE_G3          0x07
#define RMCP_ASF_SYSSTATE_S1_S2_S3    0x08
#define RMCP_ASF_SYSSTATE_G1          0x09
#define RMCP_ASF_SYSSTATE_S5_OVERRIDE 0x0a
#define RMCP_ASF_SYSSTATE_LON         0x0b
#define RMCP_ASF_SYSSTATE_LOFF        0x0c
#define RMCP_ASF_SYSSTATE_UNKNOWN     0x0e
#define RMCP_ASF_SYSSTATE_RESERVED    0x0f

#define RMCP_ASF_WDSTATEMASK_EXPIRED  (1L << 0)

typedef struct rmcp_asf_sysstate {
    u_int8_t sysstate;
    u_int8_t wdstate;
} rmcp_asf_sysstate_t;


#define RMCP_RSP_RAKP_SUCCESS                     0x0
#define RMCP_RSP_RAKP_ERR_RESOURCES               0x01
#define RMCP_RSP_RAKP_ERR_INVALID_SID             0x02
#define RMCP_RSP_RAKP_ERR_INVALID_PAYLOAD         0x03
#define RMCP_RSP_RAKP_ERR_INVALID_AUTH_ALG        0x04
#define RMCP_RSP_RAKP_ERR_INVALID_INTEGRITY_ALG   0x05
#define RMCP_RSP_RAKP_ERR_NOMATCH_AUTH            0x06
#define RMCP_RSP_RAKP_ERR_NOMATCH_INTEGRITY       0x07
#define RMCP_RSP_RAKP_ERR_INACTIVE_SID            0x08
#define RMCP_RSP_RAKP_ERR_INVALID_ROLE            0x09
#define RMCP_RSP_RAKP_ERR_UNAUTH_ROLE             0x0a
#define RMCP_RSP_RAKP_ERR_RESOURCES_ROLE          0x0b
#define RMCP_RSP_RAKP_ERR_INVALID_NAMELEN         0x0c
#define RMCP_RSP_RAKP_ERR_UNAUTH_NAME             0x0d
#define RMCP_RSP_RAKP_ERR_UNAUTH_GUID             0x0e
#define RMCP_RSP_RAKP_ERR_INVALID_INTEGRITY_VALUE 0x0f



/**
 * A Whole Bunch O' Function Defs!
 */

/**
 * These are convenience functions that help us get the right stuff from perl
 * to C.  Fortunately, we just need a generator from string to u_int8_t.
 */
u_int8_t *swr_fill_str(char *str,int len);

/* since I can't figure out how to pass a pointer-to-pointer-to-struct from
 * perl, just wrap the ping and get functions.
 */
rmcp_error_t swr_rmcp_asf_ping(rmcp_ctx_t *rit,
			       rmcp_asf_supported_t *supported);
rmcp_error_t swr_rmcp_asf_get_capabilities(rmcp_ctx_t *rit,
					   rmcp_asf_capabilities_t *cap);
rmcp_error_t swr_rmcp_asf_get_sysstate(rmcp_ctx_t *rit,
				       rmcp_asf_sysstate_t *state);


/** 
 * These are a bunch of high-level functions that perform all relevant ASF
 * functions for the user, and give all info they might want...
 */
rmcp_error_t rmcp_asf_ping(rmcp_ctx_t *rit,
			   rmcp_asf_supported_t **supported);
rmcp_error_t rmcp_asf_get_capabilities(rmcp_ctx_t *rit,
				       rmcp_asf_capabilities_t **cap);
rmcp_error_t rmcp_asf_get_sysstate(rmcp_ctx_t *rit,
				   rmcp_asf_sysstate_t **state);
/**
 * XXX: fix reset, up, cycle, to take boot options for inclusion in the data
 * field.  For now, just set a 0-length data field.
 */
rmcp_error_t rmcp_asf_reset(rmcp_ctx_t *rit);
rmcp_error_t rmcp_asf_power_up(rmcp_ctx_t *rit);
rmcp_error_t rmcp_asf_power_cycle(rmcp_ctx_t *rit);
rmcp_error_t rmcp_asf_power_down(rmcp_ctx_t *rit);

char *rmcp_error_tostr(rmcp_error_t rerrno);
char *rmcp_asf_sysstate_tostr(int sysstate);
char *rmcp_asf_wdstate_tostr(int wdstate);
char *rmcp_rsp_rakp_msg_code_tostr(int code);

void rmcp_set_debug_level(int debug);
void rmcp_set_enable_warn_err(int enable);

/** 
 * Initialize the control structure.
 */
rmcp_ctx_t *rmcp_ctx_init(int timeout,int retries);
/** 
 * Tell control we want to use secure RMCP.  Must be called before rmcp_open!
 */
rmcp_error_t rmcp_ctx_setsecure(rmcp_ctx_t *rit,
				u_int8_t role,
				u_int8_t *role_key,int role_key_len,
				u_int8_t *gen_key,int gen_key_len);
/** 
 * Tell control about a special username (optional by the protocol spec).  Must
 * be called before rmcp_open!
 */
rmcp_error_t rmcp_ctx_setuid(rmcp_ctx_t *rit,char *uid,int uid_len);

/**
 * Grab a socket, secure or not, and "connect" it.
 * The user can set any sockopts desired, but we do set it in nonblocking
 * mode so we can check for timeouts and dropped packets.
 */
rmcp_error_t rmcp_open(rmcp_ctx_t *rit,char *hostname);
/** 
 * This function reads a whole datagram according to the RMCP and ASF
 * protocols, then mallocs a buf of the appropriate size and sets the buf and
 * len.  If there is no data to read, set buf to NULL and return NO_DATA
 */
rmcp_error_t rmcp_raw_read(rmcp_ctx_t *rit,
			   u_int8_t **buf,int *len);
/**
 * This function implements a blocking recv, but checks for timeouts for
 * the user.  The message is all malloc'd up and stored in msg.  Any errors are
 * returned and msg will be NULL.  Any received message that validates and
 * needs an ACK will be ACK'd by this function before it returns.
 */
rmcp_error_t rmcp_recv_msg_and_ack(rmcp_ctx_t *rit,rmcp_msg_t **msg);
/**
 * This function simply checks the rmcp_ctx_t struct and the msg fields,
 * then marshalls it and sends the bytes.  It does NOT wait for an ack.  It
 * DOES fill in the seqno from the rmcp_ctx_t struct, because we want all 
 * outbound msgs to be ordered -- and if users build several msgs before
 * sending them, this will fail.
 */
rmcp_error_t rmcp_send_msg(rmcp_ctx_t *rit,rmcp_msg_t *msg);
/**
 * This function doesn't return until it gets an ACK (unless it is not supposed
 * to be ACK'd!!).  Any errors (i.e., timeout, retries exceeded, are returned).
 */
rmcp_error_t rmcp_send_msg_wait_ack(rmcp_ctx_t *rit,
				    rmcp_msg_t *sendmsg);
/**
 * Does everything necessary to establish a secure session (i.e., open session,
 * run RAKP, establish session key).  Then the user can use this connection
 * to send normal requests, just as if not using secure RMCP.
 */
rmcp_error_t rmcp_start_secure_session(rmcp_ctx_t *rit);
/** 
 * If a session is open, close it and close the connection.
 */
rmcp_error_t rmcp_finalize(rmcp_ctx_t *rit);


/**
 * This function takes an rmcp message and marshalls it for sending, including
 * performing any necessary HMAC ops for the RSP trailer.
 */
u_int8_t *rmcp_marshall(rmcp_ctx_t *rit,rmcp_msg_t *msg,int *len);
/**
 * This function takes a buffer and validates the message and dumps it in
 * an rmcp_msg_t struct for processing.
 *
 * It also notes in the return struct if the msg is valid, if we've passed the
 * RAKP steps and are in a protected session.
 */
rmcp_msg_t *rmcp_unmarshall(rmcp_ctx_t *rit,u_int8_t *buf,int len);

/**
 * This function takes a recv'd msg and sends an ack.  Users do not need to
 * call this function; rmcp_recv_msg_blocking sends an ack after unmarshalling
 * and validating the message.
 */
rmcp_error_t rmcp_build_ack(rmcp_ctx_t *rit,
			    rmcp_msg_t *orig,
			    rmcp_msg_t **ack);
/**
 * This function initializes a message structure according to the info struct,
 * and returns it.  If the info struct specifies secure RMCP, but has or has
 * not yet finished the RSP RAKP steps, are both taken into account (i.e., if
 * it's not secure, no rsp hdr/tlr; if it is secure, but no RAKP complete, only
 * hdr; if secure and RAKP complete, both hdr/tlr and validation).
 */
rmcp_msg_t *rmcp_build_asf_msg(rmcp_ctx_t *rit);
/**
 * This function frees every part of a message -- including the message itself.
 */
void rmcp_free_asf_msg(rmcp_msg_t *msg);
/**
 * This function builds a "presence ping" message in the supplied msg arg.
 * i.e., it initializes the subfields appropriately.
 */
void rmcp_fill_asf_ping(rmcp_ctx_t *rit,rmcp_msg_t *msg);

void rmcp_fill_asf_capabilities_request(rmcp_ctx_t *rit,rmcp_msg_t *msg);

void rmcp_fill_asf_sysstate_request(rmcp_ctx_t *rit,rmcp_msg_t *msg);


/**
 * This function decodes an RMCP Presence Pong into an allocated struct.
 */
rmcp_asf_supported_t *rmcp_decode_asf_supported(rmcp_msg_t *msg);
void rmcp_print_asf_supported(rmcp_asf_supported_t *rs,FILE *file);
/**
 * This function decodes an ASF-RMCP Capabilities Response into an allocated
 * struct.
 */
rmcp_asf_capabilities_t *rmcp_decode_asf_capabilities(rmcp_msg_t *msg);
void rmcp_print_asf_capabilities(rmcp_asf_capabilities_t *rs,FILE *file);
/** 
 * This function decodes an ASF-RMCP System State Response into an allocated
 * struct.
 */
rmcp_asf_sysstate_t *rmcp_decode_asf_sysstate(rmcp_msg_t *msg);
void rmcp_print_asf_sysstate(rmcp_asf_sysstate_t *rs,FILE *file);

#endif /* __RMCP_H__ */
