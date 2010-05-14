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
  public class RequestSliverGet extends Request
  {
    public function RequestSliverGet(newCm : ComponentManager,
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
	  opName = "Getting sliver";
	  op.reset(Geni.getSliver);
	  op.addField("slice_urn", sliceUrn);
	  op.addField("credentials", new Array(credential.slice));
	  op.setExactUrl(cm.getUrl());
	  return op;
    }

        override public function fail() : Request
        {
           return next;
        }

    override public function complete(code : Number, response : Object,
                                      credential : Credential) : Request
    {
      var result : Request = next;
      if (code == 0)
      {
		// Sliver exists
        cm.setSliver(String(response.value));
	    var cred:XML = new XML(response.value);
	    cm.setSliverUrn(cred.credential.target_urn);
		result = new RequestSliverResolve(cm, sliceUrn, null);
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
