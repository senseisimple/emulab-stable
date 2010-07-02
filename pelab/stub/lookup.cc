/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// lookup.cc

// Provide lookups of the sender and receiver indices based on a
// number of different keys.

// TODO: Add phasing out of old connections

#include "stub.h"

#include <set>
#include <map>
#include <utility>

struct Stub
{
  Stub(unsigned long newIp = 0, unsigned short newPort = 0)
    : ip(newIp)
    , stub_port(newPort)
  {
  }
  bool operator<(Stub const & right) const
  {
    return ip < right.ip || (ip == right.ip && stub_port < right.stub_port);
  }
  unsigned long ip;
  unsigned short stub_port;
};

struct Address
{
  Address(unsigned long newIp = 0, unsigned short newSource = 0,
          unsigned short newDest = 0)
    : ip(newIp)
    , source_port(newSource)
    , dest_port(newDest)
  {
  }
  bool operator<(Address const & right) const
  {
    return ip < right.ip || (ip == right.ip &&
                             (source_port < right.source_port ||
                               (source_port == right.source_port
                                && dest_port < right.dest_port)));
  }
  unsigned long ip;
  unsigned short source_port;
  unsigned short dest_port;
};


void init_connection(connection * conn)
{
  if (conn != NULL)
  {
    conn->valid = 0;
    conn->sockfd = -1;
    conn->ip = 0;
    conn->stub_port = 0;
    conn->source_port = 0;
    conn->dest_port = 0;
    conn->last_usetime = time(NULL);
    conn->pending.is_pending = 0;
  }
}

//-----------------------------------------------------------------
// Each of these functions uses and manipulates the sender database.

static std::set<int> empty_senders;
static std::map<Stub, int> stub_senders;

void add_empty_sender(int index)
{
    empty_senders.insert(index);
}

void for_each_readable_sender(handle_index function, fd_set * read_fds_copy)
{
  std::map<Stub, int>::iterator pos = stub_senders.begin();
  std::map<Stub, int>::iterator limit = stub_senders.end();
  for (; pos != limit; ++pos)
  {
    int index = pos->second;
    if (FD_ISSET(snddb[index].sockfd, read_fds_copy))
    {
      function(index);
    }
  }
}

int replace_sender_by_stub_port(unsigned long ip, unsigned short stub_port,
                                int sockfd, fd_set * read_fds)
{
  int result = FAILED_LOOKUP;
  time_t now = time(NULL);
  Stub key(ip, stub_port);
  std::map<Stub, int>::iterator pos = stub_senders.find(key);
  if (pos != stub_senders.end())
  {
    // There is already a connection. Get rid of the old connection.
    int index = pos->second;
    snddb[index].last_usetime = now;
    snddb[index].pending.is_pending = 0;
    if (snddb[index].sockfd != -1)
    {
        FD_CLR(snddb[index].sockfd, read_fds);
        close(snddb[index].sockfd);
    }
    if (sockfd > maxfd)
    {
      maxfd = sockfd;
    }
    snddb[index].sockfd = sockfd;
    FD_SET(snddb[index].sockfd, read_fds);
    result = index;
  }
  else if (! empty_senders.empty())
  {
    // There aren't any connections. Choose one of the empty slots.
    int index = * empty_senders.begin();
    empty_senders.erase(index);
    // This will not overlap with anything because we checked it above.
    // Therefore we don't have to check the return code now.
    stub_senders.insert(std::make_pair(key, index));
    snddb[index].valid = 1;
    snddb[index].ip = ip;
    snddb[index].stub_port = stub_port;
    snddb[index].source_port = 0;
    snddb[index].dest_port = 0;
    FD_SET(sockfd, read_fds);
    if (sockfd > maxfd)
    {
      maxfd = sockfd;
    }
    snddb[index].sockfd = sockfd;
    snddb[index].last_usetime = now;
    snddb[index].pending.is_pending = 0;
    result = index;
  }
  return result;
}

