#include "mtp.h"

#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>


int mtp_receive_packet(int fd,struct mtp_packet **packet) {
  unsigned int length;
  char clength[4];
  char *buf = NULL;
  int i;
  int retval;

  if (packet == NULL) {
	return MTP_PP_ERROR_ARGS;
  }

  retval = read(fd,(void*)(&clength),4);

  if (retval == -1) {
	return MTP_PP_ERROR_READ;
  }
  else {
	/* length is currently in bytes 0,1,2,and 3, in network byte order */
	length = ntohl((uint32_t)(clength));

	if (length > MTP_PACKET_MAXLEN) {
	  return MTP_PP_ERROR_LENGTH;
	}

	/* read the rest of the packet into buf */
	int remaining_length = length - 4;
	buf = (char*)malloc(sizeof(char)*(length+1));
	if (buf == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}

	// copy the header bytes to the new buf
	for (i = 0; i < 4; ++i) {
	  buf[i] = clength[i];
	}
	
	retval = read(fd,(void*)(buf+sizeof(char)*MTP_PACKET_HEADER_LEN),remaining_length);
	if (retval == -1) {
	  free(buf);
	  return MTP_PP_ERROR_READ;
	}

	retval = mtp_decode_packet(buf,packet);
	free(buf);
	return retval;
  }

}  

