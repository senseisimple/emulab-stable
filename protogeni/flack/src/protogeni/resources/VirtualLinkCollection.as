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
	public final class VirtualLinkCollection
	{
		public var collection:Vector.<VirtualLink>
		public function VirtualLinkCollection()
		{
			this.collection = new Vector.<VirtualLink>();
			/*if(source != null) {
			for each(var link:VirtualLink in source)
			collection.push(link);
			}*/
		}
		
		public function add(l:VirtualLink):void {
			this.collection.push(l);
		}
		
		public function remove(l:VirtualLink):void
		{
			var idx:int = collection.indexOf(l);
			if(idx > -1)
				this.collection.splice(idx, 1);
		}
		
		public function contains(l:VirtualLink):Boolean
		{
			return this.collection.indexOf(l) > -1;
		}
		
		public function get length():int{
			return this.collection.length;
		}
		
		public function getById(id:String):VirtualLink
		{
			for each(var link:VirtualLink in this.collection)
			{
				if(link.clientId == id)
					return link;
			}
			return null;
		}
		
		public function getForNode(vn:VirtualNode):VirtualLinkCollection
		{
			var vlc:VirtualLinkCollection = new VirtualLinkCollection();
			for each(var vl:VirtualLink in this.collection)
			{
				for each(var vli:VirtualInterface in vl.interfaces.collection)
				{
					if(vli.owner == vn)
					{
						vlc.add(vl);
						break;
					}
				}
			}
			return vlc;
		}
	}
}