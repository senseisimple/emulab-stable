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
	import com.mattism.http.xmlrpc.JSLoader;
	
	import flash.system.Security;
	import flash.utils.Dictionary;

	import mx.core.FlexGlobals;
	
	import protogeni.GeniDispatcher;
	import protogeni.GeniHandler;
	import protogeni.Util;
	
  public class Main
  {
	// Returns the main class
	public static function Application():protogeniflash {
		return FlexGlobals.topLevelApplication as protogeniflash;
	}
	
	public static function checkLoadCrossDomain(url:String, protogeniSite:Boolean = true):void
	{
		var baseUrl:String = Util.tryGetBaseUrl(url);
		if (visitedSites[baseUrl] != true)
		{
			visitedSites[baseUrl] = true;
			var crossdomainUrl:String = baseUrl;
			if(protogeniSite)
				crossdomainUrl += "/protogeni/crossdomain.xml";
			else
				crossdomainUrl += "/crossdomain.xml";
			Security.loadPolicyFile(crossdomainUrl);
			log.appendMessage(new LogMessage(crossdomainUrl, "Loading CrossDomain", "Attempting to load a crossdomain.xml file so that calls may be made with the server located there.", false, LogMessage.TYPE_OTHER));
		}
	}
	
	public static function setCertBundle(c:String):void
	{
		certBundle = c;
		if(useJavascript)
			JSLoader.setServerCertificate(certBundle);
	}

	[Bindable]
	public static var geniHandler:GeniHandler;
	public static var geniDispatcher:GeniDispatcher;

	private static var visitedSites:Dictionary = new Dictionary();
	public static var certBundle:String;
	public static var debugMode:Boolean = false;
	[Bindable]
	public static var useJavascript:Boolean = false;
	
	public static var protogeniOnly:Boolean = false;
	
	public static var log:LogHandler = new LogHandler();
  }
}
