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
	
	import protogeni.StringUtil;
	import protogeni.Util;
	import protogeni.resources.AggregateManager;
	import protogeni.resources.GeniManager;
	import protogeni.resources.ProtogeniRspecProcessor;
	
	/**
	 * Gets the GENI AM API version used by the manager using the GENI AM API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestGetVersionAm extends Request
	{
		private var aggregateManager:GeniManager;
		
		public function RequestGetVersionAm(newManager:GeniManager):void
		{
			super("GetVersion (" + StringUtil.shortenString(newManager.Url, 15) + ")",
				"Getting the version of the aggregate manager for " + newManager.Hrn,
				CommunicationUtil.getVersionAm,
				true,
				true,
				true);
			ignoreReturnCode = true;
			aggregateManager = newManager;
			
			op.setExactUrl(aggregateManager.Url);
		}
		
		// Should return Request or RequestQueueNode
		override public function complete(code:Number, response:Object):*
		{
			var r:Request = null;
			try
			{
				if(response.geni_api != null)
					aggregateManager.Version = response.geni_api;
				aggregateManager.inputRspecVersions = new Vector.<Number>();
				aggregateManager.outputRspecVersions = new Vector.<Number>();
				var maxInputRspecVersion:Number = -1;
				var usesProtogeniRspec:Boolean = false;
				if(response.request_rspec_versions != null) {
					for each(var requestRspecVersion:Object in response.request_rspec_versions) {
						if(String(requestRspecVersion.type).toLowerCase() == "protogeni") {
							usesProtogeniRspec = true;
							var requestVersion:Number = Number(requestRspecVersion.version);
							aggregateManager.inputRspecVersions.push(requestVersion);
							if(requestVersion > maxInputRspecVersion)
								maxInputRspecVersion = requestVersion;
						}
					}
				}
				var maxOutputRspecVersion:Number = -1;
				if(response.ad_rspec_versions != null) {
					for each(var adRspecVersion:Object in response.ad_rspec_versions) {
						if(String(adRspecVersion.type).toLowerCase() == "protogeni") {
							usesProtogeniRspec = true;
							var adVersion:Number = Number(adRspecVersion.version);
							aggregateManager.outputRspecVersions.push(adVersion);
							if(adVersion > maxOutputRspecVersion)
								maxOutputRspecVersion = adVersion;
						}
					}
				}
				if(response.default_ad_rspec != null) {
					if(String(response.default_ad_rspec.type).toLowerCase() == "protogeni") {
						usesProtogeniRspec = true;
						aggregateManager.outputRspecDefaultVersion = Number(response.default_ad_rspec.version);
					}
				}
				
				// Make sure aggregate uses protogeni if it supports it
				if(usesProtogeniRspec) {

					// Set output version
					aggregateManager.outputRspecVersion = Math.min(Util.defaultRspecVersion, maxOutputRspecVersion);
					
					// Set input version
					aggregateManager.inputRspecVersion = Math.min(Util.defaultRspecVersion, maxInputRspecVersion);
					
					aggregateManager.rspecProcessor = new ProtogeniRspecProcessor(aggregateManager);
				}
				
				// if supported, switch to pg_rspec
				r = new RequestListResourcesAm(aggregateManager);
				r.forceNext = true;
			}
			catch(e:Error)
			{
			}
			
			return r;
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):*
		{
			aggregateManager.errorMessage = event.toString();
			aggregateManager.errorDescription = "";
			if(aggregateManager.errorMessage.search("#2048") > -1)
				aggregateManager.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + aggregateManager.VisitUrl();
			else if(aggregateManager.errorMessage.search("#2032") > -1)
				aggregateManager.errorDescription = "IO error, possibly due to the server being down";
			else if(aggregateManager.errorMessage.search("timed"))
				aggregateManager.errorDescription = event.text;
			
			aggregateManager.Status = GeniManager.STATUS_FAILED;
			Main.geniDispatcher.dispatchGeniManagerChanged(aggregateManager);
			
			return null;
		}
		
		override public function cancel():void
		{
			aggregateManager.Status = GeniManager.STATUS_UNKOWN;
			Main.geniDispatcher.dispatchGeniManagerChanged(aggregateManager);
			op.cleanup();
		}
	}
}
