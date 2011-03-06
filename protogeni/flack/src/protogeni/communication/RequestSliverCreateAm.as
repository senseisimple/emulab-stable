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
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
	
	public final class RequestSliverCreateAm extends Request
	{
		public var sliver:Sliver;
		
		public function RequestSliverCreateAm(s:Sliver):void
		{
			super("SliverCreate",
				"Creating sliver on " + s.manager.Hrn + " for slice named " + s.slice.hrn,
				CommunicationUtil.createSliverAm);
			ignoreReturnCode = true;
			sliver = s;
			s.created = false;
			op.pushField(sliver.slice.urn.full);
			op.pushField([sliver.slice.credential]);
			op.pushField(sliver.getRequestRspec().toXMLString());
			// Internal API error: <Fault 102: "person_id 1: AddPersonKey: Invalid key_fields['key'] value: expected string, got struct">
			var userKeys:Array = [];
			for each(var keyObject:Object in sliver.slice.creator.keys) {
				userKeys.push(keyObject.key);
			}
			op.pushField([{urn:Main.geniHandler.CurrentUser.urn.full, keys:userKeys}]);
			op.setExactUrl(sliver.manager.Url);
			op.timeout = 360;
		}
		
		override public function complete(code:Number, response:Object):*
		{
			try
			{
				sliver.created = true;
				sliver.staged = false;
				sliver.rspec = new XML(response);
				sliver.parseRspec();
				
				var old:Slice = Main.geniHandler.CurrentUser.slices.getByUrn(sliver.slice.urn.full);
				if(old != null)
				{
					if(old.slivers.getByUrn(sliver.urn.full) != null)
						old.slivers.remove(old.slivers.getByUrn(sliver.urn.full));
					old.slivers.add(sliver);
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
				LogHandler.viewConsole();
			}
			
			return null;
		}
	}
}
