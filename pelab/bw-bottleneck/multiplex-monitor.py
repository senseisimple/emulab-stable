#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

# Monitors the application's behaviour and report to the network model.

import sys
import os
import time
import socket
import select
import re
import traceback
import errno
import math
from optparse import OptionParser
sys.path.append("/usr/testbed/lib")
#from tbevent import EventClient, address_tuple, ADDRESSTUPLE_ALL

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

command_to_string = {NEW_CONNECTION_COMMAND: 'New Connection',
                     TRAFFIC_MODEL_COMMAND: 'Traffic Model',
                     CONNECTION_MODEL_COMMAND: 'Connection Model',
                     SENSOR_COMMAND: 'Sensor',
                     CONNECT_COMMAND: 'Connect',
                     TRAFFIC_WRITE_COMMAND: 'Write',
                     DELETE_CONNECTION_COMMAND: 'Delete'}

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
UDP_STATE_SENSOR = 11
UDP_PACKET_SENSOR = 12
UDP_THROUGHPUT_SENSOR = 13
UDP_MINDELAY_SENSOR = 14
UDP_MAXDELAY_SENSOR = 15
UDP_RTT_SENSOR = 16
UDP_LOSS_SENSOR = 17
UDP_AVG_THROUGHPUT_SENSOR = 18

CONNECTION_SEND_BUFFER_SIZE = 0
CONNECTION_RECEIVE_BUFFER_SIZE = 1
CONNECTION_MAX_SEGMENT_SIZE = 2
CONNECTION_USE_NAGLES = 3

TCP_CONNECTION = 0
UDP_CONNECTION = 1

KEY_SIZE = 32

# Client connection to event server.  Make this global since we don't
# want to be continually connecting and disconnecting (and creating/destroying
# the eventsys object).  The object is actually instantiated at the top of
# the main_loop function.
TBEVENT_SERVER = "event-server.emulab.net"
evclient = None

#emulated_to_interface = {}
initial_filename = ''
multiplex_filename = ''
this_experiment = ''
pid = ''
eid = ''
this_ip = ''
source_interface = ''
stub_ip = ''
stub_port = 0
netmon_output_version = 3
magent_version = 1

is_fake = False

class Dest:
  def __init__(self, tup=None):
    if tup == None:
      self.local_port = ''
      self.remote_ip = ''
      self.remote_port = ''
    else:
      self.local_port = tup[0]
      self.remote_ip = tup[1]
      self.remote_port = tup[2]

  def toTuple(self):
    return (self.local_port, self.remote_ip, self.remote_port)

class Path:
  def __init__(self, newBandwidths):
    self.bandwidths = newBandwidths
    self.flowCount = 0

  def getBandwidth(self, index):
    if len(self.bandwidths) == 0:
      return 0
    elif index > len(self.bandwidths):
      return self.bandwidths[len(self.bandwidths) - 1]
    else:
      return self.bandwidths[index - 1]

class Bottleneck:
  def __init__(self):
    self.ip_to_path = {}
    self.totalFlows = 0

  def addPath(self, ip, bandwidths):
    self.ip_to_path[ip] = Path(bandwidths)

  def addFlow(self, ip):
    self.totalFlows += 1
    self.ip_to_path[ip].flowCount += 1

  def removeFlow(self, ip):
    self.totalFlows -= 1
    self.ip_to_path[ip].flowCount -= 1

  def getDests(self):
    return ",".join(self.ip_to_path.keys())

  def getBandwidth(self):
    total = 0.0
    if self.totalFlows == 0:
      for path in self.ip_to_path.values():
        if len(path.bandwidths) > 0:
          total = max(path.bandwidths[0], total)
    else:
      denom = float(self.totalFlows)
      for path in self.ip_to_path.values():
        total += float(path.getBandwidth(self.totalFlows)) * path.flowCount / denom
    return int(total)

class Connection:
  def __init__(self, new_dest, new_number):
    # The actual local port bound to a connection. Used for events.
    self.dest = new_dest
    # A throughput measurement is only passed on as an event if it is
    # larger than the last bandwidth from that connection

    # Initialized when a RemoteIP or SendTo command is received
    self.last_bandwidth = 0
    # Initialized when a Connect or SendTo command is received
    self.prev_time = 0.0
    # Initialized by start_real_connection() or lookup()
    self.number = new_number
    # Initialized by finalize_real_connection() or lookup()
    self.is_connected = False
    # Initialized when a RemoteIP is received
    self.is_valid = True

