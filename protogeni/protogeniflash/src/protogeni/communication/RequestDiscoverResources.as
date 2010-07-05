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

package protogeni.communication
{
  import flash.events.ErrorEvent;
  import flash.events.SecurityErrorEvent;

  public class RequestDiscoverResources extends Request
  {
    public function RequestDiscoverResources(newCm : ComponentManager,
                                             newNext : Request) : void
    {
      super(newCm.getName());
      cm = newCm;
      next = newNext;
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
      op.addField("credentials", new Array(credential.slice));
      op.setUrl(cm.getUrl());
      return op;
    }

    override public function fail(event : ErrorEvent) : Request
    {
      var state = ComponentManager.IO_FAILURE;
      if (event is SecurityErrorEvent)
      {
        state = ComponentManager.SECURITY_FAILURE;
      }
      cm.resourceFailure(state);
      return next;
    }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      var result : Request = next;
      if (code == 0)
      {
        cm.resourceSuccess(response.value);
      }
      else
      {
        cm.resourceFailure(ComponentManager.INTERNAL_FAILURE);
      }
      return result;
    }

    private var cm : ComponentManager;
    public var next : Request;
  }
}
