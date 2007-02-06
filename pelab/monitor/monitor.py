#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

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
from optparse import OptionParser
sys.path.append("/usr/testbed/lib")
from tbevent import EventClient, address_tuple, ADDRESSTUPLE_ALL

EVENT_FORWARD_PATH = 0
EVENT_BACKWARD_PATH = 1
TENTATIVE_THROUGHPUT = 2
AUTHORITATIVE_BANDWIDTH = 3
  
NEW_CONNECTION_COMMAND = 0
TRAFFIC_MODEL_COMMAND = 1
CONNECTION_MODEL_COMMAND = 2
SENSOR_COMMAND = 3
CONNECT_COMMAND = 4
TRAFFIC_WRITE_COMMAND = 5
DELETE_CONNECTION_COMMAND = 6

NULL_SENSOR = 0
STATE_SENSOR = 1
PACKET_SENSOR = 2
DELAY_SENSOR = 3
MIN_DELAY_SENSOR = 4
MAX_DELAY_SENSOR = 5
THROUGHPUT_SENSOR = 6
EWMA_THROUGHPUT_SENSOR = 7
LEAST_SQUARES_THROUGHPUT = 8
TSTHROUGHPUT_SENSOR = 9
AVERAGE_THROUGHPUT_SENSOR = 10

CONNECTION_SEND_BUFFER_SIZE = 0
CONNECTION_RECEIVE_BUFFER_SIZE = 1
CONNECTION_MAX_SEGMENT_SIZE = 2
CONNECTION_USE_NAGLES = 3

TCP_CONNECTION = 0
UDP_CONNECTION = 1

# Client connection to event server.  Make this global since we don't
# want to be continually connecting and disconnecting (and creating/destroying
# the eventsys object).  The object is actually instantiated at the top of
# the main_loop function.
TBEVENT_SERVER = "event-server.emulab.net"
evclient = None

emulated_to_real = {}
real_to_emulated = {}
emulated_to_interface = {}
ip_mapping_filename = ''
initial_filename = ''
this_experiment = ''
pid = ''
eid = ''
this_ip = ''
stub_ip = ''
stub_port = 0
netmon_output_version = 2

class Connection:
  def __init__(self):
    # The actual local port bound to a connection. Used for events.
    self.local_port = ''
    # A throughput measurement is only passed on as an event if it is
    # larger than the last bandwidth from that connection
    self.last_bandwidth = 0
    self.prev_time = 0.0

initial_connection_bandwidth = {}
connection_map = {}

total_size = 0
last_total = -1

##########################################################################

def main_loop():
  global total_size, last_total

  # Initialize
  read_args()
  conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#  conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
  sys.stdout.write("stub_ip is " + stub_ip + ":" + str(stub_port) + "\n")
  sys.stdout.flush()
  conn.connect((stub_ip, stub_port))
  poll = select.poll()
  poll.register(sys.stdin, select.POLLIN)
  poll.register(conn, select.POLLIN)
  done = False

  while not done:
    # Reset
#    max_time = time.time() + quanta

    # Collect data until the next quanta boundary
    fdlist = poll.poll()
    for pos in fdlist:
      if pos[0] == sys.stdin.fileno() and (pos[1] & select.POLLIN) != 0 and not done:
          # A line of data from tcpdump is available.
          try:
#            sys.stdout.write('get_next_packet()\n')
            get_next_packet(conn)
          except EOFError:
            sys.stdout.write('Done: Got EOF on stdin\n')
            done = 1
      elif pos[0] == conn.fileno() and (pos[1] & select.POLLIN) != 0 and not done:
        sys.stdout.write('receive_characteristic()\n')
        # A record for change in link characteristics is available.
        done = not receive_characteristic(conn)
      elif not done:
        sys.stdout.write('fd: ' + str(pos[0]) + ' conn-fd: ' + str(conn.fileno()) + '\n')
      # Update the stub
    if total_size != last_total:
