#include <iostream.h>
#include <fstream.h>
#include <iomanip.h>
#include <string.h>
#include <math.h>

const int NUMPORTS= 24;
const int NUMTESTS= 5;
const int REPEAT= 10;

int Stats(float T[], float& Max, float& Min, float& Avg, float& Var);

void WriteLogs(float T[NUMPORTS][NUMTESTS][REPEAT]);

void StatLogs();

int main(int argc, char* argv[])
{
  // number of lines in a log file
  if (argc <2 || argc > 4)
    {
      cout << "Invalid command line:"<<endl;
      for( int n=0;n<argc;n++)
	{
	  cout << argv[n]<<" ";
	}
      cout << "\nSyntax is: "<<argv[0]<<" [-c] [-l] [-o|<filename>]"<<endl;
      cout << argv[0]<< " takes a log file of times (output of time command)";
      cout << "\nand performs some statistical analyses on them.";
      cout << "\nThis program also generates a condensed time file, stripped";
      cout << "\nof all uneccesary data, and stores it in <filename>.c for";
      cout << "\nlater reuse. The -c option takes a condensed file to get";
      cout << "\nthe statistics. The -l (ell) option makes a log file for";
      cout << "\neach type of operation and combines stats from many files.";
      cout << "\nThe -o option means output only. It is used only with the";
      cout << "\n-c and -l options and repeats the statistical calculations";
      cout << "\non the log files without adding more data to them.";
      cout << "\nThe current settings are "<<NUMPORTS<<" Ports, ";
      cout << NUMTESTS <<" tests, and\n"<<REPEAT<<"repetitions."<<endl;
      return -1;
    }
  bool cfile= (strcmp(argv[1],"-c")==0);
  bool portlogs=false;
  if (argc >=3)
    portlogs= (strcmp(argv[2],"-l")==0);
  char* filename;
  char filename2[30];
  if (!cfile && !portlogs)
    filename=argv[1];
  else if (cfile && portlogs)
    filename=argv[3];
  else
    filename=argv[2];
  ifstream log;
  strncpy(filename2,filename,30);
  strcat(filename2,".c");
  ofstream clog;
  bool outonly=false;
  if (strcmp(filename,"-o")==0)
    outonly=true;
  else
    log.open(filename);
  if(!cfile)
    clog.open(filename2);

  cout << setiosflags(ios::right | ios::fixed | ios::showpoint);
  cout <<setprecision(3);

  //cout << "Debug: cf "<<int(cfile) <<" ports"<<int(portlogs)<< "  ";
  //cout << "File:"<<filename<<"  2:"<<filename2<<endl;
  
  if (outonly)
    {
      StatLogs();
      return 0;
    }
  
  // Read all data into T - for 24x5x10, holds 1200 floats x 4 bytes = 4.8KB
  float T[NUMPORTS][NUMTESTS][REPEAT];
  char str[10]="         ";
  for(int x=0; x<NUMPORTS ; x++)
    for(int y=0; y<REPEAT ; y++)
      {
	for(int z=0; z<NUMTESTS;z++)
	  {
	    log >> T[x][z][y];
	    if (!cfile)
	      {
		while(strcmp(str,"sys")!=0)
		  log >>str;
		str="         ";
		clog << T[x][z][y]<< " ";
	      }
	  }
	if (!cfile)
	  clog << "\n";
      }
  log.close();
  clog.close();

  if (portlogs)
    {
      WriteLogs(T);
      StatLogs();
      return 0;
    }
  
  cout << "Port| Operation            | Avg.  | Var.   | Max. | Min. |\n";
  cout << "----+----------------------+-------+--------+------+------+\n";

  float max, min, avg, var;
  float MX[NUMTESTS],MN[NUMTESTS],AV[NUMTESTS],VR[NUMTESTS];
  for(int y=0; y<NUMTESTS ; y++)
    {
      MX[y]=0;
      MN[y]=0;
      AV[y]=0;
      VR[y]=0;
    }
  
  for(int x=0; x<NUMPORTS ; x++)
    for(int y=0; y<NUMTESTS ; y++)
      {
	cout << setw(3) << x+1 << " | ";
	switch (y)
	  {
	  case 0:
	    cout << "Disable             ";
	    break;
	  case 1:
	    cout << "Enable, Auto-config.";
	    break;
	  case 2:
	    cout << "Full Duplex 100MBits";
	    break;
	  case 3:
	    cout << "Half Duplex 10MBits ";
	    break;
	  case 4:
	    cout << "Auto-configuration  ";
	    break;
	  default:
	    cout << "Unknown             ";
	  }
	Stats(T[x][y], max, min, avg, var);
	cout << setprecision(3);
	cout << " | " << avg;
	cout << setprecision(4);
	cout << " | " << var;
	cout << setprecision(2);
	cout << " | " << max;
	cout << " | " << min;
	cout << " | ";
	if ( (avg + var > max) && (avg - var <  min) )
	  cout << "***1Var*** ";
	if ( (avg + 2*var < max) || (avg - 2*var > min) )
	  cout << "2*Var ";
	if ( var < .15 )
	  cout << "(LowVar) ";
	if ( var > .6 )
	  cout << "HighVar ";
	if ( var > .4 && var <=.6)
	  cout << "MedVar ";
	cout << endl;
	MX[y]+=max;
	MN[y]+=min;
	AV[y]+=avg;
	VR[y]+=var;
      }
  
  cout << "\n"<<endl;

  cout << "Operation            | Avg.  | Var.   | Max. | Min. |\n";
  cout << "---------------------+-------+--------+------+------+\n";
  for(int y=0; y<NUMTESTS ; y++)
    {
      switch (y)
	{
	case 0:
	  cout << "Disable             ";
	  break;
	case 1:
	  cout << "Enable, Auto-config.";
	  break;
	case 2:
	  cout << "Full Duplex 100MBits";
	  break;
	case 3:
	  cout << "Half Duplex 10MBits ";
	  break;
	case 4:
	    cout << "Auto-configuration  ";
	    break;
	default:
	  cout << "Unknown             ";
	}
      cout << setprecision(3);
      cout << " | " << (AV[y]/NUMPORTS);
      cout << setprecision(4);
      cout << " | " << (VR[y]/NUMPORTS);
      cout << setprecision(2);
      cout << " | " << (MX[y]/NUMPORTS);
      cout << " | " << (MN[y]/NUMPORTS);
      cout <<endl;
    } 

  cout << endl;

}  
      


