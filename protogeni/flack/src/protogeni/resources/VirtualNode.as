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
		public static const STATUS_CHANGING:String = "changing";
		public static const STATUS_READY:String = "ready";
		public static const STATUS_NOTREADY:String = "notready";
		public static const STATUS_FAILED:String = "failed";
		public static const STATUS_UNKOWN:String = "unknown";
		
		[Bindable]
		public var physicalNode:PhysicalNode;
		public var _exclusive:Boolean;
		[Bindable]
		public var clientId:String;
		[Bindable]
		public var sliverId:String;
		public var manager:GeniManager;
		public var superNode:VirtualNode;
		public var subNodes:Array = new Array();
		
		[Bindable]
		public var interfaces:VirtualInterfaceCollection;
		public var links:ArrayCollection = new ArrayCollection();
		
		[Bindable]
		public var slivers:SliverCollection;
		
		[Bindable]
		public var error:String;
		[Bindable]
		public var state:String = "N/A";
		[Bindable]
		public var status:String = "N/A";
		
		public var flackX:int = -1;
		public var flackY:int = -1;
		
		public var rspec:XML;
		
		public var sliverType:String;
		
		public var installServices:Vector.<InstallService> = new Vector.<InstallService>();
		public var executeServices:Vector.<ExecuteService> = new Vector.<ExecuteService>();
		public var loginServices:Vector.<LoginService> = new Vector.<LoginService>();
		
		// Depreciated
		public var virtualizationType:String = "emulab-vnode";
		public var virtualizationSubtype:String = "emulab-openvz";
		public var uuid:String;
		[Bindable]
		public var diskImage:String = "";
		
		public function VirtualNode(owner:Sliver)
		{
			slivers = new SliverCollection();
			if(owner != null)
			{
				slivers.addItem(owner);
				manager = owner.manager;
			}
				
			interfaces = new VirtualInterfaceCollection();
			// depreciated for v2
			var controlInterface:VirtualInterface = new VirtualInterface(this);
			controlInterface.id = "control";
			interfaces.Add(controlInterface);
		}
		
		public function get Exclusive():Boolean {
			return _exclusive;
		}
		
		public function set Exclusive(value:Boolean):void {
			_exclusive = value;
			// TODO: support more
			if(_exclusive)
				sliverType = "raw-pc";
			else
				sliverType = "emulab-openvz";
		}
		
		public function setToPhysicalNode(node:PhysicalNode):void
		{
			physicalNode = node;
			clientId = node.name;
			manager = node.manager;
			sliverId = node.id;
			Exclusive = node.exclusive;
		}
		
		public function setDiskImage(img:String):void
		{
			if(img != null && img.length > 0)
			{
				if(img.length > 3 && img.substr(0, 3) == "urn")
					this.diskImage = img;
				else
					this.diskImage = DiskImage.getDiskImageLong(img, this.manager);
			} else
				this.diskImage = "";
			
		}
		
		public function allocateInterface():VirtualInterface
		{
			if(!IsBound())
			{
				var newVirtualInterface:VirtualInterface = new VirtualInterface(this);
				newVirtualInterface.id = this.clientId + ":if" + this.interfaces.collection.length;
				//newVirtualInterface.role = PhysicalNodeInterface.ROLE_EXPERIMENTAL;
				newVirtualInterface.bandwidth = 100000;
				//newVirtualInterface.isVirtual = true;
				return newVirtualInterface;
			} else {
				for each (var candidate:PhysicalNodeInterface in physicalNode.interfaces.collection)
				{
					if (candidate.role == PhysicalNodeInterface.ROLE_EXPERIMENTAL)
					{
						var success:Boolean = true;
						for each (var check:VirtualInterface in interfaces)
						{
							if(check.IsBound() && check.physicalNodeInterface == candidate)
								success = false;
								break;
						}
						if(success)
						{
							var newPhysicalInterface:VirtualInterface = new VirtualInterface(this);
							newPhysicalInterface.physicalNodeInterface = candidate;
							newPhysicalInterface.id = this.clientId + ":if" + this.interfaces.collection.length;
							return newPhysicalInterface;
						}
					}
				}
			}
			return null;
		}
		
		public function IsBound():Boolean {
			return physicalNode != null;
		}
		
		// Gets all connected physical nodes
		public function GetPhysicalNodes():ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var nodeInterface:VirtualInterface in interfaces.collection) {
				for each(var nodeLink:VirtualLink in nodeInterface.virtualLinks) {
					for each(var nodeLinkInterface:VirtualInterface in nodeLink.interfaces)
					{
						if(nodeLinkInterface != nodeInterface && !ac.contains(nodeLinkInterface.owner.physicalNode))
							 ac.addItem(nodeLinkInterface.owner.physicalNode);
					}
				}
			}
			return ac;
		}
		
		public function GetAllNodes():ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var virtualLink:VirtualLink in links) {
				if(virtualLink.firstNode != this && !ac.contains(virtualLink.firstNode))
					ac.addItem(virtualLink.firstNode);
				if(virtualLink.secondNode != this && !ac.contains(virtualLink.secondNode))
					ac.addItem(virtualLink.secondNode);
			}
			return ac;
		}
		
		// Gets all virtual links connected to this node
		public function GetLinksForPhysical(n:PhysicalNode):ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			
			for each(var i:VirtualInterface in interfaces) {
				for each(var l:VirtualLink in i.virtualLinks) {
					for each(var nl:VirtualInterface in l.interfaces) {
						if(nl != i && nl.owner.physicalNode == n && !ac.contains(l)) {
							ac.addItem(l);
						}
					}
				}
			}
			return ac;
		}
		
		public function GetLinks(n:VirtualNode):ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();

			for each(var i:VirtualInterface in interfaces.collection) {
				for each(var l:VirtualLink in i.virtualLinks) {
					for each(var nl:VirtualInterface in l.interfaces) {
						if(nl != i && nl.owner == n && !ac.contains(l)) {
							ac.addItem(l);
						}
					}
				}
			}
			return ac;
		}
	}
}