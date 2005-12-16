/***************************************************************************
                          dsdk.h  -  Abstract base class for all devices.
                             -------------------
    begin                : Wed Jun 9 2004
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

#ifndef DSDK_H
#define DSDK_H

/// Abstract base class for DataqSDK library
/**
* General interface to be overided by every device class.
* Every device will have the same functions, but each implements
* them in their own specific way.
*/
class dsdk
{
  public:
    dsdk();  ///< Simple initialization.
    virtual ~dsdk(); ///< Most likely overwritten by superclass.

    /** @addtogroup GetProperties
    * @{
    */
    /// Number of channels being scanned.
    virtual const int ADChannelCount();
    virtual const long int ADCounter();
    /// Number of data points in input buffer.
    virtual const long int AvailableData();
    virtual const long int BurstCounter();
    /// Device file path and name used to connect to the device.
    virtual const char *const DeviceFile();
    /// Number of data points required before NewData() fires.
    virtual const long int EventPoint();
    /// The device's model number.
    virtual const int InfoBoardID();
    /// The device's input measurement setting.
    virtual const bool InfoPGL();
    /// The device's firmware revision.
    virtual const int InfoRev();
    /// The device's serial number.
    virtual const char *const InfoSerial();
    /// Maximum sampling rate of combined channels.
    virtual const double MaxBurstRate();
    /// Actual sample rate.
    virtual const double SampleRate();
    // predefined levels of limits around the TrigLevel to be passed
    // before the trigger goes off
    virtual const int TrigHysteresisIdx();
    // the trigger point (in counts) at which the trigger will go off
    virtual const int TrigLevel();
    virtual const int TrigMode();
    // trigger channel (zero-based)
    virtual const int TrigScnChnIdx();
    // 0 = trigger on the rising slope (negative)
    // 1 = trigger on the falling slope (positive)
    virtual const int TrigSlope();
    // number of scans to acquire after trigger occurs, and stop
    virtual const int TrigPostLength();
    // number of scans to acquire before trigger occurs, and stop
    virtual const int TrigPreLength();
    /** @} */

    /** @addtogroup SetProperties
    * @{
    */
    /// Number of channels to scan.
    virtual void ADChannelCount(const int ChannelCount);
    virtual void ADCounter(const long int Counter);
    virtual void BurstCounter(const long int BurstCounter);
    /// Device file path and name used to connect to the device.
    virtual void DeviceFile(const char *const DeviceFile);
    /// Number of data points required before NewData() fires.
    virtual void EventPoint(const long int EventPnt);
    /// Maximum sampling rate of combined channels.
    virtual void MaxBurstRate(const double MaxBurstRt);
    /// Requested sample rate.
    virtual void SampleRate(const double SampleRt);
    // predefined levels of limits around the TrigLevel to be passed
    // before the trigger goes off
    virtual void TrigHysteresisIdx(const int Hidx);
    // the trigger point (in counts) at which the trigger will go off
    virtual void TrigLevel(const int Level);
    virtual void TrigMode(const int Mode);
    // trigger channel (zero-based)
    virtual void TrigScnChnIdx(const int SCidx);
    // 0 = trigger on the rising slope (negative)
    // 1 = trigger on the falling slope (positive)
    virtual void TrigSlope(const int Slope);
    // number of scans to acquire after trigger occurs, and stop
    virtual void TrigPostLength(const int PostLength);
    // number of scans to acquire before trigger occurs, and stop
    virtual void TrigPreLength(const int PreLength);
    /** @} */

    /** @addtogroup Methods
    * @{
    */
    /// Map software channels and physical channels.
    virtual void ADChannelList(const int *const ChannelList);
    // array index corresponds to physical channel
    // (both are zero-based)
    virtual void ADDiffList(const int *const DiffList);
    // assigns the gain index to supporting devices
    // array index corresponds to physical channel
    // (both are zero-based)
    virtual void ADGainList(const int *const GainList);
    /// Change the IOS setting for each channel.
    virtual void ADMethodList(const int *const MethodList);
    // sends 'value' to a DAC 'port'
    virtual void DAOutput(const int value, const int port);
    virtual const long int DigitalInput();
    // sends 'value' to all digital ports
    // which port depends on the bit position in value
    // what value (0, 1) depends on the corresponding bit in 'value'
    virtual void DigitalOutput(const int value);
    virtual void GetData();
    /// Get acquired data from device.
    virtual void GetDataEx(short int *iArray, const int Count);
    virtual void GetDataFrame();
    virtual void GetDataFrameEx(short int *iArray, const int Count);
    /// Start acquisition.
    virtual void Start();
    /// Stop acquisition.
    virtual void Stop();
    /** @} */

    /** @addtogroup EventOccurMethods
    * Return true when the event occurs and update their arguments
    * with the proper value.
    * @{
    */
    /// Determines the last library error.
    virtual const bool ControlError(long int &Code);
    /// Determines whether new data is available according to the event point.
    virtual const bool NewData(long int &Count);
    /// Determines whether the input buffer OR the device buffer overflowed.
    virtual const bool OverRun();
    /** @} */

  protected:
    /// Activate device connection.
    virtual void DeviceConnect();
    /// Safely deactivate device connection.
    virtual void DeviceDisconnect();

    int m_ADChannelCount;
    long int m_ADCounter;
    long int m_BurstCounter;
    long int m_EventPoint;
    double m_MaxBurstRate;
    double m_SampleRate;
    int m_TrigHysteresisIdx;
    int m_TrigLevel;
    int m_TrigMode;
    int m_TrigScnChnIdx;
    int m_TrigSlope;
    int m_TrigPostLength;
    int m_TrigPreLength;

    int *m_ADChannelList;
    int *m_ADDiffList;
    int *m_ADGainList;
    int *m_ADMethodList;

    char *m_device_file;  ///< Path & name of device file.
    bool m_acquiring_data;  ///< True when acquiring data.
    long int m_last_error; ///< Keep track of last error code.

  private:
    /// Do not allow copying of this class.
    dsdk(const dsdk &copy){};
};

#endif

