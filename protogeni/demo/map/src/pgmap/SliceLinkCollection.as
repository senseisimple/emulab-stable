package pgmap
{
	import mx.collections.ArrayCollection;

	public class SliceLinkCollection extends ArrayCollection
	{
		public function SliceLinkCollection(source:Array=null)
		{
			super(source);
		}
		
		public function hasLinkFor(first:SliceNode, second:SliceNode):Boolean
		{
			for each(var sl:SliceLink in this) {
				if(sl.isForNodes(first, second))
					return true;
			}
			return false;
		}
		
		public function getLinksFor(node:SliceNode):SliceLinkCollection
		{
			var newSliceCollection:SliceLinkCollection = new SliceLinkCollection();
			for each(var sl:SliceLink in this) {
				if(sl.hasNode(node))
					newSliceCollection.addItem(sl);
			}
			return newSliceCollection;
		}
	}
}