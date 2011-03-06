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
  import com.mattism.http.xmlrpc.MethodFault;
  
  import flash.events.ErrorEvent;

  public final class RequestGeniBundle extends Request
  {
    public function RequestGeniBundle():void
    {
		super("CertBundle",
			"Getting cert bundle",
			null,
			true);
		op.setExactUrl(Main.geniHandler.certBundleUrl);
		op.type = Operation.HTTP;
		op.timeout = 20;
	}
	
	override public function complete(code:Number, response:Object):*
	{
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			FlackCache.instance.geniBundle = response as String;
		}
		
		return null;
	}
	
	override public function fail(event:ErrorEvent, fault:MethodFault):*
	{
		if(FlackCache.instance.geniBundle.length == 0)
			FlackCache.instance.geniBundle = (new FallbackGeniBundle()).toString();
	}
  }
}
