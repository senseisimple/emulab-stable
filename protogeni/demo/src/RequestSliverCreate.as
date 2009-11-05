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
    public function RequestSliverCreate(newManager : ComponentManager,
                                        newNodes : ActiveNodes,
                                        newRspec : String) : void
    {
      super(newManager.getName());
      manager = newManager;
      nodes = newNodes;
      rspec = newRspec;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      if (manager.getTicket() == null)
      {
        nodes.changeState(manager, ActiveNodes.PLANNED, ActiveNodes.CREATED);
        opName = "Getting Ticket";
        op.reset(Geni.getTicket);
        op.addField("credential", credential.slice);
        op.addField("rspec", rspec);
        op.addField("impotent", Request.IMPOTENT);
        op.setUrl(manager.getUrl());
      }
      else
      {
        opName = "Redeeming Ticket";
        op.reset(Geni.redeemTicket);
        op.addField("credential", credential.slice);
        op.addField("ticket", manager.getTicket());
        op.addField("impotent", Request.IMPOTENT);
        op.addField("keys", credential.ssh);
        op.setUrl(manager.getUrl());
      }
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      var result : Request = null;
      if (code == 0)
      {
        if (manager.getTicket() == null)
        {
          var ticket : String = response.value;
          manager.setTicket(ticket);
//          setSliverIds(ticket);
          result = new RequestSliverCreate(manager, nodes, rspec);
        }
        else
        {
          if (manager.getVersion() == 0)
          {
            manager.setSliver(response.value);
            setSliverIds(response.value);
          }
          else
          {
            manager.setSliver(response.value[0]);
            manager.setManifest(response.value[1]);
          }

          if (! nodes.hasTunnels(manager))
          {
            nodes.commitState(manager);
          }
          else
          {
            var newRspec = nodes.getXml(true, manager);
            result = new RequestSliverUpdate(manager, nodes, newRspec,
                                             true);
          }
        }
      }
      else
      {
        result = releaseTicket();
        nodes.revertState(manager);
      }
      return result;
    }

    override public function fail() : Request
    {
      var result : Request = releaseTicket();
      nodes.revertState(manager);
      return result;
    }

    function releaseTicket() : Request
    {
      var result : Request = null;
      if (manager.getTicket() != null)
      {
        result = new RequestReleaseTicket(manager);
      }
      return result;
    }

    function setSliverIds(signedCredStr : String) : void
    {
      var signedCred : XML = XML(signedCredStr);
      for each (var node in signedCred.descendants("node"))
      {
        var name : String = node.elements("nickname");
        var sliverId : String = node.elements("sliver_uuid");
        if (name != "" && sliverId != "")
        {
          nodes.setSliverId(manager, name, sliverId);
        }
      }
    }

    var manager : ComponentManager;
    var nodes : ActiveNodes;
    var rspec : String;
  }
}
