package protogeni.resources
{
	public class PipeCollection
	{
		public var collection:Vector.<Pipe>;
		public function PipeCollection()
		{
			collection = new Vector.<Pipe>();
		}
		
		public function add(p:Pipe):void {
			this.collection.push(p);
		}
		
		public function remove(p:Pipe):void
		{
			var idx:int = collection.indexOf(p);
			if(idx > -1)
				this.collection.splice(idx, 1);
		}
		
		public function get length():int{
			return this.collection.length;
		}
		
		public function getFor(source:VirtualInterface, destination:VirtualInterface):Pipe {
			for each(var pipe:Pipe in this.collection) {
				if(pipe.source == source && pipe.destination == destination)
					return pipe;
			}
			return null;
		}
		
		public function getForSource(source:VirtualInterface):PipeCollection {
			var result:PipeCollection = new PipeCollection();
			for each(var pipe:Pipe in this.collection) {
				if(pipe.source == source)
					result.add(pipe);
			}
			return result;
		}
		
		public function getForDestination(destination:VirtualInterface):PipeCollection {
			var result:PipeCollection = new PipeCollection();
			for each(var pipe:Pipe in this.collection) {
				if(pipe.destination == destination)
					result.add(pipe);
			}
			return result;
		}
		
		public function getForAny(test:VirtualInterface):PipeCollection {
			var result:PipeCollection = new PipeCollection();
			for each(var pipe:Pipe in this.collection) {
				if(pipe.destination == test || pipe.source == test)
					result.add(pipe);
			}
			return result;
		}
	}
}