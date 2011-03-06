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
	import protogeni.resources.AggregateManager;
	import protogeni.resources.GeniManager;

  public final class RequestGetVersionAm extends Request
  {
    public function RequestGetVersionAm(newAm:AggregateManager) : void
    {
      super("GetVersion (" + StringUtil.shortenString(newAm.Url, 15) + ")", "Getting the version of the aggregate manager for " + newAm.Hrn, CommunicationUtil.getVersionAm, true, true, true);
	  ignoreReturnCode = true;
	  am = newAm;
	  //op.setUrl(am.Url);
	  op.setExactUrl(am.Url);
	  am.lastRequest = this;
    }

	// Should return Request or RequestQueueNode
	override public function complete(code : Number, response : Object) : *
	{
		var r:Request = null;
		try
		{
			am.Version = response.geni_api;
			r = new RequestListResourcesAm(am);
			r.forceNext = true;
		}
		catch(e:Error)
		{
		}
		
		return r;
	}
	
	override public function fail(event:ErrorEvent, fault:MethodFault) : *
	{
		am.errorMessage = event.toString();
		am.errorDescription = "";
		if(am.errorMessage.search("#2048") > -1)
			am.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + am.VisitUrl();
		else if(am.errorMessage.search("#2032") > -1)
			am.errorDescription = "IO error, possibly due to the server being down";
		else if(am.errorMessage.search("timed"))
			am.errorDescription = event.text;

		am.Status = GeniManager.STATUS_FAILED;
		Main.geniDispatcher.dispatchGeniManagerChanged(am);
		
		return null;
	}
	
	override public function cancel():void
	{
		am.Status = GeniManager.STATUS_UNKOWN;
		Main.geniDispatcher.dispatchGeniManagerChanged(am);
		op.cleanup();
	}
	
	private var am:AggregateManager;
  }
}