#      sys.stdout.write('Total Size: ' + str(total_size) + '\n')
      last_total = total_size
#    sys.stdout.write('Loop end\n')

##########################################################################

def read_args():
  global ip_mapping_filename, this_experiment, this_ip, stub_ip, stub_port
  global pid, eid, evclient
  global initial_filename

  usage = "usage: %prog [options]"
  parser = OptionParser(usage=usage)
  parser.add_option("--mapping", action="store", type="string",
                    dest="ip_mapping_filename", metavar="MAPPING_FILE",
                    help="File mapping IP addresses on Emulab to those on PlanetLab (required)")
  parser.add_option("--experiment", action="store", type="string",
                    dest="this_experiment", metavar="EXPERIMENT_NAME",
                    help="Experiment name of the form pid/eid (required)")
  parser.add_option("--ip", action="store", type="string",
                    dest="this_ip", metavar="X.X.X.X",
                    help="The IP address the monitor will use (required)")
  parser.add_option("--stub-ip", action="store", type="string",
                    dest="stub_ip", metavar="X.X.X.X",
                    help="The IP address of the stub (if not specified, defaults to the one in the ip mapping)")
  parser.add_option("--stub-port", action="store", type="int",
                    dest="stub_port", default=4200,
                    help="The port used to connect to the stub (defaults to 4200)")
  parser.add_option("--initial", action="store", type="string",
                    dest="initial_filename", metavar="INITIAL_CONDITIONS_FILE",
                    help="File giving initial conditions for connections (defaults to no shaping)")

  (options, args) = parser.parse_args()
  ip_mapping_filename = options.ip_mapping_filename
  this_experiment = options.this_experiment
  this_ip = options.this_ip
  stub_ip = options.stub_ip
  stub_port = options.stub_port
  initial_filename = options.initial_filename

  if len(args) != 0:
    parser.print_help()
    parser.error("Invalid argument(s): " + str(args))
  if ip_mapping_filename == None:
    parser.print_help()
    parser.error("Missing --mapping=MAPPING_FILE option")
  if this_experiment == None:
    parser.print_help()
    parser.error("Missing --experiment=EXPERIMENT_NAME option")
  if this_ip == None:
    parser.print_help()
    parser.error("Missing --ip=X.X.X.X option")

  (pid,eid) = this_experiment.split('/')

  # XXX: The name of the event server is hardwired here, and due to the
  #      lack of autoconf, isn't even an autoconf var.  Could add a command
  #      line option to set it if needed.
  # XXX: This HAS to be done before here, becuase read_args can fire
  #      off set_link().
  #      
  evclient = EventClient(server=TBEVENT_SERVER,
                         keyfile="/proj/%s/exp/%s/tbdata/eventkey" % (pid,eid))

  populate_ip_tables()
  if stub_ip == None:
    sys.stdout.write('stub_ip was None before\n')
    stub_ip = emulated_to_real[this_ip]
  sys.stdout.write('stub_ip: ' + stub_ip + '\n')
  sys.stdout.write('this_ip: ' + this_ip + '\n')
  sys.stdout.write('emulated_to_real: ' + str(emulated_to_real) + '\n')
  if initial_filename != None:
    read_initial_conditions()

##########################################################################

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

##########################################################################

# Format of an initial conditions file is:
#
# List of lines, where each line is of the format:
# <source-ip> <dest-ip> <bandwidth> <delay> <loss>
#
# Where source and dest ip addresses are in x.x.x.x format, and delay and
# bandwidth are integral values in milliseconds and kilobits per second
# respectively.
def read_initial_conditions():
  global initial_connection_bandwidth
  input = open(initial_filename, 'r')
  line = input.readline()
  while line != '':
    # Don't worry about loss for now. Just discard the value.
    fields = line.strip().split(' ', 4)
    if len(fields) == 5 and fields[0] == this_ip:
      set_link(fields[0], fields[1], 'delay=' + str(int(fields[3])/2))
      set_link(fields[0], fields[1], 'bandwidth=' + fields[2])
      initial_connection_bandwidth[fields[1]] = int(fields[2])
    line = input.readline()

