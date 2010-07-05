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
	
	// Node allocated into a sliver/slice which has a physical node underneath
	public class VirtualNode
	{
		public function VirtualNode(owner:Sliver)
		{
			sliver = owner;
		}
		
		[Bindable]
		public var physicalNode:PhysicalNode;
		
		public var virtualId:String;
		public var virtualizationType:String;
		public var sliverUrn:String;
		
		// Component
		[Bindable]
		public var name:String;
		public var manager:ComponentManager;
		public var urn:String;
		public var isVirtual:Boolean;
		public var isShared:Boolean;
		public var isBgpMux:Boolean;
		public var upstreamAs:String;
		public var superNode:PhysicalNode;
		
		[Bindable]
		public var interfaces:ArrayCollection = new ArrayCollection();
		public var links:ArrayCollection = new ArrayCollection();
		
		public var rspec:XML;
		public var sliver:Sliver;
		public var hostname:String;
		
		public function setToPhysicalNode(node:PhysicalNode):void
		{
			physicalNode = node;
			name = node.name;
			manager = node.manager;
			urn = node.urn;
			superNode = node.subNodeOf;
			isVirtual = false;
			isShared = !node.exclusive;
			// deal with rest
		}
		
		public function setToVirtualNode(virtualName:String,
										 virtualManager:ComponentManager,
										 virtualUrn:String,
										 newIsShared:Boolean):void
		{
			name = virtualName;
			manager = virtualManager;
			urn = virtualUrn;
			superNode = null;
			isVirtual = true;
			isShared = newIsShared;
			// deal with rest
		}
		
		public function allocateInterface():VirtualInterface
		{
			if(isVirtual)
			{
				var newVirtualInterface:VirtualInterface = new VirtualInterface(this);
				newVirtualInterface.id = "virt-" + String(interfaces.length);
				newVirtualInterface.role = PhysicalNodeInterface.EXPERIMENTAL;
				newVirtualInterface.isVirtual = true;
				newVirtualInterface.bandwidth = 100000;
				return newVirtualInterface;
			} else {
				for each (var candidate:PhysicalNodeInterface in physicalNode.interfaces)
				{
					if (candidate.role == PhysicalNodeInterface.EXPERIMENTAL)
					{
						var success:Boolean = true;
						for each (var check:VirtualInterface in interfaces)
						{
							if(!check.isVirtual && check.physicalNodeInterface == candidate)
								success = false;
								break;
						}
						if(success)
						{
							var newPhysicalInterface:VirtualInterface = new VirtualInterface(this);
							newPhysicalInterface.isVirtual = false;
							newPhysicalInterface.id = candidate.id;
							newPhysicalInterface.role = candidate.role;
							newPhysicalInterface.physicalNodeInterface = candidate;
							return newPhysicalInterface;
						}
					}
				}
			}
			return null;
		}
		
		// Gets all connected physical nodes
		public function GetNodes():ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var nodeInterface:VirtualInterface in interfaces) {
				for each(var nodeLink:VirtualLink in nodeInterface.virtualLinks) {
					for each(var nodeLinkInterface:VirtualInterface in nodeLink.interfaces)
					{
						if(nodeLinkInterface != nodeInterface && !ac.contains(nodeLinkInterface.virtualNode.physicalNode))
							 ac.addItem(nodeLinkInterface.virtualNode.physicalNode);
					}
				}
			}
			return ac;
		}
		
		// Gets all virtual links connected to this node
		public function GetLinks(n:PhysicalNode):ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			
			for each(var i:VirtualInterface in interfaces) {
				for each(var l:VirtualLink in i.virtualLinks) {
					for each(var nl:VirtualInterface in l.interfaces) {
						if(nl != i && nl.virtualNode.physicalNode == n && !ac.contains(l)) {
							ac.addItem(l);
						}
					}
				}
			}
			return ac;
		}
	}
}