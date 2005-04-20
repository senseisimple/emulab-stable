/**
 * Implementation
 **/
module CntToLedsAndRfmM {
  provides {
    interface StdControl;
  }
  uses {
    interface CC1000Control;
  }
}
implementation {

  /**
   * Initialize the component.
   * 
   * @return Always returns <code>SUCCESS</code>
   **/
  command result_t StdControl.init() {
    return SUCCESS;
  }


  /**
   * Start things up.  This just sets the rate for the clock component.
   * 
   * @return Always returns <code>SUCCESS</code>
   **/
  command result_t StdControl.start() {
    return call CC1000Control.SetRFPower(0x0f);
  }

  /**
   * Halt execution of the application.
   * This just disables the clock component.
   * 
   * @return Always returns <code>SUCCESS</code>
   **/
  command result_t StdControl.stop() {
  }

}


