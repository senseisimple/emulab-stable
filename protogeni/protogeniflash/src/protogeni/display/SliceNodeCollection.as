package protogeni.display
{
	import mx.collections.ArrayCollection;
	
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.VirtualNode;
	
	public class SliceNodeCollection extends ArrayCollection
	{
		public function SliceNodeCollection(source:Array=null)
		{
			super(source);
		}
		
		public function containsPhysicalNode(node:PhysicalNode):Boolean
		{
			for each (var sn:SliceNode in this)
			{
				if(sn.node.physicalNode == node)
					return true;
			}
			return false;
		}
		
		public function containsVirtualNode(node:VirtualNode):Boolean
		{
			for each (var sn:SliceNode in this)
			{
				if(sn.node == node)
					return true;
			}
			return false;
		}
		
		public function getForPhysicalNode(node:PhysicalNode):SliceNode
		{
			for each (var sn:SliceNode in this)
			{
				if(sn.node.physicalNode == node)
					return sn;
			}
			return null;
		}
		
		public function seperateNodesByComponentManager():Array
		{
			var completedCms:ArrayCollection = new ArrayCollection();
			var completed:Array = new Array();
			for each(var sn:SliceNode in this)
			{
				if(!completedCms.contains(sn.node.manager))
				{
					var cmNodes:SliceNodeCollection = new SliceNodeCollection();
					for each(var sncm:SliceNode in this)
					{
						if(sncm.node.manager == sn.node.manager)
							cmNodes.addItem(sncm);
					}
					if(cmNodes.length > 0)
						completed.push(cmNodes);
				}
			}
			return completed;
		}
	}
}