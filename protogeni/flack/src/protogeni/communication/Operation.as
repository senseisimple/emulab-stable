﻿/* GENIPUBLIC-COPYRIGHT
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

package protogeni.communication
{
	import com.mattism.http.xmlrpc.ConnectionImpl;
	
	import flash.events.ErrorEvent;
	import flash.events.Event;
	import flash.events.HTTPStatusEvent;
	import flash.events.IOErrorEvent;
	import flash.events.SecurityErrorEvent;
	import flash.events.TimerEvent;
	import flash.net.URLRequest;
	import flash.utils.Timer;
	
	import protogeni.NetUtil;
	import protogeni.Util;
	
	/**
	 * Handles XML-RPC or HTTP calls for requests
	 * 
	 * @author mstrum
	 * 
	 */
	public class Operation
	{
		public static const XMLRPC:int = 0;
		public static const HTTP:int = 1;
		
		private static var SERVER_PATH:String = "/protogeni/xmlrpc/";
		
		private var module:String;
		private var method:String;
		private var url:String;
		private var param:Object;
		private var success:Function;
		private var failure:Function;
		private var node:Request;
		public var type:int;
		
		private var server:ConnectionImpl;
		private var loader:Object
		
		public var timeout:int = 60;
		
		public function Operation(qualifiedMethod:Array = null,
								  newNode:Request = null,
								  newType:int = 0):void
		{
			server = null;
			node = newNode;
			reset(qualifiedMethod);
			type = newType;
		}
		
		public function reset(qualifiedMethod:Array):void
		{
			cleanup();
			if (qualifiedMethod != null)
			{
				module = qualifiedMethod[0];
				method = qualifiedMethod[1];
			}
			clearFields();
			resetUrl();
			if (server != null)
			{
				server.cleanup();
				server = null;
			}
			success = null;
			failure = null;
		}
		
		public function resetUrl():void
		{
			url = "https://" + CommunicationUtil.defaultHost + SERVER_PATH;
			if (module != null)
			{
				url += module;
			}
		}
		
		public function setUrl(newUrl:String):void
		{
			url = newUrl;
			if (module != null)
			{
				if(url.charAt(url.length-1) == "/")
					url += module;
				else
					url += "/" + module;
			}
		}
		
		public function setExactUrl(newUrl:String):void
		{
			url = newUrl;
		}
		
		public function getUrl():String
		{
			return url;
		}
		
		public function addField(key:String, value:Object):void
		{
			if(param == null)
				setParameterized();
			param[key] = value;
		}
		
		public function pushField(value:Object):void
		{
			if(!(param is Array))
				setPositioned();
			param.push(value);
		}
		
		public function clearFields():void
		{
			param = null;
		}
		
		public function setPositioned():void
		{
			param = [];
		}
		
		public function setParameterized():void
		{
			param = new Object();
		}
		
		public var delaySeconds:int = 0;
		private var delayTimer:Timer;
		
		public function call(newSuccess:Function, newFailure:Function):void
		{
			NetUtil.checkLoadCrossDomain(url);
			success = newSuccess;
			failure = newFailure;
			
			if(delaySeconds == 0)
				makeCall();
			else {
				delayTimer = new Timer(delaySeconds*1000, 1);
				delayTimer.addEventListener(TimerEvent.TIMER, delayedCall);
				delayTimer.start();
			}
		}
		
		public function delayedCall(e:TimerEvent):void {
			delayTimer.removeEventListener(TimerEvent.TIMER, delayedCall);
			makeCall();
		}
		
		private function makeCall():void {
			switch(type)
			{
				case XMLRPC:
					try
					{
						server = new ConnectionImpl(url);
						server.timeout = timeout;
						server.addEventListener(Event.COMPLETE, callSuccess);
						server.addEventListener(ErrorEvent.ERROR, callFailure);
						server.addEventListener(SecurityErrorEvent.SECURITY_ERROR, callFailure);
						if(param is Array)
						{
							for each(var o:* in param) {
								server.addParam(o, null);
							}
						}	
						else if(param != null)
							server.addParam(param, "struct");
						server.call(method);
					}
					catch (e:Error)
					{
						LogHandler.appendMessage(new LogMessage(url, "Error", "\n\nException on XMLRPC Call: "
							+ e.toString() + "\n\n", true));
					}
					break;
				case HTTP:
					var request:URLRequest = new URLRequest(url);
					//				request.method = URLRequestMethod.GET;
					loader = NetUtil.GetLoader();
					//				loader.dataFormat = URLLoaderDataFormat.TEXT;
					loader.addEventListener(Event.COMPLETE, callSuccess);
					loader.addEventListener(ErrorEvent.ERROR, callFailure);
					loader.addEventListener(IOErrorEvent.IO_ERROR, callFailure);
					loader.addEventListener(SecurityErrorEvent.SECURITY_ERROR, callFailure);
					//loader.addEventListener(HTTPStatusEvent.HTTP_STATUS, httpStatus);
					try
					{
						loader.load(request);
					}
					catch (e:Error)
					{
						LogHandler.appendMessage(new LogMessage(url, "Error", "\n\nException on HTTP Call: "
							+ e.toString() + "\n\n", true));
					}
					break;
				default:
					throw Error("Unknown Operation type");
			}
		}
		
		public function httpStatus(event:HTTPStatusEvent):void
		{
			trace(event.toString());
		}
		
		public function getSent():String
		{
			switch(type)
			{
				case XMLRPC:
					return getSendXml();
					break;
				case HTTP:
					return "";
					break;
				default:
					return "";
			}
		}
		
		private function getSendXml():String
		{
			try
			{
				var x:XML = server._method.getXml();
			} catch(e:Error)
			{
				return "";
			}
			
			if(x == null)
				return "";
			else
				return x.toString();
		}
		
		public function getResponse():String
		{
			switch(type)
			{
				case XMLRPC:
					return getResponseXml();
					break;
				case HTTP:
					return this.loader.data as String;
					break;
				default:
					return "";
			}
		}
		
		private function getResponseXml():String
		{
			return String(server._response.data);
		}
		
		private function callSuccess(event:Event):void
		{
			//      cleanup();
			
			switch(type)
			{
				case XMLRPC:
					try{
						success(node, Number(server.getResponse().code), server.getResponse());
					} catch(e:Error) {
						success(node, null, server.getResponse());
					}
					break;
				case HTTP:
					success(node, CommunicationUtil.GENIRESPONSE_SUCCESS, loader.data);
					break;
				default:
					throw Error("Success function not indicated");
			}
			
		}
		
		private function callFailure(event:ErrorEvent):void
		{
			//      cleanup();
			switch(type)
			{
				case XMLRPC:
					failure(node, event, server.getFault());
					break;
				case HTTP:
					failure(node, event, null);
					break;
				default:
					failure(node, event, null);
			}
			
		}
		
		public function cleanup():void
		{
			if (server != null)
			{
				server.removeEventListener(Event.COMPLETE, callSuccess);
				server.removeEventListener(ErrorEvent.ERROR, callFailure);
				server.removeEventListener(SecurityErrorEvent.SECURITY_ERROR,
					callFailure);
				server.cleanup();
				server = null;
				loader = null;
			}
		}
	}
}
