package protogeni.resources
{
	public class SliverType
	{
		public var name:String;
		public var diskImages:Vector.<DiskImage>;
		
		public function SliverType(newName:String = "")
		{
			name = newName;
			diskImages = new Vector.<DiskImage>();
		}
	}
}