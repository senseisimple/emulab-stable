/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "rmcp.h"

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <errno.h>
#include <netdb.h>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>


static int rmcp_debug_level = 0;

#define INFO(level,fmt,...) { \
  if (level <= rmcp_debug_level) { \
      fputs("INFO: ",stderr); \
      fputs(__FUNCTION__,stderr); \
      fprintf(stderr,": " fmt "\n", ## __VA_ARGS__); \
  } \
}
#define WARN(fmt,...) { \
  fputs("WARN: ",stderr); \
  fputs(__FUNCTION__,stderr); \
  fprintf(stderr,": " fmt "\n", ## __VA_ARGS__); \
}
#define ERROR(fmt,...) { \
  fprintf(stderr,"ERROR:%d: ",__LINE__); \
  fprintf(stderr,"%s: " fmt "\n", __FUNCTION__, ## __VA_ARGS__); \
}

void rmcp_set_debug_level(int debug) {
    rmcp_debug_level = debug;
}

/**
 * need a generic protocol read function that reads packets -- and it should
 * not need to know anything about protocol state.  all packets can be read
 * deterministically.  we can overlay higher-level protocol functions and 
 * auto handle acks and dropped packets in higher-level functions -- but they
 * can still make use of the packet marshalling functions.
 */

static u_int8_t rmcp_incr_seqno(rmcp_ctx_t *ctx);
static u_int8_t rmcp_rsp_incr_seqno(rmcp_ctx_t *ctx);

static void rmcp_rsp_fill_hdr(rmcp_ctx_t *ctx,rmcp_msg_t *msg);
static void rmcp_fill_hdr(rmcp_ctx_t *ctx,rmcp_msg_t *msg);
/** this function DOES allocate a data segment of requested length and
 * copies the data in */
static void rmcp_fill_asf_data(rmcp_ctx_t *ctx,rmcp_msg_t *msg,
			       u_int8_t type,u_int8_t data_len,u_int8_t *data);
static void rmcp_rsp_fill_tlr(rmcp_ctx_t *ctx,rmcp_msg_t *msg);

static void rmcp_fill_all(rmcp_ctx_t *ctx,rmcp_msg_t *msg,
			  u_int8_t type,u_int8_t data_len,u_int8_t *data);

static char *rmcp_rsp_rakp_msg_code_str(int code);



rmcp_error_t rmcp_asf_ping(rmcp_ctx_t *ctx,
			   rmcp_asf_supported_t **supported) {
    rmcp_msg_t *send;
    rmcp_msg_t *recv;
    rmcp_error_t retval;

    send = rmcp_build_asf_msg(ctx);
    rmcp_fill_asf_ping(ctx,send);

    retval = rmcp_send_msg_wait_ack(ctx,send);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(send);
	ERROR("error 0x%x",retval);
	return retval;
    }
    else {
	rmcp_free_asf_msg(send);
    }

    retval = rmcp_recv_msg_and_ack(ctx,&recv);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(recv);
	ERROR("error 0x%x",retval);
	return retval;
    }

    /* check if the received msg is a presence pong */
    /* if not, it's a protocol error! */
    if (recv->hdr->class == RMCP_CLASS_ASF && recv->data 
	&& recv->data->type == RMCP_ASF_TYPE_PRESENCE_PONG) {
	*supported = rmcp_decode_asf_supported(recv);
	INFO(1,"got supported");
	rmcp_free_asf_msg(recv);
	return RMCP_SUCCESS;
    }
    
    ERROR("protocol error",retval);
    rmcp_free_asf_msg(recv);
    return RMCP_ERR_PROTOCOL;
}
	
rmcp_error_t rmcp_asf_get_capabilities(rmcp_ctx_t *ctx,
				       rmcp_asf_capabilities_t **cap) {
    rmcp_msg_t *send;
    rmcp_msg_t *recv;
    rmcp_error_t retval;

    send = rmcp_build_asf_msg(ctx);
    rmcp_fill_asf_capabilities_request(ctx,send);

    retval = rmcp_send_msg_wait_ack(ctx,send);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(send);
	ERROR("error 0x%x",retval);
	return retval;
    }
    else {
	rmcp_free_asf_msg(send);
    }

    retval = rmcp_recv_msg_and_ack(ctx,&recv);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(recv);
	ERROR("error 0x%x",retval);
	return retval;
    }

    /* check if the received msg is a presence pong */
    /* if not, it's a protocol error! */
    if (recv->hdr->class == RMCP_CLASS_ASF && recv->data 
	&& recv->data->type == RMCP_ASF_TYPE_CAPABILITIES_RESPONSE) {
	*cap = rmcp_decode_asf_capabilities(recv);
	INFO(1,"got capabilities");
	rmcp_free_asf_msg(recv);
	return RMCP_SUCCESS;
    }
    
    ERROR("protocol error",retval);
    rmcp_free_asf_msg(recv);
    return RMCP_ERR_PROTOCOL;
}
rmcp_error_t rmcp_asf_get_sysstate(rmcp_ctx_t *ctx,
				   rmcp_asf_sysstate_t **state) {
    rmcp_msg_t *send;
    rmcp_msg_t *recv;
    rmcp_error_t retval;

    send = rmcp_build_asf_msg(ctx);
    rmcp_fill_asf_sysstate_request(ctx,send);

    retval = rmcp_send_msg_wait_ack(ctx,send);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(send);
	ERROR("error 0x%x",retval);
	return retval;
    }
    else {
	rmcp_free_asf_msg(send);
    }

    retval = rmcp_recv_msg_and_ack(ctx,&recv);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(recv);
	ERROR("error 0x%x",retval);
	return retval;
    }

    /* check if the received msg is a presence pong */
    /* if not, it's a protocol error! */
    if (recv->hdr->class == RMCP_CLASS_ASF && recv->data 
	&& recv->data->type == RMCP_ASF_TYPE_SYSSTATE_RESPONSE) {
	*state = rmcp_decode_asf_sysstate(recv);
	INFO(1,"got sysstate");
	rmcp_free_asf_msg(recv);
	return RMCP_SUCCESS;
    }
    
    ERROR("protocol error",retval);
    rmcp_free_asf_msg(recv);
    return RMCP_ERR_PROTOCOL;
}
/**
 * XXX: fix reset, up, cycle, to take boot options for inclusion in the data
 * field.  For now, just set a 0-length data field.
 */
rmcp_error_t rmcp_asf_reset(rmcp_ctx_t *ctx) {
    rmcp_msg_t *send;
    rmcp_msg_t *recv;
    rmcp_error_t retval;
    u_int8_t buf[4+1+2+2+2];
    int wc;
    u_int32_t tmp;

    send = rmcp_build_asf_msg(ctx);

    /* build the power commands data buf */
    wc = 0;
    tmp = htonl(RMCP_IANA_ENT_ID);
    memcpy(&buf[wc],(u_int8_t *)&tmp,sizeof(u_int32_t));
    wc += sizeof(u_int32_t);
    /* XXX: no special commands for now! */
    memset(&buf[wc],0,7);
    wc += 7;

    rmcp_fill_all(ctx,send,RMCP_ASF_TYPE_RESET,wc,buf);

    retval = rmcp_send_msg_wait_ack(ctx,send);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(send);
	ERROR("error 0x%x",retval);
	return retval;
    }
    else {
	rmcp_free_asf_msg(send);
    }

    return RMCP_SUCCESS;
}
rmcp_error_t rmcp_asf_power_up(rmcp_ctx_t *ctx) {
    rmcp_msg_t *send;
    rmcp_msg_t *recv;
    rmcp_error_t retval;
    u_int8_t buf[4+1+2+2+2];
    int wc;
    u_int32_t tmp;

    send = rmcp_build_asf_msg(ctx);

    /* build the power commands data buf */
    wc = 0;
    tmp = htonl(RMCP_IANA_ENT_ID);
    memcpy(&buf[wc],(u_int8_t *)&tmp,sizeof(u_int32_t));
    wc += sizeof(u_int32_t);
    /* XXX: no special commands for now! */
    memset(&buf[wc],0,7);
    wc += 7;

    rmcp_fill_all(ctx,send,RMCP_ASF_TYPE_POWER_UP,wc,buf);

    retval = rmcp_send_msg_wait_ack(ctx,send);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(send);
	ERROR("error 0x%x",retval);
	return retval;
    }
    else {
	rmcp_free_asf_msg(send);
    }

    return RMCP_SUCCESS;
}
rmcp_error_t rmcp_asf_power_cycle(rmcp_ctx_t *ctx) {
    rmcp_msg_t *send;
    rmcp_msg_t *recv;
    rmcp_error_t retval;
    u_int8_t buf[4+1+2+2+2];
    int wc;
    u_int32_t tmp;

    send = rmcp_build_asf_msg(ctx);

    /* build the power commands data buf */
    wc = 0;
    tmp = htonl(RMCP_IANA_ENT_ID);
    memcpy(&buf[wc],(u_int8_t *)&tmp,sizeof(u_int32_t));
    wc += sizeof(u_int32_t);
    /* XXX: no special commands for now! */
    memset(&buf[wc],0,7);
    wc += 7;

    rmcp_fill_all(ctx,send,RMCP_ASF_TYPE_POWER_CYCLE_RESET,wc,buf);

    retval = rmcp_send_msg_wait_ack(ctx,send);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(send);
	ERROR("error 0x%x",retval);
	return retval;
    }
    else {
	rmcp_free_asf_msg(send);
    }

    return RMCP_SUCCESS;
}

