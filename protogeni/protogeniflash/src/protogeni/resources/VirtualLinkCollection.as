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
				if(link.id == id)
					return link;
			}
			return null;
		}
	}
}