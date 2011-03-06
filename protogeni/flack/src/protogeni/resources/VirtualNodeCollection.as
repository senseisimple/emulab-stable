package protogeni.resources
{
	public final class VirtualNodeCollection
	{
		public var collection:Vector.<VirtualNode>;
		public function VirtualNodeCollection()
		{
			this.collection = new Vector.<VirtualNode>();
			/*if(source != null) {
				for each(var node:VirtualNode in source)
					collection.push(node);
			}*/
		}
		
		public function add(n:VirtualNode):void {
			this.collection.push(n);
		}
		
		public function remove(n:VirtualNode):void
		{
			var idx:int = collection.indexOf(n);
			if(idx > -1)
				this.collection.splice(idx, 1);
		}
		
		public function contains(n:VirtualNode):Boolean
		{
			return this.collection.indexOf(n) > -1;
		}
		
		public function get length():int{
			return this.collection.length;
		}
		
		public function getById(id:String):VirtualNode
		{
			for each(var node:VirtualNode in this.collection)
			{
				if(node.clientId == id)
					return node;
			}
			return null;
		}
		
		public function getByInterfaceId(id:String):VirtualInterface
		{
			for each(var node:VirtualNode in this.collection)
			{
				var vi:VirtualInterface = node.interfaces.GetByID(id);
				if(vi != null)
					return vi;
			}
			return null;
		}
		
		public function getByUrn(urn:String):VirtualNode
		{
			for each(var node:VirtualNode in this.collection)
			{
				if(node.sliverId == urn)
					return node;
			}
			return null;
		}
	}
}