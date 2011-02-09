package protogeni.resources
{
	import mx.collections.ArrayList;

	public class PlanetLabAggregateManager extends AggregateManager
	{
		public var networkName:String;
		public var sites:ArrayList;
		
		public function PlanetLabAggregateManager()
		{
			super();
			adParser = new PlanetLabAdParser(this);
			sites = new ArrayList();
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