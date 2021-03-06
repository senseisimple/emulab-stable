﻿/* GENIPUBLIC-COPYRIGHT
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
	import protogeni.resources.IdnUrn;
	import protogeni.resources.Key;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
	
	/**
	 * Allocates resources to a sliver using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliverCreate extends Request
	{
		public var sliver:Sliver;
		
		public function RequestSliverCreate(s:Sliver, rspec:XML = null):void
		{
			super("SliverCreate",
				"Creating sliver on " + s.manager.Hrn + " for slice named " + s.slice.hrn,
				CommunicationUtil.createSliver,
				true);
			sliver = s;
			s.created = false;
			s.staged = false;
			s.status = "";
			s.state = "";
			
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
			
			// Build up the args
			op.addField("slice_urn", sliver.slice.urn.full);
			if(rspec != null)
				op.addField("rspec", rspec.toXMLString());
			else
				op.addField("rspec", sliver.slice.slivers.getCombined().getRequestRspec(true).toXMLString());
			var keys:Array = [];
			for each(var key:Key in sliver.slice.creator.keys) {
				keys.push({type:key.type, key:key.value});
			}
			op.addField("keys", keys);
			op.addField("credentials", [sliver.slice.credential]);
			op.setUrl(sliver.manager.Url);
			op.timeout = 360;
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				sliver.credential = response.value[0];
				var cred:XML = new XML(sliver.credential);
				sliver.urn = new IdnUrn(cred.credential.target_urn);
				sliver.created = true;
				
				sliver.manifest = new XML(response.value[1]);
				sliver.rspec = sliver.manifest;
				sliver.parseRspec();

				var old:Slice = Main.geniHandler.CurrentUser.slices.getByUrn(sliver.slice.urn.full);
				if(old != null)
				{
					var oldSliver:Sliver = old.slivers.getByGm(sliver.manager);
					if(oldSliver != null)
						old.slivers.remove(oldSliver);
					old.slivers.add(sliver);
				}
				
				return new RequestSliverStatus(sliver);
			}
			else
			{
				failed();
			}
			
			return null;
		}
		
		private function failed():void {
			sliver.status = Sliver.STATUS_FAILED;
			sliver.state = Sliver.STATE_NA;
			Alert.show("Failed to create sliver on " + sliver.manager.Hrn);
			
			/*
			// Cancel remaining calls
			var tryDeleteNode:RequestQueueNode = this.node.next;
			while(tryDeleteNode != null && tryDeleteNode.item is RequestSliverCreate && (tryDeleteNode.item as RequestSliverCreate).sliver.slice == sliver.slice)
			{
				(tryDeleteNode.item as RequestSliverCreate).sliver.status = Sliver.STATUS_FAILED;
				(tryDeleteNode.item as RequestSliverCreate).sliver.state = Sliver.STATE_NA;
				Main.geniHandler.requestHandler.remove(tryDeleteNode.item, false);
				tryDeleteNode = tryDeleteNode.next;
			}
			
			// Show the error
			LogHandler.viewConsole();
			*/
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):*
		{
			failed();
			
			return null;
		}
		
		override public function cleanup():void {
			super.cleanup();
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice, GeniEvent.ACTION_POPULATING);
		}
	}
}
