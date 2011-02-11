package protogeni.resources
{
	import mx.collections.ArrayList;

	public class PlanetlabAggregateManager extends AggregateManager
	{
		public var registryUrl:String;
		public var networkName:String;
		public var sites:ArrayList;
		public var Authority:String;
		
		public function PlanetlabAggregateManager()
		{
			super();
			this.Url = "https://planet-lab.org:12346";
			this.registryUrl = "https://planet-lab.org:12345";
			this.Hrn = "planet-lab.am";
			this.Authority = "planet-lab.org";
			this.Urn = "urn:publicid:IDN+planet-lab.org+authority+am";
			this.sites = new ArrayList();
			
			this.rspecProcessor = new PlanetlabRspecProcessor(this);
		}
		
		public function getSiteWithName(n:String):Site
		{
			for each(var s:Site in sites) {
				if(s.name == n)
					return s;
			}
			return null;
		}
		
		public function getSiteWithId(i:String):Site
		{
			for each(var s:Site in sites) {
				if(s.id == i)
					return s;
			}
			return null;
		}
	}
}