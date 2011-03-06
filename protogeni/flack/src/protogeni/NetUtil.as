package protogeni
{
	import flash.external.ExternalInterface;
	import flash.net.URLRequest;
	import flash.net.URLVariables;
	import flash.net.navigateToURL;
	
	public final class NetUtil
	{
		public static function showSetup():void
		{
			navigateToURL(new URLRequest("https://www.protogeni.net/trac/protogeni/wiki/FlackManual#Setup"), "_blank");
		}
		public static function showManual():void
		{
			navigateToURL(new URLRequest("https://www.protogeni.net/trac/protogeni/wiki/FlackManual"), "_blank");
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
		
		public static function tryGetBaseUrl(url:String):String
		{
			var hostPattern:RegExp = /^(http(s?):\/\/([^\/]+))(\/.*)?$/;
			var match : Object = hostPattern.exec(url);
			if (match != null)
				return match[1];
			else
				return url;
		}
		
		public static  function getBrowserName():String
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
	}
}