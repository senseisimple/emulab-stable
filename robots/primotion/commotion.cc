/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* Garcia robot motion program (command and interactive mode)
 *
 * Dan Flickinger
 *
 * 2004/09/13
 * 2004/11/17
 */

#include "dgrobot/commotion.h"

void interact(grobot & Robot);
void drive(char *filename, grobot & Robot);
void print_acpValue(acpValue *v);
void printHELP();


int main(int argc, char **argv)
{

  char commandIN [64];	 // command in
  char callbackMSG[128]; // callback message
  char * pCPRIM;	 // pointer to Command PRIMitive character
  char * pCARG;		 // pointer to Commmand ARGument string
  
  int fileMode = 0;	 // file in mode for commands
  
 

  // battery parameters
  float battLevel, battVoltage;
  
  // wheel odometry parameters (distance and velocity)
  float dLeft, dRight;
  float vLeft, vRight;
  
  // GOTO displacement (Dx, Dy) and final orientation angle (Rf)
  float Dx, Dy, Rf;

  ifstream fileIN;
  ofstream fileOUT;
  
  strcpy(commandIN, "");
 
  grobot robot1;
 
  
  fileOUT.open("vpos.log", ios::out | ios::app);
  if (!(fileOUT.is_open())) {
    // file not opened for some reason
    cout << "ERROR opening log file!" << endl;
  } else {
    cout << "Appending wheel position and velocity data to 'vpos.log'" << endl;
  }
  
  cout << "COMMAND: (h for help)" << endl;
     
  
  while (strcmp(commandIN, "q") != 0) {
    cout << endl << "? ";
    
    // get a command
    if (fileMode) {
      // read a line from file
      if (fileIN.is_open()) {
      if (fileIN.eof()) {
        // done reading, drop back to command mode
        cout << "Finished reading command file." << endl;
        fileIN.close();
        fileMode = 0;
      } else {
        fileIN.getline(commandIN, 64);
        cout << commandIN << endl;
      }
    } else {
      // file not open
      cout << "FILE CLOSED." << endl;;
      fileMode = 0;
    }
    } else {
      // read from console input
      cin.getline(commandIN, 64);
    }
    
    if (strcmp(commandIN, "h") == 0) {
      // print HELP     
      printHELP();
    } else {
      // not help

      
      if (strcmp(commandIN, "") !=0) {
     
     pCPRIM = strtok(commandIN, " ");
     pCARG = strtok(NULL, " ");
 

     if (strcmp(pCPRIM, "q") == 0) {
       // quit
       cout << "QUIT..." << endl;
     } else if (strcmp(pCPRIM, "s") == 0) {
       // set wheelspeed
       // (This value is persistent for all primitives.)
       
       acpValue wheelSpeed((float)(atof(pCARG)));
       robot1.garcia.setNamedValue("speed", &wheelSpeed);
       cout << "Set wheelspeed to " << pCARG << endl;
       
     } else if (strcmp(pCPRIM, "a") == 0) {
       // set stall threshold
       
       acpValue stallThreshold((float)(atof(pCARG)));
       robot1.garcia.setNamedValue("stall-threshold", &stallThreshold);
       cout << "Set stall threshold to " << pCARG << endl;
       
     } else if (strcmp(pCPRIM, "b") == 0) {
       // get battery level
       
       battLevel = robot1.garcia.getNamedValue("battery-level")->getFloatVal();
       battVoltage = robot1.garcia.getNamedValue("battery-voltage")->getFloatVal();
       cout << "Battery: " << 100 * battLevel << "%, "
            << battVoltage << " V" << endl;
       
     } else if (strcmp(pCPRIM, "o") == 0) {
       // read wheel odometry
       
       dLeft = robot1.garcia.getNamedValue("distance-left")->getFloatVal();
       dRight = robot1.garcia.getNamedValue("distance-right")->getFloatVal();
       
       cout << "Odometry: left = " << dLeft
            << ", right = " << dRight << endl;
     } else if (strcmp(pCPRIM, "i") == 0) {
       // enter interactive mode
       interact(robot1);
       
     } else if (strcmp(pCPRIM, "v") == 0) {
       // get acpValue
       
       acpValue *retValue = robot1.garcia.getNamedValue(pCARG);
       if (retValue == NULL) {
         cout << "Invalid property" << endl;
       } else {
         print_acpValue(retValue);
       }
       
     } else if (strcmp(pCPRIM, "f") == 0) {
       // set file read mode
       
       fileIN.open(pCARG, ios::in);
       if (fileIN.is_open()) {
         cout << "Opened command file: " << pCARG << endl;
         fileMode = 1;
       } else {
         cout << "FAILED to open command file: " << pCARG << endl;
         fileMode = 0;
       }
       
     } else if (strcmp(pCPRIM, "d") == 0) {
       // set file read mode for driving
       drive(pCARG, robot1);
       
     } else {
       // parse a motion command
  
 
             
       if (strcmp(pCPRIM, "m") == 0) {
         // move primitive
         cout << "Executing MOVE (distance: " << pCARG << ")" << endl;
         fileOUT << "# MOVE " << pCARG << ":" << endl;
    
         robot1.pbMove((float)(atof(pCARG)));
       }
    
       if (strcmp(pCPRIM, "p") == 0) {
         // pivot primitive
         cout << "Executing PIVOT (angle: " << pCARG << ")" << endl;
         fileOUT << "# PIVOT " << pCARG << ":" << endl;
   
         robot1.pbPivot((float)(atof(pCARG)));
       }   
       
       if (strcmp(pCPRIM, "g") == 0) {
         // goto displacement-orientation (3 primitives)
        
         cout << "Executing GOTO (Dx=";
         fileOUT << "# GOTO Dx=";
         
         Dx = atof(pCARG);
         cout << Dx << ", Dy=";
         fileOUT << Dx << " Dy=";
         
         pCARG = strtok(NULL, " ");
         Dy = atof(pCARG);
         cout << Dy << ", Rf=";
         fileOUT << Dy << " Rf=";
         
         pCARG = strtok(NULL, " ");
         Rf = atof(pCARG);
         cout << Rf << ")" << endl;
         fileOUT << Rf << ":" << endl;
         
         robot1.dgoto(Dx, Dy, Rf);
         
       }
       
       

       while (!robot1.garcia.getNamedValue("idle")->getBoolVal()) {
         robot1.garcia.handleCallbacks(100);
         
         // wheel speed is only accessible while the NULL primitive is running
         dLeft = robot1.garcia.getNamedValue("distance-left")->getFloatVal();
         dRight = robot1.garcia.getNamedValue("distance-right")->getFloatVal();
         // vLeft = robot1.garcia.getNamedValue("damped-speed-left")->getFloatVal();
         // vRight = robot1.garcia.getNamedValue("damped-speed-right")->getFloatVal();
         
         // use numerical differentiation instead
         // FIXME
         vLeft = 0.0f;
         vRight = 0.0f;
         
         // log data to outfile
         fileOUT << dLeft << " "
                 << dRight << endl;
         
         
       } // end while garcia is idle
     } // end parse a motion command
   } // end not a blank command ("")
 } // end not help
 } // end while command in is not 'q'

 
 fileOUT.close();
 return 0;
  
}


