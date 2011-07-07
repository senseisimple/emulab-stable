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

package protogeni.communication
{
	import com.mattism.http.xmlrpc.MethodFault;
	
	import flash.events.ErrorEvent;
	
	/**
	 * Common values/functions for communication in the GENI world
	 * @author mstrum
	 * 
	 */
	public final class CommunicationUtil
	{
		public static const TIMEOUT:uint = 4000;
		
		// ProtoGENI response codes
		public static const GENIRESPONSE_SUCCESS:int = 0;
		public static const GENIRESPONSE_BADARGS:int  = 1;
		public static const GENIRESPONSE_ERROR:int = 2;
		public static const GENIRESPONSE_FORBIDDEN:int = 3;
		public static const GENIRESPONSE_BADVERSION:int = 4;
		public static const GENIRESPONSE_SERVERERROR:int = 5;
		public static const GENIRESPONSE_TOOBIG:int = 6;
		public static const GENIRESPONSE_REFUSED:int = 7;
		public static const GENIRESPONSE_TIMEDOUT:int = 8;
		public static const GENIRESPONSE_DBERROR:int = 9;
		public static const GENIRESPONSE_RPCERROR:int = 10;
		public static const GENIRESPONSE_UNAVAILABLE:int = 11;
		public static const GENIRESPONSE_SEARCHFAILED:int = 12;
		public static const GENIRESPONSE_UNSUPPORTED:int = 13;
		public static const GENIRESPONSE_BUSY:int = 14;
		public static const GENIRESPONSE_EXPIRED:int = 15;
		public static const GENIRESPONSE_INPROGRESS:int = 16;
		
		//XML-RPC fault codes
		public static const XMLRPC_CURRENTLYNOTAVAILABLE:int = 503;
		
		public static function GeniresponseToString(value:int):String
		{
			switch(value) {
				case GENIRESPONSE_SUCCESS:return "Success";
				case GENIRESPONSE_BADARGS: return "Malformed arguments";
				case GENIRESPONSE_ERROR: return "General Error";
				case GENIRESPONSE_FORBIDDEN:return "Forbidden";
				case GENIRESPONSE_BADVERSION:return "Bad version";
				case GENIRESPONSE_SERVERERROR:return "Server error";
				case GENIRESPONSE_TOOBIG:return "Too big";
				case GENIRESPONSE_REFUSED:return "Refused";
				case GENIRESPONSE_TIMEDOUT:return "Timed out";
				case GENIRESPONSE_DBERROR:return "Database error";
				case GENIRESPONSE_RPCERROR:return "RPC error";
				case GENIRESPONSE_UNAVAILABLE:return "Unavailable";
				case GENIRESPONSE_SEARCHFAILED:return "Search failed";
				case GENIRESPONSE_UNSUPPORTED:return "Unsupported";
				case GENIRESPONSE_BUSY:return "Busy";
				case GENIRESPONSE_EXPIRED:return "Expired";
				case GENIRESPONSE_INPROGRESS:return "In progress";
				default: return "Other";
			}
		}
		
		private static const sa:String = "sa";
		private static const cm:String = "cm";
		private static const ses:String = "ses";
		private static const ch:String = "ch";
		private static const am:String = "am";
		
		public static const defaultHost:String = "boss.emulab.net";
		
		//public static var sesUrl:String = "https://myboss.emulab.geni.emulab.net/protogeni/xmlrpc/";
		public static const sesUrl:String = "https://www.emulab.net/protogeni/xmlrpc/";
		
		public static const chUrl:String = "https://boss.emulab.net/protogeni/xmlrpc/";
		
		// SA
		public static var getCredential:Array = new Array(sa, "GetCredential");
		public static var getKeys:Array = new Array(sa, "GetKeys");
		public static var resolve:Array = new Array(sa, "Resolve");
		public static var remove:Array = new Array(sa, "Remove");
		public static var register:Array = new Array(sa, "Register");
		public static var renewSlice:Array = new Array(sa, "RenewSlice");
		
		public static var getVersion:Array = new Array(cm, "GetVersion");
		public static var discoverResources:Array = new Array(cm, "DiscoverResources");
		public static var resolveResource:Array = new Array(cm, "Resolve");
		// AM
		public static var getVersionAm:Array = new Array(am, "GetVersion");
		public static var listResourcesAm:Array = new Array(am, "ListResources");
		public static var deleteSliverAm:Array = new Array(am, "DeleteSliver");
		public static var createSliverAm:Array = new Array(am, "CreateSliver");
		public static var sliverStatusAm:Array = new Array(am, "SliverStatus");
		public static var renewSliverAm:Array = new Array(am, "RenewSliver");
		public static var shutdownAm:Array = new Array(am, "Shutdown");
		// PL
		public static var resolvePl:Array = new Array(am, "Resolve");
		// Ticket
		public static var getTicket:Array = new Array(cm, "GetTicket");
		public static var updateTicket:Array = new Array(cm, "UpdateTicket");
		public static var redeemTicket:Array = new Array(cm, "RedeemTicket");
		public static var releaseTicket:Array = new Array(cm, "ReleaseTicket");
		// Slice
		public static var sliceStatus:Array = new Array(cm, "SliceStatus");
		public static var deleteSlice:Array = new Array(cm, "DeleteSlice");
		// Sliver
		public static var getSliver:Array = new Array(cm, "GetSliver");
		public static var sliverStatus:Array = new Array(cm, "SliverStatus");
		public static var sliverTicket:Array = new Array(cm, "SliverTicket");
		public static var createSliver:Array = new Array(cm, "CreateSliver");
		public static var startSliver:Array = new Array(cm, "StartSliver");
		public static var stopSliver:Array = new Array(cm, "StopSliver");
		public static var restartSliver:Array = new Array(cm, "RestartSliver");
		public static var updateSliver:Array = new Array(cm, "UpdateSliver");
		public static var renewSliver:Array = new Array(cm, "RenewSlice");
		
		public static var map:Array = new Array(ses, "Map");
		
		public static var listComponents:Array = new Array(ch, "ListComponents");
		public static var whoAmI:Array = new Array(ch, "WhoAmI");
		
		public static function getResponse(name:String,
										   url:String,
										   xml:String):String
		{
			return "\n-----------------------------------------\n"
			+ "RESPONSE: " + name + "\n"
				+ "URL: " + url + "\n"
				+ "-----------------------------------------\n\n"
				+ xml;
		}
		
		public static function getSend(name:String,
									   url:String,
									   xml:String):String
		{
			return "\n-----------------------------------------\n"
			+ "SEND: " + name + "\n"
				+ "URL: " + url + "\n"
				+ "-----------------------------------------\n\n"
				+ xml;
		}
		
		public static function getFailure(opName:String,
										  url:String,
										  event:ErrorEvent,
										  fault:MethodFault):String
		{
			if (fault != null)
			{
				return "FAILURE fault: " + opName + ": " + fault.getFaultString()
					+ "\nURL: " + url + "\n"
					+ "\n";
			}
			else
			{
				return "FAILURE event: " + opName + ": " + event.toString()
					+ "\nURL: " + url + "\n"
					+ "\n";
			}
		}
	}
}
