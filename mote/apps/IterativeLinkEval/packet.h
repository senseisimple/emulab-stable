//#ifndef __PACKET_H__
//#define __PACKET_H__

#define MOTE_ADDR_ANY  0x0

typedef enum { 
    AM_PKT_CMD = 10,
    AM_PKT_RADIO_RECV = 11,
    /* not used in the mig stuff... just between motes */
    AM_PKT_BCAST = 12,
} pkt_t;

typedef enum {
    CMD_TYPE_BCAST = 0,
    CMD_TYPE_BCAST_MULT,
    CMD_TYPE_RADIO_OFF,
    CMD_TYPE_RADIO_ON,
    CMD_TYPE_SET_MOTE_ID,
    CMD_TYPE_SET_RF_POWER,
    CMD_TYPE_HEARTBEAT,
} pkt_cmd_type_t;

typedef enum {
    CMD_STATUS_SUCCESS = 0,
    CMD_STATUS_ERROR,
    CMD_STATUS_UNKNOWN,
} pkt_cmd_status_t;

typedef enum {
    CMD_FLAGS_INCR_FIRST = 1,
} pkt_cmd_flags_t;

struct pkt_bcast {
    uint16_t src_mote_id;
    uint16_t dest_mote_id;
    uint8_t data[10];
};

typedef struct pkt_bcast pkt_bcast_t;

struct pkt_cmd {
    uint8_t cmd_no;
    uint8_t cmd_type;
    uint8_t cmd_status;
    uint8_t cmd_flags;
    
    struct pkt_bcast bcast;
    uint16_t interval;
    uint16_t msg_dup_count;
    uint16_t mote_id;
    uint8_t power;
};

typedef struct pkt_cmd pkt_cmd_t;

struct pkt_radio_recv {
    struct pkt_bcast bcast;

    uint16_t rssi;
};

typedef struct pkt_radio_recv pkt_radio_recv_t;

//#endif