rmcp_error_t rmcp_asf_power_down(rmcp_ctx_t *ctx) {
    rmcp_msg_t *send;
    rmcp_msg_t *recv;
    rmcp_error_t retval;
    u_int8_t buf[4+1+2+2+2];
    int wc;
    u_int32_t tmp;

    send = rmcp_build_asf_msg(ctx);

    /* build the power commands data buf */
    wc = 0;
    tmp = htonl(RMCP_IANA_ENT_ID);
    memcpy(&buf[wc],(u_int8_t *)&tmp,sizeof(u_int32_t));
    wc += sizeof(u_int32_t);
    /* XXX: no special commands for now! */
    memset(&buf[wc],0,7);
    wc += 7;

    rmcp_fill_all(ctx,send,RMCP_ASF_TYPE_POWER_DOWN,wc,buf);

    retval = rmcp_send_msg_wait_ack(ctx,send);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(send);
	ERROR("error 0x%x",retval);
	return retval;
    }
    else {
	rmcp_free_asf_msg(send);
    }

    return RMCP_SUCCESS;
}



static char *rmcp_rsp_rakp_msg_codes[] = {
    "No errors",
    "Insufficient resources to create a session",
    "Invalid session ID",
    "Invalid payload type",
    "Invalid authentication algorithm",
    "Invalid integrity algorithm",
    "No matching authentication payload",
    "No matching integrity payload",
    "Inactive session ID",
    "Invalid role",
    "Unauthorized role",
    "Insufficient resources to create a session at the requested role",
    "Invalid name length",
    "Unauthorized name",
    "Unauthorized GUID",
    "Invalid integrity check value",
};

static char *rmcp_rsp_rakp_msg_code_str(int code) {
    if (code < RMCP_RSP_RAKP_SUCCESS || code > RMCP_RSP_RAKP_ERR_INVALID_INTEGRITY_VALUE) {
	return "UNKNOWN RSP/RAKP MSG CODE!";
    }
    
    return rmcp_rsp_rakp_msg_codes[code];
}

rmcp_error_t rmcp_finalize(rmcp_ctx_t *ctx) {
    rmcp_msg_t *send;
    rmcp_msg_t *recv;
    rmcp_error_t retval;

    if (ctx->secure && ctx->rsp_init) {
	/* send a close session request, wait for the ack and response */
	send = rmcp_build_asf_msg(ctx);
	rmcp_fill_all(ctx,send,
		      RMCP_ASF_TYPE_CLOSE_SESSION_REQUEST,0,NULL);
	retval = rmcp_send_msg_wait_ack(ctx,send);
	if (retval != RMCP_SUCCESS) {
	    rmcp_free_asf_msg(send);
	    ERROR("error 0x%x",retval);
	    return retval;
	}
	else {
	    INFO(1,"session close request sent");
	    rmcp_free_asf_msg(send);
	}

	retval = rmcp_recv_msg_and_ack(ctx,&recv);
	if (retval != RMCP_SUCCESS) {
	    rmcp_free_asf_msg(recv);
	    ERROR("error 0x%x",retval);
	    return retval;
	}

	/* check if the received msg is a session close response, and
	 * check response code */
	/* if not, it's a protocol error! */
	if (recv->hdr->class == RMCP_CLASS_ASF && recv->data 
	    && recv->data->type == RMCP_ASF_TYPE_CLOSE_SESSION_RESPONSE) {
	    
	    if (recv->data->data[0] == RMCP_RSP_RAKP_SUCCESS) {
		rmcp_free_asf_msg(recv);
		INFO(1,"session close response received");
		close(ctx->sockfd);
		ctx->sockfd = -1;
		return RMCP_SUCCESS;
	    }
	    else {
		WARN("rsp/rakp status '%s'",
		     rmcp_rsp_rakp_msg_code_str(recv->data->data[0]));
		rmcp_free_asf_msg(recv);
		return RMCP_ERR_PROTOCOL;
	    }
	}
	
	rmcp_free_asf_msg(recv);
	ERROR("protocol error",retval);
	return RMCP_ERR_PROTOCOL;
    }
    
    /* now just dump the socket */
    close(ctx->sockfd);
    ctx->sockfd = -1;
    
    return RMCP_SUCCESS;
}

#ifdef __linux__
/* impl this fbsd function for linux */
void srandomdev(void) {
    unsigned long seed;
    int fd;

    if ((fd = open("/dev/urandom",O_RDONLY)) < 0) {
	WARN("could not open /dev/urandom");
    }
    else if ((fd = open("/dev/random",O_RDONLY)) < 0) {
	WARN("could not open /dev/random, seeding with time!");
	srandom(time(NULL));
    }
    else {
	if (read(fd,(char *)&seed,sizeof(seed)) < sizeof(seed)) {
	    WARN("could not read from rand file, seeding with time!");
	    srandom(time(NULL));
	}
	else {
	    srandom(seed);
	}
	close(fd);
    }
}
#endif

#define RMCP_RSP_OSR_PAYLOAD_HDR_LEN  4
#define RMCP_RSP_OSR_PAYLOAD_DATA_LEN 5 
/* the management console sid, then one hdr/data for auth alg, then one
 * hdr/data for integrity alg
 */
#define RMCP_RSP_OSR_TOTAL_LEN        (4 + 2 * ( RMCP_RSP_OSR_PAYLOAD_HDR_LEN + RMCP_RSP_OSR_PAYLOAD_DATA_LEN ) )
#define RMCP_RSP_OSR_AUTH_ALG_ID      0x01
#define RMCP_RSP_OSR_INTEGRITY_ALG_ID 0x02
/* default algs */
#define RMCP_RSP_OSR_AUTH_HMAC_SHA1   0x01
#define RMCP_RSP_OSR_INTEGRITY_HMAC_SHA1_96 0x01

