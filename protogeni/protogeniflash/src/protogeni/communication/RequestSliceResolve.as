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
	import mx.controls.Alert;
	
	import protogeni.Util;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;

  public class RequestSliceResolve extends Request
  {
    public function RequestSliceResolve(s:Slice, willBeCreating:Boolean = false) : void
    {
		var name:String;
		if(s.hrn == null)
			name = Util.shortenString(s.urn, 12);
		else
			name = s.hrn;
		super("SliceResolve (" + name + ")", "Resolving slice named " + s.hrn, CommunicationUtil.resolve);
		slice = s;
		isCreating = willBeCreating;
		op.addField("credential", Main.geniHandler.CurrentUser.credential);
		op.addField("urn", slice.urn);
		op.addField("type", "Slice");
		op.setExactUrl(Main.geniHandler.CurrentUser.authority.Url);
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		var newRequest:Request = null;
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			if(isCreating)
			{
				Alert.show("Slice '" + slice.urn + "' already exists");
				Main.log.setStatus("Slice exists", true);
			}
			else
			{
				slice.uuid = response.value.uuid;
				slice.creator = Main.geniHandler.CurrentUser;
				slice.hrn = response.value.hrn;
				slice.urn = response.value.urn;
				for each(var sliverCm:String in response.value.component_managers) {
					var newSliver:Sliver = new Sliver(slice);
					newSliver.manager = Main.geniHandler.GeniManagers.getByUrn(sliverCm);
					slice.slivers.addItem(newSliver);
				}
				
				Main.geniHandler.dispatchSliceChanged(slice);
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
				Main.geniHandler.rpcHandler.codeFailure(name, "Recieved GENI response other than success");
				Main.geniHandler.mapHandler.drawAll();
			}
			
		}
		
		return newRequest;
	}

    private var slice : Slice;
	private var isCreating:Boolean;
  }
}
