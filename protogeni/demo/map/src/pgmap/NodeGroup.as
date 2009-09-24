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
 
 package pgmap
{	
	import mx.collections.ArrayCollection;
	
	public class NodeGroup
	{
		public var latitude:Number = -1;
		public var longitude:Number = -1;
		public var country:String = "";
		public var city:String = "";
		
		public var owner:NodeGroupCollection = null;
		public var collection:ArrayCollection = new ArrayCollection;
		public var links:LinkGroup = null;
		
		public function NodeGroup(lat:Number, lng:Number, cnt:String, own:NodeGroupCollection)
		{
			latitude = lat;
			longitude = lng;
			country = cnt;
			owner = own;
		}
		
		public function Add(n:Node):void {
			collection.addItem(n);
		}

		public function GetByUUID(uuid:String):Node {
			for each ( var n:Node in collection ) {
				if(n.uuid == uuid)
					return n;
			}
			return null;
		}
		
		public function Available():Number {
			var cnt:Number = 0;
			for each ( var n:Node in collection ) {
				if(n.available)
					cnt++;
			}
			return cnt;
		}
		
		public function ExternalLinks():Number {
			var cnt:Number = 0;
			for each ( var n:Node in collection ) {
				for each ( var l:Link in n.GetLinks() ) {
					if(l.owner != n.owner.links)
						cnt++;
				}
			}
			return cnt;
		}
	}
}