rmcp_error_t rmcp_start_secure_session(rmcp_ctx_t *ctx) {

    rmcp_msg_t *smsg;
    rmcp_msg_t *rmsg;
    u_int32_t msid;
    u_int32_t tmp;
    u_int8_t osr_buf[RMCP_RSP_OSR_TOTAL_LEN];
    int wc = 0;
    u_int16_t len;
    u_int16_t stmp;
    rmcp_error_t retval;
    /* max size is 40, including max size uname */
    u_int8_t buf1[40];
    u_int8_t hmac_buf[EVP_MAX_MD_SIZE];
    int hmac_len = 0;
    /* ick: lengths of [msid,csid,mrand,crand,guid,role,ulength,uname] */
    /* the buf will probably not need that much space -- the uname is optional.
     */
    u_int8_t buf1v[4 + 4 + 16 + 16 + 16 + 1 + 1 + 16];
    int tmp2;
    int i;
    /* ick: lengths of [mrand,crand,role,ulength,uname] -- again, uname is
     * optional.
     */
    u_int8_t bufsik[16 + 16 + 1 + 1 + 16];
    /* buf for RAKP 3 */
    u_int8_t buf3[1 + 3 + 4 + 12];
    /* buf for RAKP 3; need to compute the validation tag for RAKP 3 */
    /* ick: lengths of [crand,msid,role,ulength,uname] */
    u_int8_t buf3v[16 + 4 + 1 + 1 + 16];
    

    /* open session request, ack, response;
     * RAKP1, ack, RAKP2, RAKP3, ack -- then we're good for the
     * secure messages
     */

    if (!ctx->secure) {
	return RMCP_ERR_NO_SECURE_CREDS;
    }

    /** THIS ONLY GETS SENT after RAKP3 succeeds, but we have to send it
     * in the session open request msg, in the data field... 
     */
    /* generate the "mgt console" session id */
    /* XXX: need to tap a "secure" rand source */
    /* XXX: eventually, these need to be stored in the db to ensure
     * against operators both hitting a machine at the same time.
     * Of course, the reality risks of this happening are astronomically low.
     */
    srandomdev();
    /* ctx->msid = (u_int32_t)random(); */
    //msid = (u_int32_t)random();
    msid = 0x00000001;

    /**
     * do the Open Session Request 
     */
    wc = 0;
    smsg = rmcp_build_asf_msg(ctx);
    /* just gotta fill in the osr data buf now */
    tmp = htonl(msid);
    memcpy(&osr_buf[wc],(u_int8_t *)&tmp,sizeof(u_int32_t));
    wc += sizeof(u_int32_t);
    /* first payload: auth alg */
    osr_buf[wc++] = RMCP_RSP_OSR_AUTH_ALG_ID;
    osr_buf[wc++] = 0x0;
    len = 8;
    stmp = htons(len);
    memcpy(&osr_buf[wc],(u_int8_t *)&stmp,sizeof(u_int16_t));
    wc += sizeof(u_int16_t);
    osr_buf[wc++] = RMCP_RSP_OSR_AUTH_HMAC_SHA1;
    memset(&osr_buf[wc],0,3);
    wc += 3;
    /* second payload: integrity alg */
    osr_buf[wc++] = RMCP_RSP_OSR_INTEGRITY_ALG_ID;
    osr_buf[wc++] = 0x0;
    len = 8;
    stmp = htons(len);
    memcpy(&osr_buf[wc],(u_int8_t *)&stmp,sizeof(u_int16_t));
    wc += sizeof(u_int16_t);
    osr_buf[wc++] = RMCP_RSP_OSR_INTEGRITY_HMAC_SHA1_96;
    memset(&osr_buf[wc],0,3);
    wc += 3;
    /* fill in the headers and data */
    rmcp_fill_all(ctx,smsg,
		      RMCP_ASF_TYPE_OPEN_SESSION_REQUEST,
		      wc,osr_buf);
    /* send... */
    retval = rmcp_send_msg_wait_ack(ctx,smsg);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(smsg);
	ERROR("error 0x%x",retval);
	return retval;
    }
    else {
	rmcp_free_asf_msg(smsg);
    }
    /* recv the response */
    retval = rmcp_recv_msg_and_ack(ctx,&rmsg);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(rmsg);
	ERROR("error 0x%x",retval);
	return retval;
    }
    if (rmsg->data 
	&& rmsg->data->type == RMCP_ASF_TYPE_OPEN_SESSION_RESPONSE) { 
	if (rmsg->data->data[0] == RMCP_RSP_RAKP_SUCCESS) {
	    /* client agrees with our pitch */
	    /* XXX: don't feel the need to check that they sent back our msid;
	     * if this library is ever extended to account full console
	     * functionality, we will need to do this...
	     */
	    ctx->csid = ntohl(*((u_int32_t *)&rmsg->data->data[8]));
	    /* XXX: also, since we sent the default auth/integrity algs, we 
	     * don't check that the response algs were the ones we selected.
	     * A device must supply those defaults, and since that's all we
	     * specified, they must answer yes if the return code was kosher.
	     */
	    rmcp_free_asf_msg(rmsg);
	    INFO(1,"open session successful");
	}
	else {
	    /* error somewhere, probably on our end... */
	    ERROR("open session response error: '%s'",
		  rmcp_rsp_rakp_msg_code_str(rmsg->data->data[0]));
	    rmcp_free_asf_msg(rmsg);
	    return RMCP_ERR_PROTOCOL;
	}
    }
    else {
	ERROR("improper response");
	return RMCP_ERR_PROTOCOL;
    }
    
    /* we're good to go for RAKP now */
    /**
     * RAKP 1
     */
    wc = 0;
    smsg = rmcp_build_asf_msg(ctx);
    /* just gotta fill in the osr data buf now */
    tmp = htonl(ctx->csid);
    memcpy(&buf1[wc],(u_int8_t *)&tmp,sizeof(u_int32_t));
    wc += sizeof(u_int32_t);
    /* XXX: this is a stupid way to get a 16-byte rand, should use openssl 
     * libs!
     */
    tmp = random();
    tmp2 = time(NULL);
    HMAC(EVP_sha1(),&tmp2,sizeof(tmp2),(unsigned char *)&tmp,sizeof(tmp),
	 hmac_buf,&hmac_len);
    memcpy(ctx->mrand,(u_int8_t *)&hmac_buf,(hmac_len > 16)?16:hmac_len);
    /* note that if the HMAC returned fewer than 16 bytes, we do use the random
     * bytes that were in ctx->mrand after it was init'd
     */
    memcpy(&buf1[wc],ctx->mrand,sizeof(ctx->mrand));
    wc += sizeof(ctx->mrand);
    buf1[wc++] = ctx->role;
    /* reserved two bytes */
    memset(&buf1[wc],0,2);
    wc += 2;
    if (ctx->uid_len > 0) {
	buf1[wc++] = ctx->uid_len;
	memcpy(&buf1[wc],ctx->uid,ctx->uid_len);
	wc += ctx->uid_len;
    }
    else {
	buf1[wc++] = 0x0;
    }
    /* now fill the msg with data */
    rmcp_fill_all(ctx,smsg,
		  RMCP_ASF_TYPE_RAKP_MSG_1,
		  wc,buf1);
    /* send */
    retval = rmcp_send_msg_wait_ack(ctx,smsg);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(smsg);
	ERROR("error 0x%x",retval);
	return retval;
    }
    else {
	rmcp_free_asf_msg(smsg);
    }
    /* recv the response: RAKP 2 */
    retval = rmcp_recv_msg_and_ack(ctx,&rmsg);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(rmsg);
	ERROR("error 0x%x",retval);
	return retval;
    }
    if (rmsg->data 
	&& rmsg->data->type == RMCP_ASF_TYPE_RAKP_MSG_2) { 
	if (rmsg->data->data[0] == RMCP_RSP_RAKP_SUCCESS) {

	    /* grab the client rand */
	    memcpy(ctx->crand,&rmsg->data->data[8],16);
	    /* grab the client GUID */
	    memcpy(ctx->cGUID,&rmsg->data->data[24],16);

	    /* check HMAC(msid,csid,mrand,crand,guid,role,ulength,uname)
	     * for validity 
	     */
	    /* build into the buf: */
	    wc = 0;
	    /*
	     * Make sure we're network byte order on the session ids...
	     */
	    tmp = htonl(msid);
	    memcpy(&buf1v[wc],(u_int8_t *)&tmp,sizeof(u_int32_t));
	    wc += sizeof(u_int32_t);
	    tmp = htonl(ctx->csid);
	    memcpy(&buf1v[wc],(u_int8_t *)&tmp,sizeof(u_int32_t));
	    wc += sizeof(u_int32_t);
	    memcpy(&buf1v[wc],(u_int8_t *)&ctx->mrand,sizeof(ctx->mrand));
	    wc += sizeof(ctx->mrand);
	    memcpy(&buf1v[wc],(u_int8_t *)&ctx->crand,sizeof(ctx->crand));
	    wc += sizeof(ctx->crand);
	    memcpy(&buf1v[wc],(u_int8_t *)&ctx->cGUID,sizeof(ctx->cGUID));
	    wc += sizeof(ctx->cGUID);
	    buf1v[wc++] = ctx->role;
	    buf1v[wc++] = ctx->uid_len;
	    if (ctx->uid_len > 0) {
		memcpy(&buf1v[wc],ctx->uid,ctx->uid_len);
		wc += ctx->uid_len;
	    }
	    /* validate it */
	    HMAC(EVP_sha1(),
		 ctx->role_key,ctx->role_key_len,
		 buf1v,wc,
		 hmac_buf,&hmac_len);
	    len = rmsg->data->data_len - 40;
	    if (strncmp(hmac_buf,&rmsg->data->data[40],len) == 0) {
		/* valid -- the role's supplied key is good! */
		INFO(1,"RAKP2 from client validates");
		rmcp_free_asf_msg(rmsg);
	    }
	    else {
		WARN("RAKP2 from client is invalid!");
		fprintf(stderr,"\t");
		for (i = 0; i < 12; ++i) {
		    fprintf(stderr," %x",rmsg->data->data[40+i]);
		}
		fprintf(stderr,"\n\t");
		for (i = 0; i < 12; ++i) {
		    fprintf(stderr," %x",hmac_buf[i]);
		}
		rmcp_free_asf_msg(rmsg);
		return RMCP_ERR_PROTOCOL;
	    }
	}
	else {
	    /* error somewhere, probably on our end... */
	    ERROR("RAKP2 response error: '%s'",
		  rmcp_rsp_rakp_msg_code_str(rmsg->data->data[0]));
	    rmcp_free_asf_msg(rmsg);
	    return RMCP_ERR_PROTOCOL;
	}
    }
    else {
	ERROR("improper response");
	return RMCP_ERR_PROTOCOL;
    }
    /* If we're valid, calculate the session key */
    wc = 0;
    memcpy(&bufsik[wc],ctx->mrand,sizeof(ctx->mrand));
    wc += sizeof(ctx->mrand);
    memcpy(&bufsik[wc],ctx->crand,sizeof(ctx->crand));
    wc += sizeof(ctx->crand);
    bufsik[wc++] = ctx->role;
    bufsik[wc++] = ctx->uid_len;
    if (ctx->uid_len > 0) {
	memcpy(&bufsik[wc],ctx->uid,ctx->uid_len);
	wc += ctx->uid_len;
    }
    HMAC(EVP_sha1(),
	 ctx->gen_key,ctx->gen_key_len,
	 bufsik,wc,
	 hmac_buf,&hmac_len);
    if (hmac_len > sizeof(ctx->sik)) {
	memcpy(ctx->sik,hmac_buf,sizeof(ctx->sik));
    }
    else {
	memset(ctx->sik,0,sizeof(ctx->sik));
	memcpy(ctx->sik,hmac_buf,hmac_len);
    }

    /* Send RAKP 3 in response */
    smsg = rmcp_build_asf_msg(ctx);
    /* first, make the buf containing the src for the validation hmac in 
     * RAKP 3:
     */
    wc = 0;
    memcpy(&buf3v[wc],ctx->crand,sizeof(ctx->crand));
    wc += sizeof(ctx->crand);
    tmp = htonl(msid);
    memcpy(&buf3v[wc],(u_int8_t *)&tmp,sizeof(u_int32_t));
    wc += sizeof(u_int32_t);
    buf3v[wc++] = ctx->role;
    buf3v[wc++] = ctx->uid_len;
    if (ctx->uid_len > 0) {
	memcpy(&buf3v[wc],ctx->uid,ctx->uid_len);
	wc += ctx->uid_len;
    }
    HMAC(EVP_sha1(),
	 ctx->role_key,ctx->role_key_len,
	 buf3v,wc,
	 hmac_buf,&hmac_len);
    
    wc = 0;
    buf3[wc++] = RMCP_RSP_RAKP_SUCCESS;
    memset(&buf3[wc],0,3);
    wc += 3;
    tmp = htonl(ctx->csid);
    memcpy(&buf3[wc],(u_int8_t *)&tmp,sizeof(u_int32_t));
    wc += sizeof(u_int32_t);
    memcpy(&buf3[wc],(u_int8_t *)hmac_buf,12);
    wc += 12;
    /* now fill the msg with data */
    rmcp_fill_all(ctx,smsg,
		  RMCP_ASF_TYPE_RAKP_MSG_3,
		  wc,buf3);
    /* send */
    retval = rmcp_send_msg_wait_ack(ctx,smsg);
    if (retval != RMCP_SUCCESS) {
	rmcp_free_asf_msg(smsg);
	ERROR("error 0x%x",retval);
	return retval;
    }
    else {
	rmcp_free_asf_msg(smsg);
    }
    INFO(1,"sent RAKP 3");

    /* ok, assuming that we didn't screw up the 3rd RAKP msg, things should be
     * good now.  Now we have to set rsp_init and save the msid, and also init
     * the rsp_seqno to 0x01.
     */

    ctx->rsp_init = 1;
    ctx->rmcp_rsp_seqno = 0x01;

    ctx->msid = msid;
    // arghHhhhh!

    return RMCP_SUCCESS;
}

