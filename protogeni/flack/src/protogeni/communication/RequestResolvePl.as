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
  
  import protogeni.GeniEvent;
  import protogeni.Util;
  import protogeni.display.DefaultWindow;
  import protogeni.display.XmlWindow;
  import protogeni.resources.GeniManager;
  import protogeni.resources.PhysicalNode;
  import protogeni.resources.PhysicalNodeGroup;
  import protogeni.resources.PlanetlabAggregateManager;
  import protogeni.resources.Site;
  
  import spark.components.TextArea;

  public class RequestResolvePl extends Request
  {
	  
    public function RequestResolvePl(newPlm:PlanetlabAggregateManager) : void
    {
		super("Resolve (" + Util.shortenString(newPlm.registryUrl, 15) + ")",
			"Resolving resources for " + newPlm.registryUrl,
			CommunicationUtil.resolvePl,
			true, true, false);
		this.ignoreReturnCode = true;
		op.timeout = 360;
		plm = newPlm;
		var a:ArrayList = new ArrayList();
		for each(var s:Site in plm.sites)
			a.addItem("plc." + s.id);
		op.pushField(a.source);
		op.pushField([Main.geniHandler.CurrentUser.credential]);	// credentials
		op.setExactUrl(plm.registryUrl);
		plm.lastRequest = this;
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		try
		{
			for each(var o:Object in response) {
				var site:Site = plm.getSiteWithHrn(o.hrn);
				if(site != null) {
					site.latitude = o.latitude;
					site.longitude = o.longitude;
					
					var ng:PhysicalNodeGroup = plm.Nodes.GetByLocation(site.latitude,site.longitude);
					var tempString:String;
					if(ng == null) {
						ng = new PhysicalNodeGroup(site.latitude, site.longitude, "", plm.Nodes);
						plm.Nodes.Add(ng);
					}
					for each(var n:PhysicalNode in site.nodes) {
						ng.Add(n);
					}
				}
			}
			
			Main.geniDispatcher.dispatchGeniManagerChanged(plm, GeniEvent.ACTION_POPULATED);
			
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
		Main.geniHandler.requestHandler.tryNext();
	}

    private var plm:PlanetlabAggregateManager;
  }
}
