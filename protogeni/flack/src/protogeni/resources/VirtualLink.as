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
		public static const TYPE_NORMAL:int = 0;
		public static const TYPE_TUNNEL:int = 1;
		public static const TYPE_ION:int = 2;
		public static const TYPE_GPENI:int = 3;
		public static function TypeToString(type:int):String {
			switch(type){
				case TYPE_NORMAL:
					return "LAN";
				case TYPE_TUNNEL:
					return "Tunnel";
				case TYPE_ION:
					return "ION";
				case TYPE_GPENI:
					return "GPENI";
			}
			return "Unknown";
		}
		
		[Bindable]
		public var clientId:String;
		[Bindable]
		public var sliverId:String;
		[Bindable]
		public var type:String;
		[Bindable]
		public var interfaces:ArrayCollection = new ArrayCollection();
		
		public var linkType:int = TYPE_NORMAL;
		public var slivers:Array;
		public var rspec:XML;
		
		// Depreciated
		public var bandwidth:Number;
		
		public function VirtualLink(owner:Sliver = null)
		{
			if(owner != null)
				slivers = new Array(owner);
			else
				slivers = new Array();
		}
		
		public function establish(first:VirtualNode, second:VirtualNode):Boolean
		{
			var firstInterface:VirtualInterface = first.allocateInterface();
			var secondInterface:VirtualInterface = second.allocateInterface();
			if(firstInterface == null || secondInterface == null)
				return false;
			clientId = slivers[0].slice.getUniqueVirtualLinkId(this);
			first.interfaces.Add(firstInterface);
			second.interfaces.Add(secondInterface);
			firstInterface.virtualLinks.addItem(this);
			secondInterface.virtualLinks.addItem(this);
			this.interfaces.addItem(firstInterface);
			this.interfaces.addItem(secondInterface);
			
			if(first.manager == second.manager)
				linkType = TYPE_NORMAL;
			 else
			 {
				 if(first.manager.supportsIon && second.manager.supportsIon && Main.useIon)
					 linkType = TYPE_ION;
				 else if (first.manager.supportsGpeni && second.manager.supportsGpeni && Main.useGpeni)
					 linkType = TYPE_GPENI;
				 else
					 this.setUpTunnels();
				 
				 // Make sure nodes are in both
				 if(!(second.slivers[0] as Sliver).nodes.contains(first))
					 (second.slivers[0] as Sliver).nodes.addItem(first);
				 if(!(first.slivers[0] as Sliver).nodes.contains(second))
					 (first.slivers[0] as Sliver).nodes.addItem(second);
				 
				 // Add relative slivers
				 if(slivers[0].manager != first.slivers[0].manager)
					 slivers.push(first.slivers[0]);
				 else if(slivers[0].manager != second.slivers[0].manager)
					 slivers.push(second.slivers[0]);
			 }
			
			for each(var s:Sliver in slivers)
				s.links.addItem(this);
			
			// depreciated
			bandwidth = Math.floor(Math.min(firstInterface.bandwidth, secondInterface.bandwidth));
			if (first.clientId.slice(0, 2) == "pg" || second.clientId.slice(0, 2) == "pg")
				bandwidth = 1000000;
			if(linkType == TYPE_GPENI || linkType == TYPE_ION)
				bandwidth = 100000;
			
			return true;
		}
		
		public function setUpTunnels():void
		{
			this.linkType = VirtualLink.TYPE_TUNNEL;
			this.type = "gre-tunnel";
			for each(var i:VirtualInterface in this.interfaces) {
				if(i.ip.length == 0) {
					i.ip = VirtualInterface.getNextTunnel();
					i.mask = "255.255.255.0";
					i.type = "ipv4";
				}
			}
		}
		
		public function remove():void
		{
			for each(var vi:VirtualInterface in this.interfaces)
			{
				if(vi.id != "control")
					vi.owner.interfaces.collection.removeItemAt(vi.owner.interfaces.collection.getItemIndex(vi));
				
				for(var i:int = 1; i < vi.owner.slivers.length; i++)
				{
					if((vi.owner.slivers.getItemAt(i) as Sliver).links.getForNode(vi.owner).length == 0)
						(vi.owner.slivers.getItemAt(i) as Sliver).nodes.remove(vi.owner);
				}
			}
			interfaces.removeAll();
			for each(var s:Sliver in slivers)
			{
				s.links.removeItemAt(s.links.getItemIndex(this));
			}
		}
		
		public function isConnectedTo(target:GeniManager) : Boolean
		{
			for each(var i:VirtualInterface in interfaces)
			{
				if(i.owner.manager == target)
					return true;
			}
			return false;
		}
		
		public function hasVirtualNode(node:VirtualNode):Boolean
		{
			for each(var i:VirtualInterface in interfaces)
			{
				if(i.owner == node)
					return true;
			}

			return false;
		}
		
		public function hasPhysicalNode(node:PhysicalNode):Boolean
		{
			for each(var i:VirtualInterface in interfaces)
			{
				if(i.owner.IsBound() && i.owner.physicalNode == node)
					return true;
			}
			return false;
		}
		
		public function supportsIon():Boolean {
			for each(var i:VirtualInterface in interfaces)
			{
				if(!i.owner.manager.supportsIon)
					return false;
			}
			return true;
		}
		
		public function supportsGpeni():Boolean {
			for each(var i:VirtualInterface in interfaces)
			{
				if(!i.owner.manager.supportsGpeni)
					return false;
			}
			return true;
		}
	}
}