rmcp_ctx_t *rmcp_ctx_init(int timeout,int retries) {
    rmcp_ctx_t *retval = NULL;
    
    retval = (rmcp_ctx_t *)malloc(sizeof(rmcp_ctx_t));
    memset(retval,0,sizeof(rmcp_ctx_t));

    retval->sockfd = -1;

    retval->timeout = timeout;
    retval->retries = retries;

    return retval;
}

rmcp_error_t rmcp_ctx_setsecure(rmcp_ctx_t *ctx,
				u_int8_t role,
				u_int8_t *role_key,int role_key_len,
				u_int8_t *gen_key,int gen_key_len) {
    rmcp_error_t retval = RMCP_SUCCESS;

    if (ctx->rsp_init || ctx->sockfd > -1) {
	retval = RMCP_ERR_ALREADY_CONNECTED;
    }
    else {
	ctx->secure = 1;
	ctx->role = role;
	if (role_key_len > 20) {
	    WARN("role key longer than 20 characters, truncating!");
	    memcpy(ctx->role_key,role_key,20);
	    ctx->role_key_len = 20;
	}
	else {
	    memcpy(ctx->role_key,role_key,role_key_len);
	    ctx->role_key_len = role_key_len;
	}
	if (gen_key_len > 20) {
	    WARN("gen key longer than 20 characters, truncating!");
	    memcpy(ctx->gen_key,gen_key,20);
	    ctx->gen_key_len = 20;
	}
	else {
	    memcpy(ctx->gen_key,gen_key,gen_key_len);
	    ctx->gen_key_len = gen_key_len;
	}
    }

    return retval;
}

rmcp_error_t rmcp_ctx_setuid(rmcp_ctx_t *ctx,char *uid,int uid_len) {
    rmcp_error_t retval = RMCP_SUCCESS;

    if (ctx->rsp_init || ctx->sockfd > -1) {
	retval = RMCP_ERR_ALREADY_CONNECTED;
    }
    else {
	ctx->uid_len = uid_len;
	if (uid_len > 16) {
	    WARN("truncated uid to 16 characters (spec max)!");
	    strncpy(ctx->uid,uid,16);
	}
	else {
	    strncpy(ctx->uid,uid,uid_len);
	}
    }

    return retval;
}

/* little helper function */
static int fill_sockaddr(char *hostname,int port,struct sockaddr_in *new_sa) {
    struct hostent *host_ptr;

    if (port < 1) {
        return -2;
    }

    new_sa->sin_family = PF_INET;
    new_sa->sin_port = htons((short) port);

    if (hostname != NULL && strlen(hostname) > 0) {
        host_ptr = gethostbyname(hostname);

        if (!host_ptr) {
            return -3;
        }

        memcpy((char *) &new_sa->sin_addr.s_addr,
               (char *) host_ptr->h_addr_list[0],
               host_ptr->h_length);
    }
    else {
        new_sa->sin_addr.s_addr = INADDR_ANY;
    }

    return 0;
}

static struct timeval recv_timeout = { 3,0 };

rmcp_error_t rmcp_open(rmcp_ctx_t *ctx,char *hostname) {

    if (ctx->rsp_init || ctx->sockfd > -1) {
	return RMCP_ERR_ALREADY_CONNECTED;
    }
    else {
	/* build up the dest */
	if (fill_sockaddr(hostname,
			  (ctx->secure)?RMCP_SECURE_PORT:RMCP_COMPAT_PORT,
			  &ctx->client_sa) < 0) {
	    return RMCP_ERR_BAD_CLIENT_NAME;
	}

	/* grab socket */
	if ((ctx->sockfd = socket(PF_INET,SOCK_DGRAM,0)) < 0) {
	    return RMCP_ERR_SOCKET;
	}
	/* connect it */
/* 	else if (connect(ctx->sockfd, */
/* 			 (struct sockaddr *)&client_sa, */
/* 			 sizeof(struct sockaddr_in)) < 0) { */
/* 	    close(ctx->sockfd); */
/* 	    ctx->sockfd = -1; */
/* 	    return RMCP_ERR_SOCKET; */
/* 	} */
	/* nonblocking mode so that timeouts are simple */
/* 	else if (fcntl(ctx->sockfd,F_SETFL,O_NONBLOCK) < 0) { */
/* 	    close(ctx->sockfd); */
/* 	    ctx->sockfd = -1; */
/* 	    return RMCP_ERR_SOCKET; */
/* 	} */
	else if (setsockopt(ctx->sockfd,
			    SOL_SOCKET,
			    SO_RCVTIMEO,
			    &recv_timeout,
			    sizeof(struct timeval)) < 0) {
	    close(ctx->sockfd);
	    ctx->sockfd = -1;
	    return RMCP_ERR_SOCKET;
	}
    }

    INFO(3,"success");

    return RMCP_SUCCESS;
}

