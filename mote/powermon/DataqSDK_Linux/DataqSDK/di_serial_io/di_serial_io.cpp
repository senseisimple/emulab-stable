/***************************************************************************
              di_serial_io.cpp  -  Functions that deal with input/output
                                   to many serial devices.
                             -------------------
    begin                : Wed Jun 23 2004
    author               : Ioan S. Popescu

Copyright (C) 2004, 2005 DATAQ Instruments, Inc. <develop@dataq.com>

This program is free software; you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation; either 
version 2 of the License, or (at your option) any later 
version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ***************************************************************************/

//FOR CYGWIN!!!!
//#include "/usr/include/w32api/winsock.h"
//END FOR CYGWIN

#include "di_serial_io.h"

#include <string.h>
#include <unistd.h>   // UNIX standard function definitions
#include <sys/ioctl.h>// IO port control function definitions
#include <time.h>     // Time function definitions
#include <fcntl.h>    // File control definitions

di_serial_io::di_serial_io()
{
  m_comm_fd = -1;
  m_baudrate = B4800;
  m_old_tiocm = 0;
  m_timeout = 2;
}

di_serial_io::~di_serial_io()
{
  if(m_comm_fd != -1)
    disconnect();
}

/**
* Returns error codes of internal functions it calls.
*
* @param dev_file Device file path and name.
* @param device Device code for specific settings. See file comment.
* @pre Inactive connection. Does not check!
* @post Connection to device file, port set up.
* @remark This only applies settings, nothing else. It doesn't lock
* the device file or check if it is locked. It quits
* on any errors.
* @return Errors returned by open()\n
* Errors returned by tcgetattr()\n
* Errors returned by cfsetispeed()\n
* Errors returned by cfsetospeed()\n
* Errors returned by tcsetattr()\n
* 0 = No error.
*/
const u_int16_t di_serial_io::connect(const char *const dev_file, const u_int8_t device)
{
  my_errno = 0;
  // open the serial port
  m_comm_fd = open(dev_file, O_RDWR | O_NOCTTY | O_NDELAY);
  if (m_comm_fd == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as open()
  }
  else
    fcntl(m_comm_fd, F_SETFL, FNDELAY); // set to non-blocking

  struct termios serial_opts; // will hold new serial port settings

  // flush send/receive buffers so settings will be set properly
  flush_receive();
  tcflush(m_comm_fd, TCOFLUSH);

  // get copy of current serial port settings to restore later
  if(tcgetattr(m_comm_fd, &m_old_termios) == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as tcgetattr()
  }

  if (!strncmp(dev_file, "/dev/tip", 8)) {
      return 0;
  }
  
  // get copy of other current serial port settings to restore later
  if(ioctl(m_comm_fd, TIOCMGET, &m_old_tiocm) == -1)
  {
    my_errno = errno;
    return my_errno;
  }

  // get current serial port settings (must modify existing settings)
  if(tcgetattr(m_comm_fd, &serial_opts) == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as tcgetattr()
  }

  // clear out settings
  serial_opts.c_cflag = 0;
  serial_opts.c_iflag = 0;
  serial_opts.c_lflag = 0;
  serial_opts.c_oflag = 0;

  // set baud rate
  if(cfsetispeed(&serial_opts, m_baudrate) == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as cfsetispeed()
  }
  if(cfsetospeed(&serial_opts, m_baudrate) == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as cfsetospeed()
  }

  // no parity
  serial_opts.c_cflag &= ~PARENB;
  serial_opts.c_iflag &= ~INPCK;

  // apply settings and check for error
  // this is done because tcsetattr() would return success if *any*
  // settings were set correctly; this way, more error checking is done
  if(tcsetattr(m_comm_fd, TCSANOW, &serial_opts) == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as tcsetattr()
  }

  serial_opts.c_cflag &= ~CSTOPB; // 1 stop bit
  serial_opts.c_cflag &= ~CSIZE;  // reset byte size
  serial_opts.c_cflag |= CS8;     // 8 bits

  // apply settings and check for error
  if(tcsetattr(m_comm_fd, TCSANOW, &serial_opts) == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as tcsetattr()
  }

  // disable hardware flow control
  serial_opts.c_cflag &= ~CRTSCTS;

  // apply settings and check for error
  if(tcsetattr(m_comm_fd, TCSANOW, &serial_opts) == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as tcsetattr()
  }

  // disable software flow control
  serial_opts.c_iflag &= ~(IXON | IXOFF | IXANY);

  // apply settings and check for error
  if(tcsetattr(m_comm_fd, TCSANOW, &serial_opts) == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as tcsetattr()
  }

  // raw I/O
  serial_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  serial_opts.c_oflag &= ~OPOST;

  // apply settings and check for error
  if(tcsetattr(m_comm_fd, TCSANOW, &serial_opts) == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as tcsetattr()
  }

  // timeouts
  serial_opts.c_cc[VMIN] = 0;
  serial_opts.c_cc[VTIME] = 10;

  // apply settings and check for error
  if(tcsetattr(m_comm_fd, TCSANOW, &serial_opts) == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as tcsetattr()
  }

  // misc. settings
  serial_opts.c_cflag |= HUPCL;
  serial_opts.c_cflag |= (CLOCAL | CREAD);

  // apply settings and check for error
  if(tcsetattr(m_comm_fd, TCSANOW, &serial_opts) == -1)
  {
    my_errno = errno;
    return my_errno; // same error codes as tcsetattr()
  }

  int status = m_old_tiocm; // get settings
  switch(device)
  {
    default:
      status |= TIOCM_DTR;  // turn on DTR and...
      status |= TIOCM_RTS;  // ...RTS lines to power up device
      // apply settings
      if(ioctl(m_comm_fd, TIOCMSET, &status) == -1)
      {
        my_errno = errno;
        return my_errno;
      }

      // wait for device to power up
      usleep(100000);
  }

  // flush send/receive buffers so settings will be set properly
  flush_receive();
  tcflush(m_comm_fd, TCOFLUSH);

  return 0;
}

