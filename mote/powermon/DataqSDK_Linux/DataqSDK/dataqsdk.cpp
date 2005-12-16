/***************************************************************************
                          dataqsdk.cpp  -  DataqSDK for Linux.
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

#include "dataqsdk.h"   // SDK interface file
#include <string>       // String functions include file
using namespace std;

int my_errno = 0;

// Supported device classes
#include "di194/di194.h"
#include "di154/di154.h"

dataqsdk::dataqsdk()
{
    m_ProductName = 0;
    m_last_error = 0;
    m_classID = 0;
}

dataqsdk::~dataqsdk()
{
  if(m_ProductName != 0)
  {
    delete [] m_ProductName;
    m_ProductName = 0;
  }
  if(m_classID != 0)
  {
    delete m_classID;
    m_classID = 0;
  }
}

// Get Properties

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @return Number of channels being scanned.
*/
const int dataqsdk::ADChannelCount()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->ADChannelCount();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const long int dataqsdk::ADCounter()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->ADCounter();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @pre Must be acquiring data.
* @return Number of data points in input buffer.
*/
const long int dataqsdk::AvailableData()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->AvailableData();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const long int dataqsdk::BurstCounter()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->BurstCounter();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @return Device file path and name used to connect to the device.
*/
const char *const dataqsdk::DeviceFile()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->DeviceFile();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @return Number of data points required before NewData() fires.
*/
const long int dataqsdk::EventPoint()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->EventPoint();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const int dataqsdk::InfoBoardID()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->InfoBoardID();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const bool dataqsdk::InfoPGL()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return false;
  }

  return m_classID->InfoPGL();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const int dataqsdk::InfoRev()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->InfoRev();
}

/**
* Gets serial number from device.
*
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @pre Device dependent.
* @return Pointer to array containing device serial number.
* @remark Calling function's responsibility to delete allocated memory.
*/
const char *const dataqsdk::InfoSerial()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->InfoSerial();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @return Actual max burst rate, after checks.
*/
const double dataqsdk::MaxBurstRate()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->MaxBurstRate();
}

/**
* Used to determine the device: properties, abilities, etc.\n
* If called before it's been set, it will return a list of devices
* that can be handled by the library, separated by the '\\0' character.
* The last entry is followed by two '\\0' characters.
*/
const char *const dataqsdk::ProductName()
{
  char *temp = 0;
  // pointer check
  if(m_ProductName == 0)  // no product loaded
  {
    // create a second memory location to hold the copy
    temp = new char[BIG_STR];
    int i=0;
    // return list of supported products
    strcpy(temp+i, "DI-194RS"); i += 9;
    strcpy(temp+i, "DI-154RS"); i += 9;
    // make sure to use double quotes around product name
    // to add other products, use this template:
    //strcpy(temp+i, [actual product name]);
    //i += [length of product name plus 1 for NULL];

    // need to have 2 '\\0' characters at the end (one is already there)
    temp[i] = 0;
  }
  else
  {
    // create a second memory location to hold the copy
    temp = new char[SMALL_STR];
    // copy string
    strcpy(temp, m_ProductName);
  }

  // return either a list of devices or current device
  return temp;
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @return Actual sample rate, after checks.
*/
const double dataqsdk::SampleRate()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->SampleRate();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const int dataqsdk::TrigHysteresisIdx()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->TrigHysteresisIdx();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const int dataqsdk::TrigLevel()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->TrigLevel();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const int dataqsdk::TrigMode()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->TrigMode();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const int dataqsdk::TrigScnChnIdx()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->TrigScnChnIdx();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const int dataqsdk::TrigSlope()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->TrigSlope();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const int dataqsdk::TrigPostLength()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->TrigPostLength();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const int dataqsdk::TrigPreLength()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->TrigPreLength();
}

// Set Properties

/**
* Overall number of channels to scan.
*
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @pre Device dependent.
* @param ChannelCount Number of channels to scan.
*/
void dataqsdk::ADChannelCount(const int ChannelCount)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->ADChannelCount(ChannelCount);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param Counter NOT FINISHED
*/
void dataqsdk::ADCounter(const long int Counter)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->ADCounter(Counter);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param BurstCounter NOT FINISHED
*/
void dataqsdk::BurstCounter(const long int BurstCounter)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->BurstCounter(BurstCounter);
}

