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
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;

	public final class RequestSliverDeleteAm extends Request
	{
		public function RequestSliverDeleteAm(s:Sliver) : void
		{
			super("SliverDelete", "Deleting sliver on " + s.manager.Hrn + " for slice named " + s.slice.hrn, CommunicationUtil.deleteSliverAm);
			ignoreReturnCode = true;
			sliver = s;
			op.pushField(sliver.slice.urn.full);
			op.pushField([sliver.slice.credential]);
			op.setExactUrl(sliver.manager.Url);
		}
		
		override public function complete(code : Number, response : Object) : *
		{
			try
			{
				if(response == true) {
					if(sliver.slice.slivers.contains(sliver))
						sliver.slice.slivers.remove(sliver);
					var old:Slice = Main.geniHandler.CurrentUser.slices.getByUrn(sliver.slice.urn.full);
					if(old != null)
					{
						if(old.slivers.getByUrn(sliver.urn.full) != null)
							old.slivers.remove(old.slivers.getByUrn(sliver.urn.full));
						Main.geniDispatcher.dispatchSliceChanged(old);
					}
				}
				Main.geniDispatcher.dispatchSliceChanged(sliver.slice);
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
