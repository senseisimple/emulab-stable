/***************************************************************************
                di_serial_io.h  -  Functions that deal with input/output
                                   to many serial devices.
                             -------------------
    begin                : Wed Jun 23 2004
    author               : Ioan S. Popescu

Copyright (C) 2004 DATAQ Instruments, Inc. <develop@dataq.com>

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

#ifndef DI_SERIAL_IO_H
#define DI_SERIAL_IO_H

#include <sys/termios.h>  // POSIX terminal control definitions
#include "../constants.h"
#include <sys/types.h>

/** @file di_serial_io.h
* Serial type connections class.
*
* Device Codes (used with connect() ):\n
* 01 = DI-194RS
*/

/// Class to handle serial type connections
/**
* This class handles setting up the port settings, connecting to the device,
* providing validated reads, send out data, open status, available data
* points, and some device controlled errors.
*/
class di_serial_io
{
  public:
    di_serial_io();  ///< Simple initialization.
    ~di_serial_io();  ///< Close connection.

    /// Connect to device and set up serial port settings.
    const u_int16_t connect(const char *const dev_file, const u_int8_t device);
    /// Restore original serial port settings and disconnect from device.
    const u_int16_t disconnect();
    /// Determines whether the serial connection is active.
    const bool is_comm_open();
    /// Read data from serial connection, validate if requested.
    const bool di_read(u_int8_t *data,
                       u_int16_t &amount,
                       const u_int8_t packet_len=2); // if zero, no validation
    /// Send data to device.
    const bool di_send(const u_int8_t *const data,
                       u_int16_t &amount,
                       const u_int8_t echo=2,
                       const u_int8_t retry=4);
    /// Returns the number of bytes in the input buffer.
    const int16_t bytes_in_receive();
    /// Clears input buffers.
    void flush_receive();

  private:
    /// True when packet matches DI signature.
    const bool di_valid(const u_int8_t *const data, const u_int8_t packet_len);
    /// Attempts to synchronize the input buffer to the beginning of a packet.
    const bool di_synchronize(u_int8_t *data,
                              const u_int8_t data_len,
                              const u_int8_t packet_len);
    /// Returns true if the 'expected' data is echoed (except NULL).
    const bool di_echo(const u_int8_t *const expected,
                       const u_int8_t amount);

    unsigned int m_baudrate;  ///< Connection baudrate.
    int m_comm_fd;  ///< Serial port file descriptor.
    struct termios m_old_termios;  ///< Original serial port settings.
    int m_old_tiocm; ///< Other original settings.
    u_int8_t m_timeout; ///< Timeout in seconds.
};

#endif

