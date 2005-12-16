/***************************************************************************
                          di154.cpp  -  Linux driver for DI-154 series
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

#include "di154.h"

/**
* - Sets up the channel order array (1 channel)
* - Sets channel count to use 1 analog channel
* - Sets up 'connection' to use the DI-154RS settings
* - Creates the following lists:
*  - m_ADChannelList\n
*    Normal order
*  - m_ADMethodList\n
*    IOS Average
* - Disables the digital channel
* - Calls sample rate function passing it the default sample rate,
*   as defined in the dsdk
*/
di154_dsdk::di154_dsdk()
{
  chan_order[0] = 0;
  m_ADChannelCount = 1;
  // create new lists
  m_ADChannelList = new int[m_ADChannelCount];
  m_ADMethodList = new int[m_ADChannelCount];
  for(int i=0; i<m_ADChannelCount; i++)
  {
    m_ADChannelList[i] = i;
    m_ADMethodList[i] = IOS_AVERAGE;
    chan_order[i] = i * DI194_CHAN_SIZE;
  }

  digital_chan = false;

  m_MaxBurstRate = 240.00;
  // set to default sample rate based on channels activated
  SampleRate(m_SampleRate);
}

/**
* Converts 'di_data' into Counts.
*
* @param di_data Pointer to array of raw data.
* @param num_chan Channel list position to check for analog or digital.
*/
const short int di154_dsdk::convert(const u_int8_t *const di_data,
                                    const u_int8_t num_chan)
{
  short int temp = 0;
  // extract and convert to counts
  // find out whether it's an analog or digital channel
  if(m_ADChannelList[num_chan] != -1) // analog channel
  {
    temp = static_cast<short int>(di_data[chan_order[m_ADChannelList[num_chan]]]
                   & 0xF7) >> 3;
    temp |= (static_cast<short int>(di_data[chan_order[m_ADChannelList[num_chan]]+1]
                   & 0xFE) << 4);
    temp <<= 4; // left justify the number
    temp ^= 0x8000;
  }
  else // digital bits
    temp = (di_data[0] & 0x06) >> 1;

  return temp;
}

