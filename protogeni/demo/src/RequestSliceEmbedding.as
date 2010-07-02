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
  class RequestSliceEmbedding extends Request
  {
    public function RequestSliceEmbedding(newManager : ComponentManager,
                                          newNodes : ActiveNodes,
                                          newRequest : String,
                                          newUrl : String) : void
    {
      super("SES");
      manager = newManager;
      nodes = newNodes;
      request = newRequest;
      url = newUrl;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      opName = "Embedding Slice";
      op.reset(Geni.map);
      op.addField("credential", credential.base);
      op.addField("advertisement", manager.getAd());
      op.addField("request", request);
      op.setUrl(url);
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      if (code == 0)
      {
        nodes.mapRequest(response.value, manager);
      }
      return null;
    }

    override public function fail() : Request
    {
      return null;
    }

    var manager : ComponentManager;
    var nodes : ActiveNodes;
    var request : String;
    var url : String;
  }
}