/**********************
 * FUNCTIONS          *
 **********************/



 
void interact(grobot & Robot) {
  // Keyboard interactive mode
  
  // terminal interaction
  struct termios old_mode;
  struct termios new_mode;

  tcgetattr(fileno(stdout),&old_mode);
  new_mode = old_mode;

  new_mode.c_lflag &= ~(ECHOKE | ECHOK | ECHOE | ECHO);
  //new_mode.c_lflag |= ECHONL;
  new_mode.c_lflag &= ~(ICANON);
  tcsetattr(fileno(stdout),TCSANOW,&new_mode);

  setbuf(stdin,(char*)NULL);

  
  cout << "Interactive mode: " <<
          "use numeric keypad,PgUP/DOWN or ijkm,ol,<space>" << endl;
  
  float Vleft = 0.0f;
  float Vright = 0.0f;
  
  // speed change increment
  float Vdelta = 0.02f;
  

  grobot robot1;
  
  int local_quit = 0;
  char c;
  
  while (!local_quit) {
    // interactive loop
 
    // handle the callbacks (check and see if a behavior terminated)
    robot1.garcia.handleCallbacks(10);

    fread(&c,sizeof(char),1,stdin);
 
    switch(c) {
      case '9':
      case 'o':
        // PigUp, inrease speed change
        Vdelta += 0.02f;
        break;
      case '3':
      case 'l':
        // PigDown, descrease speed change
        Vdelta -= 0.02f;
        break;
      case '8':
      case 'i':
        // UP arrow, increase forward speed
        Vleft += Vdelta;
        Vright += Vdelta;
        break;
      case '2':
      case 'm':
        // DOWN arrow, decrease forward speed
        Vleft -= Vdelta;
        Vright -= Vdelta;
        break;
      case '4':
      case 'j':
        // LEFT arrow, turn left
        Vleft -= Vdelta;
        Vright += Vdelta;
        break;
      case '6':
      case 'k':
        // RIGHT arrow, turn right
        Vleft += Vdelta;
        Vright -= Vdelta;
        break;
      case '5':
      case ' ':
        // CENTER, stop!!
        Vleft = 0.0f;
        Vright = 0.0f;
        break;
      case 'q':
        local_quit = 1;

        // Don't forget to stop the robot~!
        Vleft = 0.0f;
        Vright = 0.0f;
        break;
      default:
          ;
      }
       
      // send new speed information to the robot
      Robot.setWheels(Vleft, Vright);
    }




  Robot.garcia.flushQueuedBehaviors();  
  tcsetattr(fileno(stdout),TCSANOW,&old_mode);
}