static u_int8_t rmcp_incr_seqno(rmcp_ctx_t *ctx) {
    ++(ctx->rmcp_seqno);
    /* have to jump over 0xff -- otherwise our msgs won't get ack'd */
    if (ctx->rmcp_seqno == 0xff) {
	++(ctx->rmcp_seqno);
    }
}

static u_int8_t rmcp_rsp_incr_seqno(rmcp_ctx_t *ctx) {
    if (ctx->secure) {
	++(ctx->rmcp_rsp_seqno);
    }
}

static void rmcp_rsp_fill_hdr(rmcp_ctx_t *ctx,rmcp_msg_t *msg) {
    if (ctx->secure) {
	/* if we don't have a session yet, we have to send this msg unprotected
	 *  -- which means using 0x0 and 0x0 for sid/seqno.
	 */
	if (ctx->rsp_init) {
	    msg->shdr->seqno = ctx->rmcp_rsp_seqno;
	    msg->shdr->sid = ctx->msid;
	}
	else {
	    msg->shdr->sid = RMCP_SID_BYPASS;
	    msg->shdr->seqno = 0x0;
	}
    }
}

static void rmcp_fill_hdr(rmcp_ctx_t *ctx,rmcp_msg_t *msg) {
    msg->hdr->version = RMCP_VERSION_1_0;
    msg->hdr->reserved = 0x0;
    msg->hdr->seqno = ctx->rmcp_seqno;
    /* XXX: change so it's more flexible for other protocols like IPMI */
    msg->hdr->class = RMCP_CLASS_ASF;
}

/* NOTE: this function DOES allocate a data segment. */
static void rmcp_fill_asf_data(rmcp_ctx_t *ctx,rmcp_msg_t *msg,
			       u_int8_t type,u_int8_t data_len,u_int8_t *data) {
    msg->data->iana_ent_id = RMCP_IANA_ENT_ID;
    msg->data->type = type;
    /* XXX: make this an argument so that we're more flexible... */
    //msg->data->tag = 0x0;
    msg->data->tag = msg->hdr->seqno;
    msg->data->reserved = 0x0;
    msg->data->data_len = data_len;
    msg->data->data = (u_int8_t *)malloc(sizeof(u_int8_t)*data_len);
    if (data != NULL) {
	memcpy(msg->data->data,data,data_len);
    }
    else {
        memset(msg->data->data,0,data_len);
    }
}

/* Notice that this function does nothing.  There's no point to figuring this
 * data out before marshalling.
 */
static void rmcp_rsp_fill_tlr(rmcp_ctx_t *ctx,rmcp_msg_t *msg) {
    return;
}

static void rmcp_fill_all(rmcp_ctx_t *ctx,rmcp_msg_t *msg,
			      u_int8_t type,u_int8_t data_len,u_int8_t *data) {
    rmcp_rsp_fill_hdr(ctx,msg);
    rmcp_fill_hdr(ctx,msg);
    rmcp_fill_asf_data(ctx,msg,type,data_len,data);
    rmcp_rsp_fill_tlr(ctx,msg);
}

#define RMCP_RSP_HDR_LEN      8
#define RMCP_HDR_LEN          4
/**
 * First 5 fields of the RMCP data segment, not including the (variable) data
 * field.
 */
#define RMCP_ASF_DATA_HDR_LEN 8
/* Note that we don't know the length of the RSP trailer -- we have to read
 * the padding byte by byte until we get to the pad length field. */

rmcp_error_t rmcp_raw_read(rmcp_ctx_t *ctx,
			   u_int8_t **buf,int *len) {
    u_int8_t lbuf[512]; /* this should be easily large enough for any datagrams
		      we might encounter... */
    int rc;
    int c;
    int tlen;

    tlen = sizeof(struct sockaddr_in);

    if ((rc = recvfrom(ctx->sockfd,
		       lbuf,sizeof(lbuf),
		       0,
		       (struct sockaddr *)&ctx->client_sa,&tlen)) < 0) {
	if (errno == EAGAIN) {
	    /* since we set the SO_RCVTIMEO sockopt, we get a timeout after
	     * recv_timeout secs/usecs
	     */
	    WARN("timeout");
	    return RMCP_ERR_TIMEOUT;
	}
	else {
	    ERROR("recvfrom: %s",strerror(errno));
	    return RMCP_ERR_SOCKET;
	}
    }
    else {
	/* process the msg */
	
	/* if we get the requested bytes, take a look at the class field, make
	 * sure we're reading ASF (XXX: or IPMI -- later!) */ 
	
	c = 0;

	if (ctx->secure) {
	    c += RMCP_RSP_HDR_LEN;
	}
	c += RMCP_HDR_LEN;

	if ((lbuf[c-1] & 0xf) != RMCP_CLASS_ASF) {
	    WARN("unknown RMCP class 0x%x",lbuf[rc-1]);
	    return RMCP_ERR_UNKNOWN_CLASS;
	}

	/* now that we've read an ASF-RMCP packet, copy and bomb out */
	*buf = (u_int8_t *)malloc(rc);
	memcpy(*buf,lbuf,rc);
	*len = rc;

	return RMCP_SUCCESS;
    }

    /* we shouldn't get here... */
    //return RMCP_ERR_UNKNOWN;

}

rmcp_error_t rmcp_recv_msg_and_ack(rmcp_ctx_t *ctx,rmcp_msg_t **msg) {
    struct timeval tv,ntv;
    u_int8_t *buf;
    int len;
    double dt;
    rmcp_error_t retval;
    rmcp_msg_t *ack;

    gettimeofday(&tv,NULL);
    
    while (1) {
	retval = rmcp_raw_read(ctx,&buf,&len);
	if (retval == RMCP_SUCCESS) {
	    break;
	}
/* 	else if (retval == RMCP_ERR_NO_DATA) { */
/* 	    gettimeofday(&ntv,NULL); */
/* 	    /\* check if we're over the timeout *\/ */
/* 	    dt = (ntv.tv_sec + ntv.tv_usec/1000000.0) - (tv.tv_sec + tv.tv_usec/1000000.0); */
/* 	    if (dt > ctx->timeout) { */
/* 		WARN("timeout"); */
/* 		return RMCP_ERR_TIMEOUT; */
/* 	    } */

/* 	    usleep(1000); */
/* 	} */
	else if (retval == RMCP_ERR_TIMEOUT) {
	    WARN("timeout");
	    return retval;
	}
	else {
	    /* serious error, return it */
	    ERROR("unknown error 0x%x",retval);
	    return retval;
	}
    }

    /* once we've read the raw, we need to unmarshall it, then figure out if
     * we need/can ack the msg
     */
    *msg = rmcp_unmarshall(ctx,buf,len);

/*     if (!ctx->secure || !ctx->rsp_init  */
/* 	|| (ctx->rsp_init && msg->rsp_tlr_valid)) { */
    
    /* pass it to the rmcp_send_ack function, which does some more
     * deciding on whether the msg should go through
     *
     * we return any errors from send_ack (i.e., socket, busted pipe,
     * whatever.
     */
    if (rmcp_build_ack(ctx,*msg,&ack) == RMCP_SUCCESS) {
	INFO(1,"sending ack");
	retval = rmcp_send_msg(ctx,ack);
	rmcp_free_asf_msg(ack);
	return retval;
    }

    INFO(1,"success");
    /* notice that we just let messages that don't need acks to come through */
    return RMCP_SUCCESS;
    
}

