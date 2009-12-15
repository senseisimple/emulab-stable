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
 
 package pgmap
{
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	
	// Sliver from a slice containing all resources from the CM
	public class Sliver
	{
		public static var READY : String = "ready";
	    public static var NOTREADY : String = "notready";
	    public static var FAILED : String = "failed";
	    
		public var credential : Object = null;
		public var componentManager : ComponentManager = null;
		public var rspec : XML = null;
		
		public var status : String;
		public var sliceStatus : String;
		
		public var nodes:ArrayCollection = new ArrayCollection();
		public var links:ArrayCollection = new ArrayCollection();
		
		public var slice : Slice;
		
		public function Sliver(owner : Slice)
		{
			slice = owner;
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
      			virtualNode.virtualId = nodeXml.virtual_id;
      			virtualNode.virtualizationType = nodeXml.virtualization_type;
      			virtualNode.physicalNode = componentManager.Nodes.GetByUrn(nodeXml.component_urn);
      			for each(var ix:XML in nodeXml.children()) {
	        		if(ix.localName() == "interface") {
	        			var virtualInterface:VirtualInterface = new VirtualInterface(virtualNode);
	      				virtualInterface.id = ix.virtual_id;
	      				virtualNode.interfaces.addItem(virtualInterface);
      				}
	        	}
      			
      			virtualNode.rspec = nodeXml.copy();
      			nodes.addItem(virtualNode);
      			nodesById[virtualNode.virtualId] = virtualNode;
      			virtualNode.physicalNode.virtualNodes.addItem(virtualNode);
      		}
      		
      		for each(var linkXml:XML in linksXml)
      		{
      			var virtualLink:VirtualLink = new VirtualLink(this);
      			virtualLink.virtualId = linkXml.virtual_id;
      			virtualLink.bandwidth = linkXml.bandwidth;
      			virtualLink.type = linkXml.link_type;
      			
      			for each(var viXml:XML in linkXml.interface_ref)
      			{
      				var vid:String = viXml.virtual_interface_id;
      				var nid:String = viXml.virtual_node_id;
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
      			
      			virtualLink.rspec = linkXml.copy();
      			links.addItem(virtualLink);
      		}
		}
	}
}