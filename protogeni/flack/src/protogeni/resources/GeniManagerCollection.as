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
	
	import protogeni.GeniHandler;
	
	public final class GeniManagerCollection extends ArrayCollection
	{
		private var owner:GeniHandler;
		
		public function GeniManagerCollection(source:Array=null)
		{
			super(source);
		}
		
		public function add(gm:GeniManager):void
		{
			this.addItem(gm);
			Main.geniDispatcher.dispatchGeniManagersChanged();
		}
		
		public function clear():void
		{
			for each(var gm:GeniManager in this)
			gm.clear();
			Main.geniDispatcher.dispatchGeniManagersChanged();
		}
		
		public function getByUrn(urn:String):GeniManager
		{
			for each(var gm:GeniManager in this)
			{
				if(gm.Urn.full == urn)
					return gm;
			}
			return null;
		}
		
		public function getByHrn(hrn:String):GeniManager
		{
			for each(var gm:GeniManager in this)
			{
				if(gm.Hrn == hrn)
					return gm;
			}
			return null;
		}
	}
}