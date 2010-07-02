/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2009 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
 
 // Handles some tasks that hide implimentation between the flash and map client
 
 package
{
	import mx.controls.Alert;
	
  public class Main
  {
    public static function getConsole() : LogRoot
    {
      return log;
    }

    public static function setConole(newLog : LogRoot) : void
    {
      log = newLog;
    }
    
    public static function startTimer() : void
    {
    	d = new Date();
    }

    private static var log : LogRoot = null;
    private static var d : Date = null;
  }
}
