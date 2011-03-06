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

  public final class RequestGetVersion extends Request
  {
    public function RequestGetVersion(newCm:ProtogeniComponentManager) : void
    {
      super("GetVersion (" + StringUtil.shortenString(newCm.Url, 15) + ")", "Getting the version of the component manager for " + newCm.Hrn, CommunicationUtil.getVersion, true, true, true);
	  cm = newCm;
	  op.setUrl(cm.Url);
	  cm.lastRequest = this;
    }

	// Should return Request or RequestQueueNode
	override public function complete(code : Number, response : Object) : *
	{
		var r:Request = null;
		try
		{
			cm.Version = response.value.api;
			cm.inputRspecVersions = new Vector.<Number>();
			cm.outputRspecVersions = new Vector.<Number>();
			var maxInputRspecVersion:Number = -1;
			for each(var inputVersion:Number in response.value.input_rspec) {
				if(inputVersion) {
					cm.inputRspecVersions.push(inputVersion);
					if(inputVersion > maxInputRspecVersion)
						maxInputRspecVersion = inputVersion;
				}
			}
			for each(var outputVersion:Number in response.value.ad_rspec) {
				if(outputVersion) {
					cm.outputRspecVersions.push(outputVersion);
				}
			}
			cm.outputRspecDefaultVersion = Number(response.value.output_rspec);
			if(cm.outputRspecVersions.indexOf(cm.outputRspecDefaultVersion) == -1)
				cm.outputRspecVersions.push(cm.outputRspecDefaultVersion);
			
			// Set output version
			if(cm.Hrn == "utahemulab.cm" || cm.Hrn == "ukgeni.cm")
				cm.outputRspecVersion = 2;
			else
				cm.outputRspecVersion = 0.2;
			if(cm.outputRspecVersions.indexOf(cm.outputRspecVersion) == -1)
				cm.outputRspecVersions.push(cm.outputRspecVersion);
			
			// Set input version
			cm.inputRspecVersion = Math.min(Util.defaultRspecVersion, maxInputRspecVersion);
			
			cm.Level = response.value.level;
			r = new RequestDiscoverResources(cm);
			r.forceNext = true;
		}
		catch(e:Error)
		{
		}
		
		return r;
	}
	
	override public function fail(event:ErrorEvent, fault:MethodFault) : *
	{
		cm.errorMessage = event.toString();
		cm.errorDescription = "";
		if(cm.errorMessage.search("#2048") > -1)
			cm.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + cm.VisitUrl();
		else if(cm.errorMessage.search("#2032") > -1)
			cm.errorDescription = "IO error, possibly due to the server being down";
		else if(cm.errorMessage.search("timed"))
			cm.errorDescription = event.text;

		cm.Status = GeniManager.STATUS_FAILED;
		Main.geniDispatcher.dispatchGeniManagerChanged(cm);
		
		return null;
	}
	
	override public function cancel():void
	{
		cm.Status = GeniManager.STATUS_UNKOWN;
		Main.geniDispatcher.dispatchGeniManagerChanged(cm);
		op.cleanup();
	}
	
	private var cm:ProtogeniComponentManager;
  }
}
