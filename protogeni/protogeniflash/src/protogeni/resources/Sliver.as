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
	
	// Sliver from a slice containing all resources from the CM
	public class Sliver
	{
		public static var READY : String = "ready";
	    public static var NOTREADY : String = "notready";
	    public static var FAILED : String = "failed";
		
		public var created:Boolean = false;
	    
		public var credential : Object = null;
		public var componentManager : ComponentManager = null;
		public var rspec : XML = null;
		public var urn : String = null;
		
		public var state : String;
		public var status : String;
		
		public var nodes:ArrayCollection = new ArrayCollection();
		public var links:ArrayCollection = new ArrayCollection();
		
		public var slice : Slice;
		
		public function Sliver(owner : Slice, manager:ComponentManager = null)
		{
			slice = owner;
			componentManager = manager;
		}
		
		public function getRequestRspec():XML
		{
			var requestRspec:XML = new XML("<?xml version=\"1.0\" encoding=\"UTF-8\"?> "
				+ "<rspec "
				+ "xmlns=\"http://www.protogeni.net/resources/rspec/0.2\" "
				+ "type=\"request\" />");
			
			for each(var vn:VirtualNode in nodes)
				requestRspec.appendChild(vn.getXml());
			
			for each(var vl:VirtualLink in links)
				requestRspec.appendChild(vl.getXml());
			
			return requestRspec;
		}
		
		public function parseRspec():void
		{
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
      			//virtualNode.sliverUrn = nodeXml.@sliver_urn;
      			virtualNode.virtualizationType = nodeXml.@virtualization_type;
				if(nodeXml.@virtualization_subtype != null)
					virtualNode.virtualizationSubtype = nodeXml.@virtualization_subtype;
      			virtualNode.physicalNode = componentManager.Nodes.GetByUrn(nodeXml.@component_urn);
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
      				for each(var vi:VirtualInterface in interfacedNode.interfaces)
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
			newSliver.credential = this.credential;
			newSliver.componentManager = this.componentManager;
			newSliver.rspec = this.rspec;
			newSliver.urn = this.urn;
			newSliver.state = this.state;
			newSliver.status = this.status;
			newSliver.created = this.created;
			
			var oldNodeToCloneNode:Dictionary = new Dictionary();
			var oldInterfaceToCloneInterface:Dictionary = new Dictionary();
			for each(var node:VirtualNode in this.nodes)
			{
				var newNode:VirtualNode = new VirtualNode(newSliver);
				newNode.id = node.id;
				newNode.virtualizationType = node.virtualizationType;
				newNode.virtualizationSubtype = node.virtualizationSubtype;
				newNode.physicalNode = node.physicalNode;
				newNode.manager = node.manager;
				newNode.urn = node.urn;
				newNode.isShared = node.isShared;
				newNode.isVirtual = node.isVirtual;
				newNode.superNode = node.superNode;
				newNode.diskImage = node.diskImage;
				newNode.tarfiles = node.tarfiles;
				newNode.startupCommand = node.startupCommand;
				newNode.hostname = node.startupCommand;
				newNode.rspec = node.rspec;
				for each(var vi:VirtualInterface in node.interfaces)
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

			
			for each(var link:VirtualLink in this.links)
			{
				var newLink:VirtualLink = new VirtualLink(newSliver);
				newLink.id = link.id;
				newLink.type = link.type;
				newLink.bandwidth = link.bandwidth;
				newLink.rspec = link.rspec;
				newLink._isTunnel = link._isTunnel;
				
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