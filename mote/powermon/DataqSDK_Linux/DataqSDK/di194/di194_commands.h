/***************************************************************************
                          di194_commands.h  -  Commands that can be sent
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
#ifndef DI194_COMMANDS_H
#define DI194_COMMANDS_H 1

#include "../di_serial_io/di_serial_io.h"

/** @file di194_commands.h
* Special commands for the DI-194RS.
* All commands return true on success, false otherwise.
*/

/** @addtogroup DI194_COMMANDS
* @{
*/
/// Get serial
const bool Ncmd(di_serial_io &conn, char *sn);
/// Channels to scan
const bool Ccmd(di_serial_io &conn, const u_int8_t code=0x3F);
/// Digital channel
const bool Dcmd(di_serial_io &conn, const u_int8_t code=0);
/// Start/Stop acquisition
const bool Scmd(di_serial_io &conn, const u_int8_t code=0);
/** @} */

#endif

