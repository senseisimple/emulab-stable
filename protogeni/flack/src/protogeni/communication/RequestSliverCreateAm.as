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
	import com.mattism.http.xmlrpc.MethodFault;
	
	import flash.events.ErrorEvent;
	
	import mx.controls.Alert;
	
	import protogeni.GeniEvent;
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
		public var rspec:String = "";
		
		public function RequestSliverCreateAm(s:Sliver, useRspec:XML = null):void
		{
			super("SliverCreateAM",
				"Creating sliver on " + s.manager.Hrn + " for slice named " + s.slice.hrn,
				CommunicationUtil.createSliverAm,
				true,
				true);
			ignoreReturnCode = true;
			sliver = s;
			s.created = false;
			s.staged = false;
			s.status = "";
			s.state = "";
			op.timeout = 360;
			
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
			
			// Build up the args
			op.pushField(sliver.slice.urn.full);
			op.pushField([sliver.slice.credential]);
			if(useRspec != null)
				rspec = useRspec.toXMLString();
			else
				rspec = sliver.slice.slivers.getCombined().getRequestRspec(true).toXMLString();
			op.pushField(rspec);
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
			}
			catch(e:Error)
			{
				Alert.show("Failed to create sliver on " + sliver.manager.Hrn);
				return;
			}
			
			try
			{
				sliver.parseRspec();
				
				var old:Slice = Main.geniHandler.CurrentUser.slices.getByUrn(sliver.slice.urn.full);
				if(old != null)
				{
					if(old.slivers.getByUrn(sliver.urn.full) != null)
						old.slivers.remove(old.slivers.getByUrn(sliver.urn.full));
					old.slivers.add(sliver);
				}
				
				Main.geniDispatcher.dispatchSlicesChanged();
				
				return new RequestSliverStatusAm(sliver);
			}
			catch(e:Error)
			{
				LogHandler.appendMessage(new LogMessage(sliver.manager.Url,
					"SliverCreateAM",
					e.toString(),
					true,
					LogMessage.TYPE_END));
				/*
				// Cancel remaining calls
				var tryDeleteNode:RequestQueueNode = this.node.next;
				while(tryDeleteNode != null && tryDeleteNode is RequestSliverCreate && (tryDeleteNode as RequestSliverCreate).sliver.slice == sliver.slice)
				{
					Main.geniHandler.requestHandler.remove(tryDeleteNode.item);
					tryDeleteNode = tryDeleteNode.next;
				}
				
				// Show the error
				LogHandler.viewConsole();
				*/
			}
			
			return null;
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):* {
			Alert.show("Failed to create sliver on " + sliver.manager.Hrn);
			return super.fail(event, fault);
		}
		
		override public function cleanup():void {
			super.cleanup();
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice, GeniEvent.ACTION_POPULATING);
		}
		
		override public function getSent():String {
			return op.getSent() + "\n\n********RSPEC********\n\n" + rspec;
		}
	}
}
