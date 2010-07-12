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
	import protogeni.resources.ComponentManager;

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
		op.addField("credential", Main.protogeniHandler.CurrentUser.credential);
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
				var newCm:ComponentManager = new ComponentManager();
				newCm.Hrn = obj.hrn;
				newCm.Url = obj.url;
				newCm.Urn = obj.urn;
				Main.protogeniHandler.ComponentManagers.add(newCm);
				Main.protogeniHandler.dispatchComponentManagerChanged();
				if(startDiscoverResources)
				{
					newCm.Status = ComponentManager.INPROGRESS;
					newCalls.push(new RequestDiscoverResources(newCm));
				}
			}
			if(startSlices)
				newCalls.push(new RequestUserResolve());
		}
		else
		{
			Main.protogeniHandler.rpcHandler.codeFailure(name, "Recieved GENI response other than success");
		}
		
		return newCalls.head;
	}
	
	private var startDiscoverResources:Boolean;
	private var startSlices:Boolean;
  }
}
