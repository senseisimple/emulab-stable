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
  import flash.utils.ByteArray;
  
  import mx.utils.Base64Decoder;
  
  import protogeni.StringUtil;
  import protogeni.resources.AggregateManager;
  import protogeni.resources.GeniManager;

  public final class RequestListResourcesAm extends Request
  {
	  
    public function RequestListResourcesAm(newAm:AggregateManager) : void
    {
		super("ListResourcesAm (" + StringUtil.shortenString(newAm.Url, 15) + ")", "Listing resources for " + newAm.Url, CommunicationUtil.listResourcesAm, true, true, false);
		ignoreReturnCode = true;
		op.timeout = 60;
		am = newAm;
		op.pushField([Main.geniHandler.CurrentUser.credential]);
		op.pushField({geni_available:false, geni_compressed:true});	// geni_available:false = show all, true = show only available
		op.setExactUrl(newAm.Url);
		am.lastRequest = this;
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		var r:Request = null;
		
		try
		{
			var decodor:Base64Decoder = new Base64Decoder();
			decodor.decode(response as String);
			var bytes:ByteArray = decodor.toByteArray();
			bytes.uncompress();
			var decodedRspec:String = bytes.toString();
			
			am.Rspec = new XML(decodedRspec);
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
