package protogeni.resources
{
	import mx.collections.ArrayCollection;
	
	public class VirtualLinkCollection extends ArrayCollection
	{
		public function VirtualLinkCollection(source:Array=null)
		{
			super(source);
		}
		
		public function getById(id:String):VirtualLink
		{
			for each(var link:VirtualLink in this)
			{
				if(link.clientId == id)
					return link;
			}
			return null;
		}
		
		public function getForNode(vn:VirtualNode):VirtualLinkCollection
		{
			var vlc:VirtualLinkCollection = new VirtualLinkCollection();
			for each(var vl:VirtualLink in this)
			{
				for each(var vli:VirtualInterface in vl.interfaces)
				{
					if(vli.owner == vn)
					{
						vlc.addItem(vl);
						break;
					}
				}
			}
			return vlc;
		}
	}
}