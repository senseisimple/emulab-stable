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

  public class RequestGetCredential extends Request
  {
    public function RequestGetCredential() : void
    {
      super("GetCredential", "Getting the basic user credential", CommunicationUtil.getCredential);
	  op.setUrl(Main.geniHandler.CurrentUser.authority.Url);
    }

    override public function complete(code : Number, response : Object) : *
    {
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			Main.geniHandler.CurrentUser.credential = String(response.value);
			var cred:XML = new XML(response.value);
			Main.geniHandler.CurrentUser.urn = cred.credential.owner_urn;
			Main.geniHandler.dispatchUserChanged();
		}
		else
		{
			Main.geniHandler.requestHandler.codeFailure(name, "Recieved GENI response other than success");
		}

      return null;
    }
  }
}