class Socket:
  def __init__(self):
    # Initialized when a New is received
    self.protocol = TCP_CONNECTION
    self.dest_to_number = {}
    self.number_to_connection = {}
    # Kept consistent by lookup() and start_real_connection() and
    # finalize_real_connection()
    self.count = 0
    # Initialized by SO_RCVBUF and SO_SNDBUF
    self.receive_buffer_size = 0
    self.send_buffer_size = 0

# Searches for a dest, inserts if dest not found. Returns the connection associated with dest.
  def lookup(self, dest):
    sys.stdout.write('Looking up: ' + dest.local_port + ':'
                     + dest.remote_ip + ':' + dest.remote_port + '\n')
    if self.dest_to_number.has_key(dest.toTuple()):
      number = self.dest_to_number[dest.toTuple()]
      return self.number_to_connection[number]
    else:
      sys.stdout.write('Lookup: Failed, creating new connection\n')
      self.dest_to_number[dest.toTuple()] = self.count
      self.number_to_connection[self.count] = Connection(dest, self.count)
      self.count = self.count + 1
      return self.number_to_connection[self.count - 1]

initial_connection_bandwidth = {}
socket_map = {}

key_to_bottleneck = {}
key_to_ip = {}
ip_to_bottleneck = {}

##########################################################################

def main_loop():
  # Initialize
  read_args()
  sys.stderr.write("source interface: " + source_interface + "\n")
  conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  if not is_fake:
    #conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    sys.stdout.write("Connect: stub_ip is " + stub_ip + ":" + str(stub_port)
                     + "\n")
    conn.connect((stub_ip, stub_port))
  poll = select.poll()
  poll.register(sys.stdin, select.POLLIN)
  if not is_fake:
    poll.register(conn, select.POLLIN)
  done = False

  while not done:
    # Collect data until the next quanta boundary
    try:
      fdlist = poll.poll()
      for pos in fdlist:
        if (pos[0] == sys.stdin.fileno() and (pos[1] & select.POLLIN) != 0
            and not done):
          # A line of data from tcpdump is available.
          get_next_packet(conn)
        elif not done:
          raise Exception('ERROR: Unknown fd on select: fd: ' + str(pos[0])
                          + ' conn-fd: ' + str(conn.fileno()))
    except EOFError:
      sys.stdout.write('Done: Got an EOF on stdin\n')
      done = True
    except Exception, err:
      traceback.print_exc(10, sys.stderr)
      sys.stderr.write('----\n')
      done = True

##########################################################################

def read_args():
  global this_experiment, this_ip, stub_ip, stub_port
  global pid, eid, evclient
  global initial_filename
  global is_fake
  global source_interface, multiplex_filename, ip_to_bottleneck

  usage = "usage: %prog [options]"
  parser = OptionParser(usage=usage)
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
  parser.add_option("--fake", action="store_true", dest="is_fake")
  parser.add_option("--interface", action="store", type="string", dest="source_interface",
                    metavar="SOURCE_INTERFACE",
                    help="Name of the interface on this node's cloud.");
  parser.add_option("--multiplex", action="store", type="string",
                    dest="multiplex_filename",
                    metavar="MULTIPLEX_CONDITIONS_FILE",
                    help="File giving multiplex conditions for connections")

  (options, args) = parser.parse_args()
  this_experiment = options.this_experiment
  this_ip = options.this_ip
  stub_ip = options.stub_ip
  stub_port = options.stub_port
  initial_filename = options.initial_filename
  is_fake = options.is_fake
  source_interface = options.source_interface
  multiplex_filename = options.multiplex_filename

  if is_fake:
    sys.stderr.write('***FAKE***\n')
  else:
    sys.stderr.write('***REAL***\n')

  if len(args) != 0:
    parser.print_help()
    parser.error("Invalid argument(s): " + str(args))
  if multiplex_filename == None:
    parser.print_help()
    parser.error("Missing --multiplex=MULTIPLEX_CONDITIONS_FILE option")
  if this_experiment == None:
    parser.print_help()
    parser.error("Missing --experiment=EXPERIMENT_NAME option")
  if this_ip == None:
    parser.print_help()
    parser.error("Missing --ip=X.X.X.X option")
  if source_interface == None:
    parser.print_help()
    parser.error("Missing --interface=<elabc-elabX>")

  (pid,eid) = this_experiment.split('/')

  # XXX: The name of the event server is hardwired here, and due to the
  #      lack of autoconf, isn't even an autoconf var.  Could add a command
  #      line option to set it if needed.
  # XXX: This HAS to be done before here, becuase read_args can fire
  #      off set_link().
  #
  if not is_fake:
    evclient = EventClient(server=TBEVENT_SERVER,
                           keyfile="/proj/%s/exp/%s/tbdata/eventkey"
                           % (pid,eid))

  if initial_filename != None:
    read_initial_conditions()
  if multiplex_filename != None:
    read_multiplex_conditions()

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
    delay = math.floor(float(fields[3]))
    if len(fields) == 5 and fields[0] == this_ip:
      set_delay(fields[0], fields[1], str(delay))