int mtp_encode_packet(char **buf_ptr,struct mtp_packet *packet) {
  char *buf;
  int i,j,k;
  int buf_size;

  if (packet == NULL || buf == NULL) {
	return MTP_PP_ERROR_ARGS;
  }

  // first we have to caculate how many bytes we need for this
  // specific data -- we could've defined a static size for things,
  // but we're encoding variable-length arrays and strings in addition
  // to ints and floats -- but we didn't -- so we have to skim through.


  if (packet->opcode == MTP_CONTROL_ERROR ||
	  packet->opcode == MTP_CONTROL_NOTIFY ||
	  packet->opcode == MTP_CONTROL_INIT ||
	  packet->opcode == MTP_CONTROL_CLOSE) {
	struct mtp_control *data = packet->data.control;
	buf_size = mtp_calc_size(packet->opcode,(void *)data);

	buf = (char *)malloc(sizeof(char)*buf_size);
	*buf_ptr = buf;
	if (buf == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}

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
	// write msg field, and null-term it
	for (j = 0; j < strlen(data->msg); ++j) {
	  buf[i] = data->msg[j];
	  ++i;
	}
	buf[i] = '\0';

  }
  else if (packet->opcode == MTP_CONFIG_RMC) {
	struct mtp_config_rmc *data = packet->data.config_rmc;
	buf_size = mtp_calc_size(packet->opcode,(void *)data);
	
    buf = (char *)malloc(sizeof(char)*buf_size);
	*buf_ptr = buf;
	if (buf == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}
	
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
	  // write the id field
	  *((int *)(buf+i)) = htonl(data->robots[j].id);
	  i += 4;
	  // write the hostname field
	  int len = strlen(data->robots[j].hostname);
	  for (k = 0; k < len; ++k) {
		buf[i] = data->robots[j].hostname[k];
		++i;
	  }
	  buf[i] = '\0';
	  ++i;
	}
    // now write the global_bound data
	*((int *)(buf+i)) = htonl(data->box.horizontal);
	i += 4;
	*((int *)(buf+i)) = htonl(data->box.vertical);

  }
  else if (packet->opcode == MTP_CONFIG_VMC) {
	struct mtp_config_rmc *data = packet->data.config_rmc;
	buf_size = mtp_calc_size(packet->opcode,(void *)data);

	buf = (char *)malloc(sizeof(char)*buf_size);
	*buf_ptr = buf;
	if (buf == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}
	
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
	  // write the id field
	  *((int *)(buf+i)) = htonl(data->robots[j].id);
	  i += 4;
	  // write the hostname field
	  int len = strlen(data->robots[j].hostname);
	  for (k = 0; k < len; ++k) {
		buf[i] = data->robots[j].hostname[k];
		++i;
	  }
	  buf[i] = '\0';
	  ++i;
	}
	
  }
  else if (packet->opcode == MTP_REQUEST_POSITION) {
	struct mtp_request_position *data = packet->data.request_position;
	buf_size = mtp_calc_size(packet->opcode,(void *)data);

	buf = (char *)malloc(sizeof(char)*buf_size);
	*buf_ptr = buf;
	if (buf == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}
	
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
	buf_size = mtp_calc_size(packet->opcode,(void *)data);

	buf = (char *)malloc(sizeof(char)*buf_size);
	*buf_ptr = buf;
	if (buf == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}
	
	*((int *)buf) = htonl(buf_size);
	buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
	buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
	buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    // now write the specific data:
	i = MTP_PACKET_HEADER_LEN;
	*((int *)(buf+i)) = htonl(data->position.x);
	i += 4;
	*((int *)(buf+i)) = htonl(data->position.y);
	i += 4;
	*((int *)(buf+i)) = htonl(data->position.theta);
	i += 4;
	*((int *)(buf+i)) = htonl(data->timestamp);
	i += 4;

  }
  else if (packet->opcode == MTP_UPDATE_POSITION) {
	struct mtp_update_position *data = packet->data.update_position;
	buf_size = mtp_calc_size(packet->opcode,(void *)data);

	buf = (char *)malloc(sizeof(char)*buf_size);
	*buf_ptr = buf;
	if (buf == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}
	
	*((int *)buf) = htonl(buf_size);
	buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
	buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
	buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    // now write the specific data:
	i = MTP_PACKET_HEADER_LEN;
	*((int *)(buf+i)) = htonl(data->robot_id);
	i += 4;
	*((int *)(buf+i)) = htonl(data->position.x);
	i += 4;
	*((int *)(buf+i)) = htonl(data->position.y);
	i += 4;
	*((int *)(buf+i)) = htonl(data->position.theta);
	i += 4;
	*((int *)(buf+i)) = htonl(data->status);
	i += 4;
	*((int *)(buf+i)) = htonl(data->timestamp);
	i += 4;

  }
  else if (packet->opcode == MTP_UPDATE_ID) {
	struct mtp_update_id *data = packet->data.update_id;
	buf_size = mtp_calc_size(packet->opcode,(void *)data);

	buf = (char *)malloc(sizeof(char)*buf_size);
	*buf_ptr = buf;
	if (buf == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}
	
	*((int *)buf) = htonl(buf_size);
	buf[MTP_PACKET_HEADER_OFFSET_OPCODE] = (char)(packet->opcode);
	buf[MTP_PACKET_HEADER_OFFSET_VERSION] = (char)(packet->version);
	buf[MTP_PACKET_HEADER_OFFSET_ROLE] = (char)(packet->role);

    // now write the specific data:
	i = MTP_PACKET_HEADER_LEN;
	*((int *)(buf+i)) = htonl(data->robot_id);
	i += 4;

  }
  else if (packet->opcode == MTP_COMMAND_GOTO) {
	struct mtp_command_goto *data = packet->data.command_goto;
	buf_size = mtp_calc_size(packet->opcode,(void *)data);

	buf = (char *)malloc(sizeof(char)*buf_size);
	*buf_ptr = buf;
	if (buf == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}
	
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
	*((int *)(buf+i)) = htonl(data->position.x);
	i += 4;
	*((int *)(buf+i)) = htonl(data->position.y);
	i += 4;
	*((int *)(buf+i)) = htonl(data->position.theta);
	i += 4;

  }
  else if (packet->opcode == MTP_COMMAND_STOP) {
	struct mtp_command_stop *data = packet->data.command_stop;
	buf_size = mtp_calc_size(packet->opcode,(void *)data);

	buf = (char *)malloc(sizeof(char)*buf_size);
	*buf_ptr = buf;
	if (buf == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}
	
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
  int i,j,k;
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
	  data->robots[j].id = ntohl(*((int *)(buf+i)));
	  i += 4;
	  int len = strlen(buf+i) + 1;
	  data->robots[j].hostname = (char *)malloc(sizeof(char)*len);
	  memcpy(data->robots[j].hostname,buf+i,len);
	  i += len;
	}
	// write the global_coord box
	data->box.horizontal = ntohl(*((int *)(buf+i)));
	i += 4;
	data->box.vertical = ntohl(*((int *)(buf+i)));
	i += 4;

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
	  data->robots[j].id = ntohl(*((int *)(buf+i)));
	  i += 4;
	  int len = strlen(buf+i) + 1;
	  data->robots[j].hostname = (char *)malloc(sizeof(char)*len);
	  memcpy(data->robots[j].hostname,buf+i,len);
	  i += len;
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
	data->position.x = ntohl(*((int *)(buf+i)));
	i += 4;
	data->position.y = ntohl(*((int *)(buf+i)));
	i += 4;
	data->position.theta = ntohl(*((int *)(buf+i)));
	i += 4;
	data->timestamp = ntohl(*((int *)(buf+i)));
	i += 4;

  }
  else if (packet->opcode == MTP_UPDATE_POSITION) {
	struct mtp_update_position *data = (struct mtp_update_position *)malloc(sizeof(struct mtp_update_position));
	if (data == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}
	packet->data.update_position = data;
	data->robot_id = ntohl(*((int *)(buf+i)));
	i += 4;
	data->position.x = ntohl(*((int *)(buf+i)));
	i += 4;
	data->position.y = ntohl(*((int *)(buf+i)));
	i += 4;
	data->position.theta = ntohl(*((int *)(buf+i)));
	i += 4;
	data->status = ntohl(*((int *)(buf+i)));
	i += 4;
	data->timestamp = ntohl(*((int *)(buf+i)));
	i += 4;

  }
  else if (packet->opcode == MTP_UPDATE_ID) {
	struct mtp_update_id *data = (struct mtp_update_id *)malloc(sizeof(struct mtp_update_id));
	if (data == NULL) {
	  return MTP_PP_ERROR_MALLOC;
	}
	packet->data.update_id = data;
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
	data->position.x = ntohl(*((int *)(buf+i)));
	i += 4;
	data->position.y = ntohl(*((int *)(buf+i)));
	i += 4;
	data->position.theta = ntohl(*((int *)(buf+i)));
	i += 4;

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
  char **buf;
  int retval;

  if (packet == NULL) {
	return MTP_PP_ERROR;
  }

  retval = mtp_encode_packet(buf,packet);
  if (retval < MTP_PP_SUCCESS) {
	return retval;
  }

  // now we can write the buffer out the socket.
  retval = write(fd,*buf,retval);
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


int mtp_calc_size(int opcode,void *data) {
  int retval = -1;
  int i,j;

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
	retval += 8;
	retval += strlen(c->msg) + 1;
  }
  else if (opcode == MTP_CONFIG_RMC) {
	struct mtp_config_rmc *c = (struct mtp_config_rmc *)data;
	retval += 4;
	for (i = 0; i < c->num_robots; ++i) {
	  retval += 4;
	  retval += strlen(c->robots[i].hostname) + 1;
	}
	retval += 8;
  }
  else if (opcode == MTP_CONFIG_VMC) {
	struct mtp_config_vmc *c = (struct mtp_config_vmc *)data;
	retval += 4;
	for (i = 0; i < c->num_robots; ++i) {
	  retval += 4;
	  retval += strlen(c->robots[i].hostname) + 1;
	}
  }
  else if (opcode == MTP_REQUEST_POSITION) {
	struct mtp_request_position *c = (struct mtp_request_position *)data;
	retval += 4;
  }
  else if (opcode == MTP_REQUEST_ID) {
	struct mtp_request_id *c = (struct mtp_request_id *)data;
	retval += 12 + 4;
  }
  else if (opcode == MTP_UPDATE_POSITION) {
	struct mtp_update_position *c = (struct mtp_update_position *)data;
	retval += 4 + 12 + 4 + 4;
  }
  else if (opcode == MTP_UPDATE_ID) {
	struct mtp_update_id *c = (struct mtp_update_id *)data;
	retval += 4;
  }
  else if (opcode == MTP_COMMAND_GOTO) {
	struct mtp_command_goto *c = (struct mtp_command_goto *)data;
	retval += 4 + 4 + 12;
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
