/* Garcia robot class methods
 *
 * Dan Flickinger
 *
 * 2004/11/16
 * 2004/12/15
 */

#include "grobot.h"
#include "gcallbacks.h"


grobot::grobot() {
  // default constructor
  aIO_GetLibRef(&ioRef, &err);
  
  
  Vl = 0.0f;
  Vr = 0.0f;
  
  dleft = 0.0f;
  dright = 0.0f;
  
  dt_init = 0.0f;
  dx_est = 0.0f;
  dy_est = 0.0f;
  
  
  gotolock = 0;
  gotocomplete = 0;
  
  gotomexec = 0;
  gotomcomplete = 0;
  
  gotop1 = 0;
  gotom1 = 0;
  gotop2 = 0;
  

}



grobot::~grobot() {
  // grobot the DESTRUCTOR
  aIO_ReleaseLibRef(ioRef, &err);
 }

 

void grobot::estop() {
  // emergency stop
 
  // flush everything
  garcia.flushQueuedBehaviors();
  
//   acpValue abortI((int)(aGARCIA_ERRFLAG_ABORT));
//   garcia.setNamedValue("status", &abortI);
  
  
  // clear out goto command
  gotolock = 0;

}
  
void grobot::setWheels(float Vl, float Vr) {
  // set wheel speeds
  
//  float V_avg;
  
 
  
  acpValue lVelocity((float)(Vl));
  acpValue rVelocity((float)(Vr));
//  acpValue enable((int)(1));
 
  garcia.setNamedValue("damped-speed-left", &lVelocity);
  garcia.setNamedValue("damped-speed-right", &rVelocity);
   
  // Get the null primitive running if needed
  createNULLbehavior();
  
  // The front/rear IR rangers should be enabled automatically
  
  // calculate the threshold for detection based on the average speed
  //   V_avg = (Vl + Vr) / 2.0f;
  //   acpValue IR_threshold((float)(V_avg / 10.0f));
}



void grobot::setvPath(float Wv, float Wr) {
  // set wheelspeeds according to a path

  float b = 0.0f;
  float Vleft = 0.0f;
  float Vright = 0.0f;
  
  // b: velocity difference, v: velocity, a: track width, r: turning radius
  // vr: right wheel velocity, vl: left wheel velocity
  // b = (v*a)/(2*r)
  // vr = v+b , vl = v-b

  // based on:
  // r = (a/2) * (vr + vl)/(vr - vl)
 
  if (Wr == 0.0f) {
    // zero turning radius
    Vleft = -Wv;
    Vright = Wv;
  } else if (Wr >= 100.0f) {
    // 100 meters or greater, drive straight
    Vleft = Wv;
    Vright = Wv;
  } else {
    // turning 
    b = (Wv * TRACK_WIDTH) / (2 * Wr);
    Vleft = Wv + b;
    Vright = Wv - b;
  }

  setWheels(Vleft, Vright); 
  
}



void grobot::pbMove(float mdisplacement) {
  // execute a move primitive
  
  if (mdisplacement != 0.0f) {
    // send the move to the robot
    std::cout << "Move length: " << mdisplacement << std::endl;
    
    acpValue moveLength((float)(mdisplacement));
    pBehavior = garcia.createNamedBehavior("move", "move1");
    pBehavior->setNamedValue("distance", &moveLength);
  
    createPRIMbehavior(CBT_MOVE);
  } else {
    // a zero length move will fuckup the robot!
    std::cout << "ZERO LENGTH MOVE" << std::endl;
    // fake a successful move, and increment the callback counts
    // if this is part of a goto sequence of commands
    if (1 == gotolock) {
      ++gotomexec;
      ++gotomcomplete;
      
      dx_est = 0.0f; // move nowhere!
      dy_est = 0.0f; // move nowhere!
    }
  }
  
}

    
    
void grobot::pbPivot(float pangle) {
  // execute a pivot primitive
  
  int numturns;
  
  // make the pivot smarter
  if (fabs(pangle) == 2*M_PI) {
    // this is zero! Who sends this shit??
    //pangle = 0.0f;
    // PROBLEM: if the garcia is told to pivot 0.0,
    // !!!! BAD THINGS WILL HAPPEN !!!!
  }
  
  if (fabs(pangle) > 2*M_PI) {
    // no dancing! Reduce this.
    numturns = (int)(pangle / (2*M_PI));
    pangle = pangle - (2*M_PI * numturns);
  }
  
  if (fabs(pangle) > M_PI) {
    // turn the short way
    if (pangle >= 0) {
      pangle = -(2*M_PI - pangle);
    } else {
      pangle = pangle + 2*M_PI;
    }
  }
  
  if (pangle != 0.0f) {
    // send the pivot to the robot
    std::cout << "Pivot angle: " << pangle << std::endl;
    
    acpValue pivotAngle((float)(pangle));
    pBehavior = garcia.createNamedBehavior("pivot", "pivot1");
    pBehavior->setNamedValue("angle", &pivotAngle);
  
    createPRIMbehavior(CBT_PIVOT);
  } else {
    // a zero angle pivot will fuckup the robot!
    std::cout << "ZERO ANGLE PIVOT" << std::endl;
    
    // fake a successful pivot, and increment the callback counts
    // if this is part of a goto sequence of commands
    if (1 == gotolock) {
      ++gotomexec;
      ++gotomcomplete;
    }
    
  }
  
}



void grobot::dgoto(float Dx, float Dy, float Rf) {
  // goto a defined point and orientation
  
  // make sure that a goto is not currently executing
  if (0 == gotolock) {
   
    // set lock to anticipate first move
    gotolock = 1;
    
    
    // initialize the status variables
    gotocomplete = 0;
    
    gotomexec = 0;
    gotomcomplete = 0;
    
    gotop1 = 0;
    gotom1 = 0;
    gotop2 = 0;
     
    // calculate rotation components
    dt_init = atan2(Dy, Dx); // dt_init is private to grobot
    float Rfr = Rf - dt_init;
    float moveL = sqrt((pow(Dx,2)) + (pow(Dy,2)));

    if (0.0f == dt_init && 0.0f == moveL && 0.0f == Rfr) {
      set_gotocomplete();
    } else {
      // execute primitives

      pbPivot(dt_init);
      pbMove(moveL);
      pbPivot(Rfr);
    }
    
    
  } else {
    // if a goto is already executing, drop the command
    std::cout << "GOTO COMMAND ALREADY EXECUTING" << std::endl;
  }
  
}



void grobot::resetPosition() {
  // zeros the current odometer readings

  acpValue zvalue(0.0f);  
  garcia.setNamedValue("distance-left", &zvalue);
  garcia.setNamedValue("distance-right", &zvalue);
  
  dleft = 0.0f;
  dright = 0.0f;
  
  dx_est = 0.0f;
  dy_est = 0.0f;
  
}



void grobot::updatePosition() {
  // updates the current position
  
  // get wheel odometry:
  dleft = garcia.getNamedValue("distance-left")->getFloatVal();
  dright = garcia.getNamedValue("distance-right")->getFloatVal();

}



float grobot::getArclen() {
  // return the current arc length, assuming a nice circular arc
  // (MAKE SURE TO RESET THE POSITION AT THE BEGINNING OF EACH SEGMENT!)
  
  updatePosition();  
  return (dleft + dright) / 2.0f;
}



void grobot::getDisplacement(float &dxtemp, float &dytemp) {
  // return the current displacement (for a goto move)
 
  dxtemp = dx_est;
  dytemp = dy_est;
}



int grobot::getGstatus() {
  // return the status of the garcia robot --
  // for the current or previous primitive.
  return garcia.getNamedValue("status")->getIntVal();
}



int grobot::getGOTOstatus() {
  // return the status of the currently executing goto command
  
  int retval_gotocomplete = gotocomplete;
  
  if (0 != gotocomplete) {
    // reset the status
    gotolock = 0;
    gotocomplete = 0;
  }
  
  return retval_gotocomplete;
  
}
   
void grobot::sleepy() {
  // sleep for 50ms, allow robot to handle callbacks for another 50ms
  
  aIO_MSSleep(ioRef, 50, NULL);
  garcia.handleCallbacks(50);

}




void grobot::setCBexec(int id) {
  // set execution callback up

  std::cout << "Behavior #" << id
            << " started." << std::endl;
  
  if (0 != gotolock) {
    // a goto behavior is currently executing
    ++gotomexec;
    
    if (1 == gotomexec) {
      // set status for first pivot
      gotop1 = -1;
    } else if (2 == gotomexec) {
      // set status for move
      gotom1 = -1;
      
      // clear position estimate at the start of main move
      resetPosition();
      
    } else if (3 == gotomexec) {
      // set status for second pivot
      gotop2 = -1;
    } else {
      // oh shit: weirdness!
      std::cout << "ERROR: Execution callback count "
                << "for goto command exceeded." << std::endl;
    }
  }
  
}



void grobot::setCBstatus(int id, int stat, cb_type_t cbt) {
  // set completion callback status
  
  cout << "Behavior #" << id
       << " returned with status: " << stat
       << endl;
     
  if (0 != gotolock) {
    // a goto behavior is currently executing
    ++gotomcomplete;

    if (cbt == CBT_MOVE) {
      // get the Arclength, then estimate and store the displacement
      // (assume that the first pivot was accurate)
      float al_temp = getArclen();
      dx_est = al_temp * cos(dt_init);
      dy_est = al_temp * sin(dt_init);
    }
    
    if (1 == gotomcomplete) {
      // first pivot has finished
      gotop1 = stat;
    } else if (2 == gotomcomplete) {
      // main move has executed
      gotom1 = stat;
      
    } else if (3 == gotomcomplete) {
      // second pivot has executed
      gotop2 = stat;
      
      // the goto command has completed
      /////////////////////////////////
      
      // set the gotocomplete flag
      set_gotocomplete();
      
    } else {
      // weirdness
      std::cout << "ERROR: Completion callback count "
                << "for goto command exceeded." << std::endl;
    }
  }
  
}







//private functions:
void grobot::createNULLbehavior() {
  // create a NULL behavior if the robot is idle

  if (garcia.getNamedValue("idle")->getBoolVal()) {
    // execute NULL primitive
    
    cout << "Creating NULL primitive" << endl;
    
    acpValue acceleration(2.0f);
    pBehavior = garcia.createNamedBehavior("null", "driver");
    
    completeCB = new CallbackComplete(pBehavior, this, CBT_NONE);
    completeCBacpV.set(completeCB);
    pBehavior->setNamedValue("completion-callback", &completeCBacpV);
    
    
    pBehavior->setNamedValue("acceleration", &acceleration);
    garcia.queueBehavior(pBehavior);
  }
}



void grobot::createPRIMbehavior(cb_type_t cbt) {
  // create and execute a PRIMitive behavior
  
  // wait until garcia is ready
  int count = 0;
  while (!garcia.getNamedValue("active")->getBoolVal()) {
    std::cout << "." << std::flush;
    aIO_MSSleep(ioRef, 100, NULL);
    
    count++;
    if (count == 20) {
      std::cout << std::endl << "Robot is not responding." << std::endl;
      break;
    }
  }
  
  executeCB = new CallbackExecute(pBehavior, this);
  completeCB = new CallbackComplete(pBehavior, this, cbt);
  

  executeCBacpV.set(executeCB);
  completeCBacpV.set(completeCB);
  
  pBehavior->setNamedValue("execute-callback", &executeCBacpV);
  pBehavior->setNamedValue("completion-callback", &completeCBacpV);
  
  garcia.queueBehavior(pBehavior);
}



void grobot::set_gotocomplete() {
  // set the gotocomplete flag  
  if ((gotop1 + gotom1 + gotop2) > 0) {
    // there were problems
    gotocomplete = -1;
  } else {
    // goto completed
    gotocomplete = 1;
  }
}

