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
			cm.Status = ComponentManager.FAILED;
			this.removeImmediately = true;
			Main.protogeniHandler.dispatchComponentManagerChanged(cm);
		}
		
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
		running = false;
		if(cm.Status == ComponentManager.INPROGRESS)
			cm.Status = ComponentManager.FAILED;
		Main.protogeniHandler.rpcHandler.remove(this, false);
		Main.protogeniHandler.dispatchComponentManagerChanged(cm);
		op.cleanup();
		if(cm.Status == ComponentManager.VALID)
			Main.log.setStatus("Parsing " + cm.Hrn + " RSPEC Done",false);
		Main.protogeniHandler.rpcHandler.start();
	}

    private var cm : ComponentManager;
  }
}