int Stats(float T[], float& Max, float& Min, float& Avg, float& Var)
{

  float Total=0;
  Max=T[0];
  Min=T[0];

  for ( int n=0; n<REPEAT ; n++)
    {
      if (T[n]>Max)
	Max=T[n];
      if (T[n]<Min)
	Min=T[n];
      Total+=T[n];
    }

  Avg = Total / REPEAT;
  Var = 0;
  float temp;
  for ( int n=0; n<REPEAT ; n++)
    {
      temp=Avg-T[n];
      Var+=temp*temp;
    }
  
  return 0;
}

void WriteLogs(float T[NUMPORTS][NUMTESTS][REPEAT])
{
  ofstream dlog("time_D.log",ios::app);
  ofstream alog("time_A.log",ios::app);
  ofstream elog("time_E.log",ios::app);

  for(int x=0; x<NUMPORTS ; x++)
    {
      dlog << x+1 << " ";
      for(int y=0; y<REPEAT ; y++)
	{
	  dlog << T[x][0][y] << " ";
	}
      dlog << "\n";
    }

  for(int x=0; x<NUMPORTS ; x++)
    {
      alog << x+1 << " ";
      for(int y=0; y<REPEAT ; y++)
	{
	  alog << T[x][1][y] << " ";
	  alog << T[x][4][y] << " ";
	}
      alog << "\n";
    }

  for(int x=0; x<NUMPORTS ; x++)
    {
      elog << x+1 << " ";
      for(int y=0; y<REPEAT ; y++)
	{
	  elog << T[x][2][y] << " ";
	  elog << T[x][3][y] << " ";
	}
      elog << "\n";
    }

  alog.close();
  dlog.close();
  elog.close();

}