#      set_link(fields[0], fields[1], 'delay=' + str(delay)
#      set_link(fields[0], fields[1], 'bandwidth=' + fields[2])
      initial_connection_bandwidth[fields[1]] = int(fields[2])
    line = input.readline()

# Format of multiplex conditions file is:
#
# List of bottlenecks.
#
# Each bottleneck starts with a list of lines. Each line is:
# <dest-ip> (<bandwidth>)+
#
# There must be at least one bandwidth. Each bandwidth is the
# aggregate bandwidth of n full-connections to the <dest-ip>
#
# Each bottleneck ends with a single line containing just the
# characters "%%".

def read_multiplex_conditions():
  global ip_to_bottleneck
  input = open(multiplex_filename, 'r')
  line = input.readline().strip()
  while line != '':
    bottle = Bottleneck()
    while line != '%%' and line != '':
      fields = line.strip().split(' ')
      bottle.addPath(fields[0], fields[1:])
      ip_to_bottleneck[fields[0]] = bottle
      line = input.readline().strip()
    updateBandwidth(bottle)
    line = input.readline().strip()

##########################################################################

def get_next_packet(conn):
  line = sys.stdin.readline()
  if line == "":
      raise EOFError
  if netmon_output_version == 3:
    linexp = re.compile('^(New|RemoteIP|RemotePort|LocalPort|TCP_NODELAY|TCP_MAXSEG|SO_RCVBUF|SO_SNDBUF|Connected|Accepted|Send|SendTo|Closed|Exit): ([^ ]*) (\d+\.\d+) ([^ ]*)\n$')
  else:
    raise Exception("ERROR: Only input version 3 is supported")
  match = linexp.match(line)
  if match:
    event = match.group(1)
    key = match.group(2)
    timestamp = float(match.group(3))
    value = match.group(4)
    process_event(conn, event, key, timestamp, value)
  else:
    sys.stderr.write('ERROR: skipped line in the wrong format: ' + line + '\n')

##########################################################################

def process_event(conn, event, key, timestamp, value):
  global key_to_bottleneck, ip_to_bottleneck, key_to_ip
  if len(key) > KEY_SIZE - 2:
    sys.stderr.write('ERROR: Event has a key with > KEY_SIZE - 2 characters.'
                     + ' Truncating: ' + key + '\n')
    key = key[:KEY_SIZE - 2]
  key = key.ljust(KEY_SIZE - 2)
  if event == 'RemoteIP':
    key_to_ip[key] = value
  elif event == 'Connected':
    if key in key_to_bottleneck:
      key_to_bottleneck[key].removeFlow(key_to_ip[key])
    key_to_bottleneck[key] = ip_to_bottleneck[key_to_ip[key]]
    key_to_bottleneck[key].addFlow(key_to_ip[key])
    updateBandwidth(key_to_bottleneck[key])
  elif event == 'Closed' or event == 'Exit':
    if key in key_to_bottleneck:
      key_to_bottleneck[key].removeFlow(key_to_ip[key])
      del key_to_bottleneck[key]
      del key_to_ip[key]
