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
	import protogeni.resources.Key;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
	
	/**
	 * Allocates resources to a sliver using the GENI AM API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliverCreateAm extends Request
	{
		public var sliver:Sliver;
		
		public function RequestSliverCreateAm(s:Sliver, rspec:XML = null):void
		{
			super("SliverCreateAM",
				"Creating sliver on " + s.manager.Hrn + " for slice named " + s.slice.hrn,
				CommunicationUtil.createSliverAm,
				false,
				true);
			ignoreReturnCode = true;
			sliver = s;
			s.created = false;
			s.staged = false;
			op.timeout = 360;
			
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
			
			// Build up the args
			op.pushField(sliver.slice.urn.full);
			op.pushField([sliver.slice.credential]);
			if(rspec != null)
				op.addField("rspec", rspec.toXMLString());
			else
				op.addField("rspec", sliver.getRequestRspec(true).toXMLString());
			var userKeys:Array = [];
			for each(var key:Key in sliver.slice.creator.keys) {
				userKeys.push(key.value);
			}
			op.pushField([{urn:Main.geniHandler.CurrentUser.urn.full, keys:userKeys}]);
			op.setExactUrl(sliver.manager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			try
			{
				sliver.created = true;
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
				Main.geniDispatcher.dispatchSlicesChanged();
				
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
			
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
			
			return null;
		}
	}
}
