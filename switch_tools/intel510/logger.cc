#include <iostream.h>
#include <fstream.h>
#include <iomanip.h>
#include <string.h>
#include <math.h>

const int NUMPORTS= 24;
const int NUMTESTS= 5;
const int REPEAT= 10;

int Stats(float T[], float& Max, float& Min, float& Avg, float& Var);


int main(int argc, char* argv[])
{
  // number of lines in a log file
  if (argc !=2 && argc != 3)
    {
      cout << "Invalid command line:"<<endl;
      for( int n=0;n<argc;n++)
	{
	  cout << argv[n];
	}
      cout << "\nSyntax is: "<<argv[0]<<" [-c] <filename>"<<endl;
      cout << argv[0]<< " takes a log file of times (output of time command)";
      cout << "\nand performs some statistical analyses on them.";
      cout << "\nThis program also generates a condensed time file, stripped";
      cout << "\nof all uneccesary data, and stores it in <filename>.c for";
      cout << "\nlater reuse. The -c option takes a condensed file to get";
      cout << "\nthe statistics.";
      cout << "\nThe current filesize setting is "<<"."<<endl;
      exit(0);
    }
  bool cfile= (strcmp(argv[1],"-c")==0);
  char* filename;
  char filename2[30];
  if (!cfile)
    filename=argv[1];
  else
    filename=argv[2];
  ifstream log;
  strncpy(filename2,filename,30);
  strcat(filename2,".c");
  ofstream clog;

  log.open(filename);
  if(!cfile)
    clog.open(filename2);
  
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
  
  cout << "Port| Operation            | Avg.  | Var.   | Max. | Min. |\n";
  cout << "----+----------------------+-------+--------+------+------+\n";

  cout << setiosflags(ios::right | ios::fixed | ios::showpoint);

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