##########################################################################

def get_next_packet(conn):
  global total_size, last_total, connection_map
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
  conexp = re.compile('^(New|Connected|LocalPort|Closed|SO_RCVBUF|SO_SNDBUF): (\d+):(\d+\.\d+\.\d+\.\d+):(\d+)( ((?:\d+)(?:\.(?:\d+))?))?')
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
      if emulated_to_real.has_key(ipaddr):
        total_size = total_size + size
      key = (ipaddr, localport, remoteport)
      send_command(conn, TRAFFIC_WRITE_COMMAND, TCP_CONNECTION, ipaddr,
                     localport, remoteport,
                     save_int(int((time - connection_map[key].prev_time)*1000)) + save_int(size))
        connection_map[key].prev_time = time
  elif ((netmon_output_version == 2) and cmatch):
      #
      # Watch for new or closed connections
      #
      event = cmatch.group(1)
      localport = int(cmatch.group(2))
      ipaddr = cmatch.group(3)
      remoteport = int(cmatch.group(4))
      value_given = cmatch.group(5) != ''
      value = cmatch.group(6)
      if not value_given:
        value = '0'
      key = (ipaddr, localport, remoteport)
      if emulated_to_real.has_key(ipaddr):
        sys.stdout.write("Got a connection event: " + event + "\n")
        if event == 'New':
          connection_map[key] = Connection()
          if initial_connection_bandwidth.has_key(ipaddr):
            connection_map[key].last_bandwidth = initial_connection_bandwidth[ipaddr]
          else:
            sys.stderr.write("No initial condition for " + ipaddr + "\n")
            connection_map[key].last_bandwidth = 0
          send_command(conn, NEW_CONNECTION_COMMAND, TCP_CONNECTION, ipaddr,
                      localport, remoteport, '')
          send_command(conn, TRAFFIC_MODEL_COMMAND, TCP_CONNECTION, ipaddr,
                       localport, remoteport, '')
          send_command(conn, SENSOR_COMMAND, TCP_CONNECTION, ipaddr,
                       localport, remoteport, save_int(MIN_DELAY_SENSOR))
          send_command(conn, SENSOR_COMMAND, TCP_CONNECTION, ipaddr,
#                       localport, remoteport, save_int(NULL_SENSOR))
#          send_command(conn, SENSOR_COMMAND, TCP_CONNECTION, ipaddr,
                       localport, remoteport, save_int(MAX_DELAY_SENSOR))
#          send_command(conn, SENSOR_COMMAND, TCP_CONNECTION, ipaddr,
#                       localport, remoteport, save_int(EWMA_THROUGHPUT_SENSOR))
          send_command(conn, SENSOR_COMMAND, TCP_CONNECTION, ipaddr,
                       localport, remoteport, save_int(AVERAGE_THROUGHPUT_SENSOR))
        elif event == 'Closed':
          send_command(conn, DELETE_CONNECTION_COMMAND, TCP_CONNECTION, ipaddr,
                      localport, remoteport, '')
          set_connection(this_ip, connection_map[key].local_port, ipaddr,
                         str(remoteport), '', 'CLEAR')
        elif event == 'Connected':
          send_command(conn, CONNECT_COMMAND, TCP_CONNECTION, ipaddr,
                      localport, remoteport, '')
          connection_map[key].prev_time = float(value)