void drive(char *filename, grobot & Robot) {
  // drive with path from a file
  
  float Vleft = 0.0f;
  float Vright = 0.0f;

  
  // open the file
  ifstream fileDRIVE; 
  fileDRIVE.open(filename, ios::in);
  
  if (!(fileDRIVE.is_open())) {
    cout << "FAILED to open drive file: " << filename << endl;
  } else {
    cout << "Opened drive file: " << filename << endl;
  
    while (!(fileDRIVE.eof())) {
      // read through file line by line
      fileDRIVE >> Vleft >> Vright;
      
      // send new speed information to the robot
      Robot.setWheels(Vleft, Vright);
      Robot.sleepy();
    }
    fileDRIVE.close();
    Robot.garcia.flushQueuedBehaviors();  
  }
  
}


void print_acpValue(acpValue *v) {
  acpValue::eType t = v->getType();
  if (t == acpValue::kInt) { cout << v->getIntVal(); }
  else if (t == acpValue::kFloat) { cout << v->getFloatVal(); }
  else if (t == acpValue::kString) { cout << v->getStringVal(); }
  else if (t == acpValue::kBoolean) {
        const char *s = (v->getBoolVal())?"true":"false";
        cout << s;
  } else {
    cout << "<UNRECOGNIZED acpValue TYPE>";
  }
  cout << endl;
}



void printHELP() {
  // print HELP
  

  cout << "HELP:" << endl
       << "q        : QUIT" << endl  
       << "f <name> : Read in command file" << endl
       << "d <name> : Drive from file" << endl
       << "b        : Battery level, voltage" << endl
       << "m 0.00   : MOVE (distance)" << endl
       << "p 0.00   : PIVOT (angle)" << endl
       << "s 0.00   : set wheel speed" << endl
       << "a 0.00   : set stall threshold" << endl
       << "o        : get wheel odometry" << endl
       << "g 0.00 0.00 0.00 : goto displacement/orientation" << endl
       << "i        : enter interactive mode" << endl
       << "v <name> : print acpValue" << endl
       << endl;
       
}