/**
* Error Codes set:\n
* EINVAL = Bad parameter pointer.\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param DeviceFile Pointer to array containing device file path and name.
*/
void dataqsdk::DeviceFile(const char *const DeviceFile)
{
  // pointer check
  if(DeviceFile == 0)
  {
    m_last_error = EINVAL;
    return;
  }
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->DeviceFile(DeviceFile);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param EventPnt Number of data points required before NewData() fires.
* @remark Bounds between 0 and 32767. Set to extremes if out of bounds.
*/
void dataqsdk::EventPoint(const long int EventPnt)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->EventPoint(EventPnt);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param MaxBurstRt NOT FINISHED
*/
void dataqsdk::MaxBurstRate(const double MaxBurstRt)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->MaxBurstRate(MaxBurstRt);
}

/**
* Used to determine the device: properties, abilities, etc.
*
* Error Codes set:\n
* EINVAL = Bad parameter pointer.\n
* ENODEV = Requested device is not yet supported.
*
* @param ProductName Pointer to array containing unique name of product.
*/
void dataqsdk::ProductName(const char *const ProductName)
{
  // pointer check
  if(ProductName == 0)
  {
    m_last_error = EINVAL;
    return;
  }
  // pointer check
  if(m_ProductName == 0)
    m_ProductName = new char[SMALL_STR];

  // determine device
  if(strncmp(ProductName, "DI-194RS", SMALL_STR) == 0)
  {
    // put correct device name into variable
    strcpy(m_ProductName, "DI-194RS");
    if(m_classID != 0)
    {
      delete m_classID;
      m_classID = 0;
    }
    m_classID = new di194_dsdk;
  }
  else if(strncmp(ProductName, "DI-154RS", SMALL_STR) == 0)
  {
    // put correct device name into variable
    strcpy(m_ProductName, "DI-154RS");
    if(m_classID != 0)
    {
      delete m_classID;
      m_classID = 0;
    }
    m_classID = new di154_dsdk;
  }
  // other devices go here
  else  // no devices matched
  {
    m_last_error = ENODEV;
    return;
  }
}

/**
* Automatically sets the sample rate to min or max if requested value is
* out of bounds.
*
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @pre Device dependent.
* @param SampleRt Requested sample rate.
* @remark Bounds depend on the number of analog channels being scanned
* and the device.
*/
void dataqsdk::SampleRate(const double SampleRt)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->SampleRate(SampleRt);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param Hidx NOT FINISHED
*/
void dataqsdk::TrigHysteresisIdx(const int Hidx)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->TrigHysteresisIdx(Hidx);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param Level NOT FINISHED
*/
void dataqsdk::TrigLevel(const int Level)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->TrigLevel(Level);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param Mode NOT FINISHED
*/
void dataqsdk::TrigMode(const int Mode)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->TrigMode(Mode);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param SCidx NOT FINISHED
*/
void dataqsdk::TrigScnChnIdx(const int SCidx)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->TrigScnChnIdx(SCidx);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param Slope NOT FINISHED
*/
void dataqsdk::TrigSlope(const int Slope)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->TrigSlope(Slope);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param PostLength NOT FINISHED
*/
void dataqsdk::TrigPostLength(const int PostLength)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->TrigPostLength(PostLength);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param PreLength NOT FINISHED
*/
void dataqsdk::TrigPreLength(const int PreLength)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->TrigPreLength(PreLength);
}

// Methods

/**
* Uses list to map software channels to a different order than the physical
* channels on the device. Each index in the list represents the software
* channel (which is therefore zero based). The value at that index represents
* the physical channel (which is also zero based).
*
* Error Codes set:\n
* EINVAL = Bad parameter pointer.\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @pre Device dependent.
* @param ChannelList Pointer to array containing the channel list.
*/
void dataqsdk::ADChannelList(const int *const ChannelList)
{
  // pointer check
  if(ChannelList == 0)
  {
    m_last_error = EINVAL;
    return;
  }
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->ADChannelList(ChannelList);
}

/**
* Error Codes set:\n
* EINVAL = Bad parameter pointer.\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param DiffList NOT FINISHED
*/
void dataqsdk::ADDiffList(const int *const DiffList)
{
  // pointer check
  if(DiffList == 0)
  {
    m_last_error = EINVAL;
    return;
  }
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->ADDiffList(DiffList);
}

/**
* Error Codes set:\n
* EINVAL = Bad parameter pointer.\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param GainList NOT FINISHED
*/
void dataqsdk::ADGainList(const int *const GainList)
{
  // pointer check
  if(GainList == 0)
  {
    m_last_error = EINVAL;
    return;
  }
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->ADGainList(GainList);
}

