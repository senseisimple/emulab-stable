/* Garcia robot class methods
 *
 * Dan Flickinger
 *
 * 2004/11/16
 * 2004/12/01
 */

#include "grobot.h"


grobot::grobot() {
  // default constructor
  aIO_GetLibRef(&ioRef, &err);  
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
  
  acpValue moveLength((float)(mdisplacement));
  pBehavior = garcia.createNamedBehavior("move", "move1");
  pBehavior->setNamedValue("distance", &moveLength);
  
  createPRIMbehavior();
  
}

    
    
void grobot::pbPivot(float pangle) {
  // execute a pivot primitive
  
  acpValue pivotAngle((float)(pangle));
  pBehavior = garcia.createNamedBehavior("pivot", "pivot1");
  pBehavior->setNamedValue("angle", &pivotAngle);
  
  createPRIMbehavior();
  
}



void grobot::dgoto(float Dx, float Dy, float Rf) {
  // goto a defined point and orientation
  
  
  // calculate rotation components
  float Ri = atan2(Dy, Dx);     
  float Rfr = Rf - Ri;

  // execute primitives       
  pbPivot(Ri);
  pbMove(sqrt((pow(Dx,2)) + (pow(Dy,2))));
  pbPivot(Rfr);
  
}



void grobot::resetPosition() {
  // zeros the current odometer readings

  acpValue zvalue(0.0f);  
  garcia.setNamedValue("distance-left", &zvalue);
  garcia.setNamedValue("distance-right", &zvalue);
  
}



void grobot::updatePosition() {
  // updates the current position
  // based on wheel odometry, camera system, etc.
  
  // get wheel odometry:
  float dleft = garcia.getNamedValue("distance-left")->getFloatVal();
  float dright = garcia.getNamedValue("distance-right")->getFloatVal();
  

  // FIXME
  
  
}



float grobot::getArclen() {
  // return the current arc length, assuming a nice circular arc
  // (MAKE SURE TO RESET THE POSITION AT THE BEGINNING OF EACH SEGMENT!)
  
  
  float dleft = garcia.getNamedValue("distance-left")->getFloatVal();
  float dright = garcia.getNamedValue("distance-right")->getFloatVal();
  
  return (dleft + dright) / 2.0f;
}





int grobot::getGstatus() {
  // return the status of the garcia robot --
  // for the current or previous primitive.
  return garcia.getNamedValue("status")->getIntVal();
}


   
int grobot::sleepy() {
  // sleep for 50ms, allow robot to handle callbacks for another 50ms
  
  aIO_MSSleep(ioRef, 50, NULL);
  return garcia.handleCallbacks(50);
}




void grobot::setCBexec(int id) {
  // set execution callback up
  // FIXME
         
  cout << "Behavior #" << id
       << " started." << endl;
     
}



void grobot::setCBstatus(int id, int stat) {
  // set completion callback status
  // FIXME
  
  cout << "Behavior #" << id
       << " returned with status: " << stat
       << endl;
  
}







//private functions:
void grobot::createNULLbehavior() {
  // create a NULL behavior if the robot is idle

  if (garcia.getNamedValue("idle")->getBoolVal()) {
    // execute NULL primitive
    
    cout << "Creating NULL primitive" << endl;
    
    acpValue acceleration(2.0f);
    pBehavior = garcia.createNamedBehavior("null", "driver");
    
    completeCB = new CallbackComplete(pBehavior, this);
    completeCBacpV.set(completeCB);
    pBehavior->setNamedValue("completion-callback", &completeCBacpV);
    
    
    pBehavior->setNamedValue("acceleration", &acceleration);
    garcia.queueBehavior(pBehavior);
  }
}



void grobot::createPRIMbehavior() {
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
  completeCB = new CallbackComplete(pBehavior, this);
  

  executeCBacpV.set(executeCB);
  completeCBacpV.set(completeCB);
  
  pBehavior->setNamedValue("execute-callback", &executeCBacpV);
  pBehavior->setNamedValue("completion-callback", &completeCBacpV);
  
  garcia.queueBehavior(pBehavior);
}































