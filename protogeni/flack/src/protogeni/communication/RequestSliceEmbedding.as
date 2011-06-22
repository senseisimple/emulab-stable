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
	import mx.utils.Base64Encoder;
	
	import protogeni.resources.Sliver;
	
	/**
	 * NOT WORKING. Binds unbound resources using the slice embedding service.  Uses the ProtoGENI API.
	 * @author mstrum
	 * 
	 */
	public final class RequestSliceEmbedding extends Request
	{
		public var sliver:Sliver;
		
		public function RequestSliceEmbedding(newSliver:Sliver):void
		{
			super("SES",
				"Embedding the sliver",
				CommunicationUtil.map);
			sliver = newSliver;
			op.timeout = 500;
			
			var encoder:Base64Encoder = new Base64Encoder();
			encoder.encode(sliver.manager.Rspec.toXMLString());
			var encodedRspec:String = encoder.toString();
			
			// Build up the args
			op.addField("credential", newSliver.slice.creator.Credential);
			op.addField("advertisement", encodedRspec);
			op.addField("request", sliver.getRequestRspec(true));
			op.setUrl(CommunicationUtil.sesUrl);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				//nodes.mapRequest(response.value, manager);
			}
			
			return null;
		}
	}
}
