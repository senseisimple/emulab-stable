/***************************************************************************
                          di194.cpp  -  Linux driver for DI-194 series
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

#include "di194.h"

/**
* - Sets up the channel order array (1 channel)
* - Sets channel count to use 1 analog channel
* - Sets up 'connection' to use the DI-194RS settings
* - Creates the following lists:
*  - m_ADChannelList\n
*    Normal order
*  - m_ADMethodList\n
*    IOS Average
* - Disables the digital channel
* - Calls sample rate function passing it the default sample rate,
*   as defined in the dsdk
*/
di194_dsdk::di194_dsdk():dsdk()
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
* Disconnects first. Then, it deallocates any allocated memory used by the
* class's private members.
*/
di194_dsdk::~di194_dsdk()
{
  DeviceDisconnect();

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

// Get "Properties"

/**
* @return Number of channels being scanned.
*/
const int di194_dsdk::ADChannelCount()
{
  return m_ADChannelCount;
}

/**
* @pre Must be acquiring data.
* @return Number of data points in input buffer.
*/
const long int di194_dsdk::AvailableData()
{
  // no data if not acquiring
  if(!m_acquiring_data)
    return 0;

  return m_connection.bytes_in_receive()/DI194_CHAN_SIZE; 
}

/**
* @return Number of data points required before NewData() fires.
*/
const long int di194_dsdk::EventPoint()
{
  return m_EventPoint;
}

/**
* Gets serial number from device.
*
* Error Codes set:\n
* EBUSY = Acquiring.\n
* ENOLINK = Device not connected.\n
* Errors set by Ncmd();
*
* @pre Not acquiring.\n
* Connected.
* @return Pointer to array containing device serial number.
* @remark Calling function's responsibility to delete allocated memory.
*/
const char *const di194_dsdk::InfoSerial()
{
  // can't get serial number while acquiring data
  if(m_acquiring_data)
  {
    m_last_error = EBUSY;
    return 0;
  }
  // must be connected
  if(!m_connection.is_comm_open())
  {
    m_last_error = ENOLINK;
    return 0;
  }

  char *sn = new char[DI194_SN_LENGTH];
  if(!Ncmd(m_connection, sn))
    m_last_error = my_errno;

  return sn;
}

/**
* @return Actual sample rate, after checks.
*/
const double di194_dsdk::SampleRate()
{
  return m_SampleRate;
}

// Set "Properties"

/**
* Overall number of channels to scan. All digital ports count as one
* channel here. The digital ports are all either acquiring or not, together.
* Sets default values for anything affected by changing the number of
* channels being scanned.
* - Lists:
*  - m_ADChannelList\n
*    Normal order.
*  - m_ADMethodList\n
*    IOS Average for analog, IOS Last Point for digital.
*  - chan_order\n
*    Normal order.
* - If all channels are requested, changes last one to digital.
* - Calls SampleRate() function with current value. Allows it to be checked.
* - Tells device what channels to scan.
* 
* Error Codes set:\n
* EBUSY = Acquiring.\n
* ENOLINK = Device not connected.\n
* EBOUNDS = Parameter out of bounds.\n
* Errors set by Dcmd().\n
* Errors set by Ccmd().
*
* @pre Not acquiring.\n
* Connected.\n
* Bounds between 1 and DI194_CHANNELS
* @param ChannelCount Number of channels to scan.
*/
void di194_dsdk::ADChannelCount(const int ChannelCount)
{
  // can't set while acquiring data
  if(m_acquiring_data)
  {
    m_last_error = EBUSY;
    return;
  }
  // must be connected
  if(!m_connection.is_comm_open())
  {
    m_last_error = ENOLINK;
    return;
  }
  // bounds check
  if(ChannelCount > DI194_CHANNELS || ChannelCount < 1)
  {
    m_last_error = EBOUNDS;
    return;
  }
  // pointer check
  if(m_ADChannelList != 0)
  {
    delete [] m_ADChannelList;
    m_ADChannelList = 0;
  }
  // pointer check
  if(m_ADMethodList != 0)
  {
    delete [] m_ADMethodList;
    m_ADMethodList = 0;
  }

  m_ADChannelCount = ChannelCount;
  m_ADChannelList = new int[m_ADChannelCount];
  m_ADMethodList = new int[m_ADChannelCount];
  // initialize with default values
  for(int i=0; i<m_ADChannelCount; i++)
  {
    m_ADChannelList[i] = i;
    m_ADMethodList[i] = IOS_AVERAGE;
  }
  
  // check if all channels requested
  // if so, change last channel to digital
  if(m_ADChannelCount == DI194_CHANNELS)
  {
    m_ADChannelList[DI194_CHANNELS-1] = -1;
    // 'all channels' includes digital ports
    digital_chan = true;
    // last point makes more sense for digital
    m_ADMethodList[DI194_CHANNELS-1] = IOS_LAST_POINT;
    // setup channel order (used in conversion)
    for(int i=0; i<DI194_CHANNELS-1; i++)
      chan_order[i] = i;
  }
  else // no digital
  {
    digital_chan = false;
    for(int i=0; i<m_ADChannelCount; i++)
      chan_order[i] = i;
  }

  // set sample rate according to new channels
  SampleRate(m_SampleRate);

  /* set channels to scan for acquisition
  1   -   analog channel 1
  2   -   analog channel 2
  4   -   analog channel 3
  8   -   analog channel 4

  add together to get any combination of channels
      OR
  each bit is a channel: 4 (left) to 1 (right) using above scheme
  channel 1, binary: 0001     (decimal: 1)
  channel 2,3 binary: 0110    (decimal: 6) */
  u_int8_t chan = 0;
  for(int i=0; i<m_ADChannelCount; i++)
  {
    if(m_ADChannelList[i] > -1)
      chan |= (0x01 << m_ADChannelList[i]);
  }
  // enable digital channel input
  if(digital_chan)
  {
    if(!Dcmd(m_connection, 1))
    {
      m_last_error = my_errno;
      return;
    }
  }
  else  // disable digital channel input
  {
    if(!Dcmd(m_connection, 0))
    {
      m_last_error = my_errno;
      return;
    }
  }
  // enable analog channels
  if(!Ccmd(m_connection, chan))
  {
    m_last_error = my_errno;
    return;
  }
}

/**
* @param EventPnt Number of data points required before NewData() fires.
* @remark Bounds between 0 and 32767. Set to extremes if out of bounds.
*/
void di194_dsdk::EventPoint(const long int EventPnt)
{
  if(EventPnt < 0)
    m_EventPoint = 0;
  else if(EventPnt > 32767)
    m_EventPoint = 32767;
  else
    m_EventPoint = EventPnt;
  
  return;
}

/**
* Automatically sets the sample rate to min or max if requested value is
* out of bounds.
* 
* Error Codes set:\n
* EBUSY = Acquiring.
*
* @pre Not acquiring.
* @param SampleRt Requested sample rate.
* @remark Bounds depend on the number of analog channels being scanned.
*/
void di194_dsdk::SampleRate(const double SampleRt)
{
  // can't set while acquiring data
  if(m_acquiring_data)
  {
    m_last_error = EBUSY;
    return;
  }

  double maxsr = 0;
  double minsr = 0;
  
  if(digital_chan)  // don't include digital channel in count
  {
    if(m_ADChannelCount == 1)
    {
      maxsr = DI194_MAXBURSTRATE; // don't divide by 0
      minsr = DI194_MINBURSTRATE; // don't multiply by 0
    }
    else
    { // subtract out the digital channel
      maxsr = DI194_MAXBURSTRATE / (m_ADChannelCount - 1);
      minsr = DI194_MINBURSTRATE * (m_ADChannelCount - 1);
    }
  }
  else  // no digital channel to worry about
  {
    maxsr = DI194_MAXBURSTRATE / m_ADChannelCount;
    minsr = DI194_MINBURSTRATE * m_ADChannelCount;
  }
  // user wants minimum, set to minimum
  if(SampleRt <= minsr)
    m_SampleRate = minsr;
  // user wants maximum, set to maximum
  else if(SampleRt >= maxsr)
    m_SampleRate = maxsr;
  else  // inbetween, user is within bounds
    m_SampleRate = SampleRt;
}

// "Methods"

/**
* Uses list to map software channels to a different order than the physical
* channels on the device. Each index in the list represents the software
* channel (which is therefore zero based). The value at that index represents
* the physical channel (which is also zero based).
*
* Sets up the chan_order list to correctly map the new channel order. Tells
* device which channels to scan (it is now possible to tell the difference
* between analog and digital channels).
* 
* Error Codes set:\n
* EBUSY = Acquiring.\n
* ENOLINK = Not connected.\n
* EINVAL = Bad parameter pointer.\n
* EBOUNDS = Parameter value(s) out of bounds.\n
* Errors set by Dcmd().\n
* Errors set by Ccmd().
* 
* @pre Not acquiring.\n
* Connected.\n
* Valid pointer. Only one digital channel. At least one analog channel.\n
* Bounds between (-1) and (DI194_CHANNELS-1)
* @param ChannelList Pointer to array containing the channel list.
*/
void di194_dsdk::ADChannelList(const int *const ChannelList)
{
  // can't set while acquiring data
  if(m_acquiring_data)
  {
    m_last_error = EBUSY;
    return;
  }
  // must be connected
  if(!m_connection.is_comm_open())
  {
    m_last_error = ENOLINK;
    return;
  }
  // pointer check
  if(ChannelList == 0)
  {
    m_last_error = EINVAL;
    return;
  }
  // used to check if multiple digital channels are specified,
  // there shouldn't be
  bool check_digital = false;
  // bounds check
  for(int i=0; i<m_ADChannelCount; i++)
  {
    if(ChannelList[i] >= DI194_CHANNELS-1 || ChannelList[i] < -1)
    {
      m_last_error = EBOUNDS;
      return;
    }
    // make sure only one digital channel is specified
    if(ChannelList[i] == -1 && !check_digital)
      check_digital = true;
    else if(ChannelList[i] == -1)
    {
      m_last_error = EBOUNDS;
      return;
    }
  }

  // must have at least one analog channel enabled
  if(m_ADChannelCount == 1 && check_digital)
  {
    m_last_error = EBOUNDS;
    return;
  }

  // temporary, used for setting up channel order
  int temp_chan[DI194_CHANNELS] = {0};

  // set digital channel setting to what was determined in bounds check
  digital_chan = check_digital;

  // copy the channel list
  for(int i=0; i<m_ADChannelCount; i++)
  {
    m_ADChannelList[i] = ChannelList[i];
    temp_chan[i] = ChannelList[i];
  }

  // sort channel list, it's only 5 channels max,
  // bubble sort will work
  for(int i=1; i<m_ADChannelCount; i++)
  {
    for(int j=0; j<m_ADChannelCount-i; j++)
    {
      if(temp_chan[j] > temp_chan[j+1]) // need to swap?
      {
        temp_chan[j] = temp_chan[j] ^ temp_chan[j+1];
        temp_chan[j+1] = temp_chan[j] ^ temp_chan[j+1];
        temp_chan[j] = temp_chan[j] ^ temp_chan[j+1];
      }
    }
  }

  if(digital_chan)
  {
    // setup channel order
    for(int i=1; i<m_ADChannelCount; i++)
      chan_order[temp_chan[i]] = (i-1) * DI194_CHAN_SIZE;
  }
  else
  {
    // setup channel order
    for(int i=0; i<m_ADChannelCount; i++)
        chan_order[temp_chan[i]] = i * DI194_CHAN_SIZE;
  }

  /* set channels to scan for acquisition
  1   -   analog channel 1
  2   -   analog channel 2
  4   -   analog channel 3
  8   -   analog channel 4

  add together to get any combination of channels
      OR
  each bit is a channel: 4 (left) to 1 (right) using above scheme
  channel 1, binary: 0001     (decimal: 1)
  channel 2,3 binary: 0110    (decimal: 6) */
  u_int8_t chan = 0;
  for(int i=0; i<m_ADChannelCount; i++)
  {
    if(m_ADChannelList[i] != -1)
      chan |= (0x01 << m_ADChannelList[i]);
  }
  // enable digital channel input
  if(digital_chan)
  {
    if(!Dcmd(m_connection, 1))
    {
      m_last_error = my_errno;
      return;
    }
  }
  else  // disable digital channel input
  {
    if(!Dcmd(m_connection, 0))
    {
      m_last_error = my_errno;
      return;
    }
  }
  // enable analog channels
  if(!Ccmd(m_connection, chan))
  {
    m_last_error = my_errno;
    return;
  }
}

/**
* The method list determines what IOS method to apply to each channel.
* The list's index represents the physical channel (not the software channel).
* The value at that index represents the IOS method.
* 
* Error Codes set:\n
* EBUSY = Acquiring.\n
* EINVAL = Bad parameter pointer.\n
* EBOUNDS = Parameter value(s) out of bounds.
*
* @pre Not acquiring.\n
* Valid pointer.
* @param MethodList Pointer to array containing the IOS values to use.
*/
void di194_dsdk::ADMethodList(const int *const MethodList)
{
  // can't set while acquiring data
  if(m_acquiring_data)
  {
    m_last_error = EBUSY;
    return;
  }
  // pointer check
  if(MethodList == 0)
  {
    m_last_error = EINVAL;
    return;
  }

  // bounds check
  for(int i=0; i<m_ADChannelCount; i++)
  {
    if(MethodList[i] > IOS_GREATEST || MethodList[i] < IOS_SMALLEST)
    {
      m_last_error = EBOUNDS;
      return;
    }
  }

  // copy the list
  for(int i=0; i<m_ADChannelCount; i++)
    m_ADMethodList[i] = MethodList[i];
}

/**
* Sets up connection, connects to device. Disconnects on connection failure.
* 
* Error Codes set:\n
* EBUSY = Acquiring.\n
* ENODEV = Device file not set.\n
* EALREADY = Already connected.\n
* Errors returned by di_serial_io::connect().
* Errors set by DeviceDisconnect().\n
* Errors set by Stop().
*
* @pre Not acquiring.\n
* Device file set.\n
* Not connected.
*/
void di194_dsdk::DeviceConnect()
{
  // can't set while acquiring data
  if(m_acquiring_data)
  {
    m_last_error = EBUSY;
    return;
  }
  // device file hasn't been set
  if(m_device_file == 0)
  {
    m_last_error = ENODEV;
    return;
  }
  // already connected
  if(m_connection.is_comm_open())
  {
    m_last_error = EALREADY;
    return;
  }

  // open the serial port
  // setup the serial port
  my_errno = m_connection.connect(m_device_file, 1);
  if(my_errno != 0)
  {
    m_last_error = my_errno;
    DeviceDisconnect();
    return;
  }

  // make sure device is stopped
  m_acquiring_data = true;
  Stop();
}

/**
* Stops acquiring if it hasn't been stopped yet. Resets acquisition flag.
* Attempts to disconnect. Should reset serial connection settings back to
* their original values if at all possible.
* 
* Error Codes set:\n
* Errors returned by di_serial_io::disconnect().
*
* @post Not acquiring.\n
* Not connected.
*/
void di194_dsdk::DeviceDisconnect()
{
  // check if user forgot to stop acquiring
  if(m_acquiring_data)
    // stop acquiring first
    Stop();
  m_acquiring_data = false;
  // check if already disconnected
  if(!m_connection.is_comm_open())
    return;
  // restore original serial port settings and
  // try to close the serial port
  m_last_error = m_connection.disconnect();
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
* The function enforces the sample rate and applies the properties of the
* variously supported lists.
* 
* Error Codes set:\n
* EBOUNDS = Count is out of bounds.\n
* EINVAL = Bad parameter pointer.\n
* ENODATA = Not acquiring.\n
* Errors set by di_serial_io::di_read().
*
* @pre Bounds for Count between 1 and 32767\n
* Valid iArray pointer\n
* Acquiring
* @param iArray Pointer to array which will hold the acquired data.
* @param Count Number of data points to store into array.
*/
void di194_dsdk::GetDataEx(short int *iArray, const int Count)
{
  // bounds check
  if(Count > 32767 || Count < 1)
  {
    m_last_error = EBOUNDS;
    return;
  }
  // pointer check
  if(iArray == 0)
  {
    m_last_error = EINVAL;
    return;
  }
  // must be acquiring
  if(!m_acquiring_data)
  {
    m_last_error = ENODATA;
    return;
  }

  u_int8_t di_data[DI194_CHAN_SIZE*(DI194_CHANNELS-1)] = {0};
  int mCount = 0;
  // number of oversamples per real sample
  int srinterval = static_cast<int>(m_MaxBurstRate/ (digital_chan
                                          ? m_ADChannelCount-1
                                          : m_ADChannelCount)
                                           + 1 - m_SampleRate);
  if(srinterval <= 0)
    srinterval = 1;
  // temporarily holds original data to perform calculation
  short int temp[DI194_CHANNELS] = {0};
  // in case user asks for less data points than in a normal scan
  int small_sample = m_ADChannelCount < Count ? m_ADChannelCount : Count;
  // holds intermittent values for IOS_AVERAGE
  int64_t ios_avg[DI194_CHANNELS] = {0};
  bool first_time = true;
  u_int16_t amount = 0;
  if(digital_chan) // digital channel is embedded
  {
    amount = DI194_CHAN_SIZE*(m_ADChannelCount-1);
  }
  else  // no digital channel
  {
    amount = DI194_CHAN_SIZE*m_ADChannelCount;
  }

  // loop until enough data points have been gathered,
  // as far as blocks of enabled channels go
  for(int c=0, d=0, e=0; c<Count; c += small_sample, d=0)
  {
    first_time = true;
    for(e=0; e<small_sample; e++)
      ios_avg[e] = 0;

    // loop until the right number of samples have been gathered for
    // the specific sample rate
    for(int timer=0; timer<srinterval; timer++)
    {
      // get data
      if(m_connection.di_read(&di_data[0], amount, static_cast<u_int8_t>(amount)))
      {
        m_last_error = my_errno; // error occurred
        return;
      }
      // copy original values
      for(e=0; e<small_sample; e++)
        temp[e] = iArray[c+e];
      // convert new data to counts and store in array
      for(e=0; e<small_sample; e++)
        iArray[c + e] = static_cast<short int>(convert(&di_data[0], e));
      // increment number of oversamples per scan
      d++;
      // don't perform any calculations the first time through
      if(first_time)
      {
        first_time = false;
        for(e=0; e<small_sample; e++)
          ios_avg[e] += iArray[c+e];
        continue;
      }
      // apply method to 'original' and 'new' data values
      for(e=0; e<small_sample; e++)
      {
        switch(m_ADMethodList[e])
        {
          case IOS_RMS:  // not implemented
          case IOS_FREQ: // not implemented
          case IOS_LAST_POINT:  // last point
            // this happens by default, nothing to do
            break;
          case IOS_AVERAGE: // average of points
            ios_avg[e] += iArray[c+e];     // adds up values, after conversion above
            iArray[c+e] = ios_avg[e] / d;  // stores new average
            break;
          case IOS_MIN: // minimum of points
            if(temp[e] < iArray[c+e])
              iArray[c+e] = temp[e];
            break;
          case IOS_MAX: // maximum of points
            if(temp[e] > iArray[c+e])
              iArray[c+e] = temp[e];
            break;
        }
      }
    }
  }

  // any amount left that wasn't an even divisor of scanned blocks?
  // just use the next scanned packet
  mCount = Count % m_ADChannelCount;
  if(mCount != 0) // true if not done
  {
    // grab some more data
    if(m_connection.di_read(&di_data[0], amount, static_cast<u_int8_t>(amount)))
    {
      m_last_error = my_errno; // error occurred
      return;
    }
    // and store part of it
    for(int e=0; e<mCount; e++)
      iArray[Count - mCount + e] = static_cast<short int>(convert(&di_data[0], e));
  }
}

/**
* Tells device to start acquisition.
*
* Error Codes set:\n
* EALREADY = Acquiring.\n
* ENOLINK = Not connected.\n
* Errors set by Scmd().
*
* @pre Not acquiring.\n
* Connected.
* @remark Sets flag that device is acquiring.
*/
void di194_dsdk::Start()
{
  // don't send command if already acquiring, this is an error because
  // there is data in the buffer
  if(m_acquiring_data)
  {
    m_last_error = EALREADY;
    return;
  }
  if(!m_connection.is_comm_open()) // don't bother if not connected
  {
    m_last_error = ENOLINK;
    return;
  }

  // send command
  if(!Scmd(m_connection, 1))
  {
    m_last_error = my_errno;
    return;
  }

  m_acquiring_data = true;
}

/**
* Tells device to stop acquisition.
*
* Error Codes set:\n
* ENOLINK = Not connected.\n
* Errors set by Scmd().
*
* @pre Acquiring.\n
* Connected.
* @remark Sets acquiring flag.
*/
void di194_dsdk::Stop()
{
  // can't set while not acquiring data,
  // not really an error
  if(!m_acquiring_data)
    return;
  // must be connected
  if(!m_connection.is_comm_open())
  {
    m_last_error = ENOLINK;
    return;
  }

  // send command
  if(!Scmd(m_connection))
  {
    m_last_error = my_errno;
    return;
  }

  m_acquiring_data = false;
}

// "Event Occur" Methods

/**
* This error may be misleading if someone recompiled their kernel with
* different serial buffer sizes. Current size is: (DI_SERIAL_BUFFER_SIZE).
* 
* Error Codes set:\n
* ENOLINK = Not connected.
*
* @pre Connected.
* @return False = Everything is OK.\n
* True = One of the two buffers overflowed.
*/
const bool di194_dsdk::OverRun()
{
  // must be connected
  if(!m_connection.is_comm_open())
  {
    m_last_error = ENOLINK;
    return false;
  }

  // doesn't set an error because this function represents an error
  if(m_connection.bytes_in_receive() >= DI_SERIAL_BUFFER_SIZE)
    return true;

  return false;
}

/**
* Converts 'di_data' into Counts.
*
* @param di_data Pointer to array of raw data.
* @param num_chan Channel list position to check for analog or digital.
*/
const short int di194_dsdk::convert(const u_int8_t *const di_data,
                                    const u_int8_t num_chan)
{
  short int temp = 0;
  // extract and convert to counts
  // find out whether it's an analog or digital channel
  if(m_ADChannelList[num_chan] != -1) // analog channel
  {
    temp = static_cast<short int>(di_data[chan_order[m_ADChannelList[num_chan]]]
                   & 0xE0) >> 5;
    temp |= (static_cast<short int>(di_data[chan_order[m_ADChannelList[num_chan]]+1]
                   & 0xFE) << 2);
    temp <<= 6; // left justify the number
    temp ^= 0x8000;
  }
  else // digital bits
    temp = (di_data[0] & 0x0E) >> 1;

  return temp;
}

