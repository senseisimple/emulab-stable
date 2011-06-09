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
	import protogeni.GeniEvent;
	import protogeni.resources.Sliver;
	
	/**
	 * Gets the manifest for a sliver using the ProtoGENI API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliverResolve extends Request
	{
		private var sliver:Sliver;
		
		public function RequestSliverResolve(newSliver:Sliver):void
		{
			super("SliverResolve",
				"Resolving sliver on " + newSliver.manager.Hrn + " on slice named " + newSliver.slice.hrn,
				CommunicationUtil.resolveResource,
				true);
			sliver = newSliver;
			
			// Build up the args
			op.addField("urn", sliver.urn.full);
			op.addField("credentials", [sliver.credential]);
			op.setUrl(sliver.manager.Url);
		}
		
		override public function complete(code:Number, response:Object):*
		{
			if (code == CommunicationUtil.GENIRESPONSE_SUCCESS)
			{
				sliver.rspec = new XML(response.value.manifest);
				sliver.created = true;
				sliver.parseRspec();
				if(sliver.slice.isCreated())
					Main.geniDispatcher.dispatchSliceChanged(sliver.slice, GeniEvent.ACTION_POPULATED);
				else
					Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
				return new RequestSliverStatus(sliver);
			}
			else
			{
				// do nothing
			}
			
			return null;
		}
	}
}