#  else:
#    sys.stderr.write('ERROR: skipped line with an invalid event: '
#                     + event + '\n')

def updateBandwidth(bottle):
  command = ("/usr/testbed/bin/tevc -e " + pid + "/" + eid + " now " + source_interface
             + " MODIFY "
             + " DEST=" + bottle.getDests()

             + " LIMIT=" + str(128000 * max(bottle.totalFlows, 1))
             + " QUEUE-IN-BYTES=1"

#             + " BANDWIDTH=" + str(bottle.getBandwidth()) + " BACKFILL=0")

             + " BANDWIDTH=100000 BACKFILL="
             + str(100000 - int(bottle.getBandwidth())))

#             + " BANDWIDTH=50000 BACKFILL="
#             + str(50000 - int(bottle.getBandwidth())))
  sys.stdout.write(command + "\n")
  os.system(command)
#  set_link(this_ip, bottle.getDests(),
#           "LIMIT=" + str(128000 * max(bottle.totalFlows, 1))
#           + " QUEUE-IN-BYTES=1 BANDWIDTH=100000 BACKFILL="
#           + str(100000 - int(bottle.getBandwidth())))

def set_delay(source, dest, delay):
  command = ("/usr/testbed/bin/tevc -e " + pid + "/" + eid + " now " + source_interface
             + " MODIFY "
             + " DEST=" + dest
             + " DELAY=" + delay
             + " LIMIT=75000"
             + " QUEUE-IN-BYTES=1");
  sys.stdout.write(command + "\n")
  os.system(command)

##########################################################################

def set_connection(source, source_port, dest, dest_port, ending, event_type='MODIFY'):
  set_link(source, dest, 'srcport=' + source_port
           + ' dstport=' + dest_port + ' ' + ending, event_type)

##########################################################################

def set_link(source, dest, ending, event_type='MODIFY'):
  sys.stdout.write('Event: ' + source + ' -> ' + dest + '(' + event_type
                   + '): ' + ending + '\n')
  if is_fake:
    return 0
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
  evtuple.objname = source_interface
#  evtuple.objname   = emulated_to_interface[source]
  evnotification = evclient.create_notification(evtuple)
  evargs = ['DEST=' + dest, ]
  # Uppercase all arguments.
  for part in ending.split():
    arg, val = part.split('=',1)
    evargs.append(arg.upper() + '=' + val)
    pass
  evargstr = ' '.join(evargs)
  evnotification.setArguments(evargstr)
#  sys.stdout.write('event: ' + '(' + event_type.upper() + ') '
#                   + evargstr + '\n')
  # Must invert the return value of the notification send function as the
  # original code here returned the exit code of tevc (hence 0 means success).
  return not evclient.notify(evnotification)

##########################################################################

send_buffer = ''

def send_command(conn, key, command_id, command):
  global send_buffer
  if is_fake:
    sys.stdout.write('Command: ' + key + ' version(' + str(magent_version)
                     + ') ' + command_to_string[command_id] + '\n')
    return
  output = (save_char(command_id)
            + save_short(len(command))
            + save_char(magent_version)
            + key
            + command)
  send_buffer = send_buffer + output
  try:
    sent = conn.send(send_buffer, socket.MSG_DONTWAIT)
    send_buffer = send_buffer[sent:]
  except socket.error, inst:
    num = inst[0]
    if num != errno.EWOULDBLOCK:
      raise

##########################################################################

def load_int(str):
  return load_n(str, 4)

def save_int(number):
  return save_n(number, 4)

##########################################################################

def load_short(str):
  return load_n(str, 2)

def save_short(number):
  return save_n(number, 2)

##########################################################################

def load_char(str):
  return load_n(str, 1)

def save_char(number):
  return save_n(number, 1)

##########################################################################

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

##########################################################################

def ip_to_int(ip):
  ip_list = ip.split('.', 3)
  result = 0
  for ip_byte in ip_list:
    result = result << 8
    result = result | (int(ip_byte, 10) & 0xff)
  return result

def int_to_ip(num):
  ip_list = ['0', '0', '0', '0']
  for index in range(4):
    ip_list[3-index] = str(num & 0xff)
    num = num >> 8
  return '.'.join(ip_list)

##########################################################################

main_loop()
