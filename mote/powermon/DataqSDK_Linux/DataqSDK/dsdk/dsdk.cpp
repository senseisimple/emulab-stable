/***************************************************************************
                          dsdk.cpp  -  Abstract base class for all devices
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

#include "dsdk.h"
#include "../constants.h"  // global constants used by all of DataqSDK

dsdk::dsdk()
{
    m_ADChannelCount = 1;
    m_ADCounter = 0; // check formula
    m_BurstCounter = 1;  // check formula
    m_EventPoint = 0;
    m_MaxBurstRate = 1000.00;
    m_SampleRate = 1000.00;
    m_TrigHysteresisIdx = 0;
    m_TrigLevel = 0;
    m_TrigMode = 0;
    m_TrigScnChnIdx = 0;
    m_TrigSlope = 0;
    m_TrigPostLength = 100;
    m_TrigPreLength = 100;

    m_ADChannelList = 0;
    m_ADDiffList = 0;
    m_ADGainList = 0;
    m_ADMethodList = 0;

    m_device_file = 0;
    m_acquiring_data = false;
    m_last_error = 0;
}

dsdk::~dsdk()
{
  if(m_ADChannelList != 0)
  {
    delete [] m_ADChannelList;
    m_ADChannelList = 0;
  }
  if(m_ADDiffList != 0)
  {
    delete [] m_ADDiffList;
    m_ADDiffList = 0;
  }
  if(m_ADGainList != 0)
  {
    delete [] m_ADGainList;
    m_ADGainList = 0;
  }
  if(m_ADMethodList != 0)
  {
    delete [] m_ADMethodList;
    m_ADMethodList = 0;
  }
  if(m_device_file != 0)
  {
    delete [] m_device_file;
    m_device_file = 0;
  }
}

// Get Properties

const int dsdk::ADChannelCount()
{
  m_last_error = ENOSYS;
  return 0;
}

const long int dsdk::ADCounter()
{
  m_last_error = ENOSYS;
  return 0;
}

const long int dsdk::AvailableData()
{
  m_last_error = ENOSYS;
  return 0;
}

const long int dsdk::BurstCounter()
{
  m_last_error = ENOSYS;
  return 0;
}

const char *const dsdk::DeviceFile()
{
  return m_device_file;
}

const long int dsdk::EventPoint()
{
  m_last_error = ENOSYS;
  return 0;
}

const int dsdk::InfoBoardID()
{
  m_last_error = ENOSYS;
  return 0;
}

const bool dsdk::InfoPGL()
{
  m_last_error = ENOSYS;
  return false;
}

const int dsdk::InfoRev()
{
  m_last_error = ENOSYS;
  return 0;
}

const char *const dsdk::InfoSerial()
{
  m_last_error = ENOSYS;
  return 0;
}

const double dsdk::MaxBurstRate()
{
  m_last_error = ENOSYS;
  return 0;
}

const double dsdk::SampleRate()
{
  m_last_error = ENOSYS;
  return 0;
}

const int dsdk::TrigHysteresisIdx()
{
  m_last_error = ENOSYS;
  return 0;
}

const int dsdk::TrigLevel()
{
  m_last_error = ENOSYS;
  return 0;
}

const int dsdk::TrigMode()
{
  m_last_error = ENOSYS;
  return 0;
}

const int dsdk::TrigScnChnIdx()
{
  m_last_error = ENOSYS;
  return 0;
}

const int dsdk::TrigSlope()
{
  m_last_error = ENOSYS;
  return 0;
}

const int dsdk::TrigPostLength()
{
  m_last_error = ENOSYS;
  return 0;
}

const int dsdk::TrigPreLength()
{
  m_last_error = ENOSYS;
  return 0;
}

// Set Properties

void dsdk::ADChannelCount(const int ChannelCount)
{
  m_last_error = ENOSYS;
}

void dsdk::ADCounter(const long int Counter)
{
  m_last_error = ENOSYS;
}

void dsdk::BurstCounter(const long int BurstCounter)
{
  m_last_error = ENOSYS;
}

/**
* Allocates memory if necessary to hold the device file. The array is
* limited in size by DEV_PATH. Ensures the string is NULL terminating.
*
* @param DeviceFile Device file path and name used to connect to the device.
*/
void dsdk::DeviceFile(const char *const DeviceFile)
{
  // allocate space if necessary
  if(m_device_file == 0)
    m_device_file = new char[DEV_PATH];

  // copy device file path and name
  int i=0;
  for(i=0; i<DEV_PATH && DeviceFile[i] != 0; i++)
    m_device_file[i] = DeviceFile[i];
  // ensure the string is null terminating
  m_device_file[i<DEV_PATH?i:i-1] = 0;
  DeviceDisconnect();
  DeviceConnect();
}

