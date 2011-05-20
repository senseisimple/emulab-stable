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
	
	public class PlanetlabAggregateManager extends AggregateManager
	{
		public var registryUrl:String;
		public var networkName:String;
		public var sites:Vector.<Site>;
		
		public function PlanetlabAggregateManager()
		{
			super();
			this.Url = "https://planet-lab.org:12346/";
			this.registryUrl = "https://planet-lab.org:12345/";
			this.Hrn = "planet-lab.am";
			this.Urn = new IdnUrn("urn:publicid:IDN+planet-lab.org+authority+am");
			this.sites = new Vector.<Site>();
			this.type = GeniManager.TYPE_PLANETLAB;
			
			this.rspecProcessor = new PlanetlabRspecProcessor(this);
		}
		
		public function getSiteWithHrn(n:String):Site
		{
			for each(var s:Site in this.sites) {
				if(s.hrn == n)
					return s;
			}
			return null;
		}
		
		public function getSiteWithName(n:String):Site
		{
			for each(var s:Site in this.sites) {
				if(s.name == n)
					return s;
			}
			return null;
		}
		
		public function getSiteWithId(i:String):Site
		{
			for each(var s:Site in this.sites) {
				if(s.id == i)
					return s;
			}
			return null;
		}
	}
}