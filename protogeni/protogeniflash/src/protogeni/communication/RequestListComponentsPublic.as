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
	import mx.utils.StringUtil;
	
	import protogeni.resources.AggregateManager;
	import protogeni.resources.GeniManager;
	import protogeni.resources.PlanetlabAggregateManager;
	import protogeni.resources.ProtogeniComponentManager;

  public class RequestListComponentsPublic extends Request
  {
    public function RequestListComponentsPublic() : void
    {
      super("ListComponentsPublic", "Getting the information for the component managers", null);
	  op.setExactUrl(CommunicationUtil.defaultPublicList);
	  op.type = Operation.HTTP;
    }

	// Should return Request or RequestQueueNode
	override public function complete(code : Number, response : Object) : *
	{
		var newCalls:RequestQueue = new RequestQueue();
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			var a:Array = (response as String).split(/^(.*)$/);
			for each(var s:String in a) {
				var newCm:ProtogeniComponentManager = new ProtogeniComponentManager();
				newCm.Url = op.getUrl().substring(0, op.getUrl().lastIndexOf('/')) + s;
				newCm.Urn = s;
				newCm.Hrn = s.split('+')[1];
				Main.geniHandler.GeniManagers.add(newCm);
				newCm.Status = GeniManager.STATUS_INPROGRESS;
				Main.geniDispatcher.dispatchGeniManagerChanged(newCm);
				newCalls.push(new RequestDiscoverResourcesPublic(newCm));
			}
		}
		else
		{
			//Main.geniHandler.requestHandler.codeFailure(name, "Recieved GENI response other than success");
		}
		
		return newCalls.head;
	}
  }
}
