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
  public class RequestListComponents extends Request
  {
    public function RequestListComponents(newView : ComponentView) : void
    {
      super("Clearinghouse");
      view = newView;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
      opName = "Listing ComponentManagers";
      var opType = Geni.listComponents;
      op.reset(opType);
      op.addField("credential", credential.slice);
      op.setUrl(Geni.chUrl);
      return op;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      var result : Request = null;
      if (code == 0)
      {
        var head : RequestResourceDiscovery = null;
        for each (var input in response.value)
        {
          var current : ComponentManager
            = new ComponentManager(input.urn, input.hrn, "REMOVEME",
                                   input.url, null, 2);
          view.addManager(current);
          head = new RequestResourceDiscovery(current, head);
        }
        result = head;
      }
      else
      {
        Main.getConsole().appendText("ListComponents Failed\n");
      }
      return result;
    }

    private var view : ComponentView;
  }
}
