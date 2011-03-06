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
	import protogeni.GeniEvent;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.PlanetlabAggregateManager;
	import protogeni.resources.Site;
	
	public final class RequestSitesLocation extends Request
	{
		private var planetLabManager:PlanetlabAggregateManager;
		
		public function RequestSitesLocation(newPlm:PlanetlabAggregateManager):void
		{
			super("SitesLocation",
				"Getting Planetlab location info",
				null,
				true,
				true,
				true);
			planetLabManager = newPlm;
			op.setExactUrl("https://www.emulab.net/protogeni/planetlab-sites.xml");
			op.type = Operation.HTTP;
			planetLabManager.lastRequest = this;
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				var startTime:Date = new Date();
				var sitesXml:XML = new XML(response);
				for each(var siteXml:XML in sitesXml.children()) {
					var site:Site = planetLabManager.getSiteWithHrn(siteXml.@hrn);
					if(site != null) {
						site.latitude = siteXml.@latitude;
						site.longitude = siteXml.@longitude;
						
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
				
				if(Main.debugMode)
					LogHandler.appendMessage(new LogMessage(planetLabManager.Url, "Sites " + String((new Date()).time - startTime.time)));
				
				Main.geniDispatcher.dispatchGeniManagerChanged(planetLabManager, GeniEvent.ACTION_POPULATED);
			}
			
			return null;
		}
	}
}
