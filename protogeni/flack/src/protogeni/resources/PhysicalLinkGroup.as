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
	// Group of physical links
	public class PhysicalLinkGroup
	{
		public var latitude1:Number = -1;
		public var longitude1:Number = -1;
		public var latitude2:Number = -1;
		public var longitude2:Number = -1;
		public var owner:PhysicalLinkGroupCollection = null;
		public var collection:Vector.<PhysicalLink>;
		
		public function PhysicalLinkGroup(lat1:Number,
										  lng1:Number,
										  lat2:Number,
										  lng2:Number,
										  own:PhysicalLinkGroupCollection)
		{
			this.collection = new Vector.<PhysicalLink>();
			this.latitude1 = lat1;
			this.longitude1 = lng1;
			this.latitude2 = lat2;
			this.longitude2 = lng2;
			this.owner = own;
		}
		
		public function Add(l:PhysicalLink):void {
			this.collection.push(l);
		}
		
		public function IsSameSite():Boolean {
			return this.latitude1 == this.latitude2
				&& this.longitude1 == this.longitude2;
		}
		
		public function TotalBandwidth():Number {
			var bw:Number = 0;
			for each(var l:PhysicalLink in this.collection) {
				bw += l.capacity;
			}
			return bw;
		}
		
		public function AverageBandwidth():Number {
			return this.TotalBandwidth() / this.collection.length;
		}
		
		public function Latency():Number {
			var la:Number = 0;
			for each(var l:PhysicalLink in this.collection) {
				la += l.latency;
			}
			return la / this.collection.length;
		}
		
		public function GetManager():GeniManager
		{
			return this.collection[0].manager;
		}
	}
}