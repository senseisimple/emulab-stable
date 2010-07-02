/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
   Author: Dan Gebhardt
   Group: Flux, University of Utah

   Description:
   Linux interface to the mote power measurement system, based on a
   Dataq Instruments DI-194RS. Requires the DataqSDK library for Linux.

*/


#include "dataqsdk.h"

class PowerMeasure
{
public:
    //PowerMeasure();
    //f_channel = 1...4
    PowerMeasure( 	string* f_devicePath, 
                    int f_channel, 
                    double f_sampleRate, 
                    vector<double>* f_vTomAPoints );
    ~PowerMeasure();
    	//file to store recorded values
    void setFile( string* f_filePath, int f_sampleLimit );
    void startRecording();
    void stopRecording();
    int isCapturing();
    int loggerThreadBusy;		//TODO convert this to a semaphore
    void cal_SetZeroV();
    void cal_SetHighV(float f_highV);
    void setLastSampleRaw( short int f_raw );
    short int getLastSampleRaw( );
    float getLastSampleI( );
    void setAveRaw( double f_raw );
    double getAveRaw();
    float getAveI();
    static void readVtoItable( string f_filepath,
                               vector<double>* f_OUT_vTomAPoints );
    void enableVoltageLogging(); /* write voltage as well as current to file*/
    void disableVoltageLogging();
    void dumpStats();

  private:
    int channel;
    int CHANNELLIST[2];	
    double sampleRate;
    double getSampleRate();
    unsigned long long sampleLimit;
    vector<double>* vTomAPoints;
    string* filePath;
    dataqsdk dataq;
    long int errorCode;
    pthread_t loggerThread;
    int retLoggerThread;
    int fStopLogger;
    vector<float> cal_zeroV;	//pair of voltages: <dataq measured 0, actual 0>
    vector<float> cal_highV;	//pair of voltages: <dataq measured high, DMM measured high>
    short int lastSampleRaw;
    double averageRaw;
    short int minRaw;
    short int maxRaw;
    double total_mAH;

    short int getMinRaw();
    void setMinRaw(short int f_raw);
    short int getMaxRaw();
    void setMaxRaw(short int f_raw);
    double getTotal_mAH( );
    void setTotal_mAH(double f_mAH );

    string* devicePath;
    int fLogVoltage;
    void checkError();	//throws a DataqException if error flag on device is set
       //executed in a separate thread which saves data to disk
    static void* logData(void *);
    void setupDevice(string* f_devicePath);
    float rawToMa(short int f_raw);	//convert raw data value to milli-amp float
    float rawToV( short int f_raw );

};
