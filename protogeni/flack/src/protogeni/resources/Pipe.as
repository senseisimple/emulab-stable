package protogeni.resources
{
	/**
	 * Pipe used within a delay node to edit network properties from one interface to another
	 * 
	 * @author mstrum
	 * 
	 */
	public class Pipe
	{
		public var source:VirtualInterface;
		public var destination:VirtualInterface;
		
		public var capacity:Number;
		public var latency:Number;
		public var packetLoss:Number;
		
		public function get sourceName():String {
			return source.nodesOtherThan().collection[0].clientId;
		}
		
		public function get destName():String {
			return destination.nodesOtherThan().collection[0].clientId;
		}
		
		public function Pipe(newSource:VirtualInterface,
							 newDestination:VirtualInterface,
							 newCapacity:Number = NaN,
							 newLatency:Number = NaN,
							 newPacketLoss:Number = NaN)
		{
			source = newSource;
			destination = newDestination;
			capacity = newCapacity;
			latency = newLatency;
			packetLoss = newPacketLoss;
		}
	}
}