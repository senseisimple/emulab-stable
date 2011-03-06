package protogeni.resources
{
	public final class VirtualLinkCollection
	{
		public var collection:Vector.<VirtualLink>
		public function VirtualLinkCollection()
		{
			collection = new Vector.<VirtualLink>();
			/*if(source != null) {
				for each(var link:VirtualLink in source)
					collection.push(link);
			}*/
		}
		
		public function add(l:VirtualLink):void {
			collection.push(l);
		}
		
		public function remove(l:VirtualLink):void
		{
			var idx:int = collection.indexOf(l);
			if(idx > -1)
				collection.splice(idx, 1);
		}
		
		public function contains(l:VirtualLink):Boolean
		{
			return collection.indexOf(l) > -1;
		}
		
		public function get length():int{
			return this.collection.length;
		}
		
		public function getById(id:String):VirtualLink
		{
			for each(var link:VirtualLink in this.collection)
			{
				if(link.clientId == id)
					return link;
			}
			return null;
		}
		
		public function getForNode(vn:VirtualNode):VirtualLinkCollection
		{
			var vlc:VirtualLinkCollection = new VirtualLinkCollection();
			for each(var vl:VirtualLink in this.collection)
			{
				for each(var vli:VirtualInterface in vl.interfaces.collection)
				{
					if(vli.owner == vn)
					{
						vlc.add(vl);
						break;
					}
				}
			}
			return vlc;
		}
	}
}