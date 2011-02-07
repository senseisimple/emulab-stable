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
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;

	public class RequestSliverDeleteAm extends Request
	{
		public function RequestSliverDeleteAm(s:Sliver) : void
		{
			super("SliverDelete", "Deleting sliver on " + s.manager.Hrn + " for slice named " + s.slice.hrn, CommunicationUtil.deleteSliverAm);
			ignoreReturnCode = true;
			sliver = s;
			op.pushField(sliver.slice.urn);
			op.pushField([sliver.slice.credential]);
			op.setExactUrl(sliver.manager.Url);
		}
		
		override public function complete(code : Number, response : Object) : *
		{
			try
			{
				if(response == true) {
					if(sliver.slice.slivers.getItemIndex(sliver) > -1)
						sliver.slice.slivers.removeItemAt(sliver.slice.slivers.getItemIndex(sliver));
					var old:Slice = Main.geniHandler.CurrentUser.slices.getByUrn(sliver.slice.urn);
					if(old != null)
					{
						if(old.slivers.getByUrn(sliver.urn) != null)
							old.slivers.removeItemAt(old.slivers.getItemIndex(old.slivers.getByUrn(sliver.urn)));
						Main.geniHandler.dispatchSliceChanged(old);
					}
				}
				Main.geniHandler.dispatchSliceChanged(sliver.slice);
			}
			catch(e:Error)
			{
				// problem removing??
			}
			
			return null;
		}
		
		public var sliver:Sliver;
	}
}
