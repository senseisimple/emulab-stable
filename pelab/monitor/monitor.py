#Usage: tcpdump | python monitor.py <mapping-file> <experiment-name>
#                                   <my-address> [stub-address]
# mapping-file is a file which maps emulated addresses to real addresses.
# experiment-name is the project/experiment name on emulab. For instance
#   'tbres/pelab'.
# my-address is the IP address of the current node.
# stub-address is the IP address of the corresponding planet-lab node. If this
#   is left out, then the mapping-file is used to determine it.

import sys
import os
import time
import socket
import select
import re

CODE_BANDWIDTH = 1
CODE_DELAY = 2
CODE_LOSS = 3
CODE_LIST_DELAY = 4
CODE_MAX_DELAY = 5

PACKET_WRITE = 1
PACKET_SEND_BUFFER = 2
PACKET_RECEIVE_BUFFER = 3

emulated_to_real = {}
real_to_emulated = {}
emulated_to_interface = {}
ip_mapping_filename = ''
this_experiment = ''
this_ip = ''
stub_ip = ''
netmon_output_version = 2

total_size = 0
last_total = -1

def main_loop():
  global total_size, last_total
  # Initialize
  read_args()
  quanta = 0.5# in seconds
  conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
  sys.stdout.write("stub_ip is %s\n" % stub_ip)
  sys.stdout.flush()
  conn.connect((stub_ip, 4200))
  poll = select.poll()
  poll.register(sys.stdin, select.POLLIN)
  poll.register(conn, select.POLLIN)
  done = False

  while not done:
    # Reset
    packet_list = []
    max_time = time.time() + quanta

    # Collect data until the next quanta boundary
    while time.time() < max_time:
      fdlist = poll.poll(max(max_time - time.time(), 0) * 1000)
      for pos in fdlist:
        if pos[0] == sys.stdin.fileno() and not done:
          # A line of data from tcpdump is available.
          try:
#            sys.stdout.write('get_next_packet()\n')
            packet = get_next_packet()
          except EOFError:
#            sys.stdout.write('Done: Got EOF on stdin\n')
            done = 1
          if (packet):
            packet_list = packet_list + [packet]
        elif pos[0] == conn.fileno() and not done:
          sys.stdout.write('receive_characteristic()\n')
          # A record for change in link characteristics is available.
          done = not receive_characteristic(conn)
        elif not done:
          sys.stdout.write('fd: ' + str(pos[0]) + ' conn-fd: ' + str(conn.fileno()) + '\n')
      # Update the stub
    send_destinations(conn, packet_list)
    if total_size != last_total:
      sys.stdout.write('Total Size: ' + str(total_size) + '\n')
      last_total = total_size
#    sys.stdout.write('Loop end\n')

def read_args():
  global ip_mapping_filename, this_experiment, this_ip, stub_ip
  if len(sys.argv) >= 4:
    ip_mapping_filename = sys.argv[1]
    this_experiment = sys.argv[2]
    this_ip = sys.argv[3]
    populate_ip_tables()
    if len(sys.argv) == 5:
      stub_ip = sys.argv[4]
    else:
      stub_ip = emulated_to_real[this_ip]

def populate_ip_tables():
  input = open(ip_mapping_filename, 'r')
  line = input.readline()
  while line != '':
    fields = line.strip().split(' ', 2)
    if len(fields) == 3:
      emulated_to_real[fields[0]] = fields[1]
      real_to_emulated[fields[1]] = fields[0]
      emulated_to_interface[fields[0]] = fields[2]
    line = input.readline()

def get_next_packet():
  global total_size, last_total
  event_code = PACKET_WRITE
  line = sys.stdin.readline()
  if line == "":
      raise EOFError
  #
  # Check for a packet from netmond
  #
  # Could move this elsewhere to avoid re-compiling, but I'd like to keep it
  # with this code for better readability
  if netmon_output_version == 1:
      linexp = re.compile('^(\d+\.\d+) > (\d+\.\d+\.\d+\.\d+)\.(\d+) (\((\d+)\))?')
  elif netmon_output_version == 2:
      linexp = re.compile('^(\d+\.\d+) > (\d+):(\d+\.\d+\.\d+\.\d+):(\d+) (\((\d+)\))?')

  match = linexp.match(line)
  conexp = re.compile('^(New|Closed|SO_RCVBUF|SO_SNDBUF): (\d+):(\d+\.\d+\.\d+\.\d+):(\d+) ((\d+))?')
  cmatch = conexp.match(line)
  if match:
      localport = 0 # We may not get this one
      if netmon_output_version == 1:
          time = float(match.group(1))
          ipaddr = match.group(2)
          remoteport = int(match.group(3))
          size_given = match.group(4) != ''
          size = int(match.group(5))
      elif (netmon_output_version == 2):
          time = float(match.group(1))
          localport = int(match.group(2))
          ipaddr = match.group(3)
          remoteport = int(match.group(4))
          size_given = match.group(5) != ''
          size = int(match.group(6))

      #sys.stdout.write('dest: ' + ipaddr + ' destport: ' + str(remoteport) +
      #        ' srcport: ' + str(localport) + ' size: ' + str(size) + '\n')
      if not size_given:
          size = 0
      total_size = total_size + size
      if emulated_to_real.has_key(ipaddr):
        return (ipaddr, localport, remoteport, time, size, event_code)
      else:
        return None
  elif ((netmon_output_version == 2) and cmatch):
      #
      # Watch for new or closed connections
      #
      event = cmatch.group(1)
      localport = int(cmatch.group(2))
      ipaddr = cmatch.group(3)
      remoteport = int(cmatch.group(4))
      value_given = cmatch.group(5) != ''
      value = int(cmatch.group(6))
      if not value_given:
        value = 0

      sys.stdout.write("Got a connection event: " + event + "\n")
      if event == 'SO_RCVBUF':
        event_code = PACKET_RECEIVE_BUFFER
      elif event == 'SO_SNDBUF':
        event_code = PACKET_SEND_BUFFER
        sys.stdout.write('Packet send buffer was set to: ' + str(value) + '\n')
      else:
        return None
      if emulated_to_real.has_key(ipaddr):
        return (ipaddr, localport, remoteport, 0, value, event_code)
      else:
        return None
  else:
      sys.stdout.write('skipped line in the wrong format: ' + line)
      return None
    
def receive_characteristic(conn):
  buffer = conn.recv(16)
  if len(buffer) == 16:
    dest = real_to_emulated[int_to_ip(load_int(buffer[0:4]))]
    source_port = load_short(buffer[4:6])
    dest_port = load_short(buffer[6:8])
    command = load_int(buffer[8:12])
    value = load_int(buffer[12:16])
#    sys.stdout.write('received: ' + str(dest) + ' ' + str(source_port) + ' '
#                     + str(dest_port) + ' ' + str(command) + ' ' + str(value)
#                     + '\n')
    if command == CODE_BANDWIDTH:
      # value is bandwidth in kbps
      sys.stdout.write('Bandwidth: ' + str(value) + '\n');
      set_bandwidth(value, dest)
    elif command == CODE_DELAY:
      # value is delay in milliseconds
      sys.stdout.write('Delay: ' + str(value) + '\n');
      set_delay(value, dest)
    elif command == CODE_LOSS:
      # value is packet loss in packets per billion
      sys.stdout.write('Loss: ' + str(value) + '\n');
      set_loss(value/1000000000.0, dest)
    elif command == CODE_LIST_DELAY:
      # value is the number of samples
      buffer = conn.recv(value*8)
      # Dummynet isn't quite set up to deal with this yet, so ignore it.
      sys.stdout.write('Ignoring delay list of size: ' + str(value) + '\n')
    elif command == CODE_MAX_DELAY:
      sys.stdout.write('Max Delay: ' + str(value) + '\n');
      set_max_delay(value, dest);
    else:
      sys.stdout.write('Other: ' + str(command) + ', ' + str(value) + '\n');
    return True
  elif len(buffer) == 0:
    return False

def set_bandwidth(kbps, dest):
#  sys.stdout.write('<event> bandwidth=' + str(kbps) + '\n')
  now = time.time()
  sys.stderr.write('BANDWIDTH!purple\n')
  sys.stderr.write('BANDWIDTH!line ' + ('%0.6f' % now) + ' 0 '
                   + ('%0.6f' % now)
                   + ' ' + str(kbps*1000/8) + '\n')
  return set_link(this_ip, dest, 'bandwidth=' + str(kbps))

# Set delay on the link. We are given round trip time.
def set_delay(milliseconds, dest):
  now = time.time()
  sys.stderr.write('RTT!orange\n')
  sys.stderr.write('RTT!line ' + ('%0.6f' % now) + ' 0 ' + ('%0.6f' % now)
	+ ' ' + str(milliseconds) + '\n')
  # Set the delay from here to there to 1/2 rtt.
  error = set_link(this_ip, dest, 'delay=' + str(milliseconds/2))
  if error == 0:
    # If that succeeded, set the delay from there to here.
    return set_link(dest, this_ip, 'delay=' + str(milliseconds/2))
  else:
    return error

def set_loss(probability, dest):
  return set_link(this_ip, dest, 'plr=' + str(probability))

def set_max_delay(delay, dest):
  hertz = 10000.0
  milliseconds = 1000.0
  return set_link(this_ip, dest, 'MAXINQ=' + str(int(
    (hertz/milliseconds)*delay )))

def set_link(source, dest, ending):
  command = ('/usr/testbed/bin/tevc -e ' + this_experiment + ' now '
             + emulated_to_interface[source] + ' modify dest=' + dest + ' '
             + ending)
  sys.stdout.write('event: ' + command + '\n')
  return os.system(command)

def send_destinations(conn, packet_list):
#  sys.stdout.write('<send> total size:' + str(total_size) + ' packet count:' + str(len(packet_list)) + '\n')# + ' -- '
#                   + str(packet_list) + '\n')
  output = save_int(0) + save_int(len(packet_list))
  prev_time = 0.0
  if len(packet_list) > 0:
    prev_time = packet_list[0][3]
  for packet in packet_list:
    ip = ip_to_int(emulated_to_real[packet[0]])
    if prev_time == 0.0:
      prev_time = packet[3]
    delta = int((packet[3] - prev_time) * 1000)
    if packet[3] == 0:
      delta = 0
    output = (output + save_int(ip) + save_short(packet[1])
              + save_short(packet[2])
              + save_int(delta)
              + save_int(packet[4])
              + save_short(packet[5]))
    if packet[3] != 0:
      prev_time = packet[3]
  conn.sendall(output)

def load_int(str):
  return load_n(str, 4)

def save_int(number):
  return save_n(number, 4)

def load_short(str):
  return load_n(str, 2)

def save_short(number):
  return save_n(number, 2);

def load_n(str, n):
  result = 0
  for i in range(n):
    result = result | ((ord(str[i]) & 0xff) << (8*(n-1-i)))
  return result

def save_n(number, n):
  result = ''
  for i in range(n):
    result = result + chr((number >> ((n-1-i)*8)) & 0xff)
  return result

def ip_to_int(ip):
  ip_list = ip.split('.', 3)
  result = 0
  for ip_byte in ip_list:
    result = result << 8
    result = result | (int(ip_byte, 10) & 0xff)
  return result

#def int_to_ip(num):
#  ip_list = ['0', '0', '0', '0']
#  for ip_byte in ip_list:
#    ip_byte = str(num & 0xff)
#    num = num >> 8
#  ip_list.reverse()
#  return '.'.join(ip_list)

def int_to_ip(num):
  ip_list = ['0', '0', '0', '0']
  for index in range(4):
    ip_list[3-index] = str(num & 0xff)
    num = num >> 8
  return '.'.join(ip_list)

main_loop()
