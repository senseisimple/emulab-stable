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
  import protogeni.resources.PhysicalNode;
  import protogeni.resources.PhysicalNodeGroup;
  import protogeni.resources.PlanetlabAggregateManager;
  import protogeni.resources.Site;

  public class RequestSitesLocation extends Request
  {
	  
    public function RequestSitesLocation(newPlm:PlanetlabAggregateManager) : void
    {
		super("SitesLocation", "Getting Planetlab location info", null, true, true, true);
		plm = newPlm;
		op.setExactUrl("https://www.emulab.net/protogeni/planetlab-sites.xml");
		op.type = Operation.HTTP;
		plm.lastRequest = this;
	}
	
	override public function complete(code : Number, response : Object) : *
	{
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			var sitesXml:XML = new XML(response);
			for each(var siteXml:XML in sitesXml.children()) {
				var site:Site = plm.getSiteWithHrn(siteXml.@hrn);
				if(site != null) {
					site.latitude = siteXml.@latitude;
					site.longitude = siteXml.@longitude;
					
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

			Main.geniHandler.mapHandler.drawMap();
			Main.geniDispatcher.dispatchGeniManagerChanged(plm);
		}
		
		return null;
	}

    private var plm:PlanetlabAggregateManager;
  }
}
