
#include <errno.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>

#include "mtp.h"

static int readall(int fd,char *buffer,unsigned int len)
{
  int rc, off = 0, retval;

  assert(fd >= 0);
  assert(buffer != NULL);

  while (((rc = read(fd,&buffer[off],len - off)) > 0) ||
	 ((rc == -1) && (errno == EINTR))) {
    if (rc > 0) {
      off += rc;
    }
  }

  if (rc < 0) {
    retval = rc;
  }
  else if (off == 0) {
    errno = ENOTCONN;
    retval = -1;
  }
  else if (off != len) {
    errno = EIO;
    retval = -1;
  }
  else {
    retval = off;
  }
  
  return retval;
}

int mtp_receive_packet(int fd,struct mtp_packet **packet) {
  unsigned int length;
  uint32_t clength;
  char *buf = NULL;
  int retval;

  if (packet == NULL) {
    return MTP_PP_ERROR_ARGS;
  }

  retval = readall(fd,(char *)&clength,4);

  if (retval == -1) {
    return MTP_PP_ERROR_READ;
  }
  else {
    int remaining_length;
    
    /* length is currently in bytes 0,1,2,and 3, in network byte order */
    length = ntohl(clength);

    if (length > MTP_PACKET_MAXLEN) {
      return MTP_PP_ERROR_LENGTH;
    }

    /* read the rest of the packet into buf */
    remaining_length = length - 4;
    buf = (char*)malloc(sizeof(char)*(length+1));
    if (buf == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }

    // copy the header bytes to the new buf
    memcpy(buf,&clength,4);
    
    retval = readall(fd,&buf[MTP_PACKET_HEADER_OFFSET_OPCODE],remaining_length);
    if (retval == -1) {
      free(buf);
      return MTP_PP_ERROR_READ;
    }

    retval = mtp_decode_packet(buf,packet);
    free(buf);
    return retval;
  }

}

#define encode_float32(buf, idx, val) { \
  union { \
    float f; \
    unsigned int i; \
  } __z; \
\
  __z.f = val; \
  *((int *)((buf)+(idx))) = htonl(__z.i); \
  idx += 4; \
}

#define decode_float32(buf, idx, val) { \
  union { \
    float f; \
    unsigned int i; \
  } __z; \
\
  __z.i = ntohl(*((int *)((buf)+(idx)))); \
  val = __z.f; \
  idx += 4; \
}

/* returns 0 if little, 1 if big */
int endian() {
  union {
	short x;
	char y[sizeof(short)];
  } u;
  u.x = 0x0102;
  if (u.y[0] == 2) {
	return 0;
  }
  else {
	return 1;
  }
}
  
#define encode_float64(buf, idx, val) { \
  union { \
    double d; \
    char c[sizeof(double)]; \
  } __z; \
  __z.d = val; \
  if (endian() == 1) { \
    memcpy((char *)((buf)+(idx)),__z.c,sizeof(double)); \
    idx += 8; \
  } \
  else { \
    *((int *)((buf)+(idx))) = htonl(*((int *)(__z.c + 4))); \
    idx += 4; \
    *((int *)((buf)+(idx))) = htonl(*((int *)(__z.c))); \
    idx += 4; \
  } \
}

#define decode_float64(buf, idx, val) { \
  union { \
    double d; \
    char c[sizeof(double)]; \
  } __z; \
\
  if (endian() == 1) { \
	memcpy((char *)(__z.c),(char *)((buf)+(idx)),sizeof(double)); \
	val = __z.d; \
    idx += 8; \
  } \
  else { \
    *((int *)(__z.c + 4)) = htonl(*((int *)((buf) + (idx)))); \
    idx += 4; \
    *((int *)(__z.c)) = htonl(*((int *)((buf) + (idx)))); \
    idx += 4; \
	val = __z.d; \
  } \
}

