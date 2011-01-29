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
			slivers = new SliverCollection();
			if(owner != null)
			{
				slivers.addItem(owner);
				manager = owner.manager;
			}
				
			interfaces = new VirtualInterfaceCollection();
			var controlInterface:VirtualInterface = new VirtualInterface(this);
			controlInterface.id = "control";
			controlInterface.role = PhysicalNodeInterface.CONTROL;
			controlInterface.isVirtual = true;
			interfaces.Add(controlInterface);
		}
		
		[Bindable]
		public var physicalNode:PhysicalNode;
		
		[Bindable]
		public var id:String;
		
		[Bindable]
		public var virtualizationType:String = "emulab-vnode";
		
		[Bindable]
		public var virtualizationSubtype:String = "emulab-openvz";
		
		// Component
		public var manager:GeniManager;
		
		[Bindable]
		public var urn:String;
		public var uuid:String;
		public var isVirtual:Boolean;
		public var isShared:Boolean;
		public var superNode:VirtualNode;
		public var subNodes:Array = new Array();
		
		[Bindable]
		public var diskImage:String = "";
		public var tarfiles:String = "";
		public var startupCommand:String = "";
		
		[Bindable]
		public var interfaces:VirtualInterfaceCollection;
		public var links:ArrayCollection = new ArrayCollection();
		
		public var rspec:XML;
		
		[Bindable]
		public var slivers:SliverCollection;
		[Bindable]
		public var hostname:String;
		public var sshdport:String;
		
		[Bindable]
		public var error:String;
		[Bindable]
		public var state:String;
		[Bindable]
		public var status:String;
		
		public static var STATUS_CHANGING:String = "changing";
		public static var STATUS_READY:String = "ready";
		public static var STATUS_NOTREADY:String = "notready";
		public static var STATUS_FAILED:String = "failed";
		public static var STATUS_UNKOWN:String = "unknown";
		
		public function setToPhysicalNode(node:PhysicalNode):void
		{
			physicalNode = node;
			id = node.name;
			manager = node.manager;
			urn = node.urn;
			isVirtual = false;
			isShared = !node.exclusive;
			// deal with rest
		}
		
		public function setToVirtualNode(virtualName:String,
										 virtualManager:ComponentManager,
										 virtualUrn:String,
										 newIsShared:Boolean):void
		{
			id = virtualName;
			manager = virtualManager;
			urn = virtualUrn;
			superNode = null;
			isVirtual = true;
			isShared = newIsShared;
			// deal with rest
		}
		
		public function getDiskImageShort():String
		{
			if(diskImage.indexOf("urn:publicid:IDN+emulab.net+image+emulab-ops//") > -1)
				return diskImage.replace("urn:publicid:IDN+emulab.net+image+emulab-ops//", "");
			else
				return diskImage;
		}
		
		public function setDiskImageFromOSID(osid:String):void
		{
			this.diskImage = "urn:publicid:IDN+emulab.net+image+emulab-ops//" + osid;
		}
		
		public function setDiskImage(img:String):void
		{
			if(img != null && img.length > 0)
			{
				if(img.length > 3 && img.substr(0, 3) == "urn")
					this.diskImage = img;
				else
					this.setDiskImageFromOSID(img);
			} else
				this.diskImage = "";
			
		}
		
		public function allocateInterface():VirtualInterface
		{
			if(isVirtual)
			{
				var newVirtualInterface:VirtualInterface = new VirtualInterface(this);
				newVirtualInterface.id = "virt-int-" + this.interfaces.collection.length;
				newVirtualInterface.role = PhysicalNodeInterface.EXPERIMENTAL;
				newVirtualInterface.bandwidth = 100000;
				newVirtualInterface.isVirtual = true;
				return newVirtualInterface;
			} else {
				for each (var candidate:PhysicalNodeInterface in physicalNode.interfaces.collection)
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
							newPhysicalInterface.id = "phy-int-" + this.interfaces.collection.length;
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
		public function GetPhysicalNodes():ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var nodeInterface:VirtualInterface in interfaces.collection) {
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
						if(nl != i && nl.virtualNode.physicalNode == n && !ac.contains(l)) {
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
						if(nl != i && nl.virtualNode == n && !ac.contains(l)) {
							ac.addItem(l);
						}
					}
				}
			}
			return ac;
		}
		
		public function getXml():XML
		{
			var result : XML = <node />;
			if (!isVirtual)
				result.@component_uuid = physicalNode.urn;
			result.@component_manager_uuid = manager.Urn;
			result.@virtual_id = id;
			result.@virtualization_type = virtualizationType;
			if (isShared)
			{
				result.@virtualization_subtype = virtualizationSubtype;
				result.@exclusive = 0;
			}
			else
				result.@exclusive = 1;

			// Currently only pcs
			var nodeType:String = "pc";
			if (isShared)
				nodeType = "pcvm";
			var nodeTypeXml:XML = <node_type />;
			nodeTypeXml.@type_name = nodeType;
			nodeTypeXml.@type_slots = 1;
			result.appendChild(nodeTypeXml);
			
			if(startupCommand.length > 0)
				result.@startup_command = startupCommand;
			
			if(tarfiles.length > 0)
				result.@tarfiles = tarfiles;
			
			if(diskImage.length > 0)
			{
				var diskImageXml:XML = <disk_image />;
				diskImageXml.@name = diskImage;
				result.appendChild(diskImageXml);
			}
			
			if (superNode != null)
				result.appendChild(XML("<subnode_of>" + superNode.urn + "</subnode_of>"));
			
			for each (var current:VirtualInterface in interfaces.collection)
			{
				var interfaceXml:XML = <interface />;
				interfaceXml.@virtual_id = current.id;
				result.appendChild(interfaceXml);
			}

			return result;
		}
	}
}