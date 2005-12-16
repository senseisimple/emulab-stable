/***************************************************************************
                          dataqsdk.h  -  DataqSDK for Linux, include file.
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

#ifndef DATAQSDK_H
#define DATAQSDK_H

class dsdk;

/// DataqSDK library interface class.
class dataqsdk
{
  public:
    dataqsdk();  ///< Simple initialization.
    ~dataqsdk(); ///< Deletes allocated memory.

    /** @addtogroup GetProperties
    * @{
    */
    /// Number of channels being scanned.
    const int ADChannelCount();
    const long int ADCounter();
    /// Number of data points in input buffer.
    const long int AvailableData();
    const long int BurstCounter();
    /// Device file path and name used to connect to the device.
    const char *const DeviceFile();
    /// Number of data points required before NewData() fires.
    const long int EventPoint();
    /// The device's model number.
    const int InfoBoardID();
    /// The device's input measurement setting.
    // true = PGH, false = PGL
    const bool InfoPGL();
    /// The device's firmware revision.
    const int InfoRev();
    /// The device's serial number.
    const char *const InfoSerial();
    /// Maximum sampling rate of combined channels.
    // (# channels) * (SampleRate)
    const double MaxBurstRate();
    // used to determine the device: properties, abilities, etc.
    // if called before it's been set, it will return a list of devices,
    // that can be handled by the library, separated by the '\0' character;
    // the last entry is followed by 2 '\0' characters
    const char *const ProductName();
    /// Actual sample rate.
    const double SampleRate();
    // predefined levels of limits around the TrigLevel to be passed
    // before the trigger goes off
    const int TrigHysteresisIdx();
    // the trigger point (in counts) at which the trigger will go off
    const int TrigLevel();
    const int TrigMode();
    // trigger channel (zero-based)
    const int TrigScnChnIdx();
    // 0 = trigger on the rising slope (negative)
    // 1 = trigger on the falling slope (positive)
    const int TrigSlope();
    // number of scans to acquire after trigger occurs, and stop
    const int TrigPostLength();
    // number of scans to acquire before trigger occurs, and stop
    const int TrigPreLength();
    /** @} */

    /** @addtogroup SetProperties
    * @{
    */
    /// Number of channels to scan.
    void ADChannelCount(const int ChannelCount);
    void ADCounter(const long int Counter);
    void BurstCounter(const long int BurstCounter);
    /// Device file path and name used to connect to the device.
    void DeviceFile(const char *const DeviceFile);
    /// Number of data points required before NewData() fires.
    void EventPoint(const long int EventPnt);
    /// Maximum sampling rate of combined channels.
    // (# channels) * (SampleRate)
    void MaxBurstRate(const double MaxBurstRt);
    // used to determine the device: properties, abilities, etc.
    void ProductName(const char *const ProductName);
    /// Requested sample rate.
    void SampleRate(const double SampleRt);
    // predefined levels of limits around the TrigLevel to be passed
    // before the trigger goes off
    void TrigHysteresisIdx(const int Hidx);
    // the trigger point (in counts) at which the trigger will go off
    void TrigLevel(const int Level);
    void TrigMode(const int Mode);
    // trigger channel (zero-based)
    void TrigScnChnIdx(const int SCidx);
    // 0 = trigger on the rising slope (negative)
    // 1 = trigger on the falling slope (positive)
    void TrigSlope(const int Slope);
    // number of scans to acquire after trigger occurs, and stop
    void TrigPostLength(const int PostLength);
    // number of scans to acquire before trigger occurs, and stop
    void TrigPreLength(const int PreLength);
    /** @} */

    /** @addtogroup Methods
    * @{
    */
    /// Map software channels and physical channels.
    void ADChannelList(const int *const ChannelList);
    // array index corresponds to physical channel (zero-based)
    void ADDiffList(const int *const DiffList);
    // assigns the gain index to supporting devices
    // array index corresponds to physical channel (zero-based)
    void ADGainList(const int *const GainList);
    /// Change the IOS setting for each channel.
    void ADMethodList(const int *const MethodList);
    // sends 'value' to a DAC 'port'
    void DAOutput(const int value, const int port);
    // attempts to detect any currently connected devices and returns a
    // pointer to a string of ProductIDs separated by the '\0' character
    // with the last entry followed by 2 '\0' characters.
    // will not detect devices that aren't automatically detected by the
    // kernel (USB devices are most likely to be detected)
    const char *const DetectedDevices();
    const long int DigitalInput();
    // sends 'value' to all digital ports
    // which port depends on the bit position in value
    // what value (0, 1) depends on the corresponding bit in 'value'
    void DigitalOutput(const int value);
    void GetData();
    /// Get acquired data from device.
    void GetDataEx(short int *iArray, const int Count);
    void GetDataFrame();
    void GetDataFrameEx(short int *iArray, const int Count);
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
    /// Determines the last library error.
    const bool ControlError(long int &Code);
    /// Determines whether new data is available according to the event point.
    const bool NewData(long int &Count);
    /// Determines whether the input buffer OR the device buffer overflowed.
    const bool OverRun();
    /** @} */

  private:
    /// Do not allow copying of this class.
    dataqsdk(const dataqsdk &copy);

    char *m_ProductName; ///< Identifies current device.
    int m_last_error; ///< Interface class internal error variable.
    dsdk *m_classID; ///< Pointer to appropriate device handling code.
};

#endif

