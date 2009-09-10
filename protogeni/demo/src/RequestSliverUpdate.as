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
    public function RequestSliverUpdate(newManager : ComponentManager,
                                        newNodes : ActiveNodes,
                                        newRspec : String,
                                        newBeginTunnel : Boolean) : void
    {
      super(newManager.getName());
      manager = newManager;
      nodes = newNodes;
      rspec = newRspec;
      beginTunnel = newBeginTunnel;
      ticket = null;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      if (manager.getVersion() == 0)
      {
        if (! beginTunnel)
        {
          nodes.changeState(manager, ActiveNodes.PLANNED, ActiveNodes.CREATED);
        }
        opName = "Updating Sliver";
        op.reset(Geni.updateSliver);
        op.addField("credential", manager.getSliver());
        op.addField("rspec", rspec);
        op.addField("keys", credential.ssh);
        op.addField("impotent", Request.IMPOTENT);
      }
      else if (ticket == null)
      {
        if (! beginTunnel)
        {
          nodes.changeState(manager, ActiveNodes.PLANNED, ActiveNodes.CREATED);
        }
        opName = "Updating Ticket";
        op.reset(Geni.updateTicket);
        op.addField("credential", credential.slice);
        op.addField("ticket", manager.getTicket());
        op.addField("rspec", rspec);
      }
      else
      {
        opName = "Updating Sliver";
        op.reset(Geni.updateSliver);
        op.addField("credential", manager.getSliver());
        op.addField("ticket", ticket);
      }
      op.setUrl(manager.getUrl());
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      var result : Request = null;
      if (code == 0)
      {
        if (manager.getVersion() == 0)
        {
          nodes.commitState(manager);
        }
        else if (ticket == null)
        {
          var r = new RequestSliverUpdate(manager, nodes, rspec, beginTunnel);
          r.setTicket(response.value);
          result = r;
        }
        else
        {
          manager.setTicket(ticket);
          manager.setManifest(response.value);
          nodes.commitState(manager);
        }
      }
      else
      {
        if (ticket != null && manager.getVersion() > 0)
        {
          result = new RequestReleaseTicket(ticket, manager.getUrl(),
                                            manager.getName());
          manager.setTicket(null);
        }
        if (beginTunnel)
        {
          nodes.commitState(manager);
          nodes.revertState(manager);
        }
        else
        {
          nodes.revertState(manager);
        }
      }
      return result;
    }

    function setTicket(newTicket : String) : void
    {
      ticket = newTicket;
    }

    var manager : ComponentManager;
    var nodes : ActiveNodes;
    var rspec : String;
    var beginTunnel : Boolean;
    var ticket : String;
  }
}