void StatLogs()
{
  ifstream dlog("time_D.log");
  ifstream alog("time_A.log");
  ifstream elog("time_E.log");

  float Mean[NUMPORTS][3];
  float Var[NUMPORTS][3];
  int Num[NUMPORTS][3];

  for (int x=0; x< NUMPORTS ; x++)
    for (int y=0;y<3 ; y++)
      {
	Mean[x][y]=0;
	Var[x][y]=0;
	Num[x][y]=0;
      }

  int port;
  float inval;
  float temp;
  
  while( ! dlog.eof())
    {
      dlog >> port;
      port--;
      for ( int i = 0; i< REPEAT ; i++)
	{
	  dlog >> inval;
	  Mean[port][0]=((Mean[port][0]*Num[port][0])+inval)/(Num[port][0]+1);
	  Num[port][0]++;
	}
    }
  
  while( ! alog.eof())
    {
      alog >> port;
      port--;
      for ( int i = 0; i< REPEAT ; i++)
	{
	  alog >> inval;
	  Mean[port][1]=((Mean[port][1]*Num[port][1])+inval)/(Num[port][1]+1);
	  Num[port][1]++;
	  alog >> inval;
	  Mean[port][1]=((Mean[port][1]*Num[port][1])+inval)/(Num[port][1]+1);
	  Num[port][1]++;
	}
    }
  
  while( ! elog.eof())
    {
      elog >> port;
      port--;
      for ( int i = 0; i< REPEAT ; i++)
	{
	  elog >> inval;
	  Mean[port][2]=((Mean[port][2]*Num[port][2])+inval)/(Num[port][2]+1);
	  Num[port][2]++;
	  elog >> inval;
	  Mean[port][2]=((Mean[port][2]*Num[port][2])+inval)/(Num[port][2]+1);
	  Num[port][2]++;
	}
    }

  //At this point, all means have been calculated.
  //To get variances, we have to take each point against the mean again.

  dlog.close();
  alog.close();
  elog.close();
  dlog.open("time_D.log");
  alog.open("time_A.log");
  elog.open("time_E.log");

  while( ! dlog.eof())
    {
      dlog >> port;
      port--;
      for ( int i = 0; i< REPEAT ; i++)
	{
	  dlog >> inval;
	  temp= Mean[port][0] - inval;
	  Var[port][0]+=(temp*temp);
	}
    }
  
  while( ! alog.eof())
    {
      alog >> port;
      port--;
      for ( int i = 0; i< REPEAT ; i++)
	{
	  alog >> inval;
	  temp= Mean[port][1] - inval;
	  Var[port][1]+=(temp*temp);
	  alog >> inval;
	  temp= Mean[port][1] - inval;
	  Var[port][1]+=(temp*temp);
	}
    }
 
  while( ! elog.eof())
    {
      elog >> port;
      port--;
      for ( int i = 0; i< REPEAT ; i++)
	{
	  elog >> inval;
	  temp= Mean[port][2] - inval;
	  Var[port][2]+=(temp*temp);
	  elog >> inval;
	  temp= Mean[port][2] - inval;
	  Var[port][2]+=(temp*temp);
	}
    }

  dlog.close();
  alog.close();
  elog.close();
  
  cout << "Disable:"<<setw(4)<<Num[0][0]<<" Times/port  ";
  cout << "Auto-Config:"<<setw(4)<<Num[0][1]<<" Times/port  ";
  cout << "Explicit:"<<setw(4)<<Num[0][2]<<" Times/port\n";
  cout << "Port    Mean Variance    ";
  cout << "Port    Mean Variance        ";
  cout << "Port    Mean Variance\n";

  cout << setiosflags(ios::right | ios::fixed | ios::showpoint);
  cout << setprecision(3);

  for (int x=0; x< NUMPORTS ; x++)
    {
      cout << setw(3)<<x+1;
      cout << setw(9)<<Mean[x][0];
      cout << setw(9)<<Var[x][0];
      cout << setw(7)<<x+1;
      cout << setw(9)<<Mean[x][1];
      cout << setw(9)<<Var[x][1];
      cout << setw(11)<<x+1;
      cout << setw(9)<<Mean[x][2];
      cout << setw(9)<<Var[x][2];
      cout << "\n";
    }
 
}