#        elif event == 'LocalPort':
        elif event == 'SO_RCVBUF':
          send_command(conn, CONNECTION_MODEL_COMMAND, TCP_CONNECTION, ipaddr,
                      localport, remoteport,
                      save_int(CONNECTION_RECEIVE_BUFFER_SIZE)
                      + save_int(int(value)))
        elif event == 'SO_SNDBUF':
          send_command(conn, CONNECTION_MODEL_COMMAND, TCP_CONNECTION, ipaddr,
                      localport, remoteport,
                      save_int(CONNECTION_SEND_BUFFER_SIZE)
                       + save_int(int(value)))
        elif event == 'LocalPort':
          connection_map[key].local_port = value
          set_connection(this_ip, value,
                         ipaddr, str(remoteport), '', 'CREATE')
        else:
          sys.stdout.write('skipped line with an invalid event: '
                           + event + '\n')
      else:
        sys.stdout.write('skipped line with an invalid destination: '
                         + ipaddr + '\n')
  else:
      sys.stdout.write('skipped line in the wrong format: ' + line)

##########################################################################

def receive_characteristic(conn):
  global connection_map
  buf = conn.recv(12)
  if len(buf) != 12:
    sys.stdout.write('Event header is the wrong size. Length: '
                     + str(len(buf)) + '\n')
    return False
  eventType = load_char(buf[0:1]);
  size = load_short(buf[1:3]);
  transport = load_char(buf[3:4]);
  dest = real_to_emulated[int_to_ip(load_int(buf[4:8]))]
  source_port = load_short(buf[8:10])
  dest_port = load_short(buf[10:12])
  key = (dest, source_port, dest_port);
  real_local_port = connection_map[key].local_port
  buf = conn.recv(size);
  if len(buf) != size:
    sys.stdout.write('Event body is the wrong size.\n')
    return False
  if eventType == EVENT_FORWARD_PATH:
    # set_link(this_ip, dest, buf)
    set_connection(this_ip, real_local_port, dest, str(dest_port), buf)
  elif eventType == EVENT_BACKWARD_PATH:
    # set_link(dest, this_ip, buf)
    set_connection(dest, str(dest_port), this_ip, real_local_port, buf)
  elif eventType == TENTATIVE_THROUGHPUT:
    # There is a throughput number, but we don't know whether the link
    # is saturated or not. If the link is not saturated, then we just
    # need to make sure that emulated bandwidth >= real
    # bandwidth. This means that we output a throughput number only if
    # it is greater than our previous measurements because we don't
    # know that bandwidth has decreased.
    # If we don't know what the bandwidth is for this host, ignore tentative
    # measurments until we find out (due to an authoritative message)

    # There is no way to know whether the link is saturated or not. If
    # it was saturated and we knew it, then the event type would be
    # AUTHORITATIVE_BANDWIDTH. Therefore, we assume that it is
    # authoritative if it is greater than the previous measurement,
    # and that it is just a throughput number if it is less.
    if connection_map.has_key(key):
      if int(buf) > connection_map[key].last_bandwidth:
          connection_map[key].last_bandwidth = int(buf)
          # set_link(this_ip, dest, 'bandwidth=' + buf)
          set_connection(this_ip, real_local_port, dest, str(dest_port),
                         'bandwidth=' + buf)
      else:
        sys.stdout.write('ignored TENTATIVE_THROUGHPUT for %s - %i vs %i\n'
                         % (dest,int(buf),connection_map[key].last_bandwidth))
    else:
      sys.stdout.write('ignored TENTATIVE_THROUGHPUT for %s - no data\n' % (dest))
  elif eventType == AUTHORITATIVE_BANDWIDTH and int(buf) > 0:
    # We know that the bandwidth has definitely changed. Reset everything.
    connection_map[key].last_bandwidth = int(buf)
    # set_link(this_ip, dest, 'bandwidth=' + buf)
    set_connection(this_ip, real_local_port, dest, str(dest_port),
                   'bandwidth=' + buf)
  else:
    sys.stdout.write('Other: ' + str(eventType) + ', ' + str(value) + '\n');
  return True

##########################################################################