rmcp_error_t rmcp_send_msg_wait_ack(rmcp_ctx_t *ctx,
				    rmcp_msg_t *sendmsg) {
    /* send msg, then recv an ack if necessary */
    /* if not receive ack, do N resends (ctx->retries) */
    rmcp_error_t retval;
    rmcp_msg_t *ack = NULL;
    int i = 0;

    while (i < ctx->retries) {
	INFO(1,"retry %d",i+1);
	retval = rmcp_send_msg(ctx,sendmsg);
	if (retval != RMCP_SUCCESS) {
	    return retval;
	}
	/* do we need an ack? */
	if (sendmsg->hdr->seqno != RMCP_NOACK_SEQNO 
	    && !(sendmsg->hdr->class & RMCP_ACK_CLASS_BIT)) {
	    INFO(3,"waiting for ack");
	    retval = rmcp_recv_msg_and_ack(ctx,&ack);
	    if (retval == RMCP_ERR_TIMEOUT) {
		INFO(4,"timeout");
	    }
	    else if (retval == RMCP_SUCCESS) {
		/* check the ack and make sure it's ack'ing our sent msg */
		/* if not, call it a protocol error */
		if (ack->hdr->class & RMCP_ACK_CLASS_BIT 
		    && (ack->hdr->class & 0x7f) == sendmsg->hdr->class) {

		    rmcp_free_asf_msg(ack);
		    INFO(1,"recv'd ack");

		    /** 
		     * NOTE NOTE NOTE
		     * we're incrementing BOTH RMCP seqno and RSP seqno here!
		     */
		    if (ctx->secure && ctx->rsp_init) {
			rmcp_rsp_incr_seqno(ctx);
		    }

		    rmcp_incr_seqno(ctx);

		    return RMCP_SUCCESS;
		}
		else {
		    if (ack != NULL) {
			rmcp_free_asf_msg(ack);
		    }
		    WARN("did not recv expected ack!");
		    return RMCP_ERR_PROTOCOL;
		}
	    }
	    else {
		if (ack != NULL) {
		    rmcp_free_asf_msg(ack);
		}
		WARN("unexpected error %d",retval);
		return retval;
	    }
	}
	else {
	    return RMCP_SUCCESS;
	}

	++i;
    }

    /* if we get here, we've exceeded the max retry threshold */
    WARN("exceeded max retries");
    return RMCP_ERR_MAX_RETRIES;
}

rmcp_error_t rmcp_send_msg(rmcp_ctx_t *ctx,rmcp_msg_t *msg) {
    u_int8_t *buf;
    int len;
    int wc = 0;
    int retval;
    int tlen;

    buf = rmcp_marshall(ctx,msg,&len);

    INFO(5,"marshalled msg");

    if (buf != NULL && len > 0) {
	/* send the contents of buf */
	tlen = sizeof(struct sockaddr_in);

	if ((wc = sendto(ctx->sockfd,
			 buf,len,
			 0,
			 (struct sockaddr *)&ctx->client_sa,tlen)) < 0) {
	    if (errno == EAGAIN) {
		/* we're not currently setting a timeout on output,
		 * but if we ever do...
		 */
		WARN("timeout");
		return RMCP_ERR_TIMEOUT;
	    }
	    else {
		ERROR("sendto: %s",strerror(errno));
		return RMCP_ERR_SOCKET;
	    }
	}
	else if (wc != len) {
	    ERROR("sendto: %s",strerror(errno));
	    return RMCP_ERR_SOCKET;
	}	    

	INFO(3,"success");
    }

    free(buf);

    return RMCP_SUCCESS;
}

u_int8_t *rmcp_marshall(rmcp_ctx_t *ctx,rmcp_msg_t *msg,int *len) {
    int sz = 0;
    int i;
    u_int32_t tmp;
    u_int8_t buf[512];
    u_int8_t hmac_buf[EVP_MAX_MD_SIZE];
    int hmac_len = 0;
    u_int8_t *retval = NULL;
    
    /* rmcp rsp header */
    if (ctx->secure && msg->shdr) {
	tmp = htonl(msg->shdr->sid);
	memcpy(&buf[sz],(u_int8_t *)&tmp,sizeof(u_int32_t));
	sz += sizeof(u_int32_t);
	/* copy in the rsp seqno, but do NOT incr; let the send function do
	   that once it gets ack/response */
	tmp = htonl(msg->shdr->seqno);
	memcpy(&buf[sz],(u_int8_t *)&tmp,sizeof(u_int32_t));
	sz += sizeof(u_int32_t);

	INFO(2,"added RSP header: seqno=0x%x,session id=0x%x",
	     msg->shdr->seqno,msg->shdr->sid);
    }

    /* rmcp hdr */
    if (msg->hdr) {
	buf[sz++] = msg->hdr->version;
	buf[sz++] = msg->hdr->reserved;
	buf[sz++] = msg->hdr->seqno;
	buf[sz++] = msg->hdr->class;
    }

    /* rmcp asf data */
    if (msg->data) {
	tmp = htonl(msg->data->iana_ent_id);
	memcpy(&buf[sz],(u_int8_t *)&tmp,sizeof(u_int32_t));
	sz += sizeof(u_int32_t);
	buf[sz++] = msg->data->type;
	buf[sz++] = msg->data->tag;
	buf[sz++] = msg->data->reserved;
	buf[sz++] = msg->data->data_len;
	if (msg->data->data_len > 0) {
	    memcpy(&buf[sz],msg->data->data,msg->data->data_len);
	    sz += msg->data->data_len;
	}
    }

    /* rmcp rsp trailer */
    if (ctx->secure && ctx->rsp_init && msg->stlr) {
	/* the msg, from rsp hdr to the next hdr field in the rsp tlr, must
	 * be protected. */
	/* so we have to align those bytes to a multiple of 4 */
	/* we add 2 bytes to the current count to account for the pad length 
	 * and next hdr fields in the trailer, which we haven't added yet --
	 * padding comes first. */
	tmp = (sz + 2) % 4;
	for (i = 0; i < tmp; ++i) {
	    buf[sz++] = 0;
	}
	/* pad length */
	buf[sz++] = tmp;
	/* next hdr value, by the spec */
	buf[sz++] = msg->hdr->version;
	/* now we have to compute the HMAC on this, using the session key */
	HMAC(EVP_sha1(),
	     ctx->sik,sizeof(ctx->sik),
	     &buf[0],sz,
	     hmac_buf,&hmac_len);
	memcpy(&buf[sz],hmac_buf,12);
	sz += 12;

	INFO(2,"computed RSP trailer");

    }

    /* ok, we've marshalled, just malloc and return to user */
    if (sz > 0) {
	retval = (u_int8_t *)malloc(sizeof(u_int8_t)*sz);
	memcpy(retval,&buf[0],sz);
    }
    else {
	retval = NULL;
    }
    
    if (len != NULL) {
	*len = sz;
    }

    return retval;
}

rmcp_msg_t *rmcp_unmarshall(rmcp_ctx_t *ctx,u_int8_t *buf,int len) {
    rmcp_msg_t *msg;
    int sz = 0;
    int rsz = 0;
    u_int8_t hmac_buf[EVP_MAX_MD_SIZE];
    int hmac_len = 0;

    msg = rmcp_build_asf_msg(ctx);

    if (ctx->secure) {
	msg->shdr->sid = ntohl(*(u_int32_t *)(&buf[sz]));
	sz += sizeof(u_int32_t);
	msg->shdr->seqno = ntohl(*(u_int32_t *)(&buf[sz]));
	sz += sizeof(u_int32_t);
    }

    /* rmcp hdr */
    msg->hdr->version = buf[sz++];
    msg->hdr->reserved = buf[sz++];
    msg->hdr->seqno = buf[sz++];
    msg->hdr->class = buf[sz++];

    /* rmcp asf data */
    msg->data->iana_ent_id = ntohl(*(u_int32_t *)(&buf[sz]));
    //INFO(5,"data->iana_ent_id = 0x%x",msg->data->iana_ent_id);
    sz += sizeof(u_int32_t);
    msg->data->type = buf[sz++];
    msg->data->tag = buf[sz++];
    msg->data->reserved = buf[sz++];
    msg->data->data_len = buf[sz++];
    if (msg->data->data_len > 0) {
	msg->data->data = 
	    (u_int8_t *)malloc(sizeof(u_int8_t)*msg->data->data_len);
	memcpy(msg->data->data,&buf[sz],msg->data->data_len);
	sz += msg->data->data_len;
    }

    if (ctx->secure && ctx->rsp_init) {
	/* since we have the whole message, we read in from the end to 
	 * make life easier.
	 */
	rsz = len-1-sizeof(msg->stlr->integrity);
	memcpy(msg->stlr->integrity,
	       &buf[rsz],
	       sizeof(msg->stlr->integrity));
	msg->stlr->next_hdr = buf[--rsz];
	msg->stlr->pad_len = buf[--rsz];

	/* now validate this guy */
	HMAC(EVP_sha1(),
	     ctx->sik,sizeof(ctx->sik),
	     &buf[0],len-sizeof(msg->stlr->integrity),
	     hmac_buf,&hmac_len);

	if (strcmp(&buf[len-1-sizeof(msg->stlr->integrity)],
		   hmac_buf) == 0) {
	    msg->tlr_was_valid = 1;
	}
	else {
	    msg->tlr_was_valid = 0;
	}
    }

    INFO(3,"unmarshalled msg");

    return msg;
    
}

