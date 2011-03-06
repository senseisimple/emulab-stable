/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2008-2011 University of Utah and the Flux Group.
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
	import protogeni.resources.Sliver;

  public final class RequestTicketGet extends Request
  {
    public function RequestTicketGet(s:Sliver) : void
    {
		super("TicketGet", "Getting ticket for sliver on " + s.componentManager.Hrn + " for slice named " + s.slice.hrn, CommunicationUtil.updateTicket);
		sliver = s;
		op.addField("slice_urn", sliver.slice.urn.full);
		op.addField("ticket", ticket);
		op.addField("rspec", s.getRequestRspec());
		op.addField("credentials", new Array(sliver.slice.credential));
		op.setUrl(sliver.componentManager.Url);
    }
	
	override public function complete(code : Number, response : Object) : *
	{
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			sliver.ticket = new XML(response.value);
			return new RequestTicketRedeem(sliver, sliver.ticket);
		}
		else
		{
			// do nothing
		}
		
		return null;
	}
	
	public var sliver:Sliver;
	public var ticket:XML;
  }
}
