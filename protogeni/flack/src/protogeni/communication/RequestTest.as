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
	/**
	 * Used for testing random things...
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestTest extends Request
	{
		public function RequestTest():void
		{
			super("ListTest",
				"Just doing a test...",
				null);
			//Security.loadPolicyFile("http://pc534.emulab.net");
			op.setExactUrl("http://pc534.emulab.net/index.html");
			op.type = Operation.HTTP;
		}
		
		// Should return Request or RequestQueueNode
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				Main.log.appendMessage(new LogMessage(op.getUrl(), "ListTest", response.toString()));
			}
			else
			{
				//Main.geniHandler.requestHandler.codeFailure(name, "Recieved GENI response other than success");
			}
			
			return null;
		}
	}
}
