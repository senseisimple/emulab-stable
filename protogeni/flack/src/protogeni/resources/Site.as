package protogeni.resources
{
	import mx.collections.ArrayCollection;

	public class Site
	{
		public var id:String;
		public var name:String;
		public var nodes:Vector.<PhysicalNode>;
		public var hrn:String;
		public var latitude:Number;
		public var longitude:Number;
		
		public function Site()
		{
			nodes = new Vector.<PhysicalNode>();
		}
	}
}