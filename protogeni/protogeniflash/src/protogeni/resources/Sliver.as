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
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	
	import protogeni.Util;
	import protogeni.communication.CommunicationUtil;
	
	// Sliver from a slice containing all resources from the CM
	public class Sliver
	{
		public static var STATE_READY : String = "ready";
	    public static var STATE_NOTREADY : String = "notready";
	    public static var STATE_FAILED : String = "failed";
		
		public static var STATUS_CHANGING:String = "changing";
		public static var STATUS_READY:String = "ready";
		public static var STATUS_NOTREADY:String = "notready";
		public static var STATUS_FAILED:String = "changing";
		public static var STATUS_UNKOWN:String = "ready";
		public static var STATUS_MIXED:String = "notready";
		
		public var created:Boolean = false;
	    
		public var credential : Object = null;
		public var componentManager : ComponentManager = null;
		public var rspec : XML = null;
		public var urn : String = null;
		
		public var ticket:XML;
		public var manifest:XML;
		
		public var state : String;
		public var status : String;
		
		public var nodes:VirtualNodeCollection = new VirtualNodeCollection();
		public var links:VirtualLinkCollection = new VirtualLinkCollection();
		
		public var slice : Slice;
		
		public var validUntil:Date;
		
		public function Sliver(owner : Slice, manager:ComponentManager = null)
		{
			slice = owner;
			componentManager = manager;
		}
		
		public function getVirtualNodeFor(pn:PhysicalNode):VirtualNode
		{
			for each(var vn:VirtualNode in this.nodes)
			{
				if(vn.physicalNode == pn)
					return vn;
			}
			
			return null;
		}
		
		public function getRequestRspec():XML
		{
			var requestRspec:XML = new XML("<?xml version=\"1.0\" encoding=\"UTF-8\"?> "
				+ "<rspec "
				+ "xmlns=\""+CommunicationUtil.rspec2Namespace+"\" "
				+ "type=\"request\" />");
			
			for each(var vn:VirtualNode in nodes)
				requestRspec.appendChild(vn.getXml());
			
			for each(var vl:VirtualLink in links)
				requestRspec.appendChild(vl.getXml());
			
			return requestRspec;
		}
		
		public function parseRspec():void
		{
			nodes = new VirtualNodeCollection();
			links = new VirtualLinkCollection();
			
			var nodesById:Dictionary = new Dictionary();
			
			var linksXml : ArrayCollection = new ArrayCollection();
			var nodesXml : ArrayCollection = new ArrayCollection();
	        for each(var component:XML in rspec.children())
	        {
	        	if(component.localName() == "link")
	        		linksXml.addItem(component);
	        	else if(component.localName() == "node")
	        		nodesXml.addItem(component);
	        }
      		
      		for each(var nodeXml:XML in nodesXml)
      		{
      			var virtualNode:VirtualNode = new VirtualNode(this);
      			virtualNode.id = nodeXml.@virtual_id;
				virtualNode.manager = Main.protogeniHandler.ComponentManagers.getByUrn(nodeXml.@component_manager_urn);
				if(nodeXml.@sliver_urn != null)
					virtualNode.urn = nodeXml.@sliver_urn;
				if(nodeXml.@sliver_uuid != null)
					virtualNode.uuid = nodeXml.@sliver_uuid;
				if(nodeXml.@sshdport != null)
					virtualNode.sshdport = nodeXml.@sshdport;
				if(nodeXml.@hostname != null)
					virtualNode.hostname = nodeXml.@hostname;
      			virtualNode.virtualizationType = nodeXml.@virtualization_type;
				if(nodeXml.@virtualization_subtype != null)
					virtualNode.virtualizationSubtype = nodeXml.@virtualization_subtype;
				virtualNode.setToPhysicalNode(componentManager.Nodes.GetByUrn(nodeXml.@component_urn));
      			for each(var ix:XML in nodeXml.children()) {
	        		if(ix.localName() == "interface") {
	        			var virtualInterface:VirtualInterface = new VirtualInterface(virtualNode);
	      				virtualInterface.id = ix.@virtual_id;
	      				virtualNode.interfaces.Add(virtualInterface);
      				}
	        	}
      			
      			virtualNode.rspec = nodeXml.copy();
      			nodes.addItem(virtualNode);
      			nodesById[virtualNode.id] = virtualNode;
      			virtualNode.physicalNode.virtualNodes.addItem(virtualNode);
      		}
			
			for each(var vn:VirtualNode in nodes)
			{
				if(vn.physicalNode.subNodeOf != null)
				{
					vn.superNode = nodes.getById(vn.physicalNode.subNodeOf.name);
					nodes.getById(vn.physicalNode.subNodeOf.name).subNodes.push(vn);
				}
			}
      		
      		for each(var linkXml:XML in linksXml)
      		{
      			var virtualLink:VirtualLink = new VirtualLink(this);
      			virtualLink.id = linkXml.@virtual_id;
      			//virtualLink.sliverUrn = linkXml.@sliver_urn;
      			virtualLink.type = linkXml.@link_type;
      			
      			for each(var viXml:XML in linkXml.children()) {
      				if(viXml.localName() == "bandwidth")
      					virtualLink.bandwidth = viXml.toString();
	        		if(viXml.localName() == "interface_ref") {
	        			var vid:String = viXml.@virtual_interface_id;
	      				var nid:String = viXml.@virtual_node_id;
	      				var interfacedNode:VirtualNode = nodesById[nid];
	      				for each(var vi:VirtualInterface in interfacedNode.interfaces.collection)
	      				{
	      					if(vi.id == vid)
	      					{
	      						virtualLink.interfaces.addItem(vi);
	      						vi.virtualLinks.addItem(virtualLink);
	      						break;
	      					}
	      				}
      				}
	        	}
      			
      			virtualLink.rspec = linkXml.copy();
				virtualLink.firstNode = (virtualLink.interfaces[0] as VirtualInterface).virtualNode;
				virtualLink.secondNode = (virtualLink.interfaces[1] as VirtualInterface).virtualNode;
      			links.addItem(virtualLink);
      		}
		}
		
		public function removeOutsideReferences():void
		{
			for each(var node:VirtualNode in this.nodes)
			{
				if(node.physicalNode.virtualNodes.getItemIndex(node) > -1)
					node.physicalNode.virtualNodes.removeItemAt(node.physicalNode.virtualNodes.getItemIndex(node));
			}
		}
		
		public function clone(addOutsideReferences:Boolean = true):Sliver
		{
			var newSliver:Sliver = new Sliver(slice);
			newSliver.created = this.created;
			newSliver.credential = this.credential;
			newSliver.componentManager = this.componentManager;
			newSliver.rspec = this.rspec;
			newSliver.urn = this.urn;
			newSliver.ticket = this.ticket;
			newSliver.manifest = this.manifest;
			newSliver.state = this.state;
			newSliver.status = this.status;
			newSliver.validUntil = this.validUntil;
			
			// Nodes
			var oldNodeToCloneNode:Dictionary = new Dictionary();
			var oldInterfaceToCloneInterface:Dictionary = new Dictionary();
			var retrace:Array = new Array();
			for each(var node:VirtualNode in this.nodes)
			{
				var newNode:VirtualNode = new VirtualNode(newSliver);
				newNode.id = node.id;
				newNode.virtualizationType = node.virtualizationType;
				newNode.virtualizationSubtype = node.virtualizationSubtype;
				newNode.physicalNode = node.physicalNode;
				newNode.manager = node.manager;
				newNode.urn = node.urn;
				newNode.uuid = node.uuid;
				newNode.isShared = node.isShared;
				newNode.isVirtual = node.isVirtual;
				// supernode? add later ...
				// subnodes? add later ...
				retrace.push({clone:newNode, old:node});
				newNode.diskImage = node.diskImage;
				newNode.tarfiles = node.tarfiles;
				newNode.startupCommand = node.startupCommand;
				newNode.hostname = node.startupCommand;
				newNode.rspec = node.rspec;
				newNode.sshdport = node.sshdport;
				newNode.error = node.error;
				newNode.state = node.state;
				newNode.status = node.status;
				// slivers? add later ...
				for each(var vi:VirtualInterface in node.interfaces.collection)
				{
					var virtualInterface:VirtualInterface = new VirtualInterface(newNode);
					virtualInterface.id = vi.id;
					virtualInterface.role = vi.role;
					virtualInterface.isVirtual = vi.isVirtual;
					virtualInterface.physicalNodeInterface = vi.physicalNodeInterface;
					virtualInterface.bandwidth = vi.bandwidth;
					virtualInterface.ip = vi.ip;
					newNode.interfaces.Add(virtualInterface);
					oldInterfaceToCloneInterface[vi] = virtualInterface;
					// links? add later ...
				}
				
				if(addOutsideReferences)
					newNode.physicalNode.virtualNodes.addItem(newNode);
				newSliver.nodes.addItem(newNode);
				
				oldNodeToCloneNode[node] = newNode;
			}

			// supernode and subnodes need to be added after to ensure they were created
			for each(var check:Object in retrace)
			{
				var cloneNode:VirtualNode = check.clone;
				var oldNode:VirtualNode = check.old;
				if(oldNode.superNode != null)
					cloneNode.superNode = newSliver.nodes.getById(oldNode.id);
				if(oldNode.subNodes != null && oldNode.subNodes.length > 0)
				{
					for each(var subNode:VirtualNode in oldNode.subNodes)
						cloneNode.subNodes.push(newSliver.nodes.getById(subNode.id));
				}
			}
			
			// Links
			for each(var link:VirtualLink in this.links)
			{
				var newLink:VirtualLink = new VirtualLink(newSliver);
				newLink.id = link.id;
				newLink.type = link.type;
				newLink.bandwidth = link.bandwidth;
				newLink.firstTunnelIp = link.firstTunnelIp;
				newLink.secondTunnelIp = link.secondTunnelIp;
				newLink._isTunnel = link._isTunnel;
				newLink.rspec = link.rspec;
				// Slivers?  add later ...
				
				newLink.firstNode = oldNodeToCloneNode[link.firstNode];
				newLink.secondNode = oldNodeToCloneNode[link.secondNode];
				
				for each(var i:VirtualInterface in link.interfaces)
				{
					var newInterface:VirtualInterface = oldInterfaceToCloneInterface[i];
					newLink.interfaces.addItem(newInterface);
					newInterface.virtualLinks.addItem(newLink);
					newInterface.virtualNode.links.addItem(newLink);
				}
				
				newSliver.links.addItem(newLink);
			}
			
			return newSliver;
		}
	}
}