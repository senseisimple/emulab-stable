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
	import mx.controls.Alert;
	
	import protogeni.Util;
	import protogeni.communication.CommunicationUtil;
	import protogeni.display.ImageUtil;
	
	// Slice that a user created in ProtoGENI
	public class Slice
	{
		[Bindable]
		public var hrn:String = null;
		[Bindable]
		public var urn:String = null;
		[Bindable]
		public var name:String = null;
		public var creator:GeniUser = null;
		public var credential:String = "";
		public var slivers:SliverCollection = new SliverCollection();

		public var expires:Date;
		
		// depreciated
		public var uuid:String = null;

		public function Slice()
		{
		}
		
		public function Status():String {
			if(hrn == null) return null;
			var status:String = Sliver.STATUS_NA;
			for each(var sliver:Sliver in slivers) {
				if(sliver.status == Sliver.STATUS_FAILED)
					return Sliver.STATUS_FAILED;
				
				if(status == Sliver.STATUS_NA) status = sliver.status;
				else {
					if(sliver.status != status)
						return Sliver.STATUS_MIXED;
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
				if(!(existing.created || existing.staged))
					return false;
			}
			return true;
		}
		
		public function isStaged():Boolean {
			if(slivers == null)
				return false;
			
			for each(var existing:Sliver in this.slivers)
			{
				if(!existing.staged)
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
					if(n.manager == s.manager)
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
		
		public function getVirtualInterfaceWithId(id:String):VirtualInterface
		{
			for each(var s:Sliver in slivers)
			{
				var vi:VirtualInterface = s.nodes.getByInterfaceId(id);
				if(vi != null)
					return vi;
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
		
		public function hasSliverFor(gm:GeniManager):Boolean
		{
			for each(var s:Sliver in slivers)
			{
				if(s.manager == gm)
					return true;
			}
			return false;
		}
		
		public function getOrCreateSliverFor(gm:GeniManager):Sliver
		{
			for each(var s:Sliver in slivers)
			{
				if(s.manager == gm)
					return s;
			}
			var newSliver:Sliver = new Sliver(this, gm);
			slivers.addItem(newSliver);
			return newSliver;
		}
		
		public function clone(addOutsideReferences:Boolean = true):Slice
		{
			var newSlice:Slice = new Slice();
			newSlice.uuid = this.uuid;
			newSlice.hrn = this.hrn;
			newSlice.urn = this.urn;
			newSlice.name = this.name;
			newSlice.creator = this.creator;
			newSlice.credential = this.credential;
			newSlice.expires = this.expires;
			
			var sliver:Sliver;
			
			// Build up the basic slivers
			// Build up the slivers with nodes
			for each(sliver in this.slivers)
			{
				var newSliver:Sliver = new Sliver(newSlice);
				newSliver.created = sliver.created;
				newSliver.credential = sliver.credential;
				newSliver.manager = sliver.manager;
				newSliver.rspec = sliver.rspec;
				newSliver.urn = sliver.urn;
				newSliver.ticket = sliver.ticket;
				newSliver.manifest = sliver.manifest;
				newSliver.state = sliver.state;
				newSliver.status = sliver.status;
				newSliver.expires = sliver.expires;
				
				newSlice.slivers.addItem(newSliver);
			}
			
			var oldNodeToCloneNode:Dictionary = new Dictionary();
			var oldLinkToCloneLink:Dictionary = new Dictionary();
			var oldInterfaceToCloneInterface:Dictionary = new Dictionary();

			// Build up the slivers with nodes
			for each(sliver in this.slivers)
			{
				newSliver = newSlice.slivers.getByGm(sliver.manager);
				
				// Build up nodes
				var retrace:Array = new Array();
				for each(var node:VirtualNode in sliver.nodes)
				{
					if(oldNodeToCloneNode[node] != null)
						continue;
					var newNode:VirtualNode = new VirtualNode(newSliver);
					newNode.clientId = node.clientId;
					newNode.physicalNode = node.physicalNode;
					newNode.manager = node.manager;
					newNode.sliverId = node.sliverId;
					newNode._exclusive = node.Exclusive;
					newNode.sliverType = node.sliverType;
					for each(var executeService:ExecuteService in node.executeServices) {
						newNode.executeServices.push(new ExecuteService(executeService.command, executeService.shell));
					}
					for each(var installService:InstallService in node.installServices) {
						newNode.installServices.push(new InstallService(installService.url, installService.installPath, installService.fileType));
					}
					for each(var loginService:LoginService in node.loginServices) {
						newNode.loginServices.push(new LoginService(loginService.authentication, loginService.hostname, loginService.port, loginService.username));
					}
					// supernode? add later ...
					// subnodes? add later ...
					retrace.push({clone:newNode, old:node});
					newNode.rspec = node.rspec;
					newNode.error = node.error;
					newNode.state = node.state;
					newNode.status = node.status;
					newNode.diskImage = node.diskImage;
					newNode.flackX = node.flackX;
					newNode.flackY = node.flackY;
					// depreciated
					newNode.virtualizationType = node.virtualizationType;
					newNode.virtualizationSubtype = node.virtualizationSubtype;
					
					for each(var nodeSliver:Sliver in node.slivers)
					{
						newNode.slivers.addIfNotExisting(newSlice.slivers.getByGm(nodeSliver.manager));
						newSlice.slivers.getByUrn(nodeSliver.urn).nodes.addItem(newNode);
					}
					
					for each(var vi:VirtualInterface in node.interfaces.collection)
					{
						var newVirtualInterface:VirtualInterface = new VirtualInterface(newNode);
						newVirtualInterface.id = vi.id;
						newVirtualInterface.physicalNodeInterface = vi.physicalNodeInterface;
						newVirtualInterface.bandwidth = vi.bandwidth;
						newVirtualInterface.ip = vi.ip;
						newVirtualInterface.mask = vi.mask;
						newVirtualInterface.type = vi.type;
						newNode.interfaces.Add(newVirtualInterface);
						oldInterfaceToCloneInterface[vi] = newVirtualInterface;
						// links? add later ...
					}

					//newSliver.nodes.addItem(newNode);
					
					oldNodeToCloneNode[node] = newNode;
				}
				
				// supernode and subnodes need to be added after to ensure they were created
				for each(var check:Object in retrace)
				{
					var cloneNode:VirtualNode = check.clone;
					var oldNode:VirtualNode = check.old;
					if(oldNode.superNode != null)
						cloneNode.superNode = newSliver.nodes.getById(oldNode.clientId);
					if(oldNode.subNodes != null && oldNode.subNodes.length > 0)
					{
						for each(var subNode:VirtualNode in oldNode.subNodes)
						cloneNode.subNodes.push(newSliver.nodes.getById(subNode.clientId));
					}
				}
			}
			
			// Build up the links
			for each(sliver in this.slivers)
			{
				newSliver = newSlice.slivers.getByGm(sliver.manager);
				
				for each(var link:VirtualLink in sliver.links)
				{
					if(oldLinkToCloneLink[link] != null)
						continue;
					var newLink:VirtualLink = new VirtualLink();
					newLink.clientId = link.clientId;
					newLink.sliverId = link.sliverId;
					newLink.type = link.type;
					newLink.bandwidth = link.bandwidth;
					newLink.linkType = link.linkType;
					newLink.rspec = link.rspec;
					for each(var linkSliver:Sliver in link.slivers) {
						newLink.slivers.push(newSlice.slivers.getByGm(linkSliver.manager));
						newSlice.slivers.getByGm(linkSliver.manager).links.addItem(newLink);
					}
					
					var slivers:Vector.<Sliver> = new Vector.<Sliver>();
					for each(var i:VirtualInterface in link.interfaces)
					{
						var newInterface:VirtualInterface = oldInterfaceToCloneInterface[i];
						newLink.interfaces.addItem(newInterface);
						newInterface.virtualLinks.addItem(newLink);
					}
					
					oldLinkToCloneLink[link] = newLink;
				}
			}

			return newSlice;
		}
		
		public function ReadyIcon():Class {
			switch(Status()) {
				case Sliver.STATUS_READY : return ImageUtil.flagGreenIcon;
				case Sliver.STATUS_NOTREADY : return ImageUtil.flagYellowIcon;
				case Sliver.STATUS_FAILED : return ImageUtil.flagRedIcon;
				default : return ImageUtil.flagOrangeIcon;
			}
		}
		
		public function DisplayString():String {
			
			if(hrn == null && uuid == null) {
				return "All Resources";
			}
			
			var returnString:String;
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
						if(l.clientId.substr(0,5) == "link-")
						{
							var testHighest:int = parseInt(l.clientId.substring(5));
							if(testHighest >= highest)
								highest = testHighest+1;
						}
					} catch(e:Error)
					{
						
					}
				}
			}
			return "link-" + highest;
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
						var testHighest:int = parseInt(n.clientId.substring(n.clientId.lastIndexOf("-")+1));
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
				if(n.Exclusive)
					return "exclusive-" + highest;
				else
					return "shared-" + highest;
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
		
		public function tryImport(rspec:String):Boolean {
			var sliceRspec:XML;
			try
			{
				sliceRspec = new XML(rspec);
			}
			catch(e:Error)
			{
				Alert.show("There is a problem with the XML: " + e.toString());
				return false;
			}
			
			// Detect managers, create slivers
			try {
				var defaultNamespace:Namespace = sliceRspec.namespace();
				var detectedRspecVersion:Number;
				var detectedManagers:Vector.<GeniManager> = new Vector.<GeniManager>();
				switch(defaultNamespace.uri) {
					case CommunicationUtil.rspec01Namespace:
						detectedRspecVersion = 0.1;
						break;
					case CommunicationUtil.rspec02Namespace:
						detectedRspecVersion = 0.2;
						break;
					case CommunicationUtil.rspec2Namespace:
						detectedRspecVersion = 2;
						break;
				}
			}
			catch(e:Error)
			{
				Alert.show("Please use a compatible RSPEC");
				return false;
			}
			
			slivers.removeAll();
			
			for each(var nodeXml:XML in sliceRspec.defaultNamespace::node)
			{
				var managerUrn:String;
				if(detectedRspecVersion < 1)
					managerUrn = nodeXml.@component_manager_uuid;
				else
					managerUrn = nodeXml.@component_manager_id;
				if(managerUrn.length == 0) {
					Alert.show("All nodes must have a manager id");
					return false;
				}
				var detectedManager:GeniManager = Main.geniHandler.GeniManagers.getByUrn(managerUrn);
				if(detectedManager == null) {
					Alert.show("All nodes must have a manager id for a known manager");
					return false;
				}
				
				if(detectedManager != null && detectedManagers.indexOf(detectedManager) == -1) {
					var newSliver:Sliver = this.getOrCreateSliverFor(detectedManager);
					newSliver.rspec = sliceRspec;
					newSliver.staged = true;
					//try {
					newSliver.parseRspec();
					/*} catch(e:Error) {
					Alert.show("Error while parsing sliver RSPEC on " + detectedManager.Hrn + ": " + e.toString());
					return;
					}*/
					detectedManagers.push(detectedManager);
				}
				
			}
			
			Main.geniDispatcher.dispatchSliceChanged(this);
			return true;
		}
	}
}