void remove_sender_index(int index, fd_set * read_fds)
{
  bool was_valid = empty_senders.insert(index).second;
  if (was_valid)
  {
    Stub key(snddb[index].ip, snddb[index].stub_port);
    stub_senders.erase(key);
    snddb[index].valid = 0;
    FD_CLR(snddb[index].sockfd, read_fds);
    if (snddb[index].sockfd != -1)
    {
      close(snddb[index].sockfd);
      snddb[index].sockfd = -1;
    }
  }
}

//-----------------------------------------------------------------
// Each of these functions manipulates the receiver database.

static std::set<int> empty_receivers;
static std::set<int> pending_receivers;
static std::map<Stub, int> stub_receivers;
static std::map<Address, int> address_receivers;

void add_empty_receiver(int index)
{
    empty_receivers.insert(index);
}

void for_each_pending(handle_index function, fd_set * write_fds_copy)
{
  std::set<int>::iterator pos = pending_receivers.begin();
  std::set<int>::iterator limit = pending_receivers.end();
  for (; pos != limit; ++pos)
  {
    int index = *pos;
    if (FD_ISSET(rcvdb[index].sockfd, write_fds_copy))
    {
      function(index);
    }
  }
}

int for_each_to_monitor(send_to_monitor function, int monitor)
{
  int result = 1;
  std::map<Stub, int>::iterator pos = stub_receivers.begin();
  std::map<Stub, int>::iterator limit = stub_receivers.end();
  for (; pos != limit && result == 1; ++pos)
  {
    int index = pos->second;
    result = function(monitor, index);
  }
  return result;
}

int find_by_address(unsigned long ip, unsigned short source_port,
                    unsigned short dest_port)
{
  int result = FAILED_LOOKUP;
  Address key(ip, source_port, dest_port);
  std::map<Address, int>::iterator pos = address_receivers.find(key);
  if (pos != address_receivers.end())
  {
    result = pos->second;
  }
  return result;
}

int insert_by_address(unsigned long ip, unsigned short source_port,
                      unsigned short dest_port)
{
  int result = find_by_address(ip, source_port, dest_port);
  if (result == FAILED_LOOKUP && ! empty_receivers.empty())
  {
    // Update the lookup structures.
    int index = * empty_receivers.begin();
    empty_receivers.erase(index);
    Address address_key = Address(ip, source_port, dest_port);
    address_receivers.insert(std::make_pair(address_key, index));

    // Connect to the index.
    reset_receive_records(index, ip, source_port, dest_port);
    reconnect_receiver(index);
    result = index;
  }
  return result;
}

int insert_fake(unsigned long ip, unsigned short port)
{
  int result = find_by_address(ip, 0, 0);
  if (result == FAILED_LOOKUP && ! empty_receivers.empty())
  {
    // Update the lookup structures.
    int index = * empty_receivers.begin();
    empty_receivers.erase(index);

    Address address_key = Address(ip, 0, 0);
    address_receivers.insert(std::make_pair(address_key, index));

    Stub stub_key = Stub(ip, port);
    stub_receivers.insert(std::make_pair(stub_key, index));

    // Connect to the index.
    reset_receive_records(index, ip, 0, 0);
    rcvdb[index].sockfd = -1;
    // Set the new port and sockfd
    rcvdb[index].stub_port = port;

    // Reset sniffing records
    sniff_rcvdb[index].start = 0;
    sniff_rcvdb[index].end = 0;
    throughput[index].isValid = 0;
    last_through[index] = 0;
    buffer_full[index] = 0;
    max_delay[index] = 0;
    last_max_delay[index] = 0;
    loss_records[index].loss_counter=0;
    loss_records[index].total_counter=0;
    last_loss_rates[index]=0;
    delays[index]=0;
    last_delays[index]=0;
    delay_count[index]=0;
    max_throughput[index] = 0;
    base_rtt[index] = LONG_MAX;
//    delay_records[index].head = NULL;
//    delay_records[index].tail = NULL;
//    delay_records[index].sample_number = 0;
    result = index;
  }
  return result;
}

void reconnect_receiver(int index)
{
  struct sockaddr_in dest_addr;
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(SENDER_PORT);
  dest_addr.sin_addr.s_addr = rcvdb[index].ip;
//  memset(&(dest_addr.sin_zero), '\0', 8);
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
  {
    perror("socket");
    clean_exit(1);
  }
  int connect_error = connect(sockfd, (struct sockaddr *)&dest_addr,
                              sizeof(dest_addr));
  if (connect_error == -1)
  {
    perror("connect");
    clean_exit(1);
  }
//  int send_buf_size = 0;
//  int int_size = sizeof(send_buf_size);
//  getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &send_buf_size,
//           (socklen_t*)&int_size);
//  fprintf(stderr, "Socket buffer size: %d\n", send_buf_size);

  struct sockaddr_in source_addr;
  socklen_t len = sizeof(source_addr);
  int sockname_error = getsockname(sockfd, (struct sockaddr *)&source_addr,
                                   &len);
  if (sockname_error == -1)
  {
    perror("getsockname");
    clean_exit(1);
  }

  unsigned short stub_port = ntohs(source_addr.sin_port);
  if (sockfd > maxfd)
  {
    maxfd = sockfd;
  }

  // If there was a valid connection before, clean it up.
  if (rcvdb[index].sockfd != -1)
  {
    close(rcvdb[index].sockfd);
    Stub erase_key = Stub(rcvdb[index].ip, rcvdb[index].stub_port);
    stub_receivers.erase(erase_key);
  }

  // Add it to the lookup
  Stub insert_key = Stub(rcvdb[index].ip, stub_port);
  stub_receivers.insert(std::make_pair(insert_key, index));

  // Set the new port and sockfd
  rcvdb[index].stub_port = stub_port;
  rcvdb[index].sockfd = sockfd;

  // Reset sniffing records
  sniff_rcvdb[index].start = 0;
  sniff_rcvdb[index].end = 0;
  throughput[index].isValid = 0;
  last_through[index] = 0;
  buffer_full[index] = 0;
  max_delay[index] = 0;
  last_max_delay[index] = 0;
  loss_records[index].loss_counter=0;
  loss_records[index].total_counter=0;
  last_loss_rates[index]=0;
  delays[index]=0;
  last_delays[index]=0;
  delay_count[index]=0;
  max_throughput[index] = 0;
  base_rtt[index] = LONG_MAX;
//  remove_delay_samples(index);
}

void reset_receive_records(int index, unsigned long ip,
                           unsigned short source_port,
                           unsigned short dest_port)
{
    time_t now = time(NULL);
    rcvdb[index].valid = 1;
    rcvdb[index].ip = ip;
    rcvdb[index].source_port = source_port;
    rcvdb[index].dest_port = dest_port;
    rcvdb[index].last_usetime = now;
}

int find_by_stub_port(unsigned long ip, unsigned short stub_port)
{
  int result = FAILED_LOOKUP;
  Stub key(ip, stub_port);
  std::map<Stub, int>::iterator pos = stub_receivers.find(key);
  if (pos != stub_receivers.end())
  {
    result = pos->second;
  }
  return result;
}

void set_pending(int index, fd_set * write_fds)
{
  if (rcvdb[index].valid == 1)
  {
    pending_receivers.insert(index);
    FD_SET(rcvdb[index].sockfd, write_fds);
  }
}

void clear_pending(int index, fd_set * write_fds)
{
  if (rcvdb[index].valid == 1)
  {
    pending_receivers.erase(index);
    FD_CLR(rcvdb[index].sockfd, write_fds);
  }
}

void remove_index(int index, fd_set * write_fds)
{
  bool was_valid = empty_receivers.insert(index).second;
  if (was_valid)
  {
    Stub stub_key(rcvdb[index].ip, rcvdb[index].stub_port);
    stub_receivers.erase(stub_key);
    Address address_key(rcvdb[index].ip, rcvdb[index].source_port,
                        rcvdb[index].dest_port);
    address_receivers.erase(address_key);
    clear_pending(index, write_fds);
    rcvdb[index].valid = 0;
    if (rcvdb[index].sockfd != -1)
    {
      close(rcvdb[index].sockfd);
      rcvdb[index].sockfd = -1;
    }
    rcvdb[index].pending.is_pending = 0;
  }
}

