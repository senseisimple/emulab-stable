package protogeni.resources
{
	import protogeni.Util;
	
	public class SliverCollection
	{
		public var collection:Vector.<Sliver>;
		public function SliverCollection()
		{
			collection = new Vector.<Sliver>();
		}
		
		public function add(s:Sliver):void {
			collection.push(s);
		}
		
		public function remove(s:Sliver):void
		{
			var idx:int = collection.indexOf(s);
			if(idx > -1)
				collection.splice(idx, 1);
		}
		
		public function contains(s:Sliver):Boolean
		{
			return collection.indexOf(s) > -1;
		}
		
		public function get length():int{
			return this.collection.length;
		}
		
		public function addIfNotExisting(s:Sliver):void
		{
			for each(var sliver:Sliver in this.collection)
			{
				if(sliver.manager == s.manager)
					return;
			}
			this.collection.push(s);
		}
		
		public function clone():SliverCollection
		{
			var sc:SliverCollection = new SliverCollection();
			for each(var sliver:Sliver in this.collection)
				sc.add(sliver);
			return sc;
		}
		
		public function getByUrn(urn:String):Sliver
		{
			for each(var sliver:Sliver in this.collection)
			{
				if(sliver.urn == urn)
					return sliver;
			}
			return null;
		}
		
		public function getByGm(gm:GeniManager):Sliver
		{
			for each(var sliver:Sliver in this.collection)
			{
				if(sliver.manager == gm)
					return sliver;
			}
			return null;
		}
		
		public function getCombined():Sliver
		{
			if(this.collection.length == 0)
				return null;
			var fakeManager:ProtogeniComponentManager = new ProtogeniComponentManager();
			fakeManager.inputRspecVersion = Util.defaultRspecVersion;
			fakeManager.Hrn = "Combined";
			var newSliver:Sliver = new Sliver(null, fakeManager);
			for each(var sliver:Sliver in this.collection) {
				for each(var node:VirtualNode in sliver.nodes.collection) {
					if(!newSliver.nodes.contains(node))
						newSliver.nodes.add(node);
				}
				for each(var link:VirtualLink in sliver.links.collection) {
					if(!newSliver.links.contains(link))
						newSliver.links.add(link);
				}
			}
			return newSliver;
		}
	}
}