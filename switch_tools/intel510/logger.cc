#include <iostream.h>
#include <fstream.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
  const int times=10;
  // number of lines in a log file

  if (argc !=2)
    {
      cout << "Invalid command line:"<<endl;
      for( int n=0;n<argc;n++)
	{
	  cout << argv[n];
	}
      cout << "\nSyntax is: "<<argv[0]<<" <filename>"<<endl;
      cout << argv[0]<< " takes a log file of times (output of time command)";
      cout << "\nand performs some statistical analyses on them.";
      cout << "The current filesize setting is "<<times<<"."<<endl;
      exit(0);
    }
	
  ifstream log(argv[1]);

  float alltimes[10];
  float total=0;
  int i=0;
  char str[10]="         ";
  while( i<times)
    {
      log >> alltimes[i];       //the first item is the time
      //read the rest of the line
      while(strcmp(str,"sys")!=0)
	log >>str;
      total += alltimes[i];
      i++;
      str="         ";
    }

  float max=alltimes[0];
  float min=alltimes[0];
  float high=0;
  int numhigh=0;
  float low=0;
  int numlow=0;
  float avg= total/times;
  for ( i=0; i<times ; i++)
    {
      if (alltimes[i]>max)
	max=alltimes[i];
      if (alltimes[i]<min)
	min=alltimes[i];
      if (alltimes[i]>=avg)
	{
	  high+=alltimes[i];
	  numhigh++;
	}
      if (alltimes[i]<avg)
	{
	  low+=alltimes[i];
	  numlow++;
	}
    }

  cout << "\nStatistics for "<<argv[1]<<endl;
  cout << "=======================\n"<<endl;
  cout << "Number of Tests: "<<times<< endl;
  cout << "Total time: "<<total<<endl;
  cout << "Average time: "<<avg<<endl;
  cout << "Max. time: "<<max<<endl;
  cout << "Min. time: "<<min<<endl;
  cout << "Tests > Avg.: "<< numhigh<<endl;
  cout << "Avg. high time: " << high/numhigh <<endl;
  cout << "Tests < Avg.: "<< numlow<<endl;
  cout << "Avg. low time: " << low/numlow <<endl;
  cout << endl;
  log.close();
}  
      
