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
  class RequestSliverCreate extends Request
  {
    public function RequestSliverCreate(newCmIndex : int,
                                        newNodes : ActiveNodes,
                                        newCm : ComponentManager,
                                        newRspec : String) : void
    {
      cmIndex = newCmIndex;
      nodes = newNodes;
      cm = newCm;
      rspec = newRspec;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      if (cm.getTicket(cmIndex) == null)
      {
        nodes.changeState(cmIndex, ActiveNodes.PLANNED, ActiveNodes.CREATED);
        opName = "Getting Ticket";
        op.reset(Geni.getTicket);
        op.addField("credential", credential.slice);
        op.addField("rspec", rspec);
        op.addField("impotent", Request.IMPOTENT);
        op.setUrl(cm.getUrl(cmIndex));
      }
      else
      {
        opName = "Redeeming Ticket";
        op.reset(Geni.redeemTicket);
        op.addField("ticket", cm.getTicket(cmIndex));
        op.addField("impotent", Request.IMPOTENT);
//        op.addField("keys", credential.ssh);
        op.setUrl(cm.getUrl(cmIndex));
      }
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      var result : Request = null;
      if (code == 0)
      {
        if (cm.getTicket(cmIndex) == null)
        {
          cm.setTicket(cmIndex, response.value);
          result = new RequestSliverCreate(cmIndex, nodes, cm, rspec);
        }
        else
        {
          nodes.commitState(cmIndex);
          credential.slivers[cmIndex] = response.value;
          cm.setTicket(cmIndex, null);
        }
      }
      else
      {
        if (cm.getTicket(cmIndex) != null)
        {
          // TODO: DeleteTicket
        }
        nodes.revertState(cmIndex);
      }
      return result;
    }

    override public function fail() : void
    {
      // TODO: DeleteTicket
      nodes.revertState(cmIndex);
    }

    var cmIndex : int;
    var nodes : ActiveNodes;
    var cm : ComponentManager;
    var rspec : String;
  }
}
