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
	import flash.system.Security;
	import flash.utils.Dictionary;
	
	import mx.controls.Alert;
	import mx.core.FlexGlobals;
	
	import protogeni.GeniDispatcher;
	import protogeni.GeniHandler;
	import protogeni.Util;
	import com.mattism.http.xmlrpc.JSLoader;
	
  public class Main
  {
	// Returns the main class
	public static function Application():protogeniflash {
		return mx.core.FlexGlobals.topLevelApplication as protogeniflash;
	}
	
	public static function checkLoadCrossDomain(url:String, protogeniSite:Boolean = true):void
	{
		var baseUrl:String = Util.tryGetBaseUrl(url);
		if (visitedSites[baseUrl] != true)
		{
			visitedSites[baseUrl] = true;
			if(protogeniSite)
				Security.loadPolicyFile(baseUrl + "/protogeni/crossdomain.xml");
			else
				Security.loadPolicyFile(baseUrl + "/crossdomain.xml");
		}
	}
	
	public static function setCertBundle(c:String):void
	{
		certBundle = c;
	        JSLoader.setServerCertificate(Main.certBundle);
	}

	[Bindable]
	public static var geniHandler:GeniHandler;
	public static var geniDispatcher:GeniDispatcher;
	public static var log : LogRoot = null;
	private static var visitedSites:Dictionary = new Dictionary();
	public static var certBundle:String;
	public static var debugMode:Boolean = false;
	[Bindable]
	public static var useJavascript:Boolean = false;
	
	public static var protogeniOnly:Boolean = false;
  }
}