/**
* Disconnects at all costs. Returns error codes of internal functions.
*
* @pre Connected. Does not check!
* @post Not connected.
* @remark This function attempts to reset the serial port settings.
* Calling it before connect() will result in undefined settings.
* @return Errors returned by close().\n
* 0 = No error.
*/
const u_int16_t di_serial_io::disconnect()
{
  my_errno = 0;
  // flush send/receive buffers so settings will be set properly
  flush_receive();
  tcflush(m_comm_fd, TCOFLUSH);

  // try to reset serial port settings back to original
  tcsetattr(m_comm_fd, TCSANOW, &m_old_termios);
  // try to reset other serial port settings back to original
  ioctl(m_comm_fd, TIOCMSET, &m_old_tiocm);

  // try to close serial port
  if(close(m_comm_fd) == -1)
  {
    my_errno = errno;
    return my_errno;
  }
  m_comm_fd = -1;
  return 0;
}

/**
* @return True = Connected.\n
* False = Not connected.
*/
const bool di_serial_io::is_comm_open()
{
  if(m_comm_fd == -1)
    return false;
  else
    return true;
}

/**
* Return error codes of internal functions. Function will block until either
* enough data was read or it timed out, according to m_timeout.
*
* @param data The data read.
* @param amount The number of bytes to try and read.
* @param packet_len Length of normal packet/scan (bytes). If this value is zero,
* validation is not performed.
* @pre Connected. Does not check!
* @return True = Error occurred.\n
* False = No error.
*
* Error codes set:\n
* ENODATA = Not enough data available.\n
* Errors returned by read().\n
* ETIMEDOUT = Timed out before getting requested amount.\n
* Errors set by di_serial_io::di_synchronize().\n
* 0 = No error.
*/
const bool di_serial_io::di_read(u_int8_t *data,
                                 u_int16_t &amount,
                                 const u_int8_t packet_len)
{
  my_errno = 0;
  int16_t available = 0;
  u_int8_t retries = 4;
  time_t time_start = time(NULL);

  ioctl(m_comm_fd, FIONREAD, &available);
  // wait for data
  while((time(NULL) - time_start < m_timeout) &&  // stop when timeout reached
        (available < amount))                      // or when enough data
  {
    usleep(50);
    ioctl(m_comm_fd, FIONREAD, &available);       // is available
  }

  // timed out without any data
  if(available == 0)
  {
    my_errno = ENODATA;
    return true;
  }
  // timed out and not enough data
  if(time(NULL) - time_start >= m_timeout && available < amount)
  {
    // read as much as is available
    if(read(m_comm_fd, data, available) == -1)
    {
      my_errno = errno;
      return true;
    }

    // let caller know how much was read
    amount = available;
    my_errno = ETIMEDOUT;
    return true;
  }

  // read the amount requested
  if(read(m_comm_fd, data, amount) == -1)
  {
    my_errno = errno;
    return true;
  }

  // validate if requested
  if(packet_len > 0)
  {
    time_start = time(NULL);
    while(retries-- > 0 && !di_valid(data, packet_len))
    {
      // inside this loop means previously read data was invalid
      if(!di_synchronize(data, amount, packet_len))
      {
        return true;
      }
    }
  }

  return false;  // success
}

