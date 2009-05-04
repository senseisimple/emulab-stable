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
        op.addField("keys", credential.ssh);
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
          var ticket : String = response.value;
          cm.setTicket(cmIndex, ticket);
          setSliverIds(ticket);
          result = new RequestSliverCreate(cmIndex, nodes, cm, rspec);
        }
        else
        {
          credential.slivers[cmIndex] = response.value;
          cm.setTicket(cmIndex, null);

          if (! nodes.hasTunnels(cmIndex))
          {
            nodes.commitState(cmIndex);
          }
          else
          {
            var newRspec = nodes.getXml(cmIndex, true);
            result = new RequestSliverUpdate(cmIndex, nodes, cm, newRspec,
                                             true);
          }
        }
      }
      else
      {
        result = releaseTicket();
        nodes.revertState(cmIndex);
      }
      return result;
    }

    override public function fail() : Request
    {
      var result : Request = releaseTicket();
      nodes.revertState(cmIndex);
      return result;
    }

    function releaseTicket() : Request
    {
      var result : Request = null;
      if (cm.getTicket(cmIndex) != null)
      {
        result = new RequestReleaseTicket(cm.getTicket(cmIndex),
                                          cm.getUrl(cmIndex));
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
          nodes.setSliverId(cmIndex, name, sliverId);
        }
      }
    }

    var cmIndex : int;
    var nodes : ActiveNodes;
    var cm : ComponentManager;
    var rspec : String;
  }
}
