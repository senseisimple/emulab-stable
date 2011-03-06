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
	import protogeni.resources.AggregateManager;
	import protogeni.resources.ProtogeniComponentManager;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;

  public final class RequestSliceCredential extends Request
  {
    public function RequestSliceCredential(s:Slice) : void
    {
		super("SliceCredential", "Getting the slice credential for " + s.hrn, CommunicationUtil.getCredential, true);
		slice = s;
		op.addField("credential", Main.geniHandler.CurrentUser.credential);
		op.addField("urn", slice.urn.full);
		op.addField("type", "Slice");
		op.setUrl(Main.geniHandler.CurrentUser.authority.Url);
    }

	override public function complete(code : Number, response : Object) : *
	{
		var newCalls:RequestQueue = new RequestQueue();
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			slice.credential = String(response.value);
			Main.geniDispatcher.dispatchSliceChanged(slice);
			for each(var s:Sliver in slice.slivers.collection) {
				if(s.manager is AggregateManager)
					newCalls.push(new RequestSliverListResourcesAm(s));
				else if(s.manager is ProtogeniComponentManager)
					newCalls.push(new RequestSliverGet(s));
			}
		}
		else
		{
			// skip errors
		}
		
		return newCalls.head;
	}

    private var slice : Slice;
  }
}
