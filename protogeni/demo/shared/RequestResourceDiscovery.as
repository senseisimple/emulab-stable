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

package
{
  public class RequestResourceDiscovery extends Request
  {
    public function RequestResourceDiscovery(newCm : ComponentManager) : void
    {
      super(newCm.getName());
      cm = newCm;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      opName = "Discovering Resources";
      var opType = Geni.discoverResources;
      if (cm.getName() == "Wisconsin" || cm.getName() == "Kentucky"
        || cm.getName() == "CMU")
      {
        opType = Geni.discoverResourcesv2;
      }
      op.reset(opType);
      op.addField("credential", credential.slice);
      op.setUrl(cm.getUrl());
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      if (code == 0)
      {
        cm.resourceSuccess(response.value);
      }
      else
      {
        cm.resourceFailure();
      }
      return null;
    }

    private var cm : ComponentManager;
  }
}
