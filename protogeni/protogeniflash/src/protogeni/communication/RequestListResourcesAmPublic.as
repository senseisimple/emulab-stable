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
  import flash.utils.ByteArray;
  
  import mx.utils.Base64Decoder;
  
  import protogeni.Util;
  import protogeni.resources.AggregateManager;
  import protogeni.resources.GeniManager;

  public class RequestListResourcesAmPublic extends Request
  {
	  
    public function RequestListResourcesAmPublic(newAm:AggregateManager) : void
    {
		super("ListResourcesAmPublic (" + Util.shortenString(newAm.Url, 15) + ")", "Listing resources for " + newAm.Url, null, true, true, false);
		am = newAm;
		op.setExactUrl(am.Url);
		op.type = Operation.HTTP;
		am.lastRequest = this;
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		var r:Request = null;
		
		try
		{
			am.Rspec = new XML(response);
			am.rspecProcessor.processResourceRspec(cleanup);
			
		} catch(e:Error)
		{
			//am.errorMessage = response;
			//am.errorDescription = CommunicationUtil.GeniresponseToString(code) + ": " + am.errorMessage;
			am.Status = GeniManager.STATUS_FAILED;
			this.removeImmediately = true;
			Main.geniDispatcher.dispatchGeniManagerChanged(am);
		}
		
		return r;
	}

    override public function fail(event : ErrorEvent, fault : MethodFault) : *
    {
		am.errorMessage = event.toString();
		am.errorDescription = event.text;
		if(am.errorMessage.search("#2048") > -1)
			am.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + am.VisitUrl();
		else if(am.errorMessage.search("#2032") > -1)
			am.errorDescription = "IO error, possibly due to the server being down";
		
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
	
	override public function cleanup():void
	{
		if(am.Status == GeniManager.STATUS_INPROGRESS)
			am.Status = GeniManager.STATUS_FAILED;
		running = false;
		Main.geniHandler.requestHandler.remove(this, false);
		Main.geniDispatcher.dispatchGeniManagerChanged(am);
		op.cleanup();
		Main.geniHandler.requestHandler.tryNext();
	}

    private var am:AggregateManager;
  }
}
