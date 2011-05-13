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
				if(sliver.componentManager == s.componentManager)
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
		
		public function getByCm(cm:ComponentManager):Sliver
		{
			for each(var sliver:Sliver in this)
			{
				if(sliver.componentManager == cm)
					return sliver;
			}
			return null;
		}
	}
}