/* cubicpath.h
 *
 * class with methods for generating cubic paths
 * (moved from grobot class in grobot.h)
 *
 * Dan Flickinger
 * 2004/10/26
 * 2004/10/26
 */

// time constant between waypoints 
#define PATH_QUANTUM 0.1f

// define these to keep the pathvars more organized
#define Upi_x    pathvars[0]
#define Upi_y    pathvars[1]
#define Upf_x    pathvars[2]
#define Upf_y    pathvars[3]
#define Uvi_x    pathvars[4]
#define Uvi_y    pathvars[5]
#define Uvf_x    pathvars[6]
#define Uvf_y    pathvars[7]
#define Uti      pathvars[8]
#define Utf      pathvars[9]
/* Upi_x: initial position, x
 * Upi_y: initial position, y
 * Upf_x: final position, x
 * Upf_y: final position, y
 * Uvi_x: initial velocity, x
 * Uvi_y: initial velocity, y
 * Uvf_x: final velocity, x
 * Uvf_y: final velocity, y
 * Uti: initial time
 * Utf: final time
 */
 
 
 class cubicpath {
   public:
     cubicpath();
     
     void setCubicPath();
     void makeCubicPath(float Utc);
     void convergePath (float & Wvelocity,
                        float & Wradius);
     
     float pathvars[10]; // path generator variables
   private:
     float pathcoef[8];  // path generator coefficients
     
     float x_target;  // target position x
     float y_target;  // target position y
    
     float x_target_next;  // next target x
     float y_target_next;  // next target y
    
     float x_current;  // current position x
     float y_current;  // current position y
    
     float orel_target;  // relative orientation angle to next target
}

cubicpath::cubicpath () {
  // default constructor (expand later)
  
  x_target = 0.0f;
  y_target = 0.0f;
  
  x_target_next = 0.0f;
  y_target_next = 0.0f;
  
  x_current = 0.0f;
  y_current = 0.0f;
  
  orel_target = 0.0f;
}

void cubicpath::setCubicPath () {
  // generate a cubic trajectory
  
  // set time offset
  float Tfinal = Utf - Uti;

  // set up coefficients
  pathcoef[0] = Upi_x;
  pathcoef[1] = Upi_y;
  
  pathcoef[2] = Uvi_x;
  pathcoef[3] = Uvi_y;
  
  pathcoef[4] = (3 * (Upf_x - Upi_x) - Tfinal * (2 * Uvi_x + Uvf_x)) / (pow(Tfinal,2));
  pathcoef[5] = (3 * (Upf_y - Upi_y) - Tfinal * (2 * Uvi_y + Uvf_y)) / (pow(Tfinal,2));
  
  pathcoef[6] = (-2 * (Upf_x - Upi_x) + Tfinal * (Uvi_x + Uvf_x)) / (pow(Tfinal,3));
  pathcoef[7] = (-2 * (Upf_y - Upi_y) + Tfinal * (Uvi_y + Uvf_y)) / (pow(Tfinal,3));
  
}



void cubicpath::makeCubicPath (float Utc) {
  // generate a cubic path point for the current time + 1

  // need to generate point on step in the future in order to get
  // an orientation for the current target point

  if (Utc == Uti) {
    // the current time is the initial time
    // no point has been generated yet
    
    // get current time, with offset
    float Tcurrent = Utc - Uti;
    
    // calculate current position in path
    // WARNING!! Formatting may be misleading here!
    x_target = pathcoef[0] + 
                      Tcurrent * (pathcoef[2] +
                      Tcurrent * (pathcoef[4] +
                      Tcurrent * (pathcoef[6])));
    y_target = pathcoef[1] +
                      Tcurrent * (pathcoef[3] +
                      Tcurrent * (pathcoef[5] +
                      Tcurrent * (pathcoef[7])));
  } else {
    // shift the future point to the present
    
    x_target = x_target_next;
    y_target = y_target_next;
  }
  
  if (Utc == Utf) {
    // the current time is the final time
    // can't generate one point ahead!
    
    // assume straight path:
    orel_target = 0.0f;
    
  } else {
  
  
    // get current time, with offset
    float Tnext = Utc - Uti + PATH_QUANTUM;
    
    // calculate current position in path
    // WARNING!! Formatting may be misleading here!
    x_target_next = pathcoef[0] + 
                      Tnext * (pathcoef[2] +
                      Tnext * (pathcoef[4] +
                      Tnext * (pathcoef[6])));
    y_target_next = pathcoef[1] +
                      Tnext * (pathcoef[3] +
                      Tnext * (pathcoef[5] +
                      Tnext * (pathcoef[7])));
    
    // calculate relative angle to next target point
    orel_target = atan2(y_target_next - y_target,
                        x_target_next - x_target);
  }
}
  

void cubicpath::convergePath (float & Wvelocity,
                           float & Wradius) {

  // calculate velocity and turning radius
  // (assuming a simple circular arc to the target point.)
  
  // offset the target with the current position
  float x_delta = x_target - x_current;
  float y_delta = y_target - y_current;
  
  
  // calculate radius:
  float orel_target_sin = 2 * sin(orel_target / 2);
  if (orel_target_sin == 0.0f) {
    // infinite turning radius, set to maximum
    Wradius = 100.0f;
  } else {
    // some finite turning radius
    Wradius = (sqrt(pow(x_delta,2) + pow(y_delta,2))) / orel_target_sin;
  }
  
  // calculate the velocity based on the arclength
  // (assuming 10 ms to the next point.)
  Wvelocity = Wradius * orel_target / PATH_QUANTUM;
}
