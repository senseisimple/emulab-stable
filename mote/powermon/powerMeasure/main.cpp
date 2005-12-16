#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace std;

#include "powerMeasure.h"
#include "exceptions.h"

int main(void)
{
    vector<double> calPoints;

    PowerMeasure::readVtoItable("cal.txt",&calPoints);
    string sampleFile = "/tmp/dataq.dat";
    string serialPath = "/dev/ttyS0";
    PowerMeasure pwrMeasure( &serialPath, 2, 240.0, &calPoints );
    pwrMeasure.setFile( &sampleFile, 240*75 );
//    pwrMeasure.enableVoltageLogging();
    char c;
/*
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
    {
        system("sleep 0.5s");
        cout<<"average: "<<pwrMeasure.getAveI() <<endl;
//        cout<<"lastsample:"<<pwrMeasure.getLastSampleI()<<endl;
//        cout<<"lastsample->V "<<
//            pwrMeasure.rawToV((short int) pwrMeasure.getLastSampleRaw())<<endl;
    }


    return(0);
}
