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
  import flash.events.SecurityErrorEvent;
  import flash.utils.ByteArray;
  
  import mx.utils.Base64Decoder;
  
  import protogeni.Util;
  import protogeni.resources.AggregateManager;
  import protogeni.resources.ComponentManager;
  import protogeni.resources.GeniManager;

  public class RequestListResources extends Request
  {
	  
    public function RequestListResources(newAm:AggregateManager) : void
    {
		super("ListResources (" + Util.shortenString(newAm.Url, 15) + ")", "Listing resources for " + newAm.Url, CommunicationUtil.listResourcesAm, true, true, false);
		ignoreReturnCode = true;
		op.timeout = 60;
		am = newAm;
		op.addField("credentials", new Array(Main.protogeniHandler.CurrentUser.credential));
		var options:Object = new Object();
		options.geni_available = true;
		options.geni_compressed = true;
		op.addField("options", options);
		op.setUrl(newAm.Url);
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		try
		{
			var decodor:Base64Decoder = new Base64Decoder();
			decodor.decode(response.value);
			var bytes:ByteArray = decodor.toByteArray();
			bytes.uncompress();
			var decodedRspec:String = bytes.toString();
			
			am.Rspec = new XML(decodedRspec);
			am.processRspec(cleanup);
		} catch(e:Error)
		{
			am.errorMessage = response.output;
			am.errorDescription = CommunicationUtil.GeniresponseToString(code) + ": " + am.errorMessage;
			am.Status = GeniManager.FAILED;
			this.removeImmediately = true;
			Main.protogeniHandler.dispatchGeniManagerChanged(am);
		}
		
		return null;
	}

    override public function fail(event : ErrorEvent, fault : MethodFault) : *
    {
		am.errorMessage = event.toString();
		am.errorDescription = "";
		if(am.errorMessage.search("#2048") > -1)
			am.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + am.VisitUrl();
		else if(am.errorMessage.search("#2032") > -1)
			am.errorDescription = "IO error, possibly due to the server being down";
		else if(am.errorMessage.search("timed"))
			am.errorDescription = event.text;
		
		am.Status = GeniManager.FAILED;
		Main.protogeniHandler.dispatchGeniManagerChanged(am);

      return null;
    }
	
	override public function cancel():void
	{
		am.Status = GeniManager.UNKOWN;
		Main.protogeniHandler.dispatchGeniManagerChanged(am);
		op.cleanup();
	}
	
	override public function cleanup():void
	{
		if(am.Status == GeniManager.INPROGRESS)
			am.Status = GeniManager.FAILED;
		running = false;
		Main.protogeniHandler.rpcHandler.remove(this, false);
		Main.protogeniHandler.dispatchGeniManagerChanged(am);
		op.cleanup();
		Main.protogeniHandler.rpcHandler.start();
	}

    private var am:AggregateManager;
  }
}
