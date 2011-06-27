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
	import mx.controls.Alert;
	
	import protogeni.StringUtil;
	import protogeni.resources.IdnUrn;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
	
	/**
	 * Gets information for the slice like what managers have slivers.  Uses the ProtoGENI API
	 * @author mstrum
	 * 
	 */
	public final class RequestSliceResolve extends Request
	{
		public var slice:Slice;
		private var isCreating:Boolean;
		
		public function RequestSliceResolve(s:Slice, willBeCreating:Boolean = false):void
		{
			var name:String;
			if(s.hrn == null)
				name = StringUtil.shortenString(s.urn.full, 12);
			else
				name = s.hrn;
			super("SliceResolve (" + name + ")",
				"Resolving slice named " + s.hrn,
				CommunicationUtil.resolve);
			slice = s;
			isCreating = willBeCreating;
			
			// Build up the args
			op.addField("credential", Main.geniHandler.CurrentUser.Credential);
			op.addField("urn", slice.urn.full);
			op.addField("type", "Slice");
			op.setExactUrl(Main.geniHandler.CurrentUser.authority.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			var newRequest:Request = null;
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				if(isCreating)
				{
					Alert.show("Slice '" + slice.urn.full + "' already exists", "Error");
					Main.Application().setStatus("Slice exists", true);
				}
				else
				{
					slice.creator = Main.geniHandler.CurrentUser;
					slice.hrn = response.value.hrn;
					slice.urn = new IdnUrn(response.value.urn);
					for each(var sliverCm:String in response.value.component_managers) {
						var newSliver:Sliver = new Sliver(slice);
						newSliver.manager = Main.geniHandler.GeniManagers.getByUrn(sliverCm);
						slice.slivers.add(newSliver);
					}
					
					newRequest = new RequestSliceCredential(slice);
				}
			}
			else
			{
				if(isCreating)
				{
					newRequest = new RequestSliceRemove(slice);
				}
				else
				{
					Main.geniHandler.requestHandler.codeFailure(name, "Recieved GENI response other than success");
					//Main.geniHandler.mapHandler.drawAll();
				}
				
			}
			
			return newRequest;
		}
		
		override public function cleanup():void {
			super.cleanup();
			Main.geniDispatcher.dispatchSliceChanged(slice);
		}
	}
}
