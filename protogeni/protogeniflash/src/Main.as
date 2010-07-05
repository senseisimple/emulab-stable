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
	import mx.core.Application;
	
	import protogeni.ProtogeniHandler;
	
  public class Main
  {
	// Returns the main class
	public static function Pgmap():protogeniflash {
		return mx.core.Application.application as protogeniflash;
	}

	[Bindable]
	public static var protogeniHandler:ProtogeniHandler;
	public static var log : LogRoot = null;
  }
}
