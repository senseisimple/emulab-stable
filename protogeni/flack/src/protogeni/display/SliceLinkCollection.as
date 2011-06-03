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

package protogeni.display
{
	import mx.collections.ArrayCollection;
	
	import protogeni.resources.VirtualLink;

	public class SliceLinkCollection extends ArrayCollection
	{
		public function SliceLinkCollection(source:Array=null)
		{
			super(source);
		}
		
		public function hasLinkFor(first:SliceNode, second:SliceNode):Boolean
		{
			for each(var sl:SliceLink in this) {
				if(sl.isForNodes(first, second))
					return true;
			}
			return false;
		}
		
		public function getLinksFor(node:SliceNode):SliceLinkCollection
		{
			var newSliceCollection:SliceLinkCollection = new SliceLinkCollection();
			for each(var sl:SliceLink in this) {
				if(sl.hasNode(node))
					newSliceCollection.addItem(sl);
			}
			return newSliceCollection;
		}
		
		public function containsVirtualLink(vl:VirtualLink):Boolean
		{
			for each(var sl:SliceLink in this)
			{
				if(sl.virtualLink == vl)
					return true;
			}
			return false;
		}
		
		public function getForVirtualLink(vl:VirtualLink):SliceLink
		{
			for each(var sl:SliceLink in this)
			{
				if(sl.virtualLink == vl)
					return sl;
			}
			return null;
		}
	}
}