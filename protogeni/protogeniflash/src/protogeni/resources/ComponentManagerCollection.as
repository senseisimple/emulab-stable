package protogeni.resources
{
	import mx.collections.ArrayCollection;
	
	import protogeni.ProtogeniHandler;
	
	public class ComponentManagerCollection extends ArrayCollection
	{
		private var owner:ProtogeniHandler;
		
		public function ComponentManagerCollection(source:Array=null)
		{
			super(source);
		}
		
		public function add(cm:ComponentManager):void
		{
			this.addItem(cm);
			Main.protogeniHandler.dispatchComponentManagersChanged();
		}
		
		public function clear():void
		{
			for each(var cm : ComponentManager in this)
			{
				cm.clear();
			}
		}
		
		public function getByUrn(urn:String):ComponentManager
		{
			for each(var cm : ComponentManager in this)
			{
				if(cm.Urn == urn)
					return cm;
			}
			return null;
		}
		
		public function getByHrn(hrn:String):ComponentManager
		{
			for each(var cm : ComponentManager in this)
			{
				if(cm.Hrn == hrn)
					return cm;
			}
			return null;
		}
	}
}