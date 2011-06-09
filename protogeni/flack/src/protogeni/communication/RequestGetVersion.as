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
	import protogeni.resources.GeniManager;
	import protogeni.resources.ProtogeniComponentManager;
	
	/**
	 * Gets version and other information for a manager using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestGetVersion extends Request
	{
		private var componentManager:ProtogeniComponentManager;
		
		public function RequestGetVersion(newManager:ProtogeniComponentManager):void
		{
			super("GetVersion (" + StringUtil.shortenString(newManager.Url, 15) + ")",
				"Getting the version of the component manager for " + newManager.Hrn,
				CommunicationUtil.getVersion,
				true,
				true,
				true);
			componentManager = newManager;
			
			op.setUrl(componentManager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			var r:Request = null;
			try
			{
				componentManager.Version = response.value.api;
				componentManager.inputRspecVersions = new Vector.<Number>();
				componentManager.outputRspecVersions = new Vector.<Number>();
				var maxInputRspecVersion:Number = -1;
				for each(var inputVersion:Number in response.value.input_rspec) {
					if(inputVersion) {
						componentManager.inputRspecVersions.push(inputVersion);
						if(inputVersion > maxInputRspecVersion)
							maxInputRspecVersion = inputVersion;
					}
				}
				for each(var outputVersion:Number in response.value.ad_rspec) {
					if(outputVersion) {
						componentManager.outputRspecVersions.push(outputVersion);
					}
				}
				componentManager.outputRspecDefaultVersion = Number(response.value.output_rspec);
				if(componentManager.outputRspecVersions.indexOf(componentManager.outputRspecDefaultVersion) == -1)
					componentManager.outputRspecVersions.push(componentManager.outputRspecDefaultVersion);
				
				// Set output version
				if(componentManager.Hrn == "utahemulab.cm" || componentManager.Hrn == "ukgeni.cm")
					componentManager.outputRspecVersion = 2;
				else
					componentManager.outputRspecVersion = 0.2;
				if(componentManager.outputRspecVersions.indexOf(componentManager.outputRspecVersion) == -1)
					componentManager.outputRspecVersions.push(componentManager.outputRspecVersion);
				
				// Set input version
				componentManager.inputRspecVersion = Math.min(Util.defaultRspecVersion, maxInputRspecVersion);
				
				componentManager.Level = response.value.level;
				r = new RequestDiscoverResources(componentManager);
				r.forceNext = true;
			}
			catch(e:Error)
			{
			}
			
			return r;
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):*
		{
			componentManager.errorMessage = event.toString();
			componentManager.errorDescription = "";
			if(componentManager.errorMessage.search("#2048") > -1)
				componentManager.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + componentManager.VisitUrl();
			else if(componentManager.errorMessage.search("#2032") > -1)
				componentManager.errorDescription = "IO error, possibly due to the server being down";
			else if(componentManager.errorMessage.search("timed"))
				componentManager.errorDescription = event.text;
			
			componentManager.Status = GeniManager.STATUS_FAILED;
			Main.geniDispatcher.dispatchGeniManagerChanged(componentManager);
			
			return null;
		}
		
		override public function cancel():void
		{
			componentManager.Status = GeniManager.STATUS_UNKOWN;
			Main.geniDispatcher.dispatchGeniManagerChanged(componentManager);
			op.cleanup();
		}
	}
}
