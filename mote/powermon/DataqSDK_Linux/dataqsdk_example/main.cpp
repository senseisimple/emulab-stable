/***************************************************************************
             main.cpp - Example program on how to use the DataqSDK
                        for Linux.
 
    begin                : Wed Jun 16 2004
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
 ****************************************************************************/

#include <stdlib.h>
#include <string>
#include <iostream>
#include <iomanip>
using namespace std;
#include "dataqsdk.h"

int main(int argc, char *argv[])
{
  int MY_CHANNELS = atoi(argv[3]);
  dataqsdk my_device; // device control/connection; main object
  long int error_code = 0;
  long int new_data = 0;
  char *sn = 0;  // serial number
  int ChannelList[5] = {0,1,2,3,-1};  // used to set the channel list
  int MethodList[5] = {0,0,0,0,0};  // used to set the method list
  int points = 50; // how many scans to acquire
  points = atoi(argv[2]); // convert to integer and use it instead
  short int *iArray = new short int[points];  // will hold acquired data

  cout << "Supported devices:" << endl;
  char *temp = (char *)my_device.ProductName();
  int i=0;
  while(temp[i] != 0 && temp[i+1] != 0)
  {
    cout << &temp[i] << endl;
    i += strlen(&temp[i]);
    i++;
  }
  if(temp != 0)
  {
    delete [] temp;
    temp = 0;
  }

  cout << "Setting product name: " << flush;
  my_device.ProductName("DI-194RS");  // set
  if(my_device.ControlError(error_code))  // error check
  {
    cerr << "Error: " << error_code << endl;
    return 1;
  }
  else
    cout << my_device.ProductName(); // get
  cout << endl;

  cout << "Setting device file &" << endl;
  cout << "Connecting to device: " << flush;
  // the device file is the one the device is connected to
  // make sure the correct one is set
  my_device.DeviceFile("/dev/ttyS0");  // set
  if(my_device.ControlError(error_code))  // error check
  {
    cerr << "Error: " << error_code << endl;
    return 1;
  }
  else
    cout << my_device.DeviceFile(); // get
  cout << endl;

  cout << "Setting event point: " << flush;
  my_device.EventPoint(1);
  if(my_device.ControlError(error_code))
  {
    cerr << "Error: " << error_code << endl;
    return 1;
  }
  else
    cout << my_device.EventPoint();
  cout << endl;

  cout << "Setting channel count: " << flush;
  // good idea to set this before channel list because it resets it
  my_device.ADChannelCount(MY_CHANNELS);  // set
  if(my_device.ControlError(error_code))  // error check
  {
    cerr << "Error: " << error_code << endl;
    return 1;
  }
  else
    cout << my_device.ADChannelCount(); // get
  cout << endl;

  cout << "Setting channel list: ";
  // make sure ChannelList is at least as big as ADChannelCount,
  // this function uses ADChannelCount to determine how much of ChannelList
  // to read and set up
  my_device.ADChannelList(ChannelList); // set, doesn't have get
  if(my_device.ControlError(error_code))  // error check
  {
    cerr << "Error: " << error_code << endl;
    return 1;
  }
  else
    cout << "Success";
  cout << endl;

  cout << "Setting method list: ";
  // make sure MethodList is at least as big as ADChannelCount,
  // this function uses ADChannelCount to determine how much of MethodList
  // to read and set up
  my_device.ADMethodList(MethodList); // set, doesn't have get
  if(my_device.ControlError(error_code))  // error check
  {
    cerr << "Error: " << error_code << endl;
    return 1;
  }
  else
    cout << "Success";
  cout << endl;

  cout << "Setting sample rate: " << flush;
  // it will automatically be set to max if the given value is bigger than
  // or equal to the maximum value
  my_device.SampleRate(atof(argv[1]));
  if(my_device.ControlError(error_code))  // error check
  {
    cerr << "Error: " << error_code << endl;
    return 1;
  }
  else
    cout << my_device.SampleRate();
  cout << endl;

  cout << "Getting serial number: " << flush;
  sn = (char *)my_device.InfoSerial();  // get
  if(my_device.ControlError(error_code))  // error check
  {
    cerr << "Error: " << error_code << endl;
    return 1;
  }
  else
    cout << sn << endl;
  if(sn != 0)
  {
    delete [] sn;
    sn = 0;
  }

  cout << "Starting acquisition: " << flush;
  my_device.Start();
  if(my_device.ControlError(error_code))  // error check
  {
    cerr << "Error: " << error_code << endl;
    return 1;
  }
  else
    cout << "Acquiring..." << points << " scans" << endl;

  cout << " A.C. 1\t A.C. 2\t A.C. 3\t A.C. 4\t\t\tDigital" << endl;

  // gets one scan at a time and prints
  // it to the screen immediately
  int total = 0;
  while(total <= points)
  {
    if(my_device.NewData(new_data))
    {
      for(int i=0; i<(new_data>points-total?points-total:new_data); i++)
      {
        my_device.GetDataEx(iArray, MY_CHANNELS);
        if(my_device.ControlError(error_code))
        {
          cerr << "Error: " << error_code << endl;
          return 1;
        }
        else
        {
          // print out all the channels of the scan
          for(int j=0; j<MY_CHANNELS; j++)
          {
            if(MY_CHANNELS == 5 && j == 4)
            {
              cout << "\t\t";
              for(int xyz=0; xyz<3; xyz++)
                cout << ((iArray[j] >> xyz) & 1);
            }
            else
            {
              cout.width(7);
              cout.precision(2);
              cout.setf(ios::fixed | ios::showpoint);
              cout << (double)iArray[j]*10/32767 << ' ';
            }
          }
          cout << endl;
        }
      }
      total += new_data;
    }
    if(my_device.ControlError(error_code))
    {
      cerr << "Error: " << error_code << endl;
      return 1;
    }
  }

  if(iArray != 0)
  {
    delete [] iArray;
    iArray = 0;
  }

  cout << "Stopping acquisition... " << flush;
  my_device.Stop();
  if(my_device.ControlError(error_code))  // error check
  {
    cerr << "Error: " << error_code << endl;
    return 1;
  }
  else
    cout << "Success" << endl; 

  return 0;
}

