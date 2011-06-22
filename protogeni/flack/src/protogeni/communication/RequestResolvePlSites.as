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
	
	import mx.collections.ArrayList;
	
	import protogeni.GeniEvent;
	import protogeni.StringUtil;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.PlanetlabAggregateManager;
	import protogeni.resources.Site;
	
	/**
	 * Gets additional information for PlanetLab sites like location
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestResolvePlSites extends Request
	{
		private var planetLabManager:PlanetlabAggregateManager;
		
		public function RequestResolvePlSites(newPlanetLabManager:PlanetlabAggregateManager):void
		{
			super("Resolve (" + StringUtil.shortenString(newPlanetLabManager.registryUrl, 15) + ")",
				"Resolving resources for " + newPlanetLabManager.registryUrl,
				CommunicationUtil.resolvePl,
				true, true, false);
			this.ignoreReturnCode = true;
			op.timeout = 600;
			planetLabManager = newPlanetLabManager;
			
			// Build up the args
			var a:ArrayList = new ArrayList();
			for each(var s:Site in planetLabManager.sites)
				a.addItem("plc." + s.id);
			op.pushField(a.source);
			op.pushField([Main.geniHandler.CurrentUser.Credential]);	// credentials
			op.setExactUrl(planetLabManager.registryUrl);
			planetLabManager.lastRequest = this;
		}
		
		override public function complete(code:Number, response:Object):*
		{
			try
			{
				for each(var o:Object in response) {
					var site:Site = planetLabManager.getSiteWithHrn(o.hrn);
					if(site != null) {
						site.latitude = o.latitude;
						site.longitude = o.longitude;
						
						var ng:PhysicalNodeGroup = planetLabManager.Nodes.GetByLocation(site.latitude,site.longitude);
						var tempString:String;
						if(ng == null) {
							ng = new PhysicalNodeGroup(site.latitude, site.longitude, "", planetLabManager.Nodes);
							planetLabManager.Nodes.Add(ng);
						}
						for each(var n:PhysicalNode in site.nodes) {
							ng.Add(n);
						}
					}
				}
				
				Main.geniDispatcher.dispatchGeniManagerChanged(planetLabManager, GeniEvent.ACTION_POPULATED);
				
			} catch(e:Error)
			{
				//plm.errorMessage = response;
				//plm.errorDescription = CommunicationUtil.GeniresponseToString(code) + ": " + plm.errorMessage;
				this.removeImmediately = true;
				Main.geniDispatcher.dispatchGeniManagerChanged(planetLabManager);
			}
			
			return null;
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):*
		{
			planetLabManager.errorMessage = event.toString();
			planetLabManager.errorDescription = event.text;
			if(planetLabManager.errorMessage.search("#2048") > -1)
				planetLabManager.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + planetLabManager.VisitUrl();
			else if(planetLabManager.errorMessage.search("#2032") > -1)
				planetLabManager.errorDescription = "IO error, possibly due to the server being down";
			
			Main.geniDispatcher.dispatchGeniManagerChanged(planetLabManager);
			
			return null;
		}
		
		override public function cancel():void
		{
			Main.geniDispatcher.dispatchGeniManagerChanged(planetLabManager);
			op.cleanup();
		}
		
		override public function cleanup():void
		{
			running = false;
			Main.geniHandler.requestHandler.remove(this, false);
			Main.geniDispatcher.dispatchGeniManagerChanged(planetLabManager);
			op.cleanup();
			Main.geniHandler.requestHandler.tryNext();
		}
	}
}
