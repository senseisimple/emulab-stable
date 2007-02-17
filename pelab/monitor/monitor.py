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

##########################################################################

def main_loop():
  # Initialize
  read_args()
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
        elif (pos[0] == conn.fileno() and (pos[1] & select.POLLIN) != 0
              and not done):
          # A record for change in link characteristics is available.
          receive_characteristic(conn)
        elif not done:
          raise Exception('ERROR: Unknown fd on select: fd: ' + str(pos[0])
                          + ' conn-fd: ' + str(conn.fileno()))
    except EOFError:
      sys.stdout.write('Done: Got an EOF on stdin\n')
      done = True
    except Exception, err:
      traceback.print_exc(10, sys.stderr)
      sys.stderr.write('----\n')

##########################################################################

def read_args():
  global ip_mapping_filename, this_experiment, this_ip, stub_ip, stub_port
  global pid, eid, evclient
  global initial_filename
  global is_fake

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
  parser.add_option("--fake", action="store_true", dest="is_fake")

  (options, args) = parser.parse_args()
  ip_mapping_filename = options.ip_mapping_filename
  this_experiment = options.this_experiment
  this_ip = options.this_ip
  stub_ip = options.stub_ip
  stub_port = options.stub_port
  initial_filename = options.initial_filename
  is_fake = options.is_fake

  if is_fake:
    sys.stderr.write('***FAKE***\n')
  else:
    sys.stderr.write('***REAL***\n')

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
  if not is_fake:
    evclient = EventClient(server=TBEVENT_SERVER,
                           keyfile="/proj/%s/exp/%s/tbdata/eventkey"
                           % (pid,eid))

  populate_ip_tables()
  if stub_ip == None:
    if emulated_to_real.has_key(this_ip):
      sys.stdout.write('Init: stub_ip was None before. '
                       + 'Using ip-mapping to find stub.\n')
      stub_ip = emulated_to_real[this_ip]
    elif is_fake:
      stub_ip = 'stub_ip'
    else:
      raise Exception('ERROR: No stub_ip was given. '
                      + 'Could not look one up in ip_mapping.')
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
  line = sys.stdin.readline()
  if line == "":
      raise EOFError
  if netmon_output_version == 3:
    linexp = re.compile('^(New|RemoteIP|RemotePort|LocalPort|TCP_NODELAY|TCP_MAXSEG|SO_RCVBUF|SO_SNDBUF|Connected|Accepted|Send|SendTo|Closed): ([^ ]*) (\d+\.\d+) ([^ ]*)\n$')
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
  global socket_map
  if len(key) > KEY_SIZE - 2:
    sys.stderr.write('ERROR: Event has a key with > KEY_SIZE - 2 characters.'
                     + ' Truncating: ' + key + '\n')
    key = key[:KEY_SIZE - 2]
  key = key.ljust(KEY_SIZE - 2)
  if event == 'New':
    socket_map[key] = Socket()
  if not socket_map.has_key(key):
    raise Exception('ERROR: libnetmon event received that does not '
                    + 'correspond to a socket. Key: ' + key)
  sock = socket_map[key]
  if event == 'New':
    if value == 'TCP':
      sock.protocol = TCP_CONNECTION
    elif value == 'UDP':
      sock.protocol = UDP_CONNECTION
      sys.stderr.write('Notify: Set socket to UDP_CONNECTION: ' + key + '\n')
    else:
      sock.protocol = TCP_CONNECTION
      sys.stderr.write('ERROR: Received "New" event with invalid protocol.'
                       'Assuming TCP_CONNECTION. Protocol: ' + value + '\n')
  elif event == 'RemoteIP':
    start_real_connection(sock)
    app_connection = sock.number_to_connection[0]
    if emulated_to_real.has_key(value):
      app_connection.is_valid = True
      app_connection.dest.remote_ip = value
    else:
      sys.stderr.write('ERROR: Connection invalid: ' + key + ': ' + value
                       + '\n')
      app_connection.is_valid = False
    finalize_real_connection(conn, key, sock, app_connection)
    if initial_connection_bandwidth.has_key(value):
      sock.number_to_connection[0].last_bandwidth = initial_connection_bandwidth[value]
    else:
      sys.stderr.write("ERROR: No initial condition for " + value + "\n")
      sock.number_to_connection[0].last_bandwidth = 0
  elif event == 'RemotePort':
    start_real_connection(sock)
    sock.number_to_connection[0].dest.remote_port = value
    app_connection = sock.number_to_connection[0]
    finalize_real_connection(conn, key, sock, app_connection)
  elif event == 'LocalPort':
    start_real_connection(sock)
    sock.number_to_connection[0].dest.local_port = value
    app_connection = sock.number_to_connection[0]
    finalize_real_connection(conn, key, sock, app_connection)
  elif event == 'SO_RCVBUF':
    sock.receive_buffer_size = int(value)
  elif event == 'SO_SNDBUF':
    sock.send_buffer_size = int(value)
  elif event == 'Connected' or event == 'Accepted':
    if sock.protocol != UDP_CONNECTION:
      app_connection = sock.number_to_connection[0]
      finalize_real_connection(conn, key, sock, app_connection)
      sock.number_to_connection[0].prev_time = timestamp
  elif event == 'Send':
    app_connection = sock.number_to_connection[0]
    if app_connection.is_valid:
      send_write(conn, key, timestamp, app_connection, value)
      app_connection.prev_time = timestamp
    else:
      sys.stderr.write('Invalid Send\n')
  elif event == 'SendTo':
    # If this is a 'new connection' as well, then do not actually send
    # the sendto command. This is so that there is no '0' delta, and
    # so that trivial (single packet) connections do cause wierd
    # measurements.
    regexp = re.compile('^(\d+):(\d+\.\d+\.\d+\.\d+):(\d+):(\d+)')
    match = regexp.match(value)
    if match:
      dest = Dest()
      dest.local_port = match.group(1)
      dest.remote_ip = match.group(2)
      dest.remote_port = match.group(3)
      size = int(match.group(4))
      if emulated_to_real.has_key(dest.remote_ip):
        app_connection = sock.lookup(dest)
        if app_connection.is_connected == False:
          if initial_connection_bandwidth.has_key(app_connection.dest.remote_ip):
            app_connection.last_bandwidth = initial_connection_bandwidth[app_connection.dest.remote_ip]
          else:
            app_connection.last_bandwidth = 0
          app_connection.prev_time = timestamp
          app_connection.is_connected = True
          set_connection(this_ip, app_connection.dest.local_port,
                         app_connection.dest.remote_ip,
                         app_connection.dest.remote_port,
                         proto_to_string(sock.protocol), 'CREATE')
          send_connect(conn, key + save_short(app_connection.number),
                       sock, app_connection)
        else:
          send_write(conn, key, timestamp, app_connection, size)
          app_connection.prev_time = timestamp
  elif event == 'Closed':
    for pos in sock.number_to_connection.itervalues():
      if pos.is_valid:
        send_command(conn, key + save_short(pos.number),
                     DELETE_CONNECTION_COMMAND, '')
        set_connection(this_ip, pos.dest.local_port,
                       pos.dest.remote_ip, pos.dest.remote_port,
                       proto_to_string(sock.protocol), 'CLEAR')
  else:
    sys.stderr.write('ERROR: skipped line with an invalid event: '
                     + event + '\n')

def proto_to_string(proto):
  if proto == TCP_CONNECTION:
    return 'protocol=tcp'
  elif proto == UDP_CONNECTION:
    return 'protocol=udp'
  else:
    sys.stderr.write('ERROR: Invalid protocol received in proto_to_string: '
                     + str(proto) + '\n')
    return 'protocol=tcp'
##########################################################################
  
def start_real_connection(sock):
  if sock.count == 0:
    sock.number_to_connection[0] = Connection(Dest(), 0)
    sock.count = 1

##########################################################################
    
def finalize_real_connection(conn, key, sock, app_connection):
  dest = sock.number_to_connection[0].dest
  if (dest.local_port != ''
      and dest.remote_port != ''
      and dest.remote_ip != ''
      and not sock.number_to_connection[0].is_connected):
    sock.number_to_connection[0].is_connected = True
    sock.dest_to_number[dest.toTuple()] = 0
    if app_connection.is_valid:
      set_connection(this_ip, app_connection.dest.local_port,
                     app_connection.dest.remote_ip,
                     app_connection.dest.remote_port,
                     proto_to_string(sock.protocol), 'CREATE')
      send_connect(conn, key + save_short(0), sock, app_connection)

##########################################################################
    
def send_connect(conn, key, sock, app_connection):
  if sock.protocol == TCP_CONNECTION:
    min_delay = MIN_DELAY_SENSOR
    max_delay = MAX_DELAY_SENSOR
    average_throughput = AVERAGE_THROUGHPUT_SENSOR
  elif sock.protocol == UDP_CONNECTION:
    min_delay = UDP_MINDELAY_SENSOR
    max_delay = UDP_MAXDELAY_SENSOR
    average_throughput = UDP_AVG_THROUGHPUT_SENSOR
  else:
    return
  send_command(conn, key, NEW_CONNECTION_COMMAND,
               save_char(sock.protocol))
  send_command(conn, key, TRAFFIC_MODEL_COMMAND, '')
  send_command(conn, key, SENSOR_COMMAND, save_int(min_delay))
  send_command(conn, key, SENSOR_COMMAND, save_int(max_delay))
  #send_command(conn, key, SENSOR_COMMAND, save_int(NULL_SENSOR))
  #send_command(conn, key, SENSOR_COMMAND, save_int(EWMA_THROUGHPUT_SENSOR)
  send_command(conn, key, SENSOR_COMMAND,
               save_int(average_throughput))
  send_command(conn, key, CONNECTION_MODEL_COMMAND,
               save_int(CONNECTION_RECEIVE_BUFFER_SIZE)
               + save_int(sock.receive_buffer_size))
  send_command(conn, key, CONNECTION_MODEL_COMMAND,
               save_int(CONNECTION_SEND_BUFFER_SIZE)
               + save_int(sock.send_buffer_size))
  send_command(conn, key, CONNECT_COMMAND,
               save_int(ip_to_int(
                 emulated_to_real[app_connection.dest.remote_ip])))

def send_write(conn, key, timestamp, app_connection, size):
  send_command(conn, key + save_short(app_connection.number),
               TRAFFIC_WRITE_COMMAND,
               save_int(int((timestamp - app_connection.prev_time)*1000))
               + save_int(int(size)))

##########################################################################

RECEIVE_HEAD = 0
RECEIVE_BODY = 1

receive_head_buffer = ''
receive_body_buffer = ''
receive_state = RECEIVE_HEAD
receive_head_size = 36
receive_body_size = 0

def receive_characteristic(conn):
  global receive_head_buffer, receive_body_buffer, receive_state
  global receive_body_size

  if receive_state == RECEIVE_HEAD:
    receive_head_buffer = (receive_head_buffer
                           + conn.recv(receive_head_size
                                       - len(receive_head_buffer),
                                       socket.MSG_DONTWAIT))
    if len(receive_head_buffer) == receive_head_size:
      receive_state = RECEIVE_BODY
      receive_body_buffer = ''
      receive_body_size = load_short(receive_head_buffer[1:3])
  if receive_state == RECEIVE_BODY:
    receive_body_buffer = (receive_body_buffer
                           + conn.recv(receive_body_size
                                       - len(receive_body_buffer),
                                       socket.MSG_DONTWAIT))
    if len(receive_body_buffer) == receive_body_size:
      try:
        receive_parse_body()
      finally:
        receive_state = RECEIVE_HEAD
        receive_head_buffer = ''

def receive_parse_body():
  global socket_map
  eventType = load_char(receive_head_buffer[0:1])
  size = load_short(receive_head_buffer[1:3])
  version = load_char(receive_head_buffer[3:4])
  if version != magent_version:
    raise Exception('ERROR: Wrong version from magent: version('
                    + str(version) + ')')
  socketKey = receive_head_buffer[4:34];
  connectionKey = load_short(receive_head_buffer[34:36]);

  if not socket_map.has_key(socketKey):
    raise Exception('ERROR: magent event received that does not '
                    + 'correspond to a socket. Key: ' + socketKey)

  sock = socket_map[socketKey]

  if not sock.number_to_connection.has_key(connectionKey):
    raise Exception('ERROR: magent event received that does not '
                    + 'correspond to a connection. socketKey: ' + socketKey
                    + ' connectionKey: ' + str(connectionKey))
  app_connection = sock.number_to_connection[connectionKey]
  if eventType == EVENT_FORWARD_PATH:
    set_connection(this_ip, app_connection.dest.local_port,
                   app_connection.dest.remote_ip,
                   app_connection.dest.remote_port,
                   proto_to_string(sock.protocol) + ' ' + receive_body_buffer)
  elif eventType == EVENT_BACKWARD_PATH:
    set_connection(app_connection.dest.remote_ip,
                   app_connection.dest.remote_port,
                   this_ip, app_connection.dest.local_port,
                   proto_to_string(sock.protocol) + ' ' + receive_body_buffer)
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
    if int(receive_body_buffer) > app_connection.last_bandwidth:
      app_connection.last_bandwidth = int(receive_body_buffer)
      set_connection(this_ip, app_connection.dest.local_port,
                     app_connection.dest.remote_ip,
                     app_connection.dest.remote_port,
                     proto_to_string(sock.protocol)
                     + ' bandwidth=' + receive_body_buffer)
    else:
      sys.stdout.write('Recieve: Ignored TENTATIVE_THROUGHPUT for '
                       + app_connection.dest.remote_ip + ' - '
                       + receive_body_buffer + ' vs '
                       + str(app_connection.last_bandwidth)
                       + '\n')
  elif eventType == AUTHORITATIVE_BANDWIDTH and int(receive_body_buffer) > 0:
    # We know that the bandwidth has definitely changed. Reset everything.
    app_connection.last_bandwidth = int(receive_body_buffer)
    set_connection(this_ip, app_connection.dest.local_port,
                   app_connection.dest.remote_ip,
                   app_connection.dest.remote_port,
                   proto_to_string(sock.protocol)
                   + ' bandwidth=' + receive_body_buffer)
  else:
    raise Exception('ERROR: Unknown command type: ' + str(eventType)
                    + ', ' + str(receive_body_buffer));

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
