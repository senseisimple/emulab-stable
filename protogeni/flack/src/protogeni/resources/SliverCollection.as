package protogeni.resources
{
	import mx.collections.ArrayCollection;
	
	import protogeni.Util;
	
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
		
		public function getCombined():Sliver
		{
			if(this.length == 0)
				return null;
			var fakeManager:ProtogeniComponentManager = new ProtogeniComponentManager();
			fakeManager.inputRspecVersion = Util.defaultRspecVersion;
			fakeManager.Hrn = "Combined";
			var newSliver:Sliver = new Sliver(null, fakeManager);
			for each(var sliver:Sliver in this) {
				for each(var node:VirtualNode in sliver.nodes) {
					if(!newSliver.nodes.contains(node))
						newSliver.nodes.addItem(node);
				}
				for each(var link:VirtualLink in sliver.links) {
					if(!newSliver.links.contains(link))
						newSliver.links.addItem(link);
				}
			}
			return newSliver;
		}
	}
}