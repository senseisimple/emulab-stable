/* EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
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
  class RequestResourceDiscovery extends Request
  {
    public function RequestResourceDiscovery(newCmIndex : int,
                                             newCm : ComponentManager) : void
    {
      cmIndex = newCmIndex;
      cm = newCm;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      opName = "Requesting Resources";
      op.reset(Geni.discoverResources);
      op.addField("credential", credential.slice);
      op.setUrl(cm.getUrl(cmIndex));
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      if (code == 0)
      {
        cm.populateNodes(cmIndex, response.value);
      }
      else
      {
        cm.failResources(cmIndex);
      }
      return null;
    }

    var cmIndex : int;
    var cm : ComponentManager;
  }
}
