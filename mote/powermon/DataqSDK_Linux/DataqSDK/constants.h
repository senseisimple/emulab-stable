/***************************************************************************
                          constants.h  -  Constants used for the entire
                                          library. All constants go here.
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
#ifndef MYCONSTANTS_H
#define MYCONSTANTS_H

#include <errno.h>
#include <sys/types.h>

#define DI194

/// Global error variable for the library.
extern int my_errno;
/// Integer to ASCII hex conversion table.
const u_int8_t asciihex[] = "0123456789ABCDEF";
/// ASCII hex to integer conversion table.
const u_int8_t inthex[128] =
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
 2, 3, 4, 5, 6, 7, 8, 9, 0, 0,
 0, 0, 0, 0, 0,10,11,12,13,14,
 15,0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0,10,11,12,
 13,14,15,0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0};

/// Max size for some string (just to have a max).
const int BIG_STR = 1024;
/// Max size for some small string.
const int SMALL_STR = 32;
/// Max path and filename length for a device file.
const int DEV_PATH = 64;
/**
* Max serial port buffer size in bytes, could be different if someone
* recompiled the kernel with a different size buffer.
*/
const int DI_SERIAL_BUFFER_SIZE = 4096;

/** @addtogroup SN_LENGTH
* Serial Number length, in characters.
* @{
*/
#ifdef DI194
const int DI194_SN_LENGTH = 10;
#endif
/** @} */
/// Key size, in characters.
#ifdef DI194
const int DI194_KEY_SIZE = 10;
#endif

/** @addtogroup CHANNELS
* Max channels per device.
* @{
*/
#ifdef DI194
const int DI194_CHANNELS = 5;
#endif
/** @} */

/** @addtogroup CHAN_SIZE
* Channel size (in bytes).
* @{
*/
#ifdef DI194
const int DI194_CHAN_SIZE = 2;
#endif
/** @} */

/** @addtogroup BURST_RATES
* Min/Max burst rates, throughput, min/max divisors.
* @{
*/
#ifdef DI194
const double DI194_MAXBURSTRATE = 240.00*4;
const double DI194_MINBURSTRATE = DI194_MAXBURSTRATE / 32767;
#endif
/** @} */

/** @addtogroup IOS_CODES
* IOS (Intelligent Over Sampling) Codes
* @{
*/
const u_int8_t IOS_SMALLEST = 0;
const u_int8_t IOS_AVERAGE = 0;
const u_int8_t IOS_MAX = 1;
const u_int8_t IOS_MIN = 2;
const u_int8_t IOS_RMS = 3;
const u_int8_t IOS_FREQ = 4;
const u_int8_t IOS_LAST_POINT = 5;
const u_int8_t IOS_GREATEST = 5;
/** @} */

/** @addtogroup ADDITIONAL_CODES
* Additional error codes defined here.
* @{
*/
const int EBOUNDS = 200;  ///< Argument out of bounds.
const int EBADRSVP = 201; ///< Device isn't responding properly.
/** @} */

#endif

