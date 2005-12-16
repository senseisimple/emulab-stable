/***************************************************************************
                          di194_commands.cpp  -  Commands that can be sent
                                                 to a DI-194 device.
                             -------------------
    begin                : Thu Jun 3 2004
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
#include "di194_commands.h"
#include <unistd.h>

/* file di194_commands.cpp
* Special commands for the DI-194RS. All commands return true on success,
* false otherwise.
*/

/** @addtogroup DI194_COMMANDS
* @{
*/
/**
* @pre Connection object is set up and connected. Does not check!
* @param conn Serial connection.
* @param sn Pointer to an array that will hold the serial number.
* @return True = Successful.\n
* False = Failure.
*/
const bool Ncmd(di_serial_io &conn, char *sn)
{
  u_int8_t data[3] = {0, 'N', 'Z'};  // data to send
  for(int i = 0; i < DI194_SN_LENGTH; i++)
    sn[i] = 0;  // will hold serial number, blank for now
  u_int16_t amount = 3;

  if(conn.di_send(&data[0], amount)) // send data
    return false; // error in send
  amount = DI194_SN_LENGTH;
  // read serial number
  if(conn.di_read(reinterpret_cast<u_int8_t *>(sn), amount, 0))
    return false; // error in read

  return true; // no error in read
}

/**
* Channels 0000 to 1111. Each bit represents the physical channel to enable.
* \n Dec. value -  channel
* \n 1          -  analog channel 1
* \n 2          -  analog channel 2
* \n 4          -  analog channel 3
* \n 8          -  analog channel 4
* \n add together to get any combination of channels
* \n     OR
* \n each bit is a channel: 4 (left) to 1 (right) using above scheme
* \n channel 1, binary: 0001     (decimal: 1)
* \n channel 2,3 binary: 0110    (decimal: 6)
*
* @pre Connection object is set up and connected. Does not check!
* @param conn Serial connection.
* @param code The channel code to send (integer).
* @return True = Successful.\n
* False = Failure.
*/
const bool Ccmd(di_serial_io &conn, const u_int8_t code)
{
  u_int8_t data[3] = {0, 'C'};
  data[2] = code; // data to send
  if(data[2] > 0xF)   // bounds check
    data[2] = 0;
  u_int16_t amount = 3;

  if(conn.di_send(&data[0], amount)) // send data
    return false; // error in send

  return true;
}

/**
* Code values:\n
* 0 = Output square wave on digital channels.\n
* 1 = Input digital channels.\n
*
* @pre Connection object is set up and connected. Does not check!
* @param conn Serial connection.
* @param code The digital channel code to send (integer).
* @return True = Successful.\n
* False = Failure.
*/
const bool Dcmd(di_serial_io &conn, const u_int8_t code)
{
  u_int8_t data[3] = {0, 'D'};
  data[2] = code+48; // data to send
  if(data[2] != '1' && data[2] != '0')  // bounds check
    data[2] = '1';
  u_int16_t amount = 3;

  if(conn.di_send(&data[0], amount)) // send data
    return false; // error in send

  return true;
}

/**
* Data aqcuisition Start/Stop.\n
* Start = 1.\n
* Stop  = 0.\n
*
* @pre Connection object is set up and connected. Does not check!
* @param conn Serial connection.
* @param code The code (integer).
* @return True = Successful.\n
* False = Failure.
*/
const bool Scmd(di_serial_io &conn, const u_int8_t code)
{
  u_int8_t data[3] = {0, 'S'};
  data[2] = code+48; // data to send
  if(data[2] != '1' && data[2] != '0')  // bounds check
    data[2] = '0';
  u_int16_t amount = 3;

  if(data[2] == '1')
  {
    // make sure device starts
    if(conn.di_send(&data[0], amount, 2, 1))
    {
      if(my_errno == EBADRSVP)
      {
        // didn't work first time, stop and try again
        data[2] = '0';
        if(conn.di_send(&data[0], amount, 2, 1))
        {
          // must be an error
          return false;
        }
        data[2] = code + 48;
        if(conn.di_send(&data[0], amount, 2, 1))
        { // must be an error
          return false;
        }
      }
      else
      {
        return false;
      }
    }
  }
  else
  {
    // get device to stop first
    if(!conn.di_send(&data[0], amount, 2, 1))
      return true;

    // send command again
    if(conn.di_send(&data[0], amount, 2))
      return false;
  }

  return true;
}
/** @} */