rmcp_msg_t *rmcp_build_asf_msg(rmcp_ctx_t *ctx) {
    rmcp_msg_t *msg = NULL;

    msg = (rmcp_msg_t *)malloc(sizeof(rmcp_msg_t));

    if (!msg) {
	return NULL;
    }

    memset(msg,0,sizeof(msg));

    /* always have a header if we're on the secure port */
    if (ctx->secure) {
	msg->shdr = (rmcp_rsp_hdr_t *)malloc(sizeof(rmcp_rsp_hdr_t));
	memset(msg->shdr,0,sizeof(rmcp_rsp_hdr_t));
    }

    msg->hdr = (rmcp_hdr_t *)malloc(sizeof(rmcp_hdr_t));
    memset(msg->hdr,0,sizeof(rmcp_hdr_t));
    
    msg->data = (rmcp_asf_data_t *)malloc(sizeof(rmcp_asf_data_t));
    memset(msg->data,0,sizeof(rmcp_asf_data_t));

    /* only have a trailer if we've actually made it through the RAKP steps */
    if (ctx->rsp_init) {
	msg->stlr = (rmcp_rsp_tlr_t *)malloc(sizeof(rmcp_rsp_tlr_t));
	memset(msg->stlr,0,sizeof(rmcp_rsp_tlr_t));
    }
    else {
	msg->stlr = NULL;
    }

    return msg;
}

void rmcp_free_asf_msg(rmcp_msg_t *msg) {
    if (msg != NULL) {
	if (msg->shdr) {
	    free(msg->shdr);
	    msg->shdr = NULL;
	}
	if (msg->hdr) {
	    free(msg->hdr);
	    msg->hdr = NULL;
	}
	if (msg->data) {
	    if (msg->data->data_len && msg->data->data) {
		free(msg->data->data);
		msg->data->data = NULL;
	    }
	    free(msg->data);
	    msg->data = NULL;
	}
	if (msg->stlr) {
	    free(msg->stlr);
	    msg->stlr = NULL;
	}

	free(msg);
	msg = NULL;
    }
}

void rmcp_fill_asf_ping(rmcp_ctx_t *ctx,rmcp_msg_t *msg) {
    rmcp_rsp_fill_hdr(ctx,msg);
    rmcp_fill_hdr(ctx,msg);
    rmcp_fill_asf_data(ctx,msg,RMCP_ASF_TYPE_PRESENCE_PING,0,NULL);
    rmcp_rsp_fill_tlr(ctx,msg);
}

void rmcp_fill_asf_capabilities_request(rmcp_ctx_t *ctx,rmcp_msg_t *msg) {
    rmcp_rsp_fill_hdr(ctx,msg);
    rmcp_fill_hdr(ctx,msg);
    rmcp_fill_asf_data(ctx,msg,RMCP_ASF_TYPE_CAPABILITIES_REQUEST,0,NULL);
    rmcp_rsp_fill_tlr(ctx,msg);
}

void rmcp_fill_asf_sysstate_request(rmcp_ctx_t *ctx,rmcp_msg_t *msg) {
    rmcp_rsp_fill_hdr(ctx,msg);
    rmcp_fill_hdr(ctx,msg);
    rmcp_fill_asf_data(ctx,msg,RMCP_ASF_TYPE_SYSSTATE_REQUEST,0,NULL);
    rmcp_rsp_fill_tlr(ctx,msg);
}

/**
 * acknowledge all packets that are not themselves acks; if they are
 * secure RMCP packets, only ack if the packet validates; and don't ack packets
 * that have a seqno of 0xff (no ack).
 */
rmcp_error_t rmcp_build_ack(rmcp_ctx_t *ctx,
			    rmcp_msg_t *orig,
			    rmcp_msg_t **ack) {
    rmcp_msg_t *new;

    if (orig->hdr->class & RMCP_ACK_CLASS_BIT
	|| orig->hdr->seqno == 0xff) {
	/* it's an ack or a packet that doesn't want an ack, return success */
	return RMCP_ERR_NO_ACK_NEEDED;
    }
    
    if (ctx->rsp_init && !orig->tlr_was_valid) {
	return RMCP_ERR_NO_ACK_INVALID;
    }

    /* send acks for everybody else */
    /* copy everything in the RSP and RMCP headers */
    *ack = rmcp_build_asf_msg(ctx);
    new = *ack;

    if (ctx->secure && new->shdr) {
	memcpy(new->shdr,orig->shdr,sizeof(rmcp_rsp_hdr_t));
    }
    else {
	if (new->shdr) {
	    free(new->shdr);
	    new->shdr = NULL;
	}
    }

    memcpy(new->hdr,orig->hdr,sizeof(rmcp_hdr_t));
    /* now change the MSB in the class field to identify as an ack */
    new->hdr->class |= RMCP_ACK_CLASS_BIT;

    /* dealloc the asf data; don't need it */
    if (new->data) {
	if (new->data->data) {
	    free(new->data->data);
	    new->data->data = NULL;
	}
	free(new->data);
	new->data = NULL;
    }

    /* dealloc rmcp rsp trailer */
    if (new->stlr) {
	free(new->stlr);
	new->stlr = NULL;
    }

    return RMCP_SUCCESS;
    
}







rmcp_asf_supported_t *rmcp_decode_asf_supported(rmcp_msg_t *msg) {
    rmcp_asf_supported_t *retval = NULL;

    retval = (rmcp_asf_supported_t *)malloc(sizeof(rmcp_asf_supported_t));

    if (retval != NULL 
	&& (msg != NULL && msg->hdr != NULL && msg->data != NULL)) {
	retval = (rmcp_asf_supported_t *)malloc(sizeof(rmcp_asf_supported_t));

	memset(retval,0,sizeof(rmcp_asf_supported_t));
	
	retval->iana_id = ntohl(*((u_int32_t*)&(msg->data->data[0])));
	retval->oem = ntohl(*((u_int32_t*)&(msg->data->data[4])));

	retval->ipmi = (msg->data->data[8] & RMCP_SUPPORTED_IPMI)?1:0;
	retval->asf_1 = (msg->data->data[8] & RMCP_SUPPORTED_ASF_1)?1:0;
	retval->secure_rmcp = (msg->data->data[9] & RMCP_SUPPORTED_SECURE)?1:0;
    };

    return retval;
}
void rmcp_print_asf_supported(rmcp_asf_supported_t *rs,FILE *file) {
    FILE *of = (file == NULL)?stdout:file;

    fprintf(of,
	    "RMCP Support:\n"
	    "  IANA Enterprise Number: %d (0x%x)\n"
	    "  OEM Defined:            %d (0x%x)\n"
	    "  Supported Entities:\n"
	    "    IPMI:                 %d\n"
	    "    ASF 1.0:              %d\n"
	    "  Supported Interactions:\n"
	    "    RMCP Security Ext:    %d\n",
	    rs->iana_id,rs->iana_id,
	    rs->oem,rs->oem,
	    rs->ipmi,rs->asf_1,rs->secure_rmcp);
    fflush(of);

    return;
}

