/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <signal.h>

using namespace std;

#include "powerMeasure.h"
#include "exceptions.h"


PowerMeasure* g_powmeas = 0; // pointer to obj to be used by sig handlers


void SIG_dump_handler(int signum)
{
//    cout<<"dump..."<<endl;
    g_powmeas->dumpStats();
}

void SIG_exit_handler(int signum)
{
    cout<<"received term signal: stopping recording..."<<endl;
    g_powmeas->stopRecording();
}

int main(int argc, char *argv[])
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_dump_handler;
    sigaction( SIGUSR1, &sa, 0 );
    sa.sa_handler = SIG_exit_handler;
    sigaction( SIGTERM, &sa, 0 );

    vector<double> calPoints;

    PowerMeasure::readVtoItable("cal.txt",&calPoints);
    string sampleFile = "/tmp/dataq.dat";
    string serialPath = "/dev/ttyS0";
    if (argc == 2)
        serialPath = argv[1];
    PowerMeasure pwrMeasure( &serialPath, 2, 240.0, &calPoints );
    g_powmeas = &pwrMeasure;
    pwrMeasure.setFile( &sampleFile, 240*0 );
//    pwrMeasure.enableVoltageLogging();
/*
    char c;
    cout<<"DO YOU WISH TO CALIBRATE? (y/n)"<<endl;
    cin>>c;
    if( c == 'y' )
    {
        while( cin.get() != '\n' ){;}	//read extra stuff from previous cin
        //need to calibrate, if not already done so:
        cout<<"CALIBRATE: 0-point. Set input channel to 0 and hit a key"<<endl;
        cin.get();
        //the "zero point" is set to whatever is the current input value
        // on the channel
        pwrMeasure.cal_SetZeroV();	
        cout<<"CALIBRATE: high-point. Set input channel to digital"
            <<" input and enter the actual voltage measured by a DMM"<<endl;
        float cal_highV = 0.0;
        cin >> cal_highV;
        //the second point on the linear calibration curve is set to what
        pwrMeasure.cal_SetHighV(cal_highV);
    }
*/    


    try{
        pwrMeasure.startRecording( );
    }catch( DataqException& datEx ){
        cout<< datEx.what() <<endl;
    }


    while( pwrMeasure.isCapturing() )
//    while(1)
    {
//        sleep(1);
        
//        pause();
        
        system("sleep 1s");
//        cout<<"average: "<<pwrMeasure.getAveI() <<endl;
//        cout<<"lastsample:"<<pwrMeasure.getLastSampleI()<<endl;
//        cout<<"lastsample->V "<<
//            pwrMeasure.rawToV((short int) pwrMeasure.getLastSampleRaw())<<endl;
    }
    
    return(0);
}
