/* GENIPUBLIC-COPYRIGHT
* Copyright (c) 2008-2011 University of Utah and the Flux Group.
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
	import mx.events.CloseEvent;
	
	import protogeni.Util;
	import protogeni.XmlUtil;
	import protogeni.communication.CommunicationUtil;
	import protogeni.communication.RequestQueueNode;
	import protogeni.display.ChooseManagerWindow;
	import protogeni.display.ImageUtil;

	/**
	 * Container for slivers
	 * 
	 * @author mstrum
	 * 
	 */
	public class Slice
	{
		[Bindable]
		public var hrn:String = null;
		[Bindable]
		public var urn:IdnUrn = null;
		public var creator:GeniUser = null;
		public var credential:String = "";
		public var slivers:SliverCollection = new SliverCollection();
		
		public var expires:Date;
		
		[Bindable]
		public var useInputRspecVersion:Number = Util.defaultRspecVersion;
		
		public function get name():String {
			if(urn != null)
				return urn.name;
			else if(hrn != null)
				return hrn;
			else
				return "*No name*";
		}
		
		public function Slice()
		{
		}
		
		public function get MakingCalls():Boolean {
			var testNode:RequestQueueNode = Main.geniHandler.requestHandler.queue.head;
			while(testNode != null) {
				try {
					if(testNode.item.sliver.slice == this)
						return true;
				} catch(e:Error) {}
				try {
					if(testNode.item.slice == this)
						return true;
				} catch(e:Error) {}
				testNode = testNode.next;
			}
			return false;
		}
		
		public function get CompletelyReady():Boolean {
			// Return false if there are any calls being made for the slice
			if(MakingCalls)
				return false;
			// Return false if any sliver hasn't gotten its status
			if(this.credential.length > 0) {
				for each(var sliver:Sliver in this.slivers.collection) {
					if(!sliver.created || sliver.status.length == 0)
						return false;
				}
				return true;
			} else
				return false;
		}
		
		public function Status():String {
			if(this.hrn == null) return null;
			var status:String = Sliver.STATUS_NA;
			for each(var sliver:Sliver in this.slivers.collection) {
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
			if(this.slivers == null)
				return false;
			
			for each(var existing:Sliver in this.slivers.collection)
			{
				if(existing.created)
					return true;
			}
			return false;
		}
		
		public function hasAllAllocatedResources():Boolean
		{
			if(this.slivers == null)
				return false;
			
			for each(var existing:Sliver in this.slivers.collection)
			{
				if(!(existing.created || existing.staged))
					return false;
			}
			return true;
		}
		
		public function isStaged():Boolean {
			if(this.slivers == null || this.slivers.collection.length == 0)
				return false;
			
			for each(var existing:Sliver in this.slivers.collection)
			{
				if(!existing.staged)
					return false;
			}
			return true;
		}
		
		public function isCreated():Boolean {
			if(this.slivers == null || this.slivers.length == 0)
				return false;
			
			for each(var existing:Sliver in this.slivers.collection)
			{
				if(!existing.created)
					return false;
			}
			return true;
		}
		
		public function GetAllNodes():VirtualNodeCollection
		{
			var nodes:VirtualNodeCollection = new VirtualNodeCollection();
			for each(var s:Sliver in this.slivers.collection)
			{
				for each(var n:VirtualNode in s.nodes.collection)
				{
					if(n.manager == s.manager)
						nodes.add(n);
				}
				
			}
			return nodes;
		}
		
		public function GetAllLinks():VirtualLinkCollection
		{
			var links:VirtualLinkCollection = new VirtualLinkCollection();
			for each(var s:Sliver in this.slivers.collection)
			{
				for each(var l:VirtualLink in s.links.collection)
				{
					if(!links.contains(l))
						links.add(l);
				}
				
			}
			return links;
		}
		
		public function GetPhysicalNodes():Vector.<PhysicalNode>
		{
			var nodes:Vector.<PhysicalNode> = new Vector.<PhysicalNode>();
			for each(var s:Sliver in this.slivers.collection)
			{
				for each(var n:VirtualNode in s.nodes.collection)
				{
					if(nodes.indexOf(n) == -1 && n.physicalNode != null)
						nodes.push(n.physicalNode);
				}
				
			}
			return nodes;
		}
		
		public function getVirtualNodeWithId(id:String):VirtualNode
		{
			for each(var s:Sliver in this.slivers.collection)
			{
				var vn:VirtualNode = s.nodes.getById(id);
				if(vn != null)
					return vn;
			}
			return null;
		}
		
		public function getVirtualInterfaceWithId(id:String):VirtualInterface
		{
			for each(var s:Sliver in this.slivers.collection)
			{
				var vi:VirtualInterface = s.nodes.getByInterfaceId(id);
				if(vi != null)
					return vi;
			}
			return null;
		}
		
		public function getVirtualLinkWithId(id:String):VirtualLink
		{
			for each(var s:Sliver in this.slivers.collection)
			{
				var vl:VirtualLink = s.links.getById(id);
				if(vl != null)
					return vl;
			}
			return null;
		}
		
		public function hasSliverFor(gm:GeniManager):Boolean
		{
			for each(var s:Sliver in this.slivers.collection)
			{
				if(s.manager == gm)
					return true;
			}
			return false;
		}
		
		public function getOrCreateSliverFor(gm:GeniManager):Sliver
		{
			for each(var s:Sliver in this.slivers.collection)
			{
				if(s.manager == gm)
					return s;
			}
			var newSliver:Sliver = new Sliver(this, gm);
			this.slivers.add(newSliver);
			return newSliver;
		}
		
		public function clone(addOutsideReferences:Boolean = true):Slice
		{
			var newSlice:Slice = new Slice();
			newSlice.hrn = this.hrn;
			newSlice.urn = new IdnUrn(this.urn.full);
			newSlice.creator = this.creator;
			newSlice.credential = this.credential;
			newSlice.expires = this.expires;
			
			var sliver:Sliver;
			
			// Build up the basic slivers
			// Build up the slivers with nodes
			for each(sliver in this.slivers.collection)
			{
				var newSliver:Sliver = new Sliver(newSlice);
				newSliver.created = sliver.created;
				newSliver.credential = sliver.credential;
				newSliver.manager = sliver.manager;
				newSliver.rspec = sliver.rspec;
				newSliver.urn = new IdnUrn(sliver.urn.full);
				newSliver.ticket = sliver.ticket;
				newSliver.manifest = sliver.manifest;
				newSliver.state = sliver.state;
				newSliver.status = sliver.status;
				newSliver.expires = sliver.expires;
				newSliver.extensionNamespaces = sliver.extensionNamespaces;
				
				newSlice.slivers.add(newSliver);
			}
			
			var oldNodeToCloneNode:Dictionary = new Dictionary();
			var oldLinkToCloneLink:Dictionary = new Dictionary();
			var oldInterfaceToCloneInterface:Dictionary = new Dictionary();
			
			// Build up the slivers with nodes
			for each(sliver in this.slivers.collection)
			{
				newSliver = newSlice.slivers.getByGm(sliver.manager);
				
				// Build up nodes
				var retrace:Array = new Array();
				for each(var node:VirtualNode in sliver.nodes.collection)
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
						newNode.executeServices.push(new ExecuteService(executeService.command,
																		executeService.shell));
					}
					for each(var installService:InstallService in node.installServices) {
						newNode.installServices.push(new InstallService(installService.url,
																		installService.installPath,
																		installService.fileType));
					}
					for each(var loginService:LoginService in node.loginServices) {
						newNode.loginServices.push(new LoginService(loginService.authentication,
																	loginService.hostname,
																	loginService.port,
																	loginService.username));
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
					newNode.flackUnbound = node.flackUnbound;
					newNode.extensionsNodes = node.extensionsNodes;
					// depreciated
					newNode.virtualizationType = node.virtualizationType;
					newNode.virtualizationSubtype = node.virtualizationSubtype;
					
					for each(var nodeSliver:Sliver in node.slivers.collection)
					{
						newNode.slivers.addIfNotExisting(newSlice.slivers.getByGm(nodeSliver.manager));
						newSlice.slivers.getByUrn(nodeSliver.urn.full).nodes.add(newNode);
					}
					
					for each(var vi:VirtualInterface in node.interfaces.collection)
					{
						var newVirtualInterface:VirtualInterface = new VirtualInterface(newNode);
						newVirtualInterface.id = vi.id;
						newVirtualInterface.physicalNodeInterface = vi.physicalNodeInterface;
						newVirtualInterface.capacity = vi.capacity;
						newVirtualInterface.ip = vi.ip;
						newVirtualInterface.mask = vi.mask;
						newVirtualInterface.type = vi.type;
						newNode.interfaces.add(newVirtualInterface);
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
						for each(var subNode:VirtualNode in oldNode.subNodes.collection)
						cloneNode.subNodes.add(newSliver.nodes.getById(subNode.clientId));
					}
				}
			}
			
			// Build up the links
			for each(sliver in this.slivers.collection)
			{
				newSliver = newSlice.slivers.getByGm(sliver.manager);
				
				for each(var link:VirtualLink in sliver.links.collection)
				{
					if(oldLinkToCloneLink[link] != null)
						continue;
					var newLink:VirtualLink = new VirtualLink();
					newLink.clientId = link.clientId;
					newLink.sliverId = link.sliverId;
					newLink.type = link.type;
					newLink.capacity = link.capacity;
					newLink.linkType = link.linkType;
					newLink.rspec = link.rspec;
					for each(var linkSliver:Sliver in link.slivers.collection) {
						newLink.slivers.add(newSlice.slivers.getByGm(linkSliver.manager));
						newSlice.slivers.getByGm(linkSliver.manager).links.add(newLink);
					}
					
					var slivers:Vector.<Sliver> = new Vector.<Sliver>();
					for each(var i:VirtualInterface in link.interfaces.collection)
					{
						var newInterface:VirtualInterface = oldInterfaceToCloneInterface[i];
						newLink.interfaces.add(newInterface);
						newInterface.virtualLinks.add(newLink);
					}
					
					oldLinkToCloneLink[link] = newLink;
				}
			}
			
			return newSlice;
		}
		
		public function ReadyIcon():Class {
			switch(this.Status()) {
				case Sliver.STATUS_READY : return ImageUtil.flagGreenIcon;
				case Sliver.STATUS_NOTREADY : return ImageUtil.flagYellowIcon;
				case Sliver.STATUS_FAILED : return ImageUtil.flagRedIcon;
				default : return ImageUtil.flagOrangeIcon;
			}
		}
		
		public function isIdUnique(o:*, id:String):Boolean {
			if(!this.GetAllLinks().isIdUnique(o, id))
				return false;
			if(!this.GetAllNodes().isIdUnique(o, id))
				return false;
			return true;
		}
		
		public function getUniqueVirtualLinkId(l:VirtualLink = null):String
		{
			var highest:int = 0;
			for each(var s:Sliver in this.slivers.collection)
			{
				for each(var l:VirtualLink in s.links.collection)
				{
					try
					{
						if(l.clientId.substr(0,5) == "link-")
						{
							var testHighest:int = parseInt(l.clientId.substring(5));
							if(testHighest >= highest)
								highest = testHighest+1;
						}
					} catch(e:Error) { }
				}
			}
			return "link-" + highest;
		}
		
		public function getUniqueVirtualNodeId(n:VirtualNode = null):String
		{
			var newId:String;
			if(n == null)
				newId = "node-";
			else
			{
				if(n.isDelayNode)
					newId = "bridge-";
				else if(n.Exclusive)
					newId = "exclusive-";
				else
					newId = "shared-";
			}
			
			var highest:int = 0;
			for each(var s:Sliver in this.slivers.collection)
			{
				for each(var testNode:VirtualNode in s.nodes.collection)
				{
					try
					{
						if(testNode.clientId.indexOf(newId) == 0) {
							var testHighest:int = parseInt(testNode.clientId.substring(testNode.clientId.lastIndexOf("-")+1));
							if(testHighest >= highest)
								highest = testHighest+1;
						}
					} catch(e:Error) {}
				}
			}
			
			return newId + highest;
		}
		
		public function getUniqueVirtualInterfaceId():String
		{
			var highest:int = 0;
			for each(var s:Sliver in this.slivers.collection)
			{
				for each(var l:VirtualLink in s.links.collection)
				{
					for each(var i:VirtualInterface in l.interfaces.collection)
					{
						try
						{
							if(i.id.substr(0,10) == "interface-")
							{
								var testHighest:int = parseInt(i.id.substring(10));
								if(testHighest >= highest)
									highest = testHighest+1;
							}
						} catch(e:Error) { }
					}
					
				}
			}
			return "interface-" + highest;
		}
		
		public function tryImport(rspec:String):Boolean {
			// Tell user they need to delete
			if(this.isCreated())
				Alert.show("The slice has resources allocated to it.  Please delete the slice before trying to import.", "Allocated Resources Exist");
			else if(this.GetAllNodes().length > 0)
				Alert.show("The slice already has resources waiting to be allocated.  Please clear the canvas before trying to import", "Resources Exist");
			else {
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
				
				var defaultNamespace:Namespace = sliceRspec.namespace();
				var detectedRspecVersion:Number;
				switch(defaultNamespace.uri) {
					case XmlUtil.rspec01Namespace:
						detectedRspecVersion = 0.1;
						break;
					case XmlUtil.rspec02Namespace:
						detectedRspecVersion = 0.2;
						break;
					case XmlUtil.rspec2Namespace:
						detectedRspecVersion = 2;
						break;
					default:
						Alert.show("Please use a compatible RSPEC");
						return false;
				}
				
				for each(var nodeXml:XML in sliceRspec.defaultNamespace::node)
				{
					var managerUrn:String;
					if(detectedRspecVersion < 1)
						managerUrn = nodeXml.@component_manager_uuid;
					else
						managerUrn = nodeXml.@component_manager_id;
					if(managerUrn.length == 0) {
						var chooseManagerWindow:ChooseManagerWindow = new ChooseManagerWindow();
						chooseManagerWindow.success = function importWithDefault(manager:GeniManager):void {
															doImport(sliceRspec, manager);
														}
						chooseManagerWindow.showWindow();
						return true;
					}
				}
				
				return this.doImport(sliceRspec);
			}
			return false;
		}
		
		public function doImport(sliceRspec:XML, defaultManager:GeniManager = null):Boolean {
			
			// Detect managers
			try {
				var defaultNamespace:Namespace = sliceRspec.namespace();
				var detectedRspecVersion:Number;
				var detectedManagers:Vector.<GeniManager> = new Vector.<GeniManager>();
				switch(defaultNamespace.uri) {
					case XmlUtil.rspec01Namespace:
						detectedRspecVersion = 0.1;
						break;
					case XmlUtil.rspec02Namespace:
						detectedRspecVersion = 0.2;
						break;
					case XmlUtil.rspec2Namespace:
						detectedRspecVersion = 2;
						break;
				}
			}
			catch(e:Error)
			{
				Alert.show("Please use a compatible RSPEC");
				return false;
			}
			
			this.slivers = new SliverCollection();
			
			// Set the unknown managers to the default manager if set
			if(defaultManager != null) {
				for each(var testNodeXml:XML in sliceRspec.defaultNamespace::node)
				{
					var testManagerUrn:String;
					if(detectedRspecVersion < 1)
						testManagerUrn = testNodeXml.@component_manager_uuid;
					else
						testManagerUrn = testNodeXml.@component_manager_id;
					if(testManagerUrn.length == 0) {
						if(detectedRspecVersion < 1)
							testNodeXml.@component_manager_uuid = defaultManager.Urn.full;
						else
							testNodeXml.@component_manager_id = defaultManager.Urn.full;
					}
				}
			}
			
			for each(var nodeXml:XML in sliceRspec.defaultNamespace::node)
			{
				var managerUrn:String;
				var detectedManager:GeniManager;
				if(detectedRspecVersion < 1)
					managerUrn = nodeXml.@component_manager_uuid;
				else
					managerUrn = nodeXml.@component_manager_id;
				if(managerUrn.length == 0) {
					Alert.show("All nodes must have a manager associated with them");
					return false;
				} else
					detectedManager = Main.geniHandler.GeniManagers.getByUrn(managerUrn);
				
				if(detectedManager == null) {
					Alert.show("All nodes must have a manager id for a known manager");
					return false;
				}
				
				if(detectedManager != null && detectedManagers.indexOf(detectedManager) == -1) {
					var newSliver:Sliver = this.getOrCreateSliverFor(detectedManager);
					newSliver.rspec = sliceRspec;
					try {
						newSliver.parseRspec();
						newSliver.staged = true;
					} catch(e:Error) {
						Alert.show("Error while parsing sliver RSPEC on " + detectedManager.Hrn + ": " + e.toString());
						return false;
					}
					detectedManagers.push(detectedManager);
				}
				
			}
			
			Main.geniDispatcher.dispatchSliceChanged(this);
			return true;
		}
	}
}