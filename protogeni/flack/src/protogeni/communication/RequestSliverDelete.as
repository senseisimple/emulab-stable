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
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
	
	/**
	 * Releases all resources in a sliver using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliverDelete extends Request
	{
		public var sliver:Sliver;
		
		public function RequestSliverDelete(s:Sliver):void
		{
			super("SliverDelete",
				"Deleting sliver on " + s.manager.Hrn + " for slice named " + s.slice.hrn,
				CommunicationUtil.deleteSlice);
			sliver = s;
			
			// Build up the args
			op.addField("slice_urn", sliver.slice.urn.full);
			op.addField("credentials", [sliver.slice.credential]);
			op.setUrl(sliver.manager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS
				|| code == CommunicationUtil.GENIRESPONSE_SEARCHFAILED)
			{
				sliver.removeOutsideReferences();
				if(sliver.slice.slivers.contains(sliver))
					sliver.slice.slivers.remove(sliver);
				var old:Slice = Main.geniHandler.CurrentUser.slices.getByUrn(sliver.slice.urn.full);
				if(old != null)
				{
					var oldSliver:Sliver = old.slivers.getByUrn(sliver.urn.full);
					if(oldSliver != null) {
						oldSliver.removeOutsideReferences();
						old.slivers.remove(old.slivers.getByUrn(sliver.urn.full));
					}
					Main.geniDispatcher.dispatchSliceChanged(old);
				}
			}
			else
			{
				Alert.show("Failed to delete sliver on " + this.sliver.manager.Hrn + ", check logs and try again.");
			}
			
			return null;
		}
		
		override public function fail(event:ErrorEvent, fault:MethodFault):* {
			Alert.show("Failed to delete sliver on " + this.sliver.manager.Hrn + ", check logs and try again.");
			return super.fail(event, fault);
		}
		
		override public function cleanup():void {
			super.cleanup();
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice, GeniEvent.ACTION_REMOVING);
		}
	}
}
