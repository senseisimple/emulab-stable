/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2009 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
 
 package protogeni.resources
{
	import mx.collections.ArrayCollection;
	
	// Link as part of a sliver/slice connecting virtual nodes
	public class VirtualLink
	{
		public function VirtualLink(owner:Sliver)
		{
			sliver = owner;
		}
		
		[Bindable]
		public var virtualId:String;
		
		[Bindable]
		public var sliverUrn:String;
		
		public var type:String;
		
		[Bindable]
		public var interfaces:ArrayCollection = new ArrayCollection();
		
		[Bindable]
		public var bandwidth:Number;
		
		public var tunnelIp:int = 0;
		public var _isTunnel:Boolean = false;
		public var tunnelLeft:VirtualNode;
		public var tunnelRight:VirtualNode;
		
		public var sliver:Sliver;

		public var rspec:XML;
		
		public var firstNode:VirtualNode;
		public var secondNode:VirtualNode;
		
		public static var tunnelNext:int = 1;
		
		public static function getNextTunnel() : int
		{
			var result:int = tunnelNext;
			tunnelNext += 2;
			return result;
		}
		
		public function establish(first:VirtualNode, second:VirtualNode):Boolean
		{
			if(first.manager != second.manager)
			{
				_isTunnel = true;
				tunnelLeft = first;
				tunnelRight = second;
				tunnelIp = getNextTunnel();
			} else {
				var firstInterface:VirtualInterface = first.allocateInterface();
				var secondInterface:VirtualInterface = second.allocateInterface();
				if(secondInterface == null || secondInterface == null)
					return false;
				first.interfaces.addItem(firstInterface);
				second.interfaces.addItem(secondInterface);
				this.interfaces.addItem(firstInterface);
				this.interfaces.addItem(secondInterface);
			}
			firstNode = first;
			secondNode = second;
			first.links.addItem(this);
			second.links.addItem(this);
			return true;
		}
		
		public function remove():void
		{
			for each(var vi:VirtualInterface in this.interfaces)
			{
				vi.virtualNode.interfaces.removeItemAt(vi.virtualNode.interfaces.getItemIndex(vi));
			}
			interfaces.removeAll();
			firstNode.links.removeItemAt(firstNode.links.getItemIndex(this));
			secondNode.links.removeItemAt(secondNode.links.getItemIndex(this));
		}
		
		public function isTunnel():Boolean
		{
			if(interfaces.length > 0)
			{
				var basicManager:ComponentManager = (interfaces[0] as VirtualInterface).virtualNode.manager;
				for each(var i:VirtualInterface in interfaces)
				{
					if(i.virtualNode.manager != basicManager)
						return true;
				}
			}
			return _isTunnel;
		}
		
		public function ipToString(ip : int) : String
		{
			var first : int = ((ip >> 8) & 0xff);
			var second : int = (ip & 0xff);
			return "192.168." + String(first) + "." + String(second);
		}
		
		public function hasTunnelTo(target : ComponentManager) : Boolean
		{
			return isTunnel() && (this.tunnelLeft.manager == target
				|| tunnelRight.manager == target);
		}
		
		public function isConnectedTo(target : ComponentManager) : Boolean
		{
			if(hasTunnelTo(target))
				return true;
			for each(var i:VirtualInterface in interfaces)
			{
				if(i.virtualNode.manager == target)
					return true;
			}
			return false;
		}
		
		public function hasVirtualNode(node:VirtualNode):Boolean
		{
			for each(var i:VirtualInterface in interfaces)
			{
				if(i.virtualNode == node)
					return true;
			}
			return false;
		}
		
		public function hasPhysicalNode(node:PhysicalNode):Boolean
		{
			for each(var i:VirtualInterface in interfaces)
			{
				if(!i.virtualNode.isVirtual && i.virtualNode.physicalNode == node)
					return true;
			}
			return false;
		}
	}
}