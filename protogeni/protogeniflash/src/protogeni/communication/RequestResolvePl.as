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
  
  import mx.collections.ArrayList;
  
  import protogeni.Util;
  import protogeni.resources.GeniManager;
  import protogeni.resources.PhysicalNode;
  import protogeni.resources.PlanetlabAggregateManager;

  public class RequestResolvePl extends Request
  {
	  
    public function RequestResolvePl(newPlm:PlanetlabAggregateManager) : void
    {
		super("Resolve (" + Util.shortenString(newPlm.registryUrl, 15) + ")",
			"Resolving resources for " + newPlm.registryUrl,
			CommunicationUtil.resolvePl,
			true, true, false);
		ignoreReturnCode = true;
		op.timeout = 360;
		plm = newPlm;
		var a:ArrayList = new ArrayList();
		for each(var n:PhysicalNode in plm.AllNodes)
			a.addItem(n.urn);
		op.pushField(a.source);	// xrns
		op.pushField([Main.geniHandler.CurrentUser.credential]);	// credentials
		op.setExactUrl(plm.registryUrl);
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		try
		{
			Main.Application().geniLocalSharedObject.data.planetlab = response;
			try
			{
				Main.Application().geniLocalSharedObject.flush();
				Main.log.appendMessage(new LogMessage("hey","hey"));
			}
			catch (e:Error)
			{
				trace("Problem saving shared object");
			}
		} catch(e:Error)
		{
			//plm.errorMessage = response;
			//plm.errorDescription = CommunicationUtil.GeniresponseToString(code) + ": " + plm.errorMessage;
			this.removeImmediately = true;
			Main.geniDispatcher.dispatchGeniManagerChanged(plm);
		}
		
		return null;
	}

    override public function fail(event : ErrorEvent, fault : MethodFault) : *
    {

		plm.errorMessage = event.toString();
		plm.errorDescription = event.text;
		if(plm.errorMessage.search("#2048") > -1)
			plm.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + plm.VisitUrl();
		else if(plm.errorMessage.search("#2032") > -1)
			plm.errorDescription = "IO error, possibly due to the server being down";
		
		Main.geniDispatcher.dispatchGeniManagerChanged(plm);

      return null;
    }
	
	override public function cancel():void
	{
		Main.geniDispatcher.dispatchGeniManagerChanged(plm);
		op.cleanup();
	}
	
	override public function cleanup():void
	{
		running = false;
		Main.geniHandler.requestHandler.remove(this, false);
		Main.geniDispatcher.dispatchGeniManagerChanged(plm);
		op.cleanup();
		Main.geniHandler.requestHandler.start();
	}

    private var plm:PlanetlabAggregateManager;
  }
}
