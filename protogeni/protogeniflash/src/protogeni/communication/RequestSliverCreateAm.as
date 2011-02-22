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
  import flash.events.ErrorEvent;
  
  import protogeni.resources.Slice;
  import protogeni.resources.Sliver;

  public class RequestSliverCreateAm extends Request
  {
    public function RequestSliverCreateAm(s:Sliver) : void
    {
		super("SliverCreate", "Creating sliver on " + s.manager.Hrn + " for slice named " + s.slice.hrn, CommunicationUtil.createSliverAm);
		ignoreReturnCode = true;
		sliver = s;
		s.created = false;
		op.pushField(sliver.slice.urn);
		op.pushField([sliver.slice.credential]);
		op.pushField(sliver.getRequestRspec().toXMLString());
		op.pushField([{urn:Main.geniHandler.CurrentUser.urn, keys:sliver.slice.creator.keys}]);
		op.setExactUrl(sliver.manager.Url);
		op.timeout = 360;
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		try
		{
			sliver.created = true;
			sliver.rspec = new XML(response);
			sliver.parseRspec();
			
			var old:Slice = Main.geniHandler.CurrentUser.slices.getByUrn(sliver.slice.urn);
			if(old != null)
			{
				if(old.slivers.getByUrn(sliver.urn) != null)
					old.slivers.removeItemAt(old.slivers.getItemIndex(old.slivers.getByUrn(sliver.urn)));
				old.slivers.addItem(sliver);
			}
			
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);

			return new RequestSliverStatusAm(sliver);
		}
		catch(e:Error)
		{
			// Cancel remaining calls
			var tryDeleteNode:RequestQueueNode = this.node.next;
			while(tryDeleteNode != null && tryDeleteNode is RequestSliverCreate && (tryDeleteNode as RequestSliverCreate).sliver.slice == sliver.slice)
			{
				Main.geniHandler.requestHandler.remove(tryDeleteNode.item);
				tryDeleteNode = tryDeleteNode.next;
			}
			
			// Show the error
			Main.log.viewConsole();
		}
		
		return null;
	}

    public var sliver:Sliver;
  }
}
