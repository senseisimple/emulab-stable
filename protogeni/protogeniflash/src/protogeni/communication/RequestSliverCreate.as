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
  
  import protogeni.resources.Slice;
  import protogeni.resources.Sliver;

  public class RequestSliverCreate extends Request
  {
    public function RequestSliverCreate(s:Sliver) : void
    {
		super("SliverCreate", "Creating sliver on " + s.manager.Hrn + " for slice named " + s.slice.hrn, CommunicationUtil.createSliver);
		sliver = s;
		s.created = false;
		op.addField("slice_urn", sliver.slice.urn);
		op.addField("rspec", sliver.getRequestRspec().toXMLString());
		op.addField("keys", sliver.slice.creator.keys);
		op.addField("credentials", new Array(sliver.slice.credential));
		op.setUrl(sliver.manager.Url);
		op.timeout = 360;
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			sliver.credential = response.value[0];
			sliver.created = true;

			sliver.rspec = new XML(response.value[1]);
			sliver.parseRspec();
			
			var old:Slice = Main.geniHandler.CurrentUser.slices.getByUrn(sliver.slice.urn);
			if(old != null)
			{
				if(old.slivers.getByUrn(sliver.urn) != null)
					old.slivers.removeItemAt(old.slivers.getItemIndex(old.slivers.getByUrn(sliver.urn)));
				old.slivers.addItem(sliver);
			}
			
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);

			return new RequestSliverStatus(sliver);
		} else if(code == CommunicationUtil.GENIRESPONSE_BUSY) {
			if(this.numTries == 8) {
				LogHandler.appendMessage(new LogMessage(this.op.getUrl(), this.name, "Reach limit of retries", true, LogMessage.TYPE_END ));
				failed();
			} else {
				LogHandler.appendMessage(new LogMessage(this.op.getUrl(), this.name, "Preparing to retry", true, LogMessage.TYPE_END ));
				op.delaySeconds = 10;
				this.forceNext = true;
				return this;
			}
			
		}
		else
		{
			failed();
		}
		
		return null;
	}
	
	private function failed():void {
		// Remove any pending SliverCreate calls
		sliver.status = Sliver.STATUS_FAILED;
		sliver.state = Sliver.STATE_FAILED;
		
		// Cancel remaining calls
		var tryDeleteNode:RequestQueueNode = this.node.next;
		while(tryDeleteNode != null && tryDeleteNode.item is RequestSliverCreate && (tryDeleteNode.item as RequestSliverCreate).sliver.slice == sliver.slice)
		{
			(tryDeleteNode.item as RequestSliverCreate).sliver.status = Sliver.STATUS_FAILED;
			(tryDeleteNode.item as RequestSliverCreate).sliver.state = Sliver.STATE_FAILED;
			Main.geniHandler.requestHandler.remove(tryDeleteNode.item, false);
			tryDeleteNode = tryDeleteNode.next;
		}
		
		Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
		
		// Show the error
		LogHandler.viewConsole();
	}
	
	override public function fail(event:ErrorEvent, fault:MethodFault) : *
	{
		failed();
		
		return null;
	}

    public var sliver:Sliver;
  }
}
