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
	import protogeni.resources.AggregateManager;
	import protogeni.resources.ComponentManager;
	import protogeni.resources.GeniManager;

  public class RequestListComponents extends Request
  {
    public function RequestListComponents(shouldDiscoverResources:Boolean, shouldStartSlices:Boolean) : void
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
			for each(var obj:Object in response.value)
			{
				var newGm:GeniManager = null;
				var ts:String = obj.url.substr(0, obj.url.length-3);
				/*switch(ts)
				{
					case "https://www.emulab.net/protogeni/xmlrpc":
					//case "https://myboss.myelab.testbed.emulab.net/protogeni/xmlrpc":
					//case "https://pg-boss.cis.fiu.edu/protogeni/xmlrpc":
					//case "https://www.uky.emulab.net/protogeni/xmlrpc":
					//case "https://www.pgeni.gpolab.bbn.com/protogeni/xmlrpc":
						var newAm:AggregateManager = new AggregateManager();
						newAm.Url = "https://boss.emulab.net/protogeni/xmlrpc/am";
						newAm.Hrn = "utahemulab.cm";
						newAm.Urn = "urn:publicid:IDN+emulab.net+authority+cm";
						Main.geniHandler.GeniManagers.add(newAm);
						newGm = newAm;
						break;
					default:*/
						var newCm:ComponentManager = new ComponentManager();
						newCm.Hrn = obj.hrn;
						newCm.Url = ts;
						newCm.Urn = obj.urn;
						Main.geniHandler.GeniManagers.add(newCm);
						newGm = newCm;
				//}
				if(startDiscoverResources)
				{
					newGm.Status = GeniManager.INPROGRESS;
					if(newGm is AggregateManager)
						newCalls.push(new RequestGetVersionAm(newGm as AggregateManager));
					else if(newGm is ComponentManager)
						newCalls.push(new RequestGetVersion(newGm as ComponentManager));
				}
				Main.geniHandler.dispatchGeniManagerChanged(newGm);
			}
			
			var planetLabAm:AggregateManager = new AggregateManager();
			planetLabAm.Url = "https://planet-lab.org:12346";
			planetLabAm.Hrn = "planet-lab.am";
			planetLabAm.Urn = "urn:publicid:IDN+planet-lab.org+authority+am";
			Main.geniHandler.GeniManagers.add(planetLabAm);
			planetLabAm.Status = GeniManager.INPROGRESS;
			newCalls.push(new RequestGetVersionAm(planetLabAm as AggregateManager));
			Main.geniHandler.dispatchGeniManagerChanged(planetLabAm);
			
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
