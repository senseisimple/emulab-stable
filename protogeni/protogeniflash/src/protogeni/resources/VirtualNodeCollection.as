package protogeni.resources
{
	import mx.collections.ArrayCollection;
	
	public class VirtualNodeCollection extends ArrayCollection
	{
		public function VirtualNodeCollection(source:Array=null)
		{
			super(source);
		}
		
		public function getById(id:String):VirtualNode
		{
			for each(var node:VirtualNode in this)
			{
				if(node.id == id)
					return node;
			}
			return null;
		}
		
		public function getByInterfaceId(id:String):VirtualInterface
		{
			for each(var node:VirtualNode in this)
			{
				var vi:VirtualInterface = node.interfaces.GetByID(id);
				if(vi != null)
					return vi;
			}
			return null;
		}
		
		public function getByUrn(urn:String):VirtualNode
		{
			for each(var node:VirtualNode in this)
			{
				if(node.urn == urn)
					return node;
			}
			return null;
		}
		
		public function remove(n:VirtualNode):void
		{
			if(this.getItemIndex(n) > -1)
				this.removeItemAt(this.getItemIndex(n));
		}
	}
}