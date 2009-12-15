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
	
	// Group of physical links
	public class PhysicalLinkGroup
	{
		public var latitude1:Number = -1;
		public var longitude1:Number = -1;
		public var latitude2:Number = -1;
		public var longitude2:Number = -1;
		public var owner:PhysicalLinkGroupCollection = null;
		public var collection:ArrayCollection = new ArrayCollection;
		
		public function PhysicalLinkGroup(lat1:Number, lng1:Number, lat2:Number, lng2:Number, own:PhysicalLinkGroupCollection)
		{
			latitude1 = lat1;
			longitude1 = lng1;
			latitude2 = lat2;
			longitude2 = lng2;
			owner = own;
		}
		
		public function Add(l:PhysicalLink):void {
			collection.addItem(l);
		}

		public function IsSameSite():Boolean {
			return latitude1 == latitude2 && longitude1 == longitude2;
		}
		
		public function TotalBandwidth():Number {
			var bw:Number = 0;
			for each(var l:PhysicalLink in collection) {
				bw += l.bandwidth;
			}
			return bw;
		}
		
		public function AverageBandwidth():Number {
			return TotalBandwidth() / collection.length;
		}
		
		public function Latency():Number {
			var la:Number = 0;
			for each(var l:PhysicalLink in collection) {
				la += l.latency;
			}
			return la / collection.length;
		}
	}
}