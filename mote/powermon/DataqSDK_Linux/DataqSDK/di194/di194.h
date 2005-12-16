/***************************************************************************
                          di194.h  -  Linux driver for DI-194 series
                                      acquisition device manufactured by
                                      DATAQ Instruments, Inc.
                             -------------------
    begin                : Mon Jun 7 2004
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

#ifndef DI194_H
#define DI194_H 1

#include "../dsdk/dsdk.h"
#include "di194_commands.h"

/// Main device class for the DI-194RS.
class di194_dsdk : public dsdk
{
  public:
    di194_dsdk();  ///< Sets up device defaults for DI-194RS.
    ~di194_dsdk(); ///< Deletes any allocated memory.

    /** @addtogroup GetProperties
    * @{
    */
    /// Number of channels being scanned.
    const int ADChannelCount();
    /// Number of data points in input buffer.
    const long int AvailableData();
    /// Number of data points required before NewData() fires.
    const long int EventPoint();
    /// Device serial number.
    const char *const InfoSerial();
    /// Actual sample rate.
    const double SampleRate();
    /** @} */

    /** @addtogroup SetProperties
    * @{
    */
    /// Number of channels to scan.
    void ADChannelCount(const int ChannelCount);
    /// Number of data points required before NewData() fires.
    void EventPoint(const long int EventPnt);
    /// Requested sample rate.
    void SampleRate(const double SampleRt);
    /** @} */

    /** @addtogroup Methods
    * @{
    */
    /// Map software channels and physical channels.
    void ADChannelList(const int *const ChannelList);
    /// Change the IOS setting for each channel.
    void ADMethodList(const int *const MethodList);
    /// Activate device connection.
    void DeviceConnect();
    /// Safely deactivate device connection.
    void DeviceDisconnect();
    /// Get acquired data from device.
    void GetDataEx(short int *iArray, const int Count);
    /// Start acquisition.
    void Start();
    /// Stop acquisition.
    void Stop();
    /** @} */

    /** @addtogroup EventOccurMethods
    * Return true when the event occurs and update their arguments
    * with the proper value.
    * @{
    */
    /// Determines whether the input buffer OR the device buffer overflowed.
    const bool OverRun();
    /** @} */

  protected:
    /// Converts 'di_data' into counts.
    virtual const short int convert(const u_int8_t *const di_data,
                                    const u_int8_t num_chan);

    bool digital_chan; ///< Whether the digital channel is enabled.
    di_serial_io m_connection; ///< Connection to device.
    int chan_order[DI194_CHANNELS]; ///< Used by convert().

  private:
    /// Do not allow copying of this class.
    /**
    * @param copy Class to copy.
    */
    di194_dsdk(const di194_dsdk &copy):dsdk(){};
};

#endif

