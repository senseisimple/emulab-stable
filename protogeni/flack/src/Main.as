/* GENIPUBLIC-COPYRIGHT
* Copyright (c) 2008-2011 University of Utah and the Flux Group.
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
 
 package
{
	import mx.core.FlexGlobals;
	
	import protogeni.GeniDispatcher;
	import protogeni.GeniHandler;
	
  /**
   * Global container for things we use
   * 
   * @author mstrum
   * 
   */
  public class Main
  {
	/**
	 * Gets the main application currently running
	 * 
	 * @return Flack
	 * 
	 */	
	public static function Application():flack {
		return FlexGlobals.topLevelApplication as flack;
	}

	[Bindable]
	public static var geniHandler:GeniHandler;
	public static var geniDispatcher:GeniDispatcher = new GeniDispatcher();

	public static var debugMode:Boolean = false;
	
	[Bindable]
	public static var useJavascript:Boolean = true;
	
	public static var protogeniOnly:Boolean = false;
	
	[Bindable]
	public static var offlineMode:Boolean = false;
	
	public static var allowCaching:Boolean = true;
	
	public static const version:String = "v2011-7-6.11:33AM";
  }
}
