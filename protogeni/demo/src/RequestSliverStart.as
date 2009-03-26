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
  class RequestSliverStart extends Request
  {
    public function RequestSliverStart(newCmIndex : int,
                                       newNodes : ActiveNodes,
                                       newUrl : String) : void
    {
      cmIndex = newCmIndex;
      nodes = newNodes;
      url = newUrl;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      nodes.changeState(cmIndex, ActiveNodes.CREATED, ActiveNodes.BOOTED);
      opName = "Booting Sliver";
      op.reset(Geni.startSliver);
      op.addField("credential", credential.slivers[cmIndex]);
      op.addField("impotent", Request.IMPOTENT);
      op.setUrl(url);
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      if (code == 0)
      {
        nodes.commitState(cmIndex);
      }
      else
      {
        nodes.revertState(cmIndex);
      }
      return null;
    }

    var cmIndex : int;
    var nodes : ActiveNodes;
    var url : String;
  }
}
