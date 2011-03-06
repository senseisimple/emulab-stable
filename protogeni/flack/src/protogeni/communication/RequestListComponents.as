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
	import protogeni.Util;
	import protogeni.resources.AggregateManager;
	import protogeni.resources.GeniManager;
	import protogeni.resources.IdnUrn;
	import protogeni.resources.PlanetlabAggregateManager;
	import protogeni.resources.ProtogeniComponentManager;

  public final class RequestListComponents extends Request
  {
    public function RequestListComponents(shouldDiscoverResources:Boolean = true, shouldStartSlices:Boolean = false) : void
    {
      super("ListComponents", "Getting the information for the component managers", CommunicationUtil.listComponents);
	  startDiscoverResources = shouldDiscoverResources;
	  startSlices = shouldStartSlices;
    }
	
	override public function start():Operation
	{
		op.addField("credential", Main.geniHandler.CurrentUser.credential);
		return op;
	}

	// Should return Request or RequestQueueNode
	override public function complete(code : Number, response : Object) : *
	{
		var newCalls:RequestQueue = new RequestQueue();
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			Main.geniHandler.clearComponents();

			for each(var obj:Object in response.value)
			{
				var newGm:GeniManager = null;
				var ts:String = obj.url.substr(0, obj.url.length-3);
				//if(ts != "https://www.emulab.net/protogeni/xmlrpc")
				//	continue;
				var newCm:ProtogeniComponentManager = new ProtogeniComponentManager();
				newCm.Hrn = obj.hrn;
				newCm.Url = ts;
				newCm.Urn = new IdnUrn(obj.urn);
				// Quick hack, giving exceptions in forge
				if(newCm.Hrn == "wigims.cm" || newCm.Hrn == "cron.cct.lsu.edu.cm" || newCm.Urn.full.toLowerCase().indexOf("etri") > 0)
					continue;
				if(newCm.Hrn == "ukgeni.cm" || newCm.Hrn == "utahemulab.cm")
					newCm.supportsIon = true;
				if(newCm.Hrn == "wail.cm" || newCm.Hrn == "utahemulab.cm")
					newCm.supportsGpeni = true;
				Main.geniHandler.GeniManagers.add(newCm);
				newGm = newCm;
				if(startDiscoverResources)
				{
					newGm.Status = GeniManager.STATUS_INPROGRESS;
					if(newGm is AggregateManager)
						newCalls.push(new RequestGetVersionAm(newGm as AggregateManager));
					else if(newGm is ProtogeniComponentManager)
						newCalls.push(new RequestGetVersion(newGm as ProtogeniComponentManager));
				}
				Main.geniDispatcher.dispatchGeniManagerChanged(newGm);
			}

			if(!Main.protogeniOnly) {
				var planetLabAm:PlanetlabAggregateManager = new PlanetlabAggregateManager();
				Main.geniHandler.GeniManagers.add(planetLabAm);
				planetLabAm.Status = GeniManager.STATUS_INPROGRESS;
				newCalls.push(new RequestGetVersionAm(planetLabAm as AggregateManager));
				Main.geniDispatcher.dispatchGeniManagerChanged(planetLabAm);
			}
			
			if(startSlices)
				newCalls.push(new RequestUserResolve());
		}
		else
		{
			Main.geniHandler.requestHandler.codeFailure(name, "Recieved GENI response other than success");
		}
		
		return newCalls.head;
	}
	
	private var startDiscoverResources:Boolean;
	private var startSlices:Boolean;
  }
}