void dsdk::EventPoint(const long int EventPnt)
{
  m_last_error = ENOSYS;
}

void dsdk::MaxBurstRate(const double MaxBurstRt)
{
  m_last_error = ENOSYS;
}

void dsdk::SampleRate(const double SampleRt)
{
  m_last_error = ENOSYS;
}

void dsdk::TrigHysteresisIdx(const int Hidx)
{
  m_last_error = ENOSYS;
}

void dsdk::TrigLevel(const int Level)
{
  m_last_error = ENOSYS;
}

void dsdk::TrigMode(const int Mode)
{
  m_last_error = ENOSYS;
}

void dsdk::TrigScnChnIdx(const int SCidx)
{
  m_last_error = ENOSYS;
}

void dsdk::TrigSlope(const int Slope)
{
  m_last_error = ENOSYS;
}

void dsdk::TrigPostLength(const int PostLength)
{
  m_last_error = ENOSYS;
}

void dsdk::TrigPreLength(const int PreLength)
{
  m_last_error = ENOSYS;
}

// Methods

void dsdk::ADChannelList(const int *const ChannelList)
{
  m_last_error = ENOSYS;
}

void dsdk::ADDiffList(const int *const DiffList)
{
  m_last_error = ENOSYS;
}

void dsdk::ADGainList(const int *const GainList)
{
  m_last_error = ENOSYS;
}

void dsdk::ADMethodList(const int *const MethodList)
{
  m_last_error = ENOSYS;
}

void dsdk::DAOutput(const int value, const int port)
{
  m_last_error = ENOSYS;
}

void dsdk::DeviceConnect()
{
  m_last_error = ENOSYS;
}

void dsdk::DeviceDisconnect()
{
  m_last_error = ENOSYS;
}

const long int dsdk::DigitalInput()
{
  m_last_error = ENOSYS;
  return 0;
}

void dsdk::DigitalOutput(const int value)
{
  m_last_error = ENOSYS;
}

void dsdk::GetData()
{
  m_last_error = ENOSYS;
}

void dsdk::GetDataEx(short int *iArray, const int Count)
{
  m_last_error = ENOSYS;
}

void dsdk::GetDataFrame()
{
  m_last_error = ENOSYS;
}

void dsdk::GetDataFrameEx(short int *iArray, const int Count)
{
  m_last_error = ENOSYS;
}

void dsdk::Start()
{
  m_last_error = ENOSYS;
}

void dsdk::Stop()
{
  m_last_error = ENOSYS;
}

// "Event Occur" Methods

/**
* @param Code Will be changed to the value of the last error.
* @return False = No error.\n
* True = Error occurred. Check the parameter for the actual value.
*/
const bool dsdk::ControlError(long int &Code)
{
  if(m_last_error != 0)
  {
    Code = m_last_error;
    m_last_error = 0;
    return true;
  }
  
  return false;
}

/**
* @param Count Number of new data points.
* @return True = There is new data available.\n
* False = There isn't any new data available.
*/
const bool dsdk::NewData(long int &Count)
{
  long int bytes = AvailableData();
  
  if(m_EventPoint != 0 && bytes >= m_EventPoint)
  {
    Count = bytes;
    return true;
  }
  
  return false;
}

/**
* This error may be misleading if someone recompiled their kernel with
* different serial buffer sizes.
* 
* Error Codes set:\n
* ENOSYS = Function not supported.
*
* @pre Connected.
* @return False = Everything is OK.\n
* True = A buffer overflowed.
*/
const bool dsdk::OverRun()
{
  m_last_error = ENOSYS;
  return false;
}