/**
* Flushes output buffer before every command. Blocks slightly if device
* signals its receive buffer is full.
*
* @pre Connected. Does not check!
* @param data Pointer to array of data to send.
* @param amount Number of bytes to send.
* @param echo Number of bytes to check for echo (except NULL).
* @param retry Number of times to try checking echo. Value of zero
* will prevent the entire send from occurring.
* @return True = Error occurred.\n
* False = No error.
*
* Error codes set:\n
* Errors set by write().\n
* EBADRSVP = Device not responding properly. Bad echo.\n
* 0 = No error.
*/
const bool di_serial_io::di_send(const u_int8_t *const data,
                                 u_int16_t &amount,
                                 const u_int8_t echo,
                                 const u_int8_t retry)
{
  my_errno = 0;
  u_int8_t tries = retry;
  int write_return = 0;

  while(tries-- > 0)
  {
    // flush send buffer to make sure command gets sent
    tcflush(m_comm_fd, TCOFLUSH);
    if(echo > 0)
      // clear receive buffer if checking echo
      flush_receive();

    // try to send all of it at once
    write_return = write(m_comm_fd, data, amount);
    if(write_return <= 0)
    {
      my_errno = errno;
      return true;  // error in send
    }

    // send whatever's left, if need to (one byte at a time)
    for(u_int16_t i = static_cast<u_int16_t>(write_return); i < amount; i++)
    {
      if(write(m_comm_fd, data + i, 1) <= 0)
      {
        my_errno = errno;
        return true;  // error in send
      }
    }

    if(echo > 0)  // check for at least 1 echo
    {
      // check if echo matches
      my_errno = 0;
      if(!di_echo(data, echo))
      {
        my_errno = EBADRSVP;
        // wait a little
        usleep(500000);
        continue;  // didn't match, try again
      }
      else if(my_errno != 0)
      {
        return true; // error in read
      }
    }

    my_errno = 0;
    return false; // data sent successfully
  }

  return true;
}

/**
* @pre Connected. Does not check!
* @return Number of bytes in the receive buffer.
*/
const int16_t di_serial_io::bytes_in_receive()
{
  int bytes = 0;
  
  ioctl(m_comm_fd, FIONREAD, &bytes);
  return bytes;
}

/**
* @pre Connected. Does not check!
* @remark No error checking.
*/
void di_serial_io::flush_receive()
{
  time_t time_start = time(NULL);

  while(bytes_in_receive() > 0 && (time(NULL) - time_start < 2))
  {
    // flush receive buffers to clear them
    tcflush(m_comm_fd, TCIFLUSH);
    usleep(50);
  }

  return;
}

/**
* Current signature format is as follows:\n
* 0 = Last bit of first byte in 'packet'\n
* 1 = Last bit of remaining bytes in 'packet'\n
*
* @param data The 'packet' to test.
* @param packet_len Normal packet size.
* @return True = 'Packet' is valid.\n
* False = Failed validation test.
*/
const bool di_serial_io::di_valid(const u_int8_t *const data,
                                  const u_int8_t packet_len)
{
  // byte 1 has zero at end
  if((data[0] & 0x1) != 0)
    return false;

  // remaining bytes have one at end
  for(int i = 1; i < packet_len; i++)
  {
    if((data[i] & 0x1) != 1)
      return false;
  }

  return true;
}

/**
* Attempts to synchronize the input buffer to the beginning of a packet,
* it stores what it thinks is a packet in 'data' while at it. Uses the
* read function without any validation checking.
*
* @pre Connected. Does not check!
* @param data Pointer to array where to store next 'packet'.
* @param data_len Length of data.
* @param packet_len Length of a normal packet/scan (bytes).
* @return False = Synchronized successfully.\n
* True = Error occurred.
*/
const bool di_serial_io::di_synchronize(u_int8_t *data,
                                             const u_int8_t data_len,
                                             const u_int8_t packet_len)
{
  u_int8_t i = 0, j = 0;

  // look for the trailing zero bit in current data
  for(i = 0; i < data_len && (data[i] & 0x1) != 0; i++);

  // copy that portion to beginning of data
  for(j = 0; i < data_len; j++, i++)
    data[j] = data[i];

  // read what is assumed to be the remainder of the packet
  u_int16_t amount = packet_len - j;
  if(di_read(data + j, amount, 0))
    return true;

  return false; // Success.
}

/**
* Checks if 'amount' bytes of 'expected' are echoed by the device.
* The function automatically excludes the beginning NULL.
*
* @pre Connected. Does not check!
* @remark The beginning NULL must be in expected.
* @param expected Pointer to array of expected data.
* @param amount Number of bytes to check.
* @return True = Echo matched.\n
* False = Echo did not match.\n
*/
const bool di_serial_io::di_echo(const u_int8_t *const expected,
                                    const u_int8_t amount)
{
  u_int16_t bytes = amount;
  u_int8_t *buf = new u_int8_t[bytes];

  if(di_read(buf, bytes, 0)) // read data
  {
    delete [] buf;
    return false;
  }
  for(unsigned int i = 1; i <= bytes; i++)  // try to match data
  {
  // need to shift expected because NULL isn't echoed
    if(expected[i] != buf[i-1])
    {
      delete [] buf;
      return false; // mismatch found
    }
  }
  delete [] buf;
  return true;  // match succeeded
}

