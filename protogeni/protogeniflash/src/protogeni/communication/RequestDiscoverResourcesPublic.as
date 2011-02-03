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
  import protogeni.resources.GeniManager;

  public class RequestDiscoverResourcesPublic extends Request
  {
	  
    public function RequestDiscoverResourcesPublic(newCm:ComponentManager) : void
    {
		super("DiscoverResourcesPublic (" + Util.shortenString(newCm.Url, 15) + ")", "Publicly discovering resources for " + newCm.Url, null, true, true, false);
		cm = newCm;
		op.setExactUrl(newCm.Url);
		op.type = Operation.HTTP;
	}
	
	override public function complete(code : Number, response : Object) : *
	{
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			cm.Rspec = new XML(response);
			cm.processRspec(cleanup);
		}
		else
		{
			cm.Status = GeniManager.FAILED;
			this.removeImmediately = true;
			Main.geniHandler.dispatchGeniManagerChanged(cm);
		}
		
		return null;
	}
	
	override public function cancel():void
	{
		cm.Status = GeniManager.UNKOWN;
		Main.geniHandler.dispatchGeniManagerChanged(cm);
		op.cleanup();
	}
	
	override public function cleanup():void
	{
		running = false;
		if(cm.Status == GeniManager.INPROGRESS)
			cm.Status = GeniManager.FAILED;
		Main.geniHandler.rpcHandler.remove(this, false);
		Main.geniHandler.dispatchGeniManagerChanged(cm);
		op.cleanup();
		if(cm.Status == GeniManager.VALID)
			Main.log.setStatus("Parsing " + cm.Hrn + " RSPEC Done",false);
		Main.geniHandler.rpcHandler.start();
	}

    private var cm : ComponentManager;
  }
}
