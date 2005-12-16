/***************************************************************************
                          di154.h  -  Linux driver for DI-154 series
                                      acquisition device manufactured by
                                      DATAQ Instruments, Inc.
                             -------------------
    begin                : Mon Aug 2 2004
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

#ifndef DI154_H
#define DI154_H 1

#include "../di194/di194.h"

/// Main device class for the DI-154RS.
class di154_dsdk : public di194_dsdk
{
  public:
    di154_dsdk();  ///< Sets up device defaults for DI-154RS.

  private:
    /// Do not allow copying of this class.
    /**
    * @param copy Class to copy.
    */
    di154_dsdk(const di154_dsdk &copy){};

    /// Converts 'di_data' into counts.
    const short int convert(const u_int8_t *const di_data,
                            const u_int8_t num_chan);
};

#endif

