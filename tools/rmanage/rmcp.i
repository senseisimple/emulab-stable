/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2007 University of Utah and the Flux Group.
 * All rights reserved.
 *
 */
%module rmcp
%{
#include "rmcp.h"
%}


%rename swr_fill_str rmcp_setstrbuf;

/**
 * Since I couldn't figure out how to pass a pointer-to-pointer-to-struct,
 * and then retrieve a meaningful value in perl, I just flip the names 'round
 * so that things make sense to people reading the C headers.
 */
%rename rmcp_asf_ping c_rmcp_asf_ping;
%rename swr_rmcp_asf_ping rmcp_asf_ping;

%rename rmcp_asf_get_capabilities c_rmcp_asf_get_capabilities;
%rename swr_rmcp_asf_get_capabilities rmcp_asf_get_capabilities;

%rename rmcp_asf_get_sysstate c_rmcp_asf_get_sysstate;
%rename swr_rmcp_asf_get_sysstate rmcp_asf_get_sysstate;

/**
 * No sane person would want to use these functions via swig.
 * If that changes, we'd have to do some funky stuff to deal with all the 
 * "nonstandard" standard types used in rmcp.(h|c).
 */
%ignore rmcp_raw_read;
%ignore rmcp_recv_msg_and_ack;
%ignore rmcp_send_msg;
%ignore rmcp_send_msg_wait_ack;
%ignore rmcp_marshall;
%ignore rmcp_unmarshall;
%ignore rmcp_build_ack;
%ignore rmcp_build_asf_msg;
%ignore rmcp_free_asf_msg;
%ignore rmcp_fill_asf_ping;
%ignore rmcp_fill_asf_capabilities_request;
%ignore rmcp_fill_asf_sysstate_request;
%ignore rmcp_decode_asf_supported;
%ignore rmcp_print_asf_supported;
%ignore rmcp_decode_asf_capabilities;
%ignore rmcp_print_asf_capabilities;
%ignore rmcp_decode_asf_sysstate;
%ignore rmcp_print_asf_sysstate;

/**
 * Lots of these identifiers and types people cannot get to since we excluded
 * the previous function list.  So, kill them too.
 */
