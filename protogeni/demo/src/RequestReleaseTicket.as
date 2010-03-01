/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2009 University of Utah and the Flux Group.
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

package
{
  class RequestReleaseTicket extends Request
  {
    public function RequestReleaseTicket(newManager : ComponentManager,
										 newSliceUrn) : void
    {
      super(newManager.getName());
      manager = newManager;
	  sliceUrn = newSliceUrn;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      opName = "Release Ticket";
      op.reset(Geni.releaseTicket);
      op.addField("slice_urn", sliceUrn);
      op.addField("credentials", new Array(credential.slice));
      op.addField("ticket", manager.getTicket());
      op.setUrl(manager.getUrl());
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      if (code == 0)
      {
        manager.setTicket(null);
      }
      return null;
    }

    var manager : ComponentManager;
	var sliceUrn : String;
  }
}
