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

package protogeni
{
	import com.mattism.http.xmlrpc.JSLoader;
	
	import flash.events.IEventDispatcher;
	import flash.external.ExternalInterface;
	import flash.net.URLLoader;
	import flash.net.URLRequest;
	import flash.net.URLVariables;
	import flash.net.navigateToURL;
	import flash.system.Security;
	import flash.utils.Dictionary;
	
	import mx.core.FlexGlobals;
	
	/**
	 * Common functions used for web stuff
	 * 
	 * @author mstrum
	 * 
	 */
	public final class NetUtil
	{
		public static function showBecomingAUser():void
		{
			navigateToURL(new URLRequest("https://www.protogeni.net/trac/protogeni/wiki/FlackManual#BecomingaUser"), "_blank");
		}
		public static function showManual():void
		{
			navigateToURL(new URLRequest("https://www.protogeni.net/trac/protogeni/wiki/FlackManual"), "_blank");
		}
		public static function showTutorial():void
		{
			navigateToURL(new URLRequest("https://www.protogeni.net/trac/protogeni/wiki/FlackTutorial"), "_blank");
		}
		
		public static function openWebsite(url:String):void
		{
			navigateToURL(new URLRequest(url), "_blank");
		}
		
		public static function openMail(recieverEmail:String, subject:String, body:String):void
		{
			var mailRequest:URLRequest = new URLRequest("mailto:" + recieverEmail);
			var mailVariables:URLVariables = new URLVariables();
			mailVariables.subject = subject;
			mailVariables.body = body;
			mailRequest.data = mailVariables;
			navigateToURL(mailRequest, "_blank");
		}
		
		public static function runningFromWebsite():Boolean {
			return FlexGlobals.topLevelApplication.url.toLowerCase().indexOf("http") == 0;
		}
		
		public static function tryGetBaseUrl(url:String):String
		{
			var hostPattern:RegExp = /^(http(s?):\/\/([^\/]+))(\/.*)?$/;
			var match : Object = hostPattern.exec(url);
			if (match != null)
				return match[1];
			else
				return url;
		}
		
		public static function getBrowserName():String
		{
			var browser:String;
			var browserAgent:String = ExternalInterface.call("function getBrowser(){return navigator.userAgent;}");
			
			if(browserAgent == null)
				return "Undefined";
			else if(browserAgent.indexOf("Firefox") >= 0)
				browser = "Firefox";
			else if(browserAgent.indexOf("Safari") >= 0)
				browser = "Safari";
			else if(browserAgent.indexOf("MSIE") >= 0)
				browser = "IE";
			else if(browserAgent.indexOf("Opera") >= 0)
				browser = "Opera";
			else
				browser = "Undefined";
			
			return (browser);
		}
		
		private static var visitedSites:Dictionary = new Dictionary();
		public static function checkLoadCrossDomain(url:String,
													protogeniSite:Boolean = true,
													force:Boolean = false):void
		{
			if(Main.useJavascript && !force)
				return;
			var baseUrl:String = tryGetBaseUrl(url);
			if (visitedSites[baseUrl] != true)
			{
				visitedSites[baseUrl] = true;
				var crossdomainUrl:String = baseUrl;
				if(protogeniSite)
					crossdomainUrl += "/protogeni/crossdomain.xml";
				else
					crossdomainUrl += "/crossdomain.xml";
				LogHandler.appendMessage(new LogMessage(crossdomainUrl,
														"Loading CrossDomain",
														"Attempting to load a crossdomain.xml file so that calls may be made with the server located there.",
														false,
														LogMessage.TYPE_OTHER));
				Security.loadPolicyFile(crossdomainUrl);
			}
		}
		
		public static function GetLoader():IEventDispatcher {
			if(Main.useJavascript)
				return new JSLoader();
			else
				return new URLLoader();
		}
	}
}