/**
* The method list determines what IOS method to apply to each channel.
* The list's index represents the physical channel (not the software channel).
* The value at that index represents the IOS method.
*
* Error Codes set:\n
* EINVAL = Bad parameter pointer.\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @pre Device dependent.
* @param MethodList Pointer to array containing the IOS values to use.
*/
void dataqsdk::ADMethodList(const int *const MethodList)
{
  // pointer check
  if(MethodList == 0)
  {
    m_last_error = EINVAL;
    return;
  }
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->ADMethodList(MethodList);
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param value NOT FINISHED
* @param port NOT FINISHED
*/
void dataqsdk::DAOutput(const int value, const int port)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->DAOutput(value, port);
}

// attempts to detect any currently connected devices and returns a
// pointer to a string of ProductNames separated by the '\\0' character
// with the last entry followed by 2 '\\0' characters.
// will not detect devices that aren't automatically detected by the
// kernel (USB devices are most likely to be detected)
const char *const dataqsdk::DetectedDevices()
{
  m_last_error = ENOSYS;
/********************************************
            NOT YET IMPLEMENTED
********************************************/
  return 0;
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*/
const long int dataqsdk::DigitalInput()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return 0;
  }

  return m_classID->DigitalInput();
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param value NOT FINISHED
*/
void dataqsdk::DigitalOutput(const int value)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->DigitalOutput(value);
}

void dataqsdk::GetData()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->GetData();
}

/**
* Stores acquired data into the array provided. iArray must already have
* memory allocated to it. Count represents the number of data points (array
* elements) to store into iArray.
*
* The function will block until it either gets the requested data OR it
* times out. There is no overall timeout value, only a timeout between
* one good scan and the next. This means that the function could take
* different amounts of time to acquire data depending on the frequency
* of good scans.
*
* The function stores the data as 'Counts'. The values have the same range
* as a 'short int', hence the data type. A Count of zero means a Voltage of
* zero on the channel.
*
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @pre Bounds for Count between 1 and 32767.\n
* Valid iArray pointer.\n
* Acquiring.
* @param iArray Pointer to array which will hold the acquired data.
* @param Count Number of data points to store into array.
*/
void dataqsdk::GetDataEx(short int *iArray, const int Count)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->GetDataEx(iArray, Count);
}

void dataqsdk::GetDataFrame()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->GetDataFrame();
}

void dataqsdk::GetDataFrameEx(short int *iArray, const int Count)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->GetDataFrameEx(iArray, Count);
}

/**
* Tells device to start acquisition.
*
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @pre Not acquiring.\n
* Connected.
* @remark Sets flag that device is acquiring.
*/
void dataqsdk::Start()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->Start();
}

/**
* Tells device to stop acquisition.
*
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @pre Acquiring.\n
* Connected.
* @remark Sets acquiring flag.
*/
void dataqsdk::Stop()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return;
  }

  m_classID->Stop();
}


// "Event Occur" Methods

/**
* Gives priority to other internal library errors over interface class errors.
*
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param Code Will be changed to the value of the last error.
* @return False = No error.\n
* True = Error occurred. Check the parameter for the actual value.
*/
const bool dataqsdk::ControlError(long int &Code)
{
  // pointer check, causes error if no product specified
  if(m_classID == 0)
  {
    Code = ENODEV;
    m_last_error = 0;
    return true;
  }

  // give priority to errors from device
  if(m_classID->ControlError(Code))
  {
    m_last_error = 0; // reset this class' errors
    return true;
  }
  // this class' errors get reported second
  else if(m_last_error != 0)
  {
    Code = m_last_error;
    m_last_error = 0;
    return true;
  }

  return false;
}

/**
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @param Count Number of new data points.
* @return True = There is new data available.\n
* False = There isn't any new data available.
*/
const bool dataqsdk::NewData(long int &Count)
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return false;
  }

  return m_classID->NewData(Count);
}

/**
* This error may be misleading if someone recompiled their kernel with
* different serial buffer sizes.
*
* Error Codes set:\n
* ENODEV = No device specified.\n
* ENOSYS = Function not supported.\n
* Other codes are device dependent.
*
* @pre Connected.
* @return False = Everything is OK.\n
* True = A buffer overflowed.
*/
const bool dataqsdk::OverRun()
{
  // pointer check
  if(m_classID == 0)
  {
    m_last_error = ENODEV;
    return false;
  }

  return m_classID->OverRun();
}

/**
* Copy constructor.
* There's no way to implement bounds checks without knowing the device.
* If the copy is meant for a different device, properties are reset to
* that device's default when the ProductName changes anyway.
*
* @param copy Device to copy.
*/
dataqsdk::dataqsdk(const dataqsdk &copy)
{
}