def set_bandwidth(kbps, dest):
#  sys.stdout.write('<event> bandwidth=' + str(kbps) + '\n')
  now = time.time()
  sys.stderr.write('BANDWIDTH!purple\n')
  sys.stderr.write('BANDWIDTH!line ' + ('%0.6f' % now) + ' 0 '
                   + ('%0.6f' % now)
                   + ' ' + str(kbps*1000/8) + '\n')
  return set_link(this_ip, dest, 'bandwidth=' + str(kbps))

##########################################################################

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

##########################################################################

def set_loss(probability, dest):
  return set_link(this_ip, dest, 'plr=' + str(probability))

##########################################################################

def set_max_delay(milliseconds, dest):
  return set_link(this_ip, dest, 'MAXINQ=' + str(milliseconds))

##########################################################################

def set_connection(source, source_port, dest, dest_port, ending, event_type='MODIFY'):
  set_link(source, dest, 'srcport=' + source_port
           + ' dstport=' + dest_port + ' protocol=tcp ' + ending, event_type)

##########################################################################

def set_link(source, dest, ending, event_type='MODIFY'):
  # Create event system address tuple to identify the notification.
  # The event is not sent through the scheduler; it is sent as an
  # immediate notification.
  global evclient
  evtuple = address_tuple()
  evtuple.host  = ADDRESSTUPLE_ALL
  evtuple.site  = ADDRESSTUPLE_ALL
  evtuple.group = ADDRESSTUPLE_ALL
  evtuple.eventtype = event_type.upper() #'MODIFY'
  evtuple.objtype = 'LINK'
  evtuple.expt = this_experiment
  evtuple.objname   = emulated_to_interface[source]
  evnotification = evclient.create_notification(evtuple)
  evargs = ['DEST=' + dest, ]
  # Uppercase all arguments.
  for part in ending.split():
    arg, val = part.split('=',1)
    evargs.append(arg.upper() + '=' + val)
    pass
  evargstr = ' '.join(evargs)
  evnotification.setArguments(evargstr)
  sys.stdout.write('event: ' + '(' + event_type.upper() + ') '
                   + evargstr + '\n')
  # Must invert the return value of the notification send function as the
  # original code here returned the exit code of tevc (hence 0 means success).
  return not evclient.notify(evnotification)

##########################################################################

def send_command(conn, command_id, protocol, ipaddr, localport, remoteport,
                 command):
  output = (save_char(command_id)
            + save_short(len(command))
            + save_char(protocol)
            + save_int(ip_to_int(emulated_to_real[ipaddr]))
            + save_short(localport)
            + save_short(remoteport)
            + command)
#  sys.stdout.write('Sending command: CHECKSUM=' + str(checksum(output)) + '\n')
  conn.sendall(output)

##########################################################################

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

##########################################################################

def load_int(str):
  return load_n(str, 4)

##########################################################################

def save_int(number):
  return save_n(number, 4)

##########################################################################

def load_short(str):
  return load_n(str, 2)

##########################################################################

def save_short(number):
  return save_n(number, 2)

##########################################################################

def load_char(str):
  return load_n(str, 1)

##########################################################################

def save_char(number):
  return save_n(number, 1)

##########################################################################

def load_n(str, n):
  result = 0
  for i in range(n):
    result = result | ((ord(str[i]) & 0xff) << (8*(n-1-i)))
  return result

##########################################################################

def save_n(number, n):
  result = ''
  for i in range(n):
    result = result + chr((number >> ((n-1-i)*8)) & 0xff)
  return result

##########################################################################

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

##########################################################################

def int_to_ip(num):
  ip_list = ['0', '0', '0', '0']
  for index in range(4):
    ip_list[3-index] = str(num & 0xff)
    num = num >> 8
  return '.'.join(ip_list)

##########################################################################

def checksum(buf):
  total = 0
  flip = 1
  for index in range(len(buf)):
    total += (ord(buf[index]) & 0xff) * flip
#    flip *= -1
  return total

##########################################################################

main_loop()
