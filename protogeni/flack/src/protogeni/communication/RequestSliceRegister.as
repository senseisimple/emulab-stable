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
	import protogeni.display.DisplayUtil;
	import protogeni.resources.Slice;

  public class RequestSliceRegister extends Request
  {
    public function RequestSliceRegister(s:Slice) : void
    {
		super("SliceRegister", "Register slice named " + s.hrn, CommunicationUtil.register);
		slice = s;
		op.addField("credential", Main.geniHandler.CurrentUser.credential);
		op.addField("hrn", slice.urn);
		op.addField("type", "Slice");
		op.setUrl(Main.geniHandler.CurrentUser.authority.Url);
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		var newRequest:Request = null;
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			slice.credential = String(response.value);
			Main.geniHandler.CurrentUser.slices.add(slice);
			Main.geniDispatcher.dispatchSlicesChanged();
			DisplayUtil.viewSlice(slice);
		}
		else
		{
			Main.geniHandler.requestHandler.codeFailure(name, "Recieved GENI response other than success");
		}
		
		return newRequest;
	}

    private var slice : Slice;
  }
}