%ignore RMCP_IANA_ENT_ID;
%ignore RMCP_VERSION_1_0;
%ignore RMCP_NOACK_SEQNO;
%ignore RMCP_ACK_CLASS_BIT;
%ignore RMCP_RSP_NXT_HDR;
%ignore RMCP_SID_BYPASS;
%ignore RMCP_ERR_MAX;
%ignore rmcp_flag_b;
%ignore rmcp_flag;
%ignore RMCP_CLASS_ASF;
%ignore RMCP_CLASS_IPMI;
%ignore rmcp_hdr;
%ignore RMCP_ASF_TYPE_RESET;
%ignore RMCP_ASF_TYPE_POWER_UP;
%ignore RMCP_ASF_TYPE_POWER_DOWN;
%ignore RMCP_ASF_TYPE_POWER_CYCLE_RESET;
%ignore RMCP_ASF_TYPE_PRESENCE_PONG;
%ignore RMCP_ASF_TYPE_CAPABILITIES_RESPONSE;
%ignore RMCP_ASF_TYPE_SYSSTATE_RESPONSE;
%ignore RMCP_ASF_TYPE_OPEN_SESSION_RESPONSE;
%ignore RMCP_ASF_TYPE_CLOSE_SESSION_RESPONSE;
%ignore RMCP_ASF_TYPE_PRESENCE_PING;
%ignore RMCP_ASF_TYPE_CAPABILITIES_REQUEST;
%ignore RMCP_ASF_TYPE_SYSSTATE_REQUEST;
%ignore RMCP_ASF_TYPE_OPEN_SESSION_REQUEST;
%ignore RMCP_ASF_TYPE_CLOSE_SESSION_REQUEST;
%ignore RMCP_ASF_TYPE_RAKP_MSG_1;
%ignore RMCP_ASF_TYPE_RAKP_MSG_2;
%ignore RMCP_ASF_TYPE_RAKP_MSG_3;
%ignore rmcp_asf_data;
%ignore rmcp_rsp_hdr;
%ignore rmcp_rsp_tlr;
%ignore rmcp_msg;
%ignore RMCP_SUPPORTED_IPMI;
%ignore RMCP_SUPPORTED_ASF_1;
%ignore RMCP_SUPPORTED_SECURE;
%ignore RMCP_ASF_COM_CD;
%ignore RMCP_ASF_COM_DIAG;
%ignore RMCP_ASF_COM_HDD_SAFE;
%ignore RMCP_ASF_COM_HDD;
%ignore RMCP_ASF_COM_PXE;
%ignore RMCP_ASF_CAP_RESET_BOTH;
%ignore RMCP_ASF_CAP_POWER_UP_BOTH;
%ignore RMCP_ASF_CAP_POWER_DOWN_BOTH;
%ignore RMCP_ASF_CAP_POWER_CYCLE_BOTH;
%ignore RMCP_ASF_CAP_RESET_SECURE;
%ignore RMCP_ASF_CAP_POWER_UP_SECURE;
%ignore RMCP_ASF_CAP_POWER_DOWN_SECURE;
%ignore RMCP_ASF_CAP_POWER_CYCLE_SECURE;
%ignore RMCP_ASF_FCAP_B1_SLEEP_BUTTON_LOCK;
%ignore RMCP_ASF_FCAP_B1_LOCK_KEYBOARD;
%ignore RMCP_ASF_FCAP_B1_RESET_BUTTON_LOCK;
%ignore RMCP_ASF_FCAP_B1_POWER_BUTTON_LOCK;
%ignore RMCP_ASF_FCAP_B1_FW_VERB_SCREEN_BLANK;
%ignore RMCP_ASF_FCAP_B2_CONFIG_DATA_RESET;
%ignore RMCP_ASF_FCAP_B2_FW_VERB_QUIET;
%ignore RMCP_ASF_FCAP_B2_FW_VERB_VERB;
%ignore RMCP_ASF_FCAP_B2_FORCED_PROG_EVENTS;
%ignore RMCP_ASF_FCAP_B2_USER_PASSWD_BYPASS;
%ignore RMCP_ASF_SYSSTATEMASK;
/**
 * We leave these in because people have to decode the state/watchdog values.
 * ignore RMCP_ASF_SYSSTATE_S0_G0;
 * ignore RMCP_ASF_SYSSTATE_S1;
 * ignore RMCP_ASF_SYSSTATE_S2;
 * ignore RMCP_ASF_SYSSTATE_S3;
 * ignore RMCP_ASF_SYSSTATE_S4;
 * ignore RMCP_ASF_SYSSTATE_S5_G2;
 * ignore RMCP_ASF_SYSSTATE_S4_S5;
 * ignore RMCP_ASF_SYSSTATE_G3;
 * ignore RMCP_ASF_SYSSTATE_S1_S2_S3;
 * ignore RMCP_ASF_SYSSTATE_G1;
 * ignore RMCP_ASF_SYSSTATE_S5_OVERRIDE;
 * ignore RMCP_ASF_SYSSTATE_LON;
 * ignore RMCP_ASF_SYSSTATE_LOFF;
 * ignore RMCP_ASF_SYSSTATE_UNKNOWN;
 * ignore RMCP_ASF_SYSSTATE_RESERVED;
 * ignore RMCP_ASF_WDSTATEMASK_EXPIRED;
 */
%ignore RMCP_RSP_RAKP_SUCCESS;
%ignore RMCP_RSP_RAKP_ERR_RESOURCES;
%ignore RMCP_RSP_RAKP_ERR_INVALID_SID;
%ignore RMCP_RSP_RAKP_ERR_INVALID_PAYLOAD;
%ignore RMCP_RSP_RAKP_ERR_INVALID_AUTH_ALG;
%ignore RMCP_RSP_RAKP_ERR_INVALID_INTEGRITY_ALG;
%ignore RMCP_RSP_RAKP_ERR_NOMATCH_AUTH;
%ignore RMCP_RSP_RAKP_ERR_NOMATCH_INTEGRITY;
%ignore RMCP_RSP_RAKP_ERR_INACTIVE_SID;
%ignore RMCP_RSP_RAKP_ERR_INVALID_ROLE;
%ignore RMCP_RSP_RAKP_ERR_UNAUTH_ROLE;
%ignore RMCP_RSP_RAKP_ERR_RESOURCES_ROLE;
%ignore RMCP_RSP_RAKP_ERR_INVALID_NAMELEN;
%ignore RMCP_RSP_RAKP_ERR_UNAUTH_NAME;
%ignore RMCP_RSP_RAKP_ERR_UNAUTH_GUID;
%ignore RMCP_RSP_RAKP_ERR_INVALID_INTEGRITY_VALUE;



/* slurp up the remnants... */
%include "rmcp.h"