int mtp_encode_packet(char **buf_ptr,struct mtp_packet *packet) {
  char *buf = *buf_ptr;
  int i,j;
  int buf_size;

  if (packet == NULL || buf_ptr == NULL) {
    return MTP_PP_ERROR_ARGS;
  }

  // first we have to caculate how many bytes we need for this
  // specific data -- we could've defined a static size for things,
  // but we're encoding variable-length arrays and strings in addition
  // to ints and floats -- but we didn't -- so we have to skim through.

  if ((buf_size = mtp_calc_size(packet->opcode,
				(void *)packet->data.control)) == -1) {
    return MTP_PP_ERROR;
  }
  
  if (buf == NULL) {
    buf = (char *)malloc(sizeof(char)*buf_size);
    *buf_ptr = buf;
    if (buf == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
  }
  
  if (packet->opcode == MTP_CONTROL_ERROR ||
      packet->opcode == MTP_CONTROL_NOTIFY ||
      packet->opcode == MTP_CONTROL_INIT ||
      packet->opcode == MTP_CONTROL_CLOSE) {
    struct mtp_control *data = packet->data.control;
    int len;
    
    *((int *)buf) = htonl(buf_size);
    buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
    buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
    buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    i = MTP_PACKET_HEADER_LEN;
    // write id field
    *((int *)(buf+i)) = htonl(data->id);
    i += 4;
    // write code field
    *((int *)(buf+i)) = htonl(data->code);
    i += 4;
    // write msg field
    strcpy(&buf[i],data->msg);
    len = strlen(&buf[i]) + 1;
    i += (len + 3) & ~3;
  }
  else if (packet->opcode == MTP_CONFIG_RMC) {
    struct mtp_config_rmc *data = packet->data.config_rmc;
    
    *((int *)buf) = htonl(buf_size);
    buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
    buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
    buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    // now write the specific data:
    i = MTP_PACKET_HEADER_LEN;
    *((int *)(buf+i)) = htonl(data->num_robots);
    i += 4;
    // now write the list
    for (j = 0; j < data->num_robots; ++j) {
      int len;
      
      // write the id field
      *((int *)(buf+i)) = htonl(data->robots[j].id);
      i += 4;
      // write the hostname field
      strcpy(&buf[i],data->robots[j].hostname);
      len = strlen(&buf[i]) + 1;
      i += (len + 3) & ~3;
    }
    // now write the global_bound data
    encode_float32(buf, i, data->box.horizontal);
    encode_float32(buf, i, data->box.vertical);
  }
  else if (packet->opcode == MTP_CONFIG_VMC) {
    struct mtp_config_rmc *data = packet->data.config_rmc;
    
    *((int *)buf) = htonl(buf_size);
    buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
    buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
    buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    // now write the specific data:
    i = MTP_PACKET_HEADER_LEN;
    *((int *)(buf+i)) = htonl(data->num_robots);
    i += 4;
    // now write the list
    for (j = 0; j < data->num_robots; ++j) {
      int len;
      
      // write the id field
      *((int *)(buf+i)) = htonl(data->robots[j].id);
      i += 4;
      // write the hostname field
      strcpy(&buf[i],data->robots[j].hostname);
      len = strlen(&buf[i]) + 1;
      i += (len + 3) & ~3;
    }
    
  }
  else if (packet->opcode == MTP_REQUEST_POSITION) {
    struct mtp_request_position *data = packet->data.request_position;
    
    *((int *)buf) = htonl(buf_size);
    buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
    buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
    buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    // now write the specific data:
    i = MTP_PACKET_HEADER_LEN;
    *((int *)(buf+i)) = htonl(data->robot_id);
    i += 4;

  }
  else if (packet->opcode == MTP_REQUEST_ID) {
    struct mtp_request_id *data = packet->data.request_id;
    
    *((int *)buf) = htonl(buf_size);
    buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
    buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
    buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    // now write the specific data:
    i = MTP_PACKET_HEADER_LEN;
	*((int *)(buf+i)) = htonl(data->request_id);
    i += 4;
    encode_float32(buf, i, data->position.x);
    encode_float32(buf, i, data->position.y);
    encode_float32(buf, i, data->position.theta);
	encode_float64(buf, i, data->position.timestamp);

  }
  else if (packet->opcode == MTP_UPDATE_POSITION) {
    struct mtp_update_position *data = packet->data.update_position;
    
    *((int *)buf) = htonl(buf_size);
    buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
    buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
    buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    // now write the specific data:
    i = MTP_PACKET_HEADER_LEN;
    *((int *)(buf+i)) = htonl(data->robot_id);
    i += 4;
    encode_float32(buf, i, data->position.x);
    encode_float32(buf, i, data->position.y);
    encode_float32(buf, i, data->position.theta);
	encode_float64(buf, i, data->position.timestamp);

    *((int *)(buf+i)) = htonl(data->status);
    i += 4;

  }
  else if (packet->opcode == MTP_UPDATE_ID) {
    struct mtp_update_id *data = packet->data.update_id;
    
    *((int *)buf) = htonl(buf_size);
    buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
    buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
    buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    // now write the specific data:
    i = MTP_PACKET_HEADER_LEN;
	*((int *)(buf+i)) = htonl(data->request_id);
    i += 4;
    *((int *)(buf+i)) = htonl(data->robot_id);
    i += 4;

  }
  else if (packet->opcode == MTP_COMMAND_GOTO) {
    struct mtp_command_goto *data = packet->data.command_goto;
    
    *((int *)buf) = htonl(buf_size);
    buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
    buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
    buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    // now write the specific data:
    i = MTP_PACKET_HEADER_LEN;
    *((int *)(buf+i)) = htonl(data->command_id);
    i += 4;
    *((int *)(buf+i)) = htonl(data->robot_id);
    i += 4;
    
    encode_float32(buf, i, data->position.x);
    encode_float32(buf, i, data->position.y);
    encode_float32(buf, i, data->position.theta);
    encode_float64(buf, i, data->position.timestamp);
  }
  else if (packet->opcode == MTP_COMMAND_STOP) {
    struct mtp_command_stop *data = packet->data.command_stop;
    
    *((int *)buf) = htonl(buf_size);
    buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
    buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
    buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    // now write the specific data:
    i = MTP_PACKET_HEADER_LEN;
    *((int *)(buf+i)) = htonl(data->command_id);
    i += 4;
    *((int *)(buf+i)) = htonl(data->robot_id);
    i += 4;

  }
  else {
    return MTP_PP_ERROR;
  }
  
  // if we get here, the packet encoded successfully into buf:
  return buf_size;
}

int mtp_decode_packet(char *buf,struct mtp_packet **packet_ptr) {
  struct mtp_packet *packet;
  int i,j;
  int len;
  
  if (buf == NULL) {
    return MTP_PP_ERROR_ARGS;
  }

  packet = (struct mtp_packet *)malloc(sizeof(struct mtp_packet));
  *packet_ptr = packet;
  if (packet == NULL) {
    return MTP_PP_ERROR_MALLOC;
  }

  packet->length = ntohl(*((int*)(buf+MTP_PACKET_HEADER_OFFSET_LENGTH)));
  packet->opcode = (char)(buf[MTP_PACKET_HEADER_OFFSET_OPCODE]);
  packet->version = (char)(buf[MTP_PACKET_HEADER_OFFSET_VERSION]);
  packet->role = (char)(buf[MTP_PACKET_HEADER_OFFSET_ROLE]);

  i = MTP_PACKET_HEADER_LEN;
  // process data field
  if (packet->opcode == MTP_CONTROL_ERROR ||
      packet->opcode == MTP_CONTROL_NOTIFY ||
      packet->opcode == MTP_CONTROL_INIT ||
      packet->opcode == MTP_CONTROL_CLOSE) {
    struct mtp_control *data = (struct mtp_control *)malloc(sizeof(struct mtp_control));
    if (data == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
    packet->data.control = data;
    data->id = ntohl(*((int *)(buf+i)));
    i += 4;
    data->code = ntohl(*((int *)(buf+i)));
    i += 4;
    len = strlen(buf+i);
    data->msg = (char *)malloc(sizeof(char)*(len + 1));
    if (data->msg == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }

    memcpy(data->msg,buf+i,len+1);
  }
  else if (packet->opcode == MTP_CONFIG_RMC) {
    struct mtp_config_rmc *data = (struct mtp_config_rmc *)malloc(sizeof(struct mtp_config_rmc));
    if (data == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
    packet->data.config_rmc = data;
    data->num_robots = ntohl(*((int *)(buf+i)));
    i += 4;
    data->robots = (struct robot_config *)malloc(sizeof(struct robot_config)*(data->num_robots));
    if (data->robots == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
    for (j = 0; j < data->num_robots; ++j) {
      int len;
      
      data->robots[j].id = ntohl(*((int *)(buf+i)));
      i += 4;
      len = strlen(buf+i) + 1;
      data->robots[j].hostname = (char *)malloc(sizeof(char)*len);
      memcpy(data->robots[j].hostname,buf+i,len);
      i += (len + 3) & ~3;
    }
    // write the global_coord box
    decode_float32(buf, i, data->box.horizontal);
    decode_float32(buf, i, data->box.vertical);
  }
  else if (packet->opcode == MTP_CONFIG_VMC) {
    struct mtp_config_vmc *data = (struct mtp_config_vmc *)malloc(sizeof(struct mtp_config_vmc));
    if (data == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
    packet->data.config_vmc = data;
    data->num_robots = ntohl(*((int *)(buf+i)));
    i += 4;
    data->robots = (struct robot_config *)malloc(sizeof(struct robot_config)*(data->num_robots));
    if (data->robots == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
    for (j = 0; j < data->num_robots; ++j) {
      int len;
      
      data->robots[j].id = ntohl(*((int *)(buf+i)));
      i += 4;
      len = strlen(buf+i) + 1;
      data->robots[j].hostname = (char *)malloc(sizeof(char)*len);
      memcpy(data->robots[j].hostname,buf+i,len);
      i += (len + 3) & ~3;
    }

  }
  else if (packet->opcode == MTP_REQUEST_POSITION) {
    struct mtp_request_position *data = (struct mtp_request_position *)malloc(sizeof(struct mtp_request_position));
    if (data == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
    packet->data.request_position = data;
    data->robot_id = ntohl(*((int *)(buf+i)));
    i += 4;

  }
  else if (packet->opcode == MTP_REQUEST_ID) {
    struct mtp_request_id *data = (struct mtp_request_id *)malloc(sizeof(struct mtp_request_id));
    if (data == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
    packet->data.request_id = data;

    data->request_id = ntohl(*((int *)(buf+i)));
    i += 4;
    decode_float32(buf, i, data->position.x);
    decode_float32(buf, i, data->position.y);
    decode_float32(buf, i, data->position.theta);
    decode_float64(buf, i, data->position.timestamp);

  }
  else if (packet->opcode == MTP_UPDATE_POSITION) {
    struct mtp_update_position *data = (struct mtp_update_position *)malloc(sizeof(struct mtp_update_position));
    if (data == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
    packet->data.update_position = data;
    data->robot_id = ntohl(*((int *)(buf+i)));
    i += 4;
    decode_float32(buf, i, data->position.x);
    decode_float32(buf, i, data->position.y);
    decode_float32(buf, i, data->position.theta);
	decode_float64(buf, i, data->position.timestamp);
    data->status = ntohl(*((int *)(buf+i)));
    i += 4;

  }
  else if (packet->opcode == MTP_UPDATE_ID) {
    struct mtp_update_id *data = (struct mtp_update_id *)malloc(sizeof(struct mtp_update_id));
    if (data == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
    packet->data.update_id = data;

	data->request_id = ntohl(*((int *)(buf+i)));
    i += 4;
    data->robot_id = ntohl(*((int *)(buf+i)));
    i += 4;

  }
  else if (packet->opcode == MTP_COMMAND_GOTO) {
    struct mtp_command_goto *data = (struct mtp_command_goto *)malloc(sizeof(struct mtp_command_goto));
    if (data == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
    packet->data.command_goto = data;
    data->command_id = ntohl(*((int *)(buf+i)));
    i += 4;
    data->robot_id = ntohl(*((int *)(buf+i)));
    i += 4;
    decode_float32(buf, i, data->position.x);
    decode_float32(buf, i, data->position.y);
    decode_float32(buf, i, data->position.theta);
	decode_float64(buf, i, data->position.timestamp);

  }
  else if (packet->opcode == MTP_COMMAND_STOP) {
    struct mtp_command_stop *data = (struct mtp_command_stop *)malloc(sizeof(struct mtp_command_stop));
    if (data == NULL) {
      return MTP_PP_ERROR_MALLOC;
    }
    packet->data.command_stop = data;
    data->command_id = ntohl(*((int *)(buf+i)));
    i += 4;
    data->robot_id = ntohl(*((int *)(buf+i)));
    i += 4;

  }
  else {
    return MTP_PP_ERROR;
  }

  return MTP_PP_SUCCESS;
  
}

int mtp_send_packet(int fd,struct mtp_packet *packet) {
  char *buf = NULL;
  int retval;

  if (packet == NULL) {
    return MTP_PP_ERROR;
  }

  retval = mtp_encode_packet(&buf,packet);
  if (retval < MTP_PP_SUCCESS) {
    return retval;
  }

  // now we can write the buffer out the socket.
  retval = write(fd,buf,retval);

  free(buf);
  buf = NULL;
  
  if (retval != -1) {
    return MTP_PP_SUCCESS;
  }
  else {
    return MTP_PP_ERROR_WRITE;
  }

}

struct mtp_packet *mtp_make_packet( unsigned char opcode,
				    unsigned char role,
				    /* data's type is only known by opcode */
				    void *data
				    ) {
  struct mtp_packet *retval = NULL;

  retval = (struct mtp_packet *)malloc(sizeof(struct mtp_packet));
  if (retval == NULL) {
    return NULL;
  }

  retval->opcode = opcode;
  retval->role = role;
  retval->length = 0;
  retval->version = MTP_VERSION;

  if (opcode == MTP_CONTROL_ERROR ||
      opcode == MTP_CONTROL_NOTIFY ||
      opcode == MTP_CONTROL_INIT ||
      opcode == MTP_CONTROL_CLOSE) {
    retval->data.control = (struct mtp_control *)data;
    retval->length = mtp_calc_size(opcode,data);
  }
  else if (opcode == MTP_CONFIG_RMC) {
    retval->data.config_rmc = (struct mtp_config_rmc *)data;
    retval->length = mtp_calc_size(opcode,data);
  }
  else if (opcode == MTP_CONFIG_VMC) {
    retval->data.config_vmc = (struct mtp_config_vmc *)data;
    retval->length = mtp_calc_size(opcode,data);
  }
  else if (opcode == MTP_REQUEST_POSITION) {
    retval->data.request_position = (struct mtp_request_position *)data;
    retval->length = mtp_calc_size(opcode,data);
  }
  else if (opcode == MTP_REQUEST_ID) {
    retval->data.request_id = (struct mtp_request_id *)data;
    retval->length = mtp_calc_size(opcode,data);
  }
  else if (opcode == MTP_UPDATE_POSITION) {
    retval->data.update_position = (struct mtp_update_position *)data;
    retval->length = mtp_calc_size(opcode,data);
  }
  else if (opcode == MTP_UPDATE_ID) {
    retval->data.update_id = (struct mtp_update_id *)data;
    retval->length = mtp_calc_size(opcode,data);
  }
  else if (opcode == MTP_COMMAND_GOTO) {
    retval->data.command_goto = (struct mtp_command_goto *)data;
    retval->length = mtp_calc_size(opcode,data);
  }
  else if (opcode == MTP_COMMAND_STOP) {
    retval->data.command_stop = (struct mtp_command_stop *)data;
    retval->length = mtp_calc_size(opcode,data);
  }
  else {
    return NULL;
  }

  return retval;
}

void mtp_free_packet(struct mtp_packet *mp) {
  if (mp != NULL) {
    int lpc;
    
    switch (mp->opcode) {
    case MTP_CONTROL_ERROR:
    case MTP_CONTROL_NOTIFY:
    case MTP_CONTROL_INIT:
    case MTP_CONTROL_CLOSE:
      free(mp->data.control->msg);
      break;

    case MTP_CONFIG_VMC:
      for (lpc = 0; lpc < mp->data.config_vmc->num_robots; lpc++) {
	free(mp->data.config_vmc->robots[lpc].hostname);
      }
      free(mp->data.config_vmc->robots);
      break;
      
    case MTP_CONFIG_RMC:
      for (lpc = 0; lpc < mp->data.config_rmc->num_robots; lpc++) {
	free(mp->data.config_rmc->robots[lpc].hostname);
      }
      free(mp->data.config_rmc->robots);
      break;
    }
    
    free(mp);
    mp = NULL;
  }
}

int mtp_calc_size(int opcode,void *data) {
  int retval = -1;
  int i;

  if (data == NULL) {
    return retval;
  }

  // standard header of one int and three chars
  retval = MTP_PACKET_HEADER_LEN;

  if (opcode == MTP_CONTROL_ERROR ||
      opcode == MTP_CONTROL_NOTIFY ||
      opcode == MTP_CONTROL_INIT ||
      opcode == MTP_CONTROL_CLOSE) {
    struct mtp_control *c = (struct mtp_control *)data;
    int len;
    
    assert(c->msg);
    retval += 8;
    len = strlen(c->msg) + 1;
    retval += (len + 3) & ~3;
  }
  else if (opcode == MTP_CONFIG_RMC) {
    struct mtp_config_rmc *c = (struct mtp_config_rmc *)data;
    retval += 4;
    for (i = 0; i < c->num_robots; ++i) {
      int len;
      
      assert(c->robots[i].hostname);
      
      retval += 4;
      len = strlen(c->robots[i].hostname) + 1;
      retval += (len + 3) & ~3;
    }
    retval += 8;
  }
  else if (opcode == MTP_CONFIG_VMC) {
    struct mtp_config_vmc *c = (struct mtp_config_vmc *)data;
    retval += 4;
    for (i = 0; i < c->num_robots; ++i) {
      int len;
      
      assert(c->robots[i].hostname);
      
      retval += 4;
      len = strlen(c->robots[i].hostname) + 1;
      retval += (len + 3) & ~3;
    }
  }
  else if (opcode == MTP_REQUEST_POSITION) {
    struct mtp_request_position *c = (struct mtp_request_position *)data;
    retval += 4;
  }
  else if (opcode == MTP_REQUEST_ID) {
    struct mtp_request_id *c = (struct mtp_request_id *)data;
    retval += 4 + 20;
  }
  else if (opcode == MTP_UPDATE_POSITION) {
    struct mtp_update_position *c = (struct mtp_update_position *)data;
    retval += 4 + 20 + 4;
  }
  else if (opcode == MTP_UPDATE_ID) {
    struct mtp_update_id *c = (struct mtp_update_id *)data;
    retval += 4 + 4;
  }
  else if (opcode == MTP_COMMAND_GOTO) {
    struct mtp_command_goto *c = (struct mtp_command_goto *)data;
    retval += 4 + 4 + 20;
  }
  else if (opcode == MTP_COMMAND_STOP) {
    struct mtp_command_stop *c = (struct mtp_command_stop *)data;
    retval += 4 + 4;
  }
  else {
    retval = -1;
  }
  
  return retval;

}

void mtp_print_packet(FILE *file, struct mtp_packet *mp)
{
  int lpc;
  
  fprintf(file,
	  "Packet: length %d; version %d; role %d\n",
	  mp->length,
	  mp->version,
	  mp->role);
  
  switch (mp->opcode) {
  case MTP_CONTROL_ERROR:
  case MTP_CONTROL_NOTIFY:
  case MTP_CONTROL_INIT:
  case MTP_CONTROL_CLOSE:
    switch (mp->opcode) {
    case MTP_CONTROL_ERROR:
      fprintf(file, " opcode:\terror\n");
      break;
    case MTP_CONTROL_NOTIFY:
      fprintf(file, " opcode:\tnotify\n");
      break;
    case MTP_CONTROL_INIT:
      fprintf(file, " opcode:\tinit\n");
      break;
    case MTP_CONTROL_CLOSE:
      fprintf(file, " opcode:\tclose\n");
      break;
    }
    fprintf(file,
	    "  id:\t\t%d\n"
	    "  code:\t\t%d\n"
	    "  msg:\t\t%s\n",
	    mp->data.control->id,
	    mp->data.control->code,
	    mp->data.control->msg);
    break;

  case MTP_CONFIG_VMC:
    fprintf(file,
	    " opcode:\tconfig-vmc\n"
	    "  num:\t\t%d\n",
	    mp->data.config_vmc->num_robots);
    for (lpc = 0;
	 lpc < mp->data.config_vmc->num_robots;
	 lpc++) {
      fprintf(file,
	      "  robot[%d]:\t%d, %s\n",
	      lpc,
	      mp->data.config_vmc->robots[lpc].id,
	      mp->data.config_vmc->robots[lpc].hostname);
    }
    break;
    
  case MTP_CONFIG_RMC:
    fprintf(file,
	    " opcode:\tconfig-rmc\n"
	    "  num:\t\t%d\n"
	    "  horiz:\t%f\n"
	    "  vert:\t\t%f\n",
	    mp->data.config_rmc->num_robots,
	    mp->data.config_rmc->box.horizontal,
	    mp->data.config_rmc->box.vertical);
    for (lpc = 0;
	 lpc < mp->data.config_rmc->num_robots;
	 lpc++) {
      fprintf(file,
	      "  robot[%d]:\t%d, %s\n",
	      lpc,
	      mp->data.config_rmc->robots[lpc].id,
	      mp->data.config_rmc->robots[lpc].hostname);
    }
    break;
    
  case MTP_REQUEST_POSITION:
    fprintf(file,
	    " opcode:\trequest-position\n"
	    "  id:\t\t%d\n",
	    mp->data.request_position->robot_id);
    break;
    
  case MTP_REQUEST_ID:
    fprintf(file,
	    " opcode:\trequest-id\n"
		"  request_id:\t\t%d\n"
	    "  x:\t\t%f\n"
	    "  y:\t\t%f\n"
	    "  theta:\t%f\n"
	    "  timestamp:\t\t%f\n",
		mp->data.request_id->request_id,
	    mp->data.request_id->position.x,
	    mp->data.request_id->position.y,
	    mp->data.request_id->position.theta,
	    mp->data.request_id->position.timestamp);
    break;
    
  case MTP_UPDATE_POSITION:
    fprintf(file,
	    " opcode:\tupdate-position\n"
	    "  id:\t\t%d\n"
	    "  x:\t\t%f\n"
	    "  y:\t\t%f\n"
	    "  theta:\t%f\n"
	    "  status:\t%d\n"
	    "  timestamp:\t%f\n",
	    mp->data.update_position->robot_id,
	    mp->data.update_position->position.x,
	    mp->data.update_position->position.y,
	    mp->data.update_position->position.theta,
	    mp->data.update_position->status,
	    mp->data.update_position->position.timestamp);
    break;
    
  case MTP_UPDATE_ID:
    fprintf(file,
	    " opcode:\tupdate-id\n"
	    "  request_id:\t%d\n"
	    "  id:\t%d\n",
	    mp->data.update_id->request_id,
	    mp->data.update_id->robot_id);
    break;
    
  case MTP_COMMAND_GOTO:
    fprintf(file,
	    " opcode:\tcommand-goto\n"
	    "  commid:\t%d\n"
	    "  id:\t\t%d\n"
	    "  x:\t\t%f\n"
	    "  y:\t\t%f\n"
	    "  theta:\t%f\n"
        "  timestamp:\t%f\n",
	    mp->data.command_goto->command_id,
	    mp->data.command_goto->robot_id,
	    mp->data.command_goto->position.x,
	    mp->data.command_goto->position.y,
	    mp->data.command_goto->position.theta,
	    mp->data.command_goto->position.timestamp);
    break;
    
  case MTP_COMMAND_STOP:
    fprintf(file,
	    " opcode:\tupdate-id\n"
	    "  commid:\t%d\n"
	    "  id:\t%d\n",
	    mp->data.command_stop->command_id,
	    mp->data.command_stop->robot_id);
    break;

  default:
    assert(0);
    break;
  }
}
