/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
* TODO:
- save "dump" to file - append existing, label with "timestamp"
- miscellaneous TODO's listed in code.
- refactor for niceness.
*/

#include <pthread.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

#include "powerMeasure.h"
#include "exceptions.h"
#include "dataqsdk.h"
#include "constants.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
const int NUMCHANNELS = 1;
const double DEFAULTSAMPLERATE = 240.0;
    //specify what method to apply to input data
const int methodList[5] = {IOS_AVERAGE,IOS_AVERAGE,
                IOS_AVERAGE,IOS_AVERAGE,IOS_AVERAGE};
//filnames of files giving for each channel: cal_zeroV and cal_highV values
//float format, binary, sequential.
const char* cal_toVfilenames[4] = {"toV0.cal","toV1.cal","toV2.cal","toV3.cal"};
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

PowerMeasure::PowerMeasure( string* f_devicePath, 
                            int f_channel, 
                            double f_sampleRate, 
                            vector<double>* f_vTomAPoints )
{
    devicePath = f_devicePath;
    channel = f_channel-1;		//TODO: Bounds checks
    CHANNELLIST[0] = channel;
    CHANNELLIST[1] = -1;
    sampleRate = f_sampleRate;
    sampleLimit = 0ULL;
    vTomAPoints = f_vTomAPoints;
    filePath = 0;
    errorCode=0;
    fStopLogger = 0;
    lastSampleRaw = 0;
    averageRaw = 0.0;
    minRaw = 0x7fff;
    maxRaw = 0;
    total_mAH = 0.0;
    fLogVoltage = 0;
    
    //read in the calibration data for the channel, if it exists
    ifstream fin;
    fin.open( cal_toVfilenames[channel], ios::in | ios::binary );
    float tmp = 0.0;
    if( !fin.fail() )
    {
        //file exists, so read from it
        fin.read( (char*)(&tmp), sizeof(float));
        cal_zeroV.push_back(tmp);
        fin.read( (char*)(&tmp), sizeof(float));
        cal_zeroV.push_back(tmp);
        fin.read( (char*)(&tmp), sizeof(float));
        cal_highV.push_back(tmp);
        fin.read( (char*)(&tmp), sizeof(float));
        cal_highV.push_back(tmp);
    }
    fin.close();
}

PowerMeasure::~PowerMeasure()
{
    fStopLogger = 1;
    //wait for logger thead to exit
    pthread_join( loggerThread, NULL );
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::setFile( string* f_filePath, int f_sampleLimit )
{
    filePath = f_filePath;
    sampleLimit = f_sampleLimit;

    ofstream fout;
    fout.open( f_filePath->c_str(), ios::out );
    //write first line to file: samplerate
    fout << sampleRate << "\n";	
    fout.close();
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::startRecording()
{
    setupDevice(devicePath);

    //start acquisition
    dataq.Start();
    checkError();

    //spawn thread to read data to file
    loggerThreadBusy = 1;
    retLoggerThread = pthread_create( 
        &loggerThread, NULL, logData, (void *)this);
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::stopRecording()
{
    fStopLogger = 1;
    pthread_join( loggerThread, NULL );    
}

///////////////////////////////////////////////////////////////////////////////
void* PowerMeasure::logData( void* ptr )
{
    PowerMeasure* pwrMeasure = (PowerMeasure*)ptr;
    cout<< "Logger thread. Path to file = "<< *pwrMeasure->filePath<<endl;

    unsigned long long sampleCnt = 1ULL;
    long int newDataCnt = 0;
    //double accumAve = 0.0;
    short int newDataPoints[240];  //space to acquire data before saving to disk
    float newCurSamples[240];

    ofstream fout;	

    pwrMeasure->averageRaw = 0;

    //save data to disk until limit reached
    //when sampleLimit == 0, record indefinitely
    while( sampleCnt <= pwrMeasure->sampleLimit 
           || pwrMeasure->sampleLimit == 0 )
    {
        //check request for thread destruction
        if( pwrMeasure->fStopLogger == 1 )
        {
            pwrMeasure->loggerThreadBusy = 0;            
            pthread_exit(NULL);
        }
        
        //check if new data is available
//        cout<<"checking for new data"<<endl;
        if( pwrMeasure->dataq.NewData(newDataCnt) )
        {
//cout<<"newDataCnt="<<newDataCnt<<endl;
            fout.open( pwrMeasure->filePath->c_str(), ofstream::app );
            for(unsigned int r=0;
                r < ((unsigned)newDataCnt 
                     > pwrMeasure->sampleLimit -sampleCnt+1  ?
                     pwrMeasure->sampleLimit-sampleCnt+1     :
                     (unsigned)newDataCnt);
                r++)
            {
                pwrMeasure->dataq.GetDataEx(newDataPoints, 1);
                pwrMeasure->checkError();

                newCurSamples[0] = pwrMeasure->rawToMa(newDataPoints[0]);
                //write to disk
                if( pwrMeasure->fLogVoltage == 1 )
                {
                    fout<<pwrMeasure->rawToV(newDataPoints[0])<<",";
                }
                
                fout<<newCurSamples[0]<<"\n";

                //set last sample
                pwrMeasure->setLastSampleRaw(newDataPoints[0]);

                //set min
                if( newDataPoints[0] < pwrMeasure->getMinRaw() )
                    pwrMeasure->setMinRaw( newDataPoints[0] );
                //set max
                if( newDataPoints[0] >  pwrMeasure->getMaxRaw() )
                    pwrMeasure->setMaxRaw( newDataPoints[0] );

                //average samples
                double ave = (pwrMeasure->getAveRaw()*(sampleCnt+r-1)
                              + newDataPoints[0])
                    / (sampleCnt+r);
                pwrMeasure->setAveRaw(ave);

                //set total mA-h
                double hrs = (sampleCnt+r+1)
                    / (double)pwrMeasure->getSampleRate()
                    / 60 / 60;

//                cout<<"ave "<<pwrMeasure->rawToMa((short int)ave);
//                cout<<"   hrs "<<hrs;
//                cout<<"  ave*hrs "<<
//                    pwrMeasure->rawToMa((short int)ave)
//                    * hrs<<endl;
                
                pwrMeasure->setTotal_mAH( 
                    (double)(
                    pwrMeasure->rawToMa((short int)ave)
                    * hrs) );
//                    ave*(sampleCnt+r+1));
//                    / 
//                    (double)pwrMeasure->getSampleRate() /
//                60);
                
                
            }
            fout.close();
        }


        sampleCnt += newDataCnt;
        newDataCnt = 0;
    }

    pwrMeasure->loggerThreadBusy = 0;
    //TODO ?? stop device??
    pthread_exit(NULL);
    return NULL;	//never reaches here
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::checkError()
{
    if( dataq.ControlError(errorCode) )
    {
        cerr<<"Error: "<<errorCode<<endl;
        throw DataqException(errorCode);
    }
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::setupDevice(string* f_devicePath)
{
    //set product name
    dataq.ProductName("DI-194RS");
    checkError();
    cout<<dataq.ProductName()<<endl;

    //set device
    dataq.DeviceFile(f_devicePath->c_str());
    checkError();

    //set event point (number of data points before NewData fires)
    dataq.EventPoint(1);
    checkError();

cout << "Setting channel count: " << flush;
    //set number of channels to scan
    dataq.ADChannelCount(NUMCHANNELS);
//dataq.ADChannelCount(4);
    checkError();
cout<<dataq.ADChannelCount();

cout << "Setting channel list: "<<CHANNELLIST<<endl;
    //set channels to scan
    dataq.ADChannelList(CHANNELLIST);
    checkError();

cout <<"Setting method list: "<<methodList<<endl;
    //set processing methods for each channel
    dataq.ADMethodList(methodList);
    checkError();

    //set sample rate
    dataq.SampleRate(sampleRate);
cout<<"Samplerate="<<dataq.SampleRate()<<endl;
    checkError();
    if( dataq.SampleRate() != sampleRate ){
        cerr<<"SampleRate set incorrectly: perhaps an invalid rate?"<<endl;
    }
}


///////////////////////////////////////////////////////////////////////////////
int PowerMeasure::isCapturing()
{
    //TODO! Convert to semaphore?
    if( loggerThreadBusy == 1 )
        return 1;
    else
        return 0;	
}


///////////////////////////////////////////////////////////////////////////////
float PowerMeasure::rawToV( short int f_raw )
{	
    float x = (float) f_raw * 10 / 32767;
    float result = 0.0;
    float m = 0.0;

    //if calibration has not been done, simply return the "non corrected" value
    if( cal_zeroV.empty() || cal_highV.empty() )
    {
        result = x;
    }else
    {		
        // incorporate calibration points to convert input
        m = (cal_highV[1]-cal_zeroV[1])/(cal_highV[0]-cal_zeroV[0]);
        //result = cal_highV[1] + m*(x - cal_zeroV[0]);
        result = cal_zeroV[1] + m*(x - cal_zeroV[0]);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
float PowerMeasure::rawToMa( short int f_raw )
{
    float volts;
    float mAmps;
    //convert to voltage
    volts = rawToV( f_raw );

    //convert to current

    //interpolate from calibration table (for V -> mA)

    unsigned int i0 = 0;
    unsigned int i1 = 1;

    //find point index values.
    while( i1*2 < vTomAPoints->size()-1 )
    {
        if( (*vTomAPoints)[i1*2] > volts )
            break;
        i1++;
        i0++;
    }

    //interpolate
    double x0 = (*vTomAPoints)[i0*2];
    double x1 = (*vTomAPoints)[i1*2];
    double y0 = (*vTomAPoints)[i0*2+1];
    double y1 = (*vTomAPoints)[i1*2+1];
    double m = (y1-y0)/(x1-x0);
    mAmps = (float) m*(volts-x1) + y1;

    return mAmps;
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::setLastSampleRaw( short int f_raw )
{
    lastSampleRaw = f_raw;
}

///////////////////////////////////////////////////////////////////////////////
short int PowerMeasure::getLastSampleRaw( )
{
    return lastSampleRaw;
}

///////////////////////////////////////////////////////////////////////////////
float PowerMeasure::getLastSampleI( )
{
    return rawToMa(getLastSampleRaw());
}


///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::setAveRaw( double f_raw )
{
    averageRaw = f_raw;
}

///////////////////////////////////////////////////////////////////////////////
double PowerMeasure::getAveRaw( )
{
    return averageRaw;
}

///////////////////////////////////////////////////////////////////////////////
float PowerMeasure::getAveI( )
{
    return rawToMa((short int)getAveRaw());
}

///////////////////////////////////////////////////////////////////////////////
short int PowerMeasure::getMinRaw( )
{
    return minRaw;    
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::setMinRaw(short int f_raw )
{
    minRaw = f_raw;
}

///////////////////////////////////////////////////////////////////////////////
short int PowerMeasure::getMaxRaw( )
{
    return maxRaw;    
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::setMaxRaw(short int f_raw )
{
    maxRaw = f_raw;
}

///////////////////////////////////////////////////////////////////////////////
double PowerMeasure::getTotal_mAH( )
{
    return total_mAH;
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::setTotal_mAH( double f_mAH )
{
    total_mAH = f_mAH;
}

///////////////////////////////////////////////////////////////////////////////
double PowerMeasure::getSampleRate( )
{
    return sampleRate;    
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::cal_SetZeroV()
{
    cal_zeroV.clear();
    if( cal_zeroV.size() > 0 )
    cout<<"ERROR! CALZERO.SIZE > 0 "<<endl;
    setupDevice(devicePath);
    //start acquisition
    dataq.Start();
    checkError();
    //hack to only record one sample. Save/restore path and limit already set
    unsigned long long tmp_sampleLimit = sampleLimit;
    string nullFilePath = "/dev/null";
    string* tmpFilePath = filePath;
    setFile( &nullFilePath, 1 );
    retLoggerThread 
        = pthread_create( &loggerThread, NULL, logData, (void *)this);
    //wait for logger thead to exit
    pthread_join( loggerThread, NULL );
    setFile( tmpFilePath, tmp_sampleLimit );
    dataq.Stop();

    cal_zeroV.push_back( (float) getLastSampleRaw()*10/32767 );
    cal_zeroV.push_back(0);

    //write to file
    ofstream fout;
    fout.open( cal_toVfilenames[channel], ios::out | ios::binary );
    cout<<"cal_zero[0]"<<cal_zeroV[0]<<endl;
    fout.write( (char*)(&cal_zeroV[0]), sizeof(float));
    fout.write( (char*)(&cal_zeroV[1]), sizeof(float));
    fout.close();
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::cal_SetHighV(float f_highV)
{
    cal_highV.clear();
    if( cal_highV.size() > 0 )
    cout<<"ERROR! CALHIGH.SIZE > 0 "<<endl;
    setupDevice(devicePath);
    //start acquisition
    dataq.Start();
    checkError();	
    //hack to only record one sample. Save/restore path and limit already set
    unsigned long long tmp_sampleLimit = sampleLimit;
    string nullFilePath = "/dev/null";
    string* tmpFilePath = filePath;
    setFile( &nullFilePath, 1 );
    retLoggerThread 
        = pthread_create( &loggerThread, NULL, logData, (void *)this);
    //wait for logger thead to exit
    pthread_join( loggerThread, NULL );
    setFile( tmpFilePath, tmp_sampleLimit );
    dataq.Stop();

    cal_highV.push_back( (float) getLastSampleRaw()*10/32767 );
    cal_highV.push_back(f_highV);

    //write to file
    fstream fout;
    fout.open( cal_toVfilenames[channel], 
            fstream::out | fstream::in | fstream::ate | fstream::binary );
    fout.write( (char*)(&cal_highV[0]), sizeof(float));
    fout.write( (char*)(&cal_highV[1]), sizeof(float));
    fout.close();
}


///////////////////////////////////////////////////////////////////////////////
/**
Format of table file
- each line has one floating point value
- each point has two values: Voltage and milliAmp
- the starting point should have a milliamp value of 0.0 (min possible current)
- the ending point should have a voltage value of 10.0 (max possible voltage)
- the final line should be only a newline
example:
1.11
0.0
1.21
0.39
2.33
4.76
6.71
21.41
10.0
33.92

*/
void PowerMeasure::readVtoItable( string f_filepath, 
                                        vector<double>* f_OUT_vTomAPoints )
{
    ifstream fin;
    fin.open( f_filepath.c_str(), ifstream::in );
    if( !fin.fail() )
    {
        f_OUT_vTomAPoints->clear();
        //read lines, adding to table
        double x = 0.0;
        char c = 0;
        while( fin.peek() != EOF )
        {
            fin>>x;
            f_OUT_vTomAPoints->push_back(x);
            fin.get(c);			// get newline character
        }
    }

    fin.close();
}


///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::enableVoltageLogging()
{
    fLogVoltage = 1;
}


///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::disableVoltageLogging()
{
    fLogVoltage = 0;
}

///////////////////////////////////////////////////////////////////////////////
void PowerMeasure::dumpStats()
{
    cout<<"last "<< rawToMa(lastSampleRaw)<<endl;
    cout<<"average "<< rawToMa((short int)averageRaw)<<endl;
    cout<<"min "<<rawToMa(minRaw)<<endl;
    cout<<"max "<<rawToMa(maxRaw)<<endl;
    cout<<"accum_mA-H "<<total_mAH<<endl;
    
}
