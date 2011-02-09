package protogeni.resources
{
	import mx.collections.ArrayCollection;

	public class Site
	{
		public var id:String;
		public var name:String;
		public var nodes:ArrayCollection;
		
		public function Site()
		{
			nodes = new ArrayCollection();
		}
	}
}