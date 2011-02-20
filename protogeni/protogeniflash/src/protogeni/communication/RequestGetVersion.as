/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
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
	
	import protogeni.Util;
	import protogeni.resources.AggregateManager;
	import protogeni.resources.GeniManager;
	import protogeni.resources.ProtogeniComponentManager;

  public class RequestGetVersion extends Request
  {
    public function RequestGetVersion(newCm:ProtogeniComponentManager) : void
    {
      super("GetVersion (" + Util.shortenString(newCm.Url, 15) + ")", "Getting the version of the component manager for " + newCm.Hrn, CommunicationUtil.getVersion, true, true, true);
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
			cm.inputRspecMaxVersion = response.value.input_rspec[0];
			cm.inputRspecMinVersion = response.value.input_rspec[0];
			for each(var n:Number in response.value.input_rspec) {
				if(cm.inputRspecMaxVersion < n)
					cm.inputRspecMaxVersion = n;
				if(cm.inputRspecMinVersion > n)
					cm.inputRspecMinVersion = n;
			}
			cm.outputRspecVersion = Number(response.value.output_rspec);
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
