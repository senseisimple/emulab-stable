
import sys
import os
import time
import socket
import select

emulated_to_real = {'10.0.0.1' : '10.1.0.1',
                    '10.0.0.2' : '10.1.0.2'}

real_to_emulated = {'10.1.0.1' : '10.0.0.1',
                    '10.1.0.2' : '10.0.0.2'}

def main_loop():
  # Initialize
  quanta = 5 # in seconds
  stub_address = 'planet0.pelab.tbres.emulab.net'
  conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  conn.connect((stub_address, 4200))
  poll = select.poll()
  poll.register(sys.stdin, select.POLLIN)
  poll.register(conn, select.POLLIN)
  done = False

  while not done:
    # Reset
    dest_set = set([])
    max_time = time.time() + quanta

    # Collect data until the next quanta boundary
    while time.time() < max_time:
      fdlist = poll.poll(max(max_time - time.time(), 0) * 1000)
      for pos in fdlist:
        if pos[0] == sys.stdin.fileno() and not done:
          # A line of data from tcpdump is available.
          ip = get_next_destination()
          dest_set.add(ip)
        elif pos[0] == conn.fileno() and not done:
          # A record for change in link characteristics is available.
          done = not receive_characteristic(conn)
        elif not done:
          sys.stdout.write('fd: ' + str(pos[0]) + ' conn-fd: ' + str(conn.fileno()) + '\n')
      # Update the stub
    send_destinations(conn, dest_set)
    sys.stdout.write('Loop-end\n')

def get_next_destination():
  line = sys.stdin.readline()
  ip_list = line.split('>', 1)[1].strip().split('.', 4)
  result = ip_list[0] + '.' + ip_list[1] + '.' + ip_list[2] + '.' + ip_list[3]
  sys.stdout.write('dest: ' + result + '\n')
  return result

def receive_characteristic(conn):
  buffer = conn.recv(12)
  if len(buffer) == 12:
    dest = load_int(buffer[0:4])
    # TODO: Map real dest back into emulated dest
    command = load_int(buffer[4:8])
    value = load_int(buffer[8:12])
    if command == 1:
      # value is bandwidth in kbps
      set_bandwidth(value)
    elif command == 2:
      # value is delay in milliseconds
      set_delay(value)
    elif command == 3:
      # value is packet loss in packets per billion
      set_loss(value/1000000000.0)
    return True
  elif len(buffer) == 0:
    return False

def load_int(str):
  result = 0
  for i in range(4):
    result = result | ((ord(str[i]) & 0xff) << (8*i))
  return result

def set_bandwidth(kbps):
  sys.stdout.write('<event> bandwidth=' + str(kbps) + '\n')
  return set_link('bandwidth=' + str(kbps))

def set_delay(milliseconds):
  return set_link('delay=' + str(milliseconds) + 'ms')

def set_loss(probability):
  return set_link('plr=' + str(probability))

def set_link(ending):
  command_base = '/usr/testbed/bin/tevc -e tbres/pelab now link0 modify '
  return os.system(command_base + ending)

def send_destinations(conn, dest_set):
  sys.stdout.write('<send> ' + str(0) + ' ' + str(len(dest_set)) + ' -- '
                   + str(dest_set) + '\n')
  output = save_int(0) + save_int(len(dest_set))
  for dest in dest_set:
    real_dest = emulated_to_real[dest]
    ip_list = real_dest.split('.', 3)
    ip = 0
    for ip_byte in ip_list:
      ip = ip << 8
      ip = ip | (int(ip_byte, 10) & 0xff)
    output = output + save_int(ip)
  conn.sendall(output)

def save_int(number):
  result = ''
  for i in range(4):
    result = result + chr(number & 0xff)
    number = number >> 8
  return result


main_loop()
