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
	
	import protogeni.display.DisplayUtil;
	
	// Slice that a user created in ProtoGENI
	public class Slice
	{
		// Status values
	    public static var CHANGING : String = "changing";
		public static var READY : String = "ready";
	    public static var NOTREADY : String = "notready";
	    public static var FAILED : String = "failed";
	    public static var UNKNOWN : String = "unknown";
	    public static var NA : String = "N/A";
	    
	    // State values
	    public static var STARTED : String = "started";
		public static var STOPPED : String = "stopped";
	    public static var MIXED : String = "mixed";
	    
		public var uuid : String = null;
		[Bindable]
		public var hrn : String = null;
		[Bindable]
		public var urn : String = null;
		public var creator : User = null;
		public var credential : String = "";
		public var slivers:SliverCollection = new SliverCollection();

		public var validUntil:Date;

		public function Slice()
		{
		}
		
		public function Status():String {
			if(hrn == null) return null;
			var status:String = NA;
			for each(var sliver:Sliver in slivers) {
				if(status == NA) status = sliver.status;
				else {
					if(sliver.status != status)
						return "mixed";
				}
			}
			return status;
		}
		
		public function hasAllocatedResources():Boolean
		{
			if(slivers == null)
				return false;
			
			for each(var existing:Sliver in this.slivers)
			{
				if(existing.created)
					return true;
			}
			return false;
		}
		
		public function hasAllAllocatedResources():Boolean
		{
			if(slivers == null)
				return false;
			
			for each(var existing:Sliver in this.slivers)
			{
				if(!existing.created)
					return false;
			}
			return true;
		}
		
		public function GetAllNodes():ArrayCollection
		{
			var nodes:ArrayCollection = new ArrayCollection();
			for each(var s:Sliver in slivers)
			{
				for each(var n:VirtualNode in s.nodes)
				{
					if(n.manager == s.componentManager)
						nodes.addItem(n);
				}
					
			}
			return nodes;
		}
		
		public function GetAllLinks():ArrayCollection
		{
			var links:ArrayCollection = new ArrayCollection();
			for each(var s:Sliver in slivers)
			{
				for each(var l:VirtualLink in s.links)
				{
					if(!links.contains(l))
						links.addItem(l);
				}
				
			}
			return links;
		}
		
		public function GetPhysicalNodes():ArrayCollection
		{
			var nodes:ArrayCollection = new ArrayCollection();
			for each(var s:Sliver in slivers)
			{
				for each(var n:VirtualNode in s.nodes)
				{
					if(!nodes.contains(n) && n.physicalNode != null)
						nodes.addItem(n.physicalNode);
				}
				
			}
			return nodes;
		}
		
		public function getVirtualNodeWithId(id:String):VirtualNode
		{
			for each(var s:Sliver in slivers)
			{
				var vn:VirtualNode = s.nodes.getById(id);
				if(vn != null)
					return vn;
			}
			return null;
		}
		
		public function getVirtualLinkWithId(id:String):VirtualLink
		{
			for each(var s:Sliver in slivers)
			{
				var vl:VirtualLink = s.links.getById(id);
				if(vl != null)
					return vl;
			}
			return null;
		}
		
		public function hasSliverFor(cm:ComponentManager):Boolean
		{
			for each(var s:Sliver in slivers)
			{
				if(s.componentManager == cm)
					return true;
			}
			return false;
		}
		
		public function getOrCreateSliverFor(cm:ComponentManager):Sliver
		{
			for each(var s:Sliver in slivers)
			{
				if(s.componentManager == cm)
					return s;
			}
			var newSliver:Sliver = new Sliver(this, cm);
			slivers.addItem(newSliver);
			return newSliver;
		}
		
		public function clone(addOutsideReferences:Boolean = true):Slice
		{
			var newSlice:Slice = new Slice();
			newSlice.uuid = this.uuid;
			newSlice.hrn = this.hrn;
			newSlice.urn = this.urn;
			newSlice.creator = this.creator;
			newSlice.credential = this.credential;
			newSlice.validUntil = this.validUntil;
			
			// Build up the basic slivers
			// Build up the slivers with nodes
			for each(var sliver:Sliver in this.slivers)
			{
				var newSliver:Sliver = new Sliver(newSlice);
				newSliver.created = sliver.created;
				newSliver.credential = sliver.credential;
				newSliver.componentManager = sliver.componentManager;
				newSliver.rspec = sliver.rspec;
				newSliver.urn = sliver.urn;
				newSliver.ticket = sliver.ticket;
				newSliver.manifest = sliver.manifest;
				newSliver.state = sliver.state;
				newSliver.status = sliver.status;
				newSliver.validUntil = sliver.validUntil;
				
				newSlice.slivers.addItem(newSliver);
			}
			
			var oldNodeToCloneNode:Dictionary = new Dictionary();
			var oldInterfaceToCloneInterface:Dictionary = new Dictionary();

			// Build up the slivers with nodes
			for each(sliver in this.slivers)
			{
				newSliver = newSlice.slivers.getByCm(sliver.componentManager);
				
				// Build up nodes
				var retrace:Array = new Array();
				for each(var node:VirtualNode in sliver.nodes)
				{
					if(node.manager != sliver.componentManager)
						continue;
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
					for each(var nodeSliver:Sliver in node.slivers)
					{
						newNode.slivers.push(newSlice.slivers.getByUrn(nodeSliver.urn));
						if(nodeSliver != sliver)
							newSlice.slivers.getByUrn(nodeSliver.urn).nodes.addItem(newNode);
					}
					
					for each(var vi:VirtualInterface in node.interfaces.collection)
					{
						var newVirtualInterface:VirtualInterface = new VirtualInterface(newNode);
						newVirtualInterface.id = vi.id;
						newVirtualInterface.role = vi.role;
						newVirtualInterface.isVirtual = vi.isVirtual;
						newVirtualInterface.physicalNodeInterface = vi.physicalNodeInterface;
						newVirtualInterface.bandwidth = vi.bandwidth;
						newVirtualInterface.ip = vi.ip;
						newNode.interfaces.Add(newVirtualInterface);
						oldInterfaceToCloneInterface[vi] = newVirtualInterface;
						// links? add later ...
					}

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
			}
			
			// Build up the links
			for each(sliver in this.slivers)
			{
				newSliver = newSlice.slivers.getByCm(sliver.componentManager);
				
				for each(var link:VirtualLink in sliver.links)
				{
					var newLink:VirtualLink = new VirtualLink(newSliver);
					newLink.id = link.id;
					newLink.type = link.type;
					newLink.bandwidth = link.bandwidth;
					newLink.firstTunnelIp = link.firstTunnelIp;
					newLink.secondTunnelIp = link.secondTunnelIp;
					newLink._isTunnel = link._isTunnel;
					newLink.rspec = link.rspec;
					for each(var linkSliver:Sliver in link.slivers)
						newLink.slivers.push(newSlice.slivers.getByUrn(linkSliver.urn));
					
					newLink.firstNode = oldNodeToCloneNode[link.firstNode];
					newLink.secondNode = oldNodeToCloneNode[link.secondNode];
					
					// Make sure it wasn't added by another sliver
					if((newLink.firstNode.slivers[0] as Sliver).links.getById(newLink.id) != null)
						continue;
					
					for each(var i:VirtualInterface in link.interfaces)
					{
						var newInterface:VirtualInterface = oldInterfaceToCloneInterface[i];
						newLink.interfaces.addItem(newInterface);
						newInterface.virtualLinks.addItem(newLink);
						newInterface.virtualNode.links.addItem(newLink);
					}
					
					newLink.firstNode.slivers[0].links.addItem(newLink);
					if(newLink.secondNode.slivers[0] != newLink.firstNode.slivers[0])
						newLink.secondNode.slivers[0].links.addItem(newLink);
				}
			}

			return newSlice;
		}
		
		public function ReadyIcon():Class {
			switch(Status()) {
				case READY : return DisplayUtil.flagGreenIcon;
				case NOTREADY : return DisplayUtil.flagYellowIcon;
				case FAILED : return DisplayUtil.flagRedIcon;
				default : return null;
			}
		}
		
		public function DisplayString():String {
			
			if(hrn == null && uuid == null) {
				return "All Resources";
			}
			
			var returnString : String;
			if(hrn == null)
				returnString = uuid;
			else
				returnString = hrn;
				
			return returnString + " (" + Status() + ")";
		}
		
		// Used to push more important slices to the top of lists
		public function CompareValue():int {
			
			if(hrn == null && uuid == null) {
				return -1;
			}
			
			if(Status() == "ready")
				return 0;
			else
				return 1;
		}
		
		public function getUniqueVirtualLinkId(l:VirtualLink = null):String
		{
			var highest:int = 0;
			for each(var s:Sliver in slivers)
			{
				for each(var l:VirtualLink in s.links)
				{
					try
					{
						if(l.id.substr(0,5) == "link-")
						{
							var testHighest:int = parseInt(l.id.substring(5));
							if(testHighest >= highest)
								highest = testHighest+1;
						}
					} catch(e:Error)
					{
						
					}
				}
			}
			return "link-" + highest;
			/*
			if(l == null)
				return "link-" + highest;
			else
			{
				if(l.isTunnel())
					return "tunnel-" + highest;
				else
					return "link-" + highest;
			}
			*/
		}
		
		public function getUniqueVirtualNodeId(n:VirtualNode = null):String
		{
			var highest:int = 0;
			for each(var s:Sliver in slivers)
			{
				for each(var n:VirtualNode in s.nodes)
				{
					try
					{
						var testHighest:int = parseInt(n.id.substring(n.id.lastIndexOf("-")+1));
						if(testHighest >= highest)
							highest = testHighest+1;
					} catch(e:Error)
					{
						
					}
				}
			}
			if(n == null)
				return "node-" + highest;
			else
			{
				if(n.isShared)
					return "shared-" + highest;
				else
					return "exclusive-" + highest;
			}
		}
		
		public function getUniqueVirtualInterfaceId():String
		{
			var highest:int = 0;
			for each(var s:Sliver in slivers)
			{
				for each(var l:VirtualLink in s.links)
				{
					for each(var i:VirtualInterface in l.interfaces)
					{
						try
						{
							if(i.id.substr(0,10) == "interface-")
							{
								var testHighest:int = parseInt(i.id.substring(10));
								if(testHighest >= highest)
									highest = testHighest+1;
							}
						} catch(e:Error)
						{
							
						}
					}
					
				}
			}
			return "interface-" + highest;
		}
	}
}