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

emulated_to_real = {}
real_to_emulated = {}
emulated_to_interface = {}
ip_mapping_filename = ''
this_experiment = ''
this_ip = ''
stub_ip = ''

total_size = 0
last_total = -1

def main_loop():
  global total_size, last_total
  # Initialize
  read_args()
  quanta = 0.5# in seconds
  conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  sys.stdout.write("stub_ip is %s\n" % stub_ip)
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
            packet = get_next_packet()
          except EOFError:
            sys.stdout.write('Done: Got EOF on stdin\n')
            done = 1
          if (packet):
            packet_list = packet_list + [packet]
        elif pos[0] == conn.fileno() and not done:
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
  line = sys.stdin.readline()
  if line == "":
      raise EOFError
  # Could move this elsewhere to avoid re-compiling, but I'd like to keep it
  # with this code for better readability
  linexp = re.compile('^(\d+\.\d+) > (\d+\.\d+\.\d+\.\d+)\.(\d+) (\((\d+)\))?$')
  match = linexp.match(line)
  if (match) :
      time = float(match.group(1))
      ipaddr = match.group(2)
      port = int(match.group(3))
      size_given = match.group(4) != ''
      size = int(match.group(5))
#      sys.stdout.write('dest: ' + ipaddr + ' port: ' + str(port) + ' size: '
#              + str(size) + '\n')
      if not size_given:
          size = 0
      total_size = total_size + size
      return (ipaddr, time, size)
  else:
      sys.stdout.write('skipped line in the wrong format: ' + line + '\n')
      return None
    
def receive_characteristic(conn):
  buffer = conn.recv(12)
  if len(buffer) == 12:
    dest = real_to_emulated[int_to_ip(load_int(buffer[0:4]))]
    command = load_int(buffer[4:8])
    value = load_int(buffer[8:12])
#    sys.stdout.write('received: ' + str(dest) + ' '
#                     + str(command) + ' ' + str(value) + '\n')
    if command == 1:
      # value is bandwidth in kbps
      set_bandwidth(value, dest)
    elif command == 2:
      # value is delay in milliseconds
      set_delay(value, dest)
#    elif command == 3:
      # value is packet loss in packets per billion
#      set_loss(value/1000000000.0, dest)
    return True
  elif len(buffer) == 0:
    return False

def set_bandwidth(kbps, dest):
  sys.stdout.write('<event> bandwidth=' + str(kbps) + '\n')
  return set_link(this_ip, dest, 'bandwidth=' + str(kbps))

# Set delay on the link. We are given round trip time.
def set_delay(milliseconds, dest):
  # Set the delay from here to there to 1/2 rtt.
  error = set_link(this_ip, dest, 'delay=' + str(milliseconds/2))
  if error == 0:
    # If that succeeded, set the delay from there to here.
    return set_link(dest, this_ip, 'delay=' + str(milliseconds/2))
  else:
    return error

def set_loss(probability, dest):
  return set_link(this_ip, dest, 'plr=' + str(probability))

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
    prev_time = packet_list[0][1]
  for packet in packet_list:
    ip = ip_to_int(emulated_to_real[packet[0]])
    output = output + save_int(ip) + save_int(int((packet[1] - prev_time)
                                                  * 1000)) + save_int(packet[2])
    prev_time = packet[1]
  conn.sendall(output)

def load_int(str):
  result = 0
  for i in range(4):
    result = result | ((ord(str[i]) & 0xff) << (8*(3-i)))
  return result

def save_int(number):
  result = ''
  for i in range(4):
    result = result + chr((number >> ((3-i)*8)) & 0xff)
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