rmcp_asf_capabilities_t *rmcp_decode_asf_capabilities(rmcp_msg_t *msg) {
    rmcp_asf_capabilities_t *retval = NULL;

    retval = (rmcp_asf_capabilities_t *)malloc(sizeof(rmcp_asf_capabilities_t));

    if (retval != NULL 
	&& (msg != NULL && msg->hdr != NULL && msg->data != NULL
	    && msg->data->type == RMCP_ASF_TYPE_CAPABILITIES_RESPONSE)) {
	retval = (rmcp_asf_capabilities_t *)malloc(sizeof(rmcp_asf_capabilities_t));

	memset(retval,0,sizeof(rmcp_asf_capabilities_t));
	
	retval->iana_id = ntohl(*((u_int32_t*)&(msg->data->data[0])));
	retval->oem = ntohl(*((u_int32_t*)&(msg->data->data[4])));

	retval->commands.force_cd_boot = 
	    (msg->data->data[9] & RMCP_ASF_COM_CD)?1:0;
	retval->commands.force_diag_boot = 
	    (msg->data->data[9] & RMCP_ASF_COM_DIAG)?1:0;
	retval->commands.force_hdd_safe_boot = 
	    (msg->data->data[9] & RMCP_ASF_COM_HDD_SAFE)?1:0;
	retval->commands.force_hdd_boot = 
	    (msg->data->data[9] & RMCP_ASF_COM_HDD)?1:0;
	retval->commands.force_pxe_boot = 
	    (msg->data->data[9] & RMCP_ASF_COM_PXE)?1:0;

	retval->capabilities.reset_both = 
	    (msg->data->data[10] & RMCP_ASF_CAP_RESET_BOTH)?1:0;
	retval->capabilities.power_up_both = 
	    (msg->data->data[10] & RMCP_ASF_CAP_POWER_UP_BOTH)?1:0;
	retval->capabilities.power_down_both = 
	    (msg->data->data[10] & RMCP_ASF_CAP_POWER_DOWN_BOTH)?1:0;
	retval->capabilities.power_cycle_both = 
	    (msg->data->data[10] & RMCP_ASF_CAP_POWER_CYCLE_BOTH)?1:0;
	retval->capabilities.reset_secure = 
	    (msg->data->data[10] & RMCP_ASF_CAP_RESET_SECURE)?1:0;
	retval->capabilities.power_up_secure = 
	    (msg->data->data[10] & RMCP_ASF_CAP_POWER_UP_SECURE)?1:0;
	retval->capabilities.power_down_secure = 
	    (msg->data->data[10] & RMCP_ASF_CAP_POWER_DOWN_SECURE)?1:0;
	retval->capabilities.power_cycle_secure = 
	    (msg->data->data[10] & RMCP_ASF_CAP_POWER_CYCLE_SECURE)?1:0;

	retval->firmware_capabilities.sleep_button_lock = 
	    (msg->data->data[11] & RMCP_ASF_FCAP_B1_SLEEP_BUTTON_LOCK)?1:0;
	retval->firmware_capabilities.lock_keyboard = 
	    (msg->data->data[11] & RMCP_ASF_FCAP_B1_LOCK_KEYBOARD)?1:0;
	retval->firmware_capabilities.reset_button_lock = 
	    (msg->data->data[11] & RMCP_ASF_FCAP_B1_RESET_BUTTON_LOCK)?1:0;
	retval->firmware_capabilities.power_button_lock = 
	    (msg->data->data[11] & RMCP_ASF_FCAP_B1_POWER_BUTTON_LOCK)?1:0;
	retval->firmware_capabilities.firmware_screen_blank = 
	    (msg->data->data[11] & RMCP_ASF_FCAP_B1_FW_VERB_SCREEN_BLANK)?1:0;

	retval->firmware_capabilities.config_data_reset = 
	    (msg->data->data[12] & RMCP_ASF_FCAP_B2_CONFIG_DATA_RESET)?1:0;
	retval->firmware_capabilities.firmware_quiet = 
	    (msg->data->data[12] & RMCP_ASF_FCAP_B2_FW_VERB_QUIET)?1:0;
	retval->firmware_capabilities.firmware_verbose = 
	    (msg->data->data[12] & RMCP_ASF_FCAP_B2_FW_VERB_VERB)?1:0;
	retval->firmware_capabilities.forced_progress_events = 
	    (msg->data->data[12] & RMCP_ASF_FCAP_B2_FORCED_PROG_EVENTS)?1:0;
	retval->firmware_capabilities.user_passwd_bypass = 
	    (msg->data->data[12] & RMCP_ASF_FCAP_B2_USER_PASSWD_BYPASS)?1:0;

    };

    return retval;
}
void rmcp_print_asf_capabilities(rmcp_asf_capabilities_t *rs,FILE *file) {
    FILE *of = (file == NULL)?stdout:file;

    fprintf(of,
	    "ASF Capabilities:\n"
	    "  IANA Enterprise Number:   %d (0x%x)\n"
	    "  OEM Defined:              %d (0x%x)\n"
	    "  (Boot) Commands:\n"
	    "    Force CD/DVD:           %d\n"
	    "    Force Diagnostics:      %d\n"
	    "    Force HDD Safe Mode:    %d\n"
	    "    Force HHD:              %d\n"
	    "    Force PXE:              %d\n"
	    "  Power Capabilities:\n"
	    "    Reset (secure):         %d\n"
	    "    Power Up (secure):      %d\n"
	    "    Power Down (secure:     %d\n"
	    "    Power Cycle (secure):   %d\n"
	    "    Reset (both):           %d\n"
	    "    Power Up (both):        %d\n"
	    "    Power Down (both:       %d\n"
	    "    Power Cycle (both):     %d\n"
	    "  Firmware Capabilities:\n"
	    "    Sleep Button Lock:      %d\n"
	    "    Lock Keyboard:          %d\n"
	    "    Reset Button Lock:      %d\n"
	    "    Power Button Lock:      %d\n"
	    "    Config Data Reset:      %d\n"
	    "    Firmware Screen Blank:  %d\n"
	    "    Firmware Quiet:         %d\n"
	    "    Firmware Verbose:       %d\n"
	    "    Forced Progress Events: %d\n"
	    "    User Password Bypass:   %d\n",
	    rs->iana_id,rs->iana_id,
	    rs->oem,rs->oem,

	    rs->commands.force_cd_boot,rs->commands.force_diag_boot,
	    rs->commands.force_hdd_safe_boot,rs->commands.force_hdd_boot,
	    rs->commands.force_pxe_boot,
	    
	    rs->capabilities.reset_secure,rs->capabilities.power_up_secure,
	    rs->capabilities.power_down_secure,
	    rs->capabilities.power_cycle_secure,
	    rs->capabilities.reset_both,rs->capabilities.power_up_both,
	    rs->capabilities.power_down_both,
	    rs->capabilities.power_cycle_both,

	    rs->firmware_capabilities.sleep_button_lock,
	    rs->firmware_capabilities.lock_keyboard,
	    rs->firmware_capabilities.reset_button_lock,
	    rs->firmware_capabilities.power_button_lock,
	    rs->firmware_capabilities.config_data_reset,
	    rs->firmware_capabilities.firmware_screen_blank,
	    rs->firmware_capabilities.firmware_quiet,
	    rs->firmware_capabilities.firmware_verbose,
	    rs->firmware_capabilities.forced_progress_events,
	    rs->firmware_capabilities.user_passwd_bypass);
    fflush(of);

    return;
}

rmcp_asf_sysstate_t *rmcp_decode_asf_sysstate(rmcp_msg_t *msg) {
    rmcp_asf_sysstate_t *retval = NULL;

    retval = (rmcp_asf_sysstate_t *)malloc(sizeof(rmcp_asf_sysstate_t));

    if (retval != NULL 
	&& (msg != NULL && msg->hdr != NULL && msg->data != NULL
	    && msg->data->type == RMCP_ASF_TYPE_SYSSTATE_RESPONSE)) {
	retval = (rmcp_asf_sysstate_t *)malloc(sizeof(rmcp_asf_sysstate_t));

	memset(retval,0,sizeof(rmcp_asf_sysstate_t));
	
	retval->sysstate = 
	    (msg->data->data[0] & RMCP_ASF_SYSSTATEMASK);
	retval->wdstate = 
	    (msg->data->data[1] & RMCP_ASF_WDSTATEMASK_EXPIRED);
    };

    return retval;
}

static char *rmcp_asf_sysstate_strings[] = {
    "S0/G0 working",
    "S1 sleeping with system hw and processor context maintained",
    "S2 sleeping, processor context lost",
    "S3 sleeping, processor and hw context lost, memory retained",
    "S4 non-volatile sleep/suspend-to-disk",
    "S5/G2 soft-off",
    "S4/S5 soft-off, particular S4/S5 state cannot be determined",
    "G3/Mechanical Off",
    "Sleeping in S1, S2, or S3 states (nondet), or Legacy SLEEP state",
    "G1 sleeping (S1-S4 nondet)",
    "S5 entered by override (i.e., 4-sec power button override)",
    "Legacy ON state (i.e., non-ACPI OS running)",
    "Legacy OFF state (i.e., non-ACPI OS off)",
    "UNDEFINED!",
    "Unknown",
    "Reserved for future definition"
};

static char *rmcp_asf_wdstate_strings[] = {
    "Watchdog Timer has expired (if it was set)",
    "Watchdog Timer is not expired"
};

void rmcp_print_asf_sysstate(rmcp_asf_sysstate_t *rs,FILE *file) {
    FILE *of = (file == NULL)?stdout:file;

    fprintf(of,
	    "ASF System State:\n"
	    "  System State:   %d (0x%x) [%s]\n"
	    "  Watchdog State: %d (ox%x) [%s]\n",
	    rs->sysstate,rs->sysstate,
	    (rs->sysstate > 0x0f)?"UNDEFINED!":rmcp_asf_sysstate_strings[rs->sysstate],
	    rs->wdstate,rs->wdstate,
	    (rs->wdstate > 1)?"UNDEFINED!":rmcp_asf_wdstate_strings[rs->wdstate]);
    fflush(of);

    return;
}
