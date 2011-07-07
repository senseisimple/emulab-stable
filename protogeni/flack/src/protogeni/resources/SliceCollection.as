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

package protogeni.resources
{
	import mx.collections.ArrayCollection;
	import mx.collections.Sort;
	import mx.collections.SortField;
	import mx.utils.ObjectUtil;
	
	public final class SliceCollection extends ArrayCollection
	{
		public function SliceCollection(source:Array=null)
		{
			super(source);
		}
		
		public function add(s:Slice):void
		{
			this.addItem(s);
			Main.geniDispatcher.dispatchSlicesChanged();
		}
		
		public function getByUrn(urn:String):Slice
		{
			for each(var existing:Slice in this)
			{
				if(existing.urn.full == urn)
					return existing;
			}
			return null;
		}
		
		public function addOrReplace(s:Slice):void
		{
			for each(var existing:Slice in this)
			{
				if(existing.urn.full == s.urn.full)
				{
					existing = s;
					Main.geniDispatcher.dispatchSlicesChanged();
					return;
				}
			}
			add(s);
		}
		
		public function removeOutsideReferences():void {
			for each(var existing:Slice in this) {
				existing.removeOutsideReferences();
			}
		}
	}
}