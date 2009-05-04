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
  class RequestSliverUpdate extends Request
  {
    public function RequestSliverUpdate(newCmIndex : int,
                                        newNodes : ActiveNodes,
                                        newCm : ComponentManager,
                                        newRspec : String,
                                        newBeginTunnel : Boolean) : void
    {
      cmIndex = newCmIndex;
      nodes = newNodes;
      cm = newCm;
      rspec = newRspec;
      beginTunnel = newBeginTunnel;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      if (! beginTunnel)
      {
        nodes.changeState(cmIndex, ActiveNodes.PLANNED, ActiveNodes.CREATED);
      }
      opName = "Updating Sliver";
      op.reset(Geni.updateSliver);
      op.addField("credential", credential.slivers[cmIndex]);
      op.addField("rspec", rspec);
      op.addField("keys", credential.ssh);
      op.addField("impotent", Request.IMPOTENT);
      op.setUrl(cm.getUrl(cmIndex));
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      var result = null;
      if (code == 0)
      {
        nodes.commitState(cmIndex);
      }
      else
      {
        if (beginTunnel)
        {
          nodes.commitState(cmIndex);
          nodes.revertState(cmIndex);
        }
        else
        {
          nodes.revertState(cmIndex);
        }
      }
      return result;
    }

    var cmIndex : int;
    var nodes : ActiveNodes;
    var cm : ComponentManager;
    var rspec : String;
    var beginTunnel : Boolean;
  }
}
