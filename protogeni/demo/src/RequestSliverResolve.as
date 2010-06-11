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
  import flash.events.ErrorEvent;

  public class RequestSliverResolve extends Request
  {
    public function RequestSliverResolve(newCm : ComponentManager,
                                                                         newSliceUrn : String,
                                             newNext : Request) : void
    {
      super(newCm.getName());
      cm = newCm;
          sliceUrn = newSliceUrn;
      next = newNext;
    }

    override public function cleanup() : void
    {
      super.cleanup();
    }

    override public function start(credential : Credential) : Operation
    {
                opName = "Resolving sliver";
                op.reset(Geni.resolveResource);
              op.addField("urn", cm.getSliverUrn());
              op.addField("credentials", new Array(cm.getSliver()));
              op.setExactUrl(cm.getUrl());
                  return op;
    }

        override public function fail(event : ErrorEvent) : Request
        {
           return next;
        }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      var result : Request = next;
      if (code == 0)
      {
                var rspec = new XML(response.value.manifest);
                var links : Array = new Array();
                for each(var component:XML in rspec.children())
                {
                        if(component.localName() == "node")
                        {
                                Main.getConsole().appendText("\nAdding node ... " + Main.menu.getComponent(component.@component_urn, cm.getName(), true) + "\n");
                        }
                        else if(component.localName() == "link")
                        {
                                links.push(component);
                        }
                }
                // TODO: Process links after nodes to make sure the nodes exist
      }
      else
      {
        // No sliver
      }
      return result;
    }

    private var cm : ComponentManager;
        private var sliceUrn : String;
    private var next : Request;
  }
}
