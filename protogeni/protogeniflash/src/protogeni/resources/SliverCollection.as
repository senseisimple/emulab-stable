package protogeni.resources
{
	import mx.collections.ArrayCollection;
	
	public class SliverCollection extends ArrayCollection
	{
		public function SliverCollection(source:Array=null)
		{
			super(source);
		}
		
		public function addIfNotExisting(s:Sliver):void
		{
			for each(var sliver:Sliver in this)
			{
				if(sliver.manager == s.manager)
					return;
			}
			this.addItem(s);
		}
		
		public function clone():SliverCollection
		{
			var sc:SliverCollection = new SliverCollection();
			for each(var sliver:Sliver in this)
			{
				sc.addItem(sliver);
			}
			return sc;
		}
		
		public function getByUrn(urn:String):Sliver
		{
			for each(var sliver:Sliver in this)
			{
				if(sliver.urn == urn)
					return sliver;
			}
			return null;
		}
		
		public function getByGm(gm:GeniManager):Sliver
		{
			for each(var sliver:Sliver in this)
			{
				if(sliver.manager == gm)
					return sliver;
			}
			return null;
		}
	}
}