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
  import protogeni.resources.ComponentManager;

  public class RequestDiscoverResources extends Request
  {
	  
    public function RequestDiscoverResources(newCm:ComponentManager) : void
    {
		super("DiscoverResources (" + Util.shortenString(newCm.Hrn, 15) + ")", "Discovering resources for " + newCm.Hrn, CommunicationUtil.discoverResources, true, true, false);
		op.timeout = 60;
		cm = newCm;
		op.addField("credentials", new Array(Main.protogeniHandler.CurrentUser.credential));
		op.addField("compress", true);
		op.setExactUrl(newCm.DiscoverResourcesUrl());
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			var decodor:Base64Decoder = new Base64Decoder();
			decodor.decode(response.value);
			var bytes:ByteArray = decodor.toByteArray();
			bytes.uncompress();
			var decodedRspec:String = bytes.toString();
			
			cm.Rspec = new XML(decodedRspec);
			/*try
			{
				cm.Rspec = new XML(decodedRspec);
			} catch(e:Error)
			{
				// Remove prefixes and try again
				var prefixPattern:RegExp = /(<[\/]*)([\w]*:)/gi;
				decodedRspec = decodedRspec.replace(prefixPattern, "$1");
				cm.Rspec = new XML(decodedRspec);
			}*/
			
			cm.processRspec(cleanup);
		}
		else
		{
			cm.errorMessage = response.output;
			cm.errorDescription = CommunicationUtil.GeniresponseToString(code) + ": " + cm.errorMessage;
			cm.Status = ComponentManager.FAILED;
			this.removeImmediately = true;
			Main.protogeniHandler.dispatchComponentManagerChanged(cm);
		}
		
		return null;
	}

    override public function fail(event : ErrorEvent, fault : MethodFault) : *
    {
		cm.errorMessage = event.toString();
		cm.errorDescription = "";
		if(cm.errorMessage.search("#2048") > -1)
			cm.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + cm.DiscoverResourcesUrl();
		else if(cm.errorMessage.search("#2032") > -1)
			cm.errorDescription = "IO error, possibly due to the server being down";
		else if(cm.errorMessage.search("timed"))
			cm.errorDescription = event.text;
		
		cm.Status = ComponentManager.FAILED;
		Main.protogeniHandler.dispatchComponentManagerChanged(cm);

      return null;
    }
	
	override public function cancel():void
	{
		cm.Status = ComponentManager.UNKOWN;
		Main.protogeniHandler.dispatchComponentManagerChanged(cm);
		op.cleanup();
	}
	
	override public function cleanup():void
	{
		if(cm.Status == ComponentManager.INPROGRESS)
			cm.Status = ComponentManager.FAILED;
		running = false;
		Main.protogeniHandler.rpcHandler.remove(this, false);
		Main.protogeniHandler.dispatchComponentManagerChanged(cm);
		op.cleanup();
		Main.protogeniHandler.rpcHandler.start();
	}

    private var cm : ComponentManager;
  }
}
