/* Definition of initial Mobile Testbed Protocol */

#ifndef __MTP_H__
#define __MTP_H__

/* version number */
#define MTP_VERSION   0x1

/* these are the possible opcodes for this protocol */
#define MTP_CONTROL_ERROR              11
#define MTP_CONTROL_NOTIFY             12
#define MTP_CONTROL_INIT               13
#define MTP_CONTROL_CLOSE              14
/* config info packet passed from EMC to VMC */
#define MTP_CONFIG_VMC                 20
/* config info packet passed from EMC to RMC */
#define MTP_CONFIG_RMC                 21
/* Position requests flow from either VMC/RMC to EMC */
#define MTP_REQUEST_POSITION           30
/* ID requests flow from VMC to EMC (in case the vision system loses track
 * of a robot -- but a position/orientation is known)
 */
#define MTP_REQUEST_ID                 31
/* I am not implementing this in the protocol yet -- our plan for the demo
 * is to send RMC a command; RMC will do its best to position it there without
 * constant feedback from the vision system; when RMC thinks the robot has 
 * arrived, it can tell EMC what it thinks its latest position is, along with
 * an 'in motion' status, and then ask for the best known position (which 
 * should be given by vision if possible (if vision is not there, then EMC will
 * return the position just given to it by the 'in motion' status update).
 */
/* #define MTP_REQUEST_POSITION_STREAM    32 */

/* These are sent to EMC from RMC and VMC */
#define MTP_UPDATE_POSITION            40
/* These are sent from EMC to VMC */
#define MTP_UPDATE_ID                  41
/* this is passed by EMC to RMC */
#define MTP_COMMAND_GOTO               50
/* EMC can force RMC to stop a specific robot, or stop all robots */
#define MTP_COMMAND_STOP               51

/* guarantees that we know what kind of a node the packet originated from 
 * or is destined to
 */
#define MTP_ROLE_VMC    2
#define MTP_ROLE_EMC    0
#define MTP_ROLE_RMC    1
#define MTP_ROLE_EMULAB 5

/* These are used in the status fields in MTP_POSITION_UPDATE packets */
#define MTP_POSITION_STATUS_IDLE     1
#define MTP_POSITION_STATUS_MOVING   2
#define MTP_POSITION_STATUS_ERROR    3
#define MTP_POSITION_STATUS_COMPLETE 4

/* we set this so that an application can only receive 32KB packets at once;
 * just a little measure to prevent denial-of-service on the server (because
 * the server will allocate a buffer based on the packet length.
 * I realize this approach will will screw up the entire socket datastream
 * if some client tries to send a massive packet, but that's their problem.
 * :-)  Obey the protocol or prepare to have your connection terminated!
 */
#define MTP_PACKET_MAXLEN 32768
#define MTP_PACKET_HEADER_LEN  7
#define MTP_PACKET_HEADER_OFFSET_LENGTH  0
#define MTP_PACKET_HEADER_OFFSET_OPCODE  4
#define MTP_PACKET_HEADER_OFFSET_VERSION 5
#define MTP_PACKET_HEADER_OFFSET_ROLE    6

/* here is the protocol definition in terms of structures */
typedef struct mtp_packet {
  /* length has no meaning for user applications -- it's just there to better
   * emphasize that at the byte level, each packet is prefixed by a length
   */
  unsigned int length;
  unsigned char opcode;
  unsigned char version;
  unsigned char role;
  
  union {
	struct mtp_control *control;
	struct mtp_config_rmc *config_rmc;
	struct mtp_config_vmc *config_vmc;
	struct mtp_request_position *request_position;
	struct mtp_request_id *request_id;
	struct mtp_update_position *update_position;
	struct mtp_update_id *update_id;
	struct mtp_command_goto *command_goto;
	struct mtp_command_stop *command_stop;
  } data;

} mtp_packet_t;

/* the main control data */
struct mtp_control {
  int id;
  /* error code num */
  int code;
  /* a message string */
  char *msg;
};

struct robot_config {
  int id;
  char *hostname;
};
struct global_bound {
  float horizontal;
  float vertical;
};

struct mtp_config_rmc {
  int num_robots;
  struct robot_config *robots;
  struct global_bound box;
};

struct mtp_config_vmc {
  int num_robots;
  struct robot_config *robots;
};

struct mtp_request_position {
  int robot_id;
};

struct position {
  float x;
  float y;
  float theta;
};

struct mtp_update_position {
  int robot_id;
  struct position position;
  int status;
  int timestamp;
};

struct mtp_request_id {
  struct position position;
  int timestamp;
};

struct mtp_update_id {
  int robot_id;
};

struct mtp_command_goto {
  int command_id;
  int robot_id;
  struct position position;
};
struct mtp_command_stop {
  int command_id;
  int robot_id;
};

/* A few helper functions */

/* return codes for receive_packet, et al */
#define MTP_PP_SUCCESS        0

#define MTP_PP_ERROR          -1

#define MTP_PP_ERROR_MALLOC   -10
#define MTP_PP_ERROR_ARGS     -11
#define MTP_PP_ERROR_READ     -12
#define MTP_PP_ERROR_LENGTH   -13
#define MTP_PP_ERROR_WRITE    -14

int mtp_receive_packet(int fd,struct mtp_packet **packet);
int mtp_send_packet(int fd,struct mtp_packet *packet);
/* you have to pass a pointer to a char * that I can fill in
 * otherwise I can't pass the length of the buffer back! 
 */
int mtp_encode_packet(char **buf,struct mtp_packet *packet);
/* buf must be null-terminated */
int mtp_decode_packet(char *buf,struct mtp_packet **packet);
struct mtp_packet *mtp_make_packet( unsigned char opcode,
									unsigned char role,
									/* data's type is only known by opcode */
									void *data
									);
/* data's type is known by opcode! */
int mtp_calc_size(int opcode,void *data);


#endif
