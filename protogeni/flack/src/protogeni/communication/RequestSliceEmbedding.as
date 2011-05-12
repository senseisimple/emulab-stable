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

package protogeni.communication
{
	import mx.utils.Base64Encoder;
	
	import protogeni.resources.Sliver;

  public class RequestSliceEmbedding extends Request
  {
    public function RequestSliceEmbedding(newSliver:Sliver) : void
    {
      super("SES","Embedding the sliver", CommunicationUtil.map);
      sliver = newSliver;
	  op.timeout = 500;
	  
	  var encoder:Base64Encoder = new Base64Encoder();
	  encoder.encode(sliver.manager.Rspec.toXMLString());
	  var encodedRspec:String = encoder.toString();
	  
	  op.addField("credential", newSliver.slice.creator.credential);
	  op.addField("advertisement", encodedRspec);
	  op.addField("request", sliver.getRequestRspec());
	  op.setUrl(CommunicationUtil.sesUrl);
    }

	override public function complete(code : Number, response : Object) : *
	{
		if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
		{
			//nodes.mapRequest(response.value, manager);
		}
		
		return null;
    }

    public var sliver:Sliver;
  }
}
