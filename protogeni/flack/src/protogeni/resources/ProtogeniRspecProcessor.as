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
	import flash.events.Event;
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Alert;
	import mx.core.FlexGlobals;
	
	import protogeni.GeniEvent;
	import protogeni.Util;
	import protogeni.XmlUtil;
	
	/**
	 * Processes ProtoGENI RSPECS.  Supports 0.1, 0.2, 2.
	 * 
	 * @author mstrum
	 * 
	 */
	public class ProtogeniRspecProcessor implements RspecProcessorInterface
	{
		private var manager:GeniManager;
		public function ProtogeniRspecProcessor(newGm:GeniManager)
		{
			this.manager = newGm;
		}
		
		private static var NODE_PARSE : int = 0;
		private static var LINK_PARSE : int = 1;
		private static var DONE : int = 2;
		
		private static var MAX_WORK : int = 40;
		
		private var myAfter:Function;
		private var myIndex:int;
		private var myState:int = NODE_PARSE;
		private var nodes:XMLList;
		private var links:XMLList;
		private var interfaceDictionary:Dictionary;
		private var nodeNameDictionary:Dictionary;
		private var subnodeList:ArrayCollection;
		private var linkDictionary:Dictionary;
		private var hasslot:Boolean = false;
		private var diskNameToImage:Dictionary;
		
		private var defaultNamespace:Namespace;
		
		private var detectedOutputRspecVersion:Number;
		public function processResourceRspec(afterCompletion:Function):void {
			this.manager.clearComponents();
			
			Main.Application().setStatus("Parsing " + this.manager.Hrn + " RSPEC", false);
			
			this.defaultNamespace = this.manager.Rspec.namespace();
			
			switch(defaultNamespace.uri) {
				case XmlUtil.rspec01Namespace:
					this.detectedOutputRspecVersion = 0.1;
					break;
				case XmlUtil.rspec02Namespace:
					this.detectedOutputRspecVersion = 0.2;
					break;
				case XmlUtil.rspec2Namespace:
					this.detectedOutputRspecVersion = 2;
					break;
				default:
					var msg:String = "RSPEC with the namespace '" +this.defaultNamespace.uri+ "' is not supported.";
					Alert.show(msg);
					LogHandler.appendMessage(new LogMessage(this.manager.Url,
															"Unsupported RSPEC",
															msg,
															true,
															LogMessage.TYPE_END));
					// FIXME: probably needs to error in a different way...
					this.manager.Status = GeniManager.STATUS_FAILED;
					afterCompletion();
					return;
			}
			
			if(this.manager.Rspec.@valid_until.length() == 1)
				this.manager.expires = Util.parseProtogeniDate(this.manager.Rspec.@valid_until);
			if(this.manager.Rspec.@expires.length() == 1)
				this.manager.expires = Util.parseProtogeniDate(this.manager.Rspec.@expires);
			if(this.manager.Rspec.@generated.length() == 1)
				this.manager.generated = Util.parseProtogeniDate(this.manager.Rspec.@generated);
			
			this.nodes = this.manager.Rspec.defaultNamespace::node;
			this.links = this.manager.Rspec.defaultNamespace::link;
			
			this.myIndex = 0;
			this.myState = NODE_PARSE;
			this.interfaceDictionary = new Dictionary();
			this.nodeNameDictionary = new Dictionary();
			this.diskNameToImage = new Dictionary();
			this.subnodeList = new ArrayCollection();
			this.myAfter = afterCompletion;
			FlexGlobals.topLevelApplication.stage.addEventListener(Event.ENTER_FRAME, parseNext);
		}
		
		private function parseNext(event:Event) : void
		{
			if(!this.hasslot && GeniManager.processing > GeniManager.maxProcessing)
				return;
			if(!this.hasslot) {
				this.hasslot = true;
				GeniManager.processing++;
			}
			
			var startTime:Date = new Date();
			if (this.myState == NODE_PARSE)	    	
			{
				this.parseNextNode();
				if(Main.debugMode)
					trace("ParseN " + String((new Date()).time - startTime.time));
			}
			else if (this.myState == LINK_PARSE)
			{
				this.parseNextLink();
				if(Main.debugMode)
					trace("ParseL " + String((new Date()).time - startTime.time));
			}
			else if (this.myState == DONE)
			{
				GeniManager.processing--;
				Main.Application().setStatus("Parsing " + manager.Hrn + " RSPEC Done",false);
				this.interfaceDictionary = null;
				this.nodeNameDictionary = null;
				this.subnodeList = null;
				this.manager.Status = GeniManager.STATUS_VALID;
				Main.geniDispatcher.dispatchGeniManagerChanged(this.manager, GeniEvent.ACTION_POPULATED);
				Main.Application().stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				if(this.myAfter != null)
					this.myAfter(this.manager);
			}
			else
			{
				Main.Application().setStatus("Fail",true);
				this.interfaceDictionary = null;
				this.nodeNameDictionary = null;
				this.subnodeList = null;
				Main.Application().stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				Alert.show("Problem parsing RSPEC");
				// Throw exception
			}
		}
		
		private function parseNextNode():void {
			var startTime:Date = new Date();
			var idx:int = 0;
			
			while(this.myIndex < this.nodes.length()) {
				try {
					var nodeXml:XML = this.nodes[myIndex];
					
					// Get location info
					var lat:Number = -72.134678;
					var lng:Number = 170.332031;
					var locationXml:XML = nodeXml.defaultNamespace::location[0];
					if(Number(locationXml.@latitude) != 0 && Number(locationXml.@longitude) != 0)
					{
						lat = Number(locationXml.@latitude);
						lng = Number(locationXml.@longitude);
					}
					
					// Assign to a group based on location
					var ng:PhysicalNodeGroup = manager.Nodes.GetByLocation(lat,lng);
					if(ng == null) {
						ng = new PhysicalNodeGroup(lat, lng, locationXml.@country, manager.Nodes);
						manager.Nodes.Add(ng);
					}
					
					var node:PhysicalNode = new PhysicalNode(ng, this.manager);
					node.name = nodeXml.@component_name;
					switch(detectedOutputRspecVersion)
					{
						case 0.1:
						case 0.2:
							node.id = nodeXml.@component_uuid;
							break;
						case 2:
						default:
							node.id = nodeXml.@component_id;
							node.exclusive = nodeXml.@exclusive == "true";
					}
					
					for each(var nodeChildXml:XML in nodeXml.children()) {
						if(nodeChildXml.namespace() == defaultNamespace) {
							if(nodeChildXml.localName() == "interface") {
								var i:PhysicalNodeInterface = new PhysicalNodeInterface(node);
								i.id = nodeChildXml.@component_id;
								i.role = PhysicalNodeInterface.RoleIntFromString(nodeChildXml.@role);
								i.publicIPv4 = nodeChildXml.@public_ipv4;
								node.interfaces.add(i);
								this.interfaceDictionary[i.id] = i;
							} else if(nodeChildXml.localName() == "hardware_type") {
								node.hardwareTypes.push(nodeChildXml.@name);
							} else if(nodeChildXml.localName() == "sliver_type") {
								var newSliverType:SliverType = new SliverType(nodeChildXml.@name);
								for each(var sliverTypeChildXml:XML in nodeChildXml.children()) {
									if(sliverTypeChildXml.localName() == "disk_image") {
										var newSliverDiskImage:DiskImage = this.diskNameToImage[String(sliverTypeChildXml.@name)];
										if(newSliverDiskImage == null) {
											newSliverDiskImage = new DiskImage(sliverTypeChildXml.@name,
												sliverTypeChildXml.@os,
												sliverTypeChildXml.@version,
												sliverTypeChildXml.@description,
												sliverTypeChildXml.@default == "true");
											this.diskNameToImage[String(sliverTypeChildXml.@name)] = newSliverDiskImage;
											manager.DiskImages.addItem(newSliverDiskImage);
										}
										newSliverType.diskImages.push(newSliverDiskImage);
									}
								}
								node.sliverTypes.push(newSliverType);
							} else if(nodeChildXml.localName() == "available") {
								switch(detectedOutputRspecVersion)
								{
									case 0.1:
									case 0.2:
										node.available = nodeChildXml.toString() == "true";
										break;
									case 2:
									default:
										node.available = nodeChildXml.@now == "true";
								}
							} else if(nodeChildXml.localName() == "relation") {
								var relationType:String = nodeChildXml.@type;
								if(relationType == "subnode_of") {
									var parentId:String = nodeChildXml.@component_id;
									if(parentId.length > 0)
										subnodeList.addItem({subNode:node, parentName:parentId});
								}
							} else if(nodeChildXml.localName() == "disk_image") {
								if(node.sliverTypes.length == 0)
									node.sliverTypes.push(new SliverType("N/A"));
								var newDiskImage:DiskImage = this.diskNameToImage[String(nodeChildXml.@name)];
								if(newDiskImage == null) {
									newDiskImage = new DiskImage(nodeChildXml.@name,
										nodeChildXml.@os,
										nodeChildXml.@version,
										nodeChildXml.@description,
										nodeChildXml.@default == "true");
									this.diskNameToImage[String(nodeChildXml.@name)] = newDiskImage;
									manager.DiskImages.addItem(newDiskImage);
								}
								node.sliverTypes[0].diskImages.push();
							}
								// Depreciated
							else if(nodeChildXml.localName() == "exclusive") {
								var exString:String = nodeChildXml.toString();
								node.exclusive = exString == "true";
							} else if(nodeChildXml.localName() == "subnode_of") {
								var parentName:String = nodeChildXml.toString();
								if(parentName.length > 0)
									subnodeList.addItem({subNode:node, parentName:parentName});
							} else if(nodeChildXml.localName() == "node_type") {
								node.hardwareTypes.push(nodeChildXml.@type_name);
							}
						}
					}
					
					node.rspec = nodeXml;
					ng.Add(node);
					this.nodeNameDictionary[node.id] = node;
					this.nodeNameDictionary[node.name] = node;
					this.manager.AllNodes.push(node);
				} catch(e:Error) {
					// skip if some problem
					if(Main.debugMode)
						trace("Skipped node which threw error in manager named " + manager.Hrn);
				}
				idx++;
				this.myIndex++;
				if(((new Date()).time - startTime.time) > 40) {
					if(Main.debugMode)
						trace("Nodes processed:" + idx);
					return;
				}
			}
			
			// Assign subnodes
			for each(var obj:Object in this.subnodeList)
			{
				var parentNode:PhysicalNode = this.nodeNameDictionary[obj.parentName];
				if(parentNode != null) {
					var subNode:PhysicalNode = obj.subNode;
					// Hack so user doesn't try to get subnodes of nodes which aren't available
					if(!parentNode.available)
						subNode.available = false;
					parentNode.subNodes.push(subNode);
					obj.subNode.subNodeOf = parentNode;
				}
			}
			
			this.myState = LINK_PARSE;
			this.myIndex = 0;
			return;
		}
		
		private function parseNextLink():void {
			var startTime:Date = new Date();
			var idx:int = 0;
			
			while(this.myIndex < this.links.length()) {
				try {
					var link:XML = this.links[myIndex];
					var l:PhysicalLink = new PhysicalLink(null);
					
					// Get interfaces used
					for each(var interfaceRefXml:XML in link.defaultNamespace::interface_ref) {
						var interfaceId:String;
						switch(detectedOutputRspecVersion)
						{
							case 0.1:
							case 0.2:
								interfaceId = interfaceRefXml.@component_interface_id;
								break;
							case 2:
							default:
								interfaceId = interfaceRefXml.@component_id;
						}
						var referencedInterface:PhysicalNodeInterface = this.interfaceDictionary[interfaceId];
						if(referencedInterface != null)
							l.interfaceRefs.add(referencedInterface);
						else {
							// error
						}
					}
					
					var lg:PhysicalLinkGroup = this.manager.Links.Get(l.interfaceRefs.collection[0].owner.GetLatitude(),
															l.interfaceRefs.collection[0].owner.GetLongitude(),
															l.interfaceRefs.collection[1].owner.GetLatitude(),
															l.interfaceRefs.collection[1].owner.GetLongitude());
					if(lg == null) {
						lg = new PhysicalLinkGroup(l.interfaceRefs.collection[0].owner.GetLatitude(),
													l.interfaceRefs.collection[0].owner.GetLongitude(),
													l.interfaceRefs.collection[1].owner.GetLatitude(),
													l.interfaceRefs.collection[1].owner.GetLongitude(),
													this.manager.Links);
						this.manager.Links.Add(lg);
					}
					l.owner = lg;
					l.name = link.@component_name;
					switch(detectedOutputRspecVersion)
					{
						case 0.1:
						case 0.2:
							l.id = link.@component_uuid;
							break;
						case 2:
						default:
							l.id = link.@component_id;
					}
					l.manager = this.manager;
					
					for each(var ix:XML in link.children()) {
						if(ix.namespace() == defaultNamespace) {
							if(ix.localName() == "property") {
								// FIXME: Hack since v1 just had link properties instead of dst & src
								if(ix.@latency.length() == 1)
									l.latency = Number(ix.@latency);
								if(ix.@packet_loss.length() == 1)
									l.packetLoss = Number(ix.@packet_loss);
								if(ix.@capacity.length() == 1)
									l.capacity = Number(ix.@capacity);
								//<property source_id="urn:publicid:IDN+emulab.net+interface+cisco8:(null)" dest_id="urn:publicid:IDN+emulab.net+interface+pc219:eth2" capacity="1000000" latency="0" packet_loss="0"/>
							} else if(ix.localName() == "link_type") {
								var s:String;
								switch(detectedOutputRspecVersion)
								{
									case 0.1:
									case 0.2:
										s = ix.@type_name;
										break;
									case 2:
									default:
										s = ix.@name;
								}
								l.linkTypes.push(s);
							}
								// Depreciated
							else if(ix.localName() == "bandwidth") {
								l.capacity = Number(ix);
							} else if(ix.localName() == "latency") {
								l.latency = Number(ix);
							} else if(ix.localName() == "packet_loss") {
								l.packetLoss = Number(ix);
							}
						}
					}
					
					l.rspec = link;
					
					lg.Add(l);
					for each(var addedInterfaces:PhysicalNodeInterface in l.interfaceRefs.collection) {
						addedInterfaces.physicalLinks.push(l);
					}
					if(lg.IsSameSite())
						l.interfaceRefs.collection[0].owner.owner.links = lg;
					this.manager.AllLinks.push(l);
				} catch(e:Error) {
					// skip if some problem
					if(Main.debugMode)
						trace("Skipped link which threw error in manager named " + manager.Hrn);
				}
				idx++;
				this.myIndex++;
				if(((new Date()).time - startTime.time) > 40) {
					if(Main.debugMode)
						trace("Links processed:" + idx);
					return;
				}
			}
			
			myState = DONE;
			return;
		}
		
		private var detectedInputRspecVersion:Number;
		public function processSliverRspec(s:Sliver):void
		{
			var defaultNamespace:Namespace = s.rspec.namespace();
			switch(defaultNamespace.uri) {
				case XmlUtil.rspec01Namespace:
					this.detectedInputRspecVersion = 0.1;
					break;
				case XmlUtil.rspec02Namespace:
					this.detectedInputRspecVersion = 0.2;
					break;
				case XmlUtil.rspec2Namespace:
					this.detectedInputRspecVersion = 2;
					break;
			}
			
			if(s.rspec.@valid_until.length() == 1)
				s.expires = Util.parseProtogeniDate(s.rspec.@valid_until);
			if(s.rspec.@expires.length() == 1)
				s.expires = Util.parseProtogeniDate(s.rspec.@expires);
			
			s.nodes = new VirtualNodeCollection();
			s.links = new VirtualLinkCollection();
			
			var nodesById:Dictionary = new Dictionary();
			var interfacesById:Dictionary = new Dictionary();
			
			var localName:String;
			
			for each(var nodeXml:XML in s.rspec.defaultNamespace::node)
			{
				var cmNode:PhysicalNode;
				var virtualNode:VirtualNode = new VirtualNode(s);
				if(this.detectedInputRspecVersion < 1)
				{
					var managerIdString:String = "";
					var componentIdString:String = "";
					if(nodeXml.@component_manager_uuid.length() == 1)
						managerIdString = nodeXml.@component_manager_uuid;
					if(nodeXml.@component_manager_urn.length() == 1)
						managerIdString = nodeXml.@component_manager_urn;
					// Don't add outside nodes ... do that if found when parsing links ...
					virtualNode.manager = Main.geniHandler.GeniManagers.getByUrn(managerIdString);
					if(virtualNode.manager != s.manager)
						continue;
					
					if(nodeXml.@component_uuid.length() == 1)
						componentIdString = nodeXml.@component_uuid;
					if(nodeXml.@component_urn.length() == 1)
						componentIdString = nodeXml.@component_urn;
					if(componentIdString.length > 0) {
						cmNode = s.manager.Nodes.GetByUrn(componentIdString);
						if(cmNode == null)
							continue;
						virtualNode.setToPhysicalNode(cmNode);
					}
				}
				else
				{
					// Don't add outside nodes ... do that if found when parsing links ...
					virtualNode.manager = Main.geniHandler.GeniManagers.getByUrn(nodeXml.@component_manager_id);
					if(virtualNode.manager != s.manager)
						continue;
					
					if(nodeXml.@component_id.length() == 1) {
						cmNode = s.manager.Nodes.GetByUrn(nodeXml.@component_id);
						// Don't add outside nodes ... do that if found when parsing links ...
						if(cmNode == null)
							continue;
						virtualNode.setToPhysicalNode(cmNode);
					}
				}
				
				if(this.detectedInputRspecVersion < 1) {
					virtualNode.clientId = nodeXml.@virtual_id;
					if(virtualNode.manager == null)
						continue;
					virtualNode._exclusive = nodeXml.@exclusive == "1";
					if(nodeXml.@sliver_urn.length() == 1)
						virtualNode.sliverId = nodeXml.@sliver_urn;
					if(nodeXml.@sliver_uuid.length() == 1)
						virtualNode.sliverId = nodeXml.@sliver_uuid;
					if(nodeXml.@sshdport.length() == 1) {
						if(virtualNode.loginServices.length == 0)
							virtualNode.loginServices.push(new LoginService());
						virtualNode.loginServices[0].port = nodeXml.@sshdport;
					}
					if(nodeXml.@hostname.length() == 1) {
						if(virtualNode.loginServices.length == 0)
							virtualNode.loginServices.push(new LoginService());
						virtualNode.loginServices[0].hostname = nodeXml.@hostname;
					}
					if(nodeXml.@virtualization_type.length() == 1)
						virtualNode.virtualizationType = nodeXml.@virtualization_type;
					if(nodeXml.@virtualization_subtype.length() == 1)
						virtualNode.virtualizationSubtype = nodeXml.@virtualization_subtype;
				} else {
					virtualNode.clientId = nodeXml.@client_id;
					if(virtualNode.manager == null)
						continue;
					virtualNode._exclusive = nodeXml.@exclusive == "true";
					virtualNode.sliverId = nodeXml.@sliver_id;
				}
				
				var sliverTypeShapingXml:XML = null;
				for each(var nodeChildXml:XML in nodeXml.children()) {
					localName = nodeChildXml.localName();
					// default namespace stuff
					if(nodeChildXml.namespace() == defaultNamespace) {
						switch(localName) {
							case "interface":
								var virtualInterface:VirtualInterface = new VirtualInterface(virtualNode);
								if(this.detectedInputRspecVersion < 1) {
									virtualInterface.id = nodeChildXml.@virtual_id;
								} else {
									virtualInterface.id = nodeChildXml.@client_id;
									for each(var ipXml:XML in nodeChildXml.children()) {
										if(ipXml.localName() == "ip") {
											virtualInterface.ip = ipXml.@address;
											virtualInterface.mask = ipXml.@mask;
											virtualInterface.type = ipXml.@type;
										}
									}
									if(virtualNode.physicalNode != null)
										virtualInterface.physicalNodeInterface = virtualNode.physicalNode.interfaces.GetByID(nodeChildXml.@component_id, false);
								}
								virtualNode.interfaces.add(virtualInterface);
								interfacesById[virtualInterface.id] = virtualInterface;
								break;
							case "disk_image":
								virtualNode.diskImage = nodeChildXml.@name;
								break;
							case "sliver_type":
								virtualNode.sliverType = nodeChildXml.@name;
								for each(var sliverTypeChild:XML in nodeChildXml.children()) {
									if(sliverTypeChild.localName() == "disk_image")
										virtualNode.diskImage = sliverTypeChild.@name;
									else if(sliverTypeChild.localName() == "sliver_type_shaping")
										sliverTypeShapingXml = sliverTypeChild;
								}
								break;
							case "services":
								for each(var servicesChild:XML in nodeChildXml.children()) {
								if(servicesChild.localName() == "login")
									virtualNode.loginServices.push(new LoginService(servicesChild.@authentication,
																					servicesChild.@hostname,
																					servicesChild.@port,
																					servicesChild.@username));
								else if(servicesChild.localName() == "install")
									virtualNode.installServices.push(new InstallService(servicesChild.@url,
																						servicesChild.@install_path,
																						servicesChild.@file_type));
								else if(servicesChild.localName() == "execute")
									virtualNode.executeServices.push(new ExecuteService(servicesChild.@command,
																						servicesChild.@shell));
							}
								break;
						}
					}
						// Extension stuff
					else if(nodeChildXml.namespace() == XmlUtil.flackNamespace)
					{
						virtualNode.flackX = int(nodeChildXml.@x);
						virtualNode.flackY = int(nodeChildXml.@y);
						if(nodeChildXml.@unbound.length() == 1)
							virtualNode.flackUnbound = nodeChildXml.@unbound == "true";
					}
				}
				
				if(sliverTypeShapingXml != null) {
					for each(var pipeXml:XML in sliverTypeShapingXml.children()) {
						virtualNode.pipes.add(new Pipe(virtualNode.interfaces.GetByID(pipeXml.@source),
							virtualNode.interfaces.GetByID(pipeXml.@dest),
							pipeXml.@capacity.length() == 1 ? Number(pipeXml.@capacity) : NaN,
							pipeXml.@latency.length() == 1 ? Number(pipeXml.@latency) : NaN,
							pipeXml.@packet_loss.length() == 1 ? Number(pipeXml.@packet_loss) : NaN));
					}
				}
				
				virtualNode.rspec = nodeXml;
				s.nodes.add(virtualNode);
				nodesById[virtualNode.clientId] = virtualNode;
				if(virtualNode.physicalNode != null)
					virtualNode.physicalNode.virtualNodes.add(virtualNode);
			}
			
			for each(var vn:VirtualNode in s.nodes.collection)
			{
				if(vn.physicalNode != null && vn.physicalNode.subNodeOf != null)
				{
					vn.superNode = s.nodes.getById(vn.physicalNode.subNodeOf.name);
					s.nodes.getById(vn.physicalNode.subNodeOf.name).subNodes.add(vn);
				}
			}
			
			for each(var linkXml:XML in s.rspec.defaultNamespace::link)
			{
				var virtualLink:VirtualLink = new VirtualLink();
				if(this.detectedInputRspecVersion < 1) {
					virtualLink.clientId = linkXml.@virtual_id;
					virtualLink.sliverId = linkXml.@sliver_urn;
					virtualLink.type = linkXml.@link_type;
				} else {
					virtualLink.clientId = linkXml.@client_id;
					virtualLink.sliverId = linkXml.@sliver_id;
					// vlantag?
				}
				
				for each(var linkChildXml:XML in linkXml.children()) {
					if(virtualLink == null)
						break;
					localName = linkChildXml.localName();
					// default namespace stuff
					if(linkChildXml.namespace() == defaultNamespace) {
						switch(localName) {
							case "bandwidth":
								virtualLink.capacity = Number(linkChildXml.toString());
								break;
							case "property":
								/*if(linkChildXml.@latency.length() == 1)
								virtualLink.latency = Number(linkChildXml.@latency);
								if(linkChildXml.@packet_loss.length() == 1)
								virtualLink.packetLoss = Number(linkChildXml.@packet_loss);*/
								if(linkChildXml.@capacity.length() == 1)
									virtualLink.capacity = Number(linkChildXml.@capacity);
								//<property source_id="urn:publicid:IDN+emulab.net+interface+cisco8:(null)" dest_id="urn:publicid:IDN+emulab.net+interface+pc219:eth2" capacity="1000000" latency="0" packet_loss="0"/>
								break;
							case "interface_ref":
								if(this.detectedInputRspecVersion < 1) {
									var vid:String = linkChildXml.@virtual_interface_id;
									var nid:String = linkChildXml.@virtual_node_id;
									var interfacedNode:VirtualNode = nodesById[nid];
									// Deal with outside node
									if(interfacedNode == null)
									{
										// Get outside node, don't add if not parsed in the other cm yet
										interfacedNode = s.slice.getVirtualNodeWithId(nid);
										if(interfacedNode == null
											|| !(interfacedNode.sliver.created
												|| interfacedNode.sliver.staged))
										{
											virtualLink = null;
											break;
										}
									}
									for each(var vi:VirtualInterface in interfacedNode.interfaces.collection)
									{
										if(vi.id == vid)
										{
											if(linkChildXml.@tunnel_ip.length() == 1)
												vi.ip = linkChildXml.@tunnel_ip;
											virtualLink.interfaces.add(vi);
											vi.virtualLinks.add(virtualLink);
											if(!virtualLink.slivers.contains(interfacedNode.sliver))
												virtualLink.slivers.add(interfacedNode.sliver);
											break;
										}
									}
								} else {
									var niid:String = linkChildXml.@client_id;
									var interfacedNodeInterface:VirtualInterface = interfacesById[niid];
									// Deal with outside node
									if(interfacedNodeInterface == null)
									{
										// Get outside node, don't add if not parsed in the other cm yet
										interfacedNodeInterface = s.slice.getVirtualInterfaceWithId(niid);
										if(interfacedNodeInterface == null
											|| !(interfacedNodeInterface.owner.sliver.created
												|| interfacedNodeInterface.owner.sliver.staged))
										{
											virtualLink = null;
											break;
										}
									}
									if(virtualLink != null) {
										virtualLink.interfaces.add(interfacedNodeInterface);
										interfacedNodeInterface.virtualLinks.add(virtualLink);
										if(!virtualLink.slivers.contains(interfacedNodeInterface.owner.sliver))
											virtualLink.slivers.add(interfacedNodeInterface.owner.sliver);
									}
								}
								break;
							case "component_hop":
								// not a tunnel
								var testString:String = linkChildXml.toXMLString();
								if(testString.indexOf("gpeni") != -1)
									virtualLink.linkType = VirtualLink.TYPE_GPENI;
								else if(testString.indexOf("ion") != -1)
									virtualLink.linkType = VirtualLink.TYPE_ION;
								break;
						}
					}
				}
				
				if(virtualLink == null)
					continue;
				
				// Make sure this sliver actually uses this link
				if(!virtualLink.slivers.contains(s))
					continue;
				
				virtualLink.rspec = linkXml;
				
				// Make sure nodes and link are in all the slivers
				for each(var checkSliver:Sliver in virtualLink.slivers.collection) {
					if(!checkSliver.links.contains(virtualLink))
						checkSliver.links.add(virtualLink);
					for each(var checkInterfaceExisting:VirtualInterface in virtualLink.interfaces.collection) {
						if(!checkSliver.nodes.contains(checkInterfaceExisting.owner))
							checkSliver.nodes.add(checkInterfaceExisting.owner);
					}
				}
				
				for each(var checkInterface:VirtualInterface in virtualLink.interfaces.collection) {
					// Deal with links between managers
					if(checkInterface.owner.manager != s.manager) {
						
						if(virtualLink.linkType == VirtualLink.TYPE_NORMAL)
							virtualLink.linkType = VirtualLink.TYPE_TUNNEL;
						
						if(!checkInterface.owner.slivers.contains(s))
							checkInterface.owner.slivers.add(s);
						
						for each(var checkOtherInterface:VirtualInterface in virtualLink.interfaces.collection) {
							if(checkOtherInterface != checkInterface
								&& !checkOtherInterface.owner.slivers.contains(checkInterface.owner.sliver)) {
								checkOtherInterface.owner.slivers.add(checkInterface.owner.sliver);
							}
							
						}
					}
				}
			}
			
			return;
		}
		
		public function generateSliverRspec(s:Sliver,
											removeNonexplicitBinding:Boolean,
											overrideRspecVersion:Number):XML
		{
			var requestRspec:XML = new XML("<?xml version=\"1.0\" encoding=\"UTF-8\"?><rspec type=\"request\" />");
			
			// Add namespaces
			var defaultNamespace:Namespace;
			var useInputRspecVersion:Number = manager.inputRspecVersion;
			if(overrideRspecVersion)
				useInputRspecVersion = overrideRspecVersion;
			switch(useInputRspecVersion) {
				case 0.1:
					defaultNamespace = new Namespace(null, XmlUtil.rspec01Namespace);
					break;
				case 0.2:
					defaultNamespace = new Namespace(null, XmlUtil.rspec02Namespace);
					break;
				case 2:
					defaultNamespace = new Namespace(null, XmlUtil.rspec2Namespace);
					break;
			}
			requestRspec.setNamespace(defaultNamespace);
			if(useInputRspecVersion >= 2)
				requestRspec.addNamespace(XmlUtil.flackNamespace);
			var xsiNamespace:Namespace = XmlUtil.xsiNamespace;
			requestRspec.addNamespace(xsiNamespace);
			
			// Add schema locations
			var schemaLocations:String;
			switch(useInputRspecVersion) {
				case 0.1:
					schemaLocations = XmlUtil.rspec01SchemaLocation;
					break;
				case 0.2:
					schemaLocations = XmlUtil.rspec02SchemaLocation;
					break;
				case 2:
					schemaLocations = XmlUtil.rspec2SchemaLocation;
					break;
			}
			for each(var testNode:VirtualNode in s.nodes.collection) {
				if(testNode.isDelayNode) {
					requestRspec.addNamespace(XmlUtil.delayNamespace);
					schemaLocations += " " + XmlUtil.delaySchemaLocation;
					break;
				}
			}
			requestRspec.@xsiNamespace::schemaLocation = schemaLocations;
			// FIXME: Need to add schemaLocation and namespaces if they were there before...
			// TOHERE
			
			for each(var vn:VirtualNode in s.nodes.collection) {
				requestRspec.appendChild(generateNodeRspec(vn, removeNonexplicitBinding, useInputRspecVersion));
			}
			
			for each(var vl:VirtualLink in s.links.collection) {
				requestRspec.appendChild(generateLinkRspec(vl, useInputRspecVersion));
			}
			
			return requestRspec;
		}
		
		public function generateNodeRspec(vn:VirtualNode,
										  removeNonexplicitBinding:Boolean,
										  overrideRspecVersion:Number = NaN):XML
		{
			
			var useInputRspecVersion:Number = manager.inputRspecVersion;
			if(overrideRspecVersion)
				useInputRspecVersion = overrideRspecVersion;
			
			var nodeXml:XML = <node />;
			if(useInputRspecVersion < 1) {
				nodeXml.@virtual_id = vn.clientId;
				nodeXml.@component_manager_uuid = vn.manager.Urn.full;
				nodeXml.@virtualization_type = vn.virtualizationType;
			} else {
				nodeXml.@client_id = vn.clientId;
				nodeXml.@component_manager_id = vn.manager.Urn.full;
			}
			
			if (vn.IsBound() &&
					!(removeNonexplicitBinding && vn.flackUnbound)) {
				if(useInputRspecVersion < 1) {
					nodeXml.@component_uuid = vn.physicalNode.id;
				} else {
					nodeXml.@component_id = vn.physicalNode.id;
				}
			}
			
			if (!vn.Exclusive)
			{
				if(useInputRspecVersion < 1) {
					nodeXml.@virtualization_subtype = vn.virtualizationSubtype;
					nodeXml.@exclusive = 0;
				} else {
					nodeXml.@exclusive = "false";
				}
			}
			else {
				if(useInputRspecVersion < 1)
					nodeXml.@exclusive = 1;
				else
					nodeXml.@exclusive = "true";
			}
			
			if(useInputRspecVersion < 2) {
				var nodeType:String = "pc";
				if (!vn.Exclusive)
					nodeType = "pcvm";
				var nodeTypeXml:XML = <node_type />;
				nodeTypeXml.@type_name = nodeType;
				nodeTypeXml.@type_slots = 1;
				nodeXml.appendChild(nodeTypeXml);
				
				if(vn.diskImage.length > 0 && useInputRspecVersion > 0.1) {
					var diskImageXml:XML = <disk_image />;
					diskImageXml.@name = vn.diskImage;
					nodeXml.appendChild(diskImageXml);
				}
			} else {
				var sliverType:XML = <sliver_type />
				sliverType.@name = vn.sliverType;
				if(vn.isDelayNode) {
					var sliverTypeShapingXml:XML = <sliver_type_shaping />;
					sliverTypeShapingXml.setNamespace(XmlUtil.delayNamespace);
					//sliverTypeShapingXml.@xmlns = XmlUtil.delayNamespace.uri;
					for each(var pipe:Pipe in vn.pipes.collection) {
						var pipeXml:XML = <pipe />;
						pipeXml.setNamespace(XmlUtil.delayNamespace);
						pipeXml.@source = pipe.source.id;
						pipeXml.@dest = pipe.destination.id;
						if(pipe.capacity)
							pipeXml.@capacity = pipe.capacity;
						if(pipe.packetLoss)
							pipeXml.@packet_loss = pipe.packetLoss;
						if(pipe.latency)
							pipeXml.@latency = pipe.latency;
						sliverTypeShapingXml.appendChild(pipeXml);
					}
					sliverType.appendChild(sliverTypeShapingXml);
				}
				if(vn.diskImage.length > 0) {
					var sliverDiskImageXml:XML = <disk_image />;
					sliverDiskImageXml.@name = vn.diskImage;
					sliverType.appendChild(sliverDiskImageXml);
				}
				nodeXml.appendChild(sliverType);
			}
			
			// Services
			if(useInputRspecVersion < 1) {
				if(vn.executeServices.length > 0) {
					nodeXml.@startup_command = vn.executeServices[0].command;
				}
				if(vn.installServices.length > 0) {
					nodeXml.@tarfiles = vn.installServices[0].url;
				}
			} else {
				var serviceXml:XML = <services />;
				for each(var executeService:ExecuteService in vn.executeServices) {
					var executeXml:XML = <execute />;
					executeXml.@command = executeService.command;
					executeXml.@shell = executeService.shell;
					serviceXml.appendChild(executeXml);
				}
				for each(var installService:InstallService in vn.installServices) {
					var installXml:XML = <install />;
					installXml.@install_path = installService.installPath;
					installXml.@url = installService.url;
					installXml.@file_type = installService.fileType;
					serviceXml.appendChild(installXml);
				}
				if(serviceXml.children().length() > 0)
					nodeXml.appendChild(serviceXml);
			}
			
			if (useInputRspecVersion < 1 && vn.superNode != null)
				nodeXml.appendChild(XML("<subnode_of>" + vn.superNode.sliverId + "</subnode_of>"));
			
			for each (var current:VirtualInterface in vn.interfaces.collection)
			{
				if(useInputRspecVersion >= 2 && current.id == "control")
					continue;
				var interfaceXml:XML = <interface />;
				if(useInputRspecVersion < 1)
					interfaceXml.@virtual_id = current.id;
				else {
					interfaceXml.@client_id = current.id;
					for each(var currentLink:VirtualLink in current.virtualLinks.collection) {
						if(currentLink.linkType == VirtualLink.TYPE_TUNNEL) {
							var tunnelXml:XML = <ip />;
							tunnelXml.@address = current.ip;
							tunnelXml.@mask = "255.255.255.0";
							tunnelXml.@type = "ipv4";
							interfaceXml.appendChild(tunnelXml);
						}
					}
				}
				nodeXml.appendChild(interfaceXml);
			}
			
			if(useInputRspecVersion >= 2) {
				var flackXml:XML = <info />;
				flackXml.setNamespace(XmlUtil.flackNamespace);
				flackXml.@x = vn.flackX;
				flackXml.@y = vn.flackY;
				flackXml.@unbound = vn.flackUnbound;
				nodeXml.appendChild(flackXml);
			}
			
			return nodeXml;
		}
		
		public function generateLinkRspec(vl:VirtualLink,
										  overrideRspecVersion:Number = NaN):XML
		{
			var useInputRspecVersion:Number = manager.inputRspecVersion;
			if(overrideRspecVersion)
				useInputRspecVersion = overrideRspecVersion;
			
			var linkXml:XML =  <link />;
			
			if(useInputRspecVersion < 1)
				linkXml.@virtual_id = vl.clientId;
			else
				linkXml.@client_id = vl.clientId;
			
			var s:Sliver;
			if(useInputRspecVersion >= 2) {
				for each(s in vl.slivers.collection) {
					var cmXml:XML = <component_manager />;
					cmXml.@name = s.manager.Urn.full;
					linkXml.appendChild(cmXml);
				}
			}
			
			if (vl.linkType != VirtualLink.TYPE_TUNNEL && vl.capacity != -1) {
				if(useInputRspecVersion < 1)
					linkXml.appendChild(XML("<bandwidth>" + vl.capacity + "</bandwidth>"));
				else {
					for(var i:int = 0; i < vl.interfaces.length; i++) {
						for(var j:int = i+1; j < vl.interfaces.length; j++) {
							var sourcePropertyXml:XML = <property />;
							sourcePropertyXml.@source_id = vl.interfaces.collection[i].id;
							sourcePropertyXml.@dest_id = vl.interfaces.collection[j].id;
							sourcePropertyXml.@capacity = vl.capacity;
							linkXml.appendChild(sourcePropertyXml);
							var destPropertyXml:XML = <property />;
							destPropertyXml.@source_id = vl.interfaces.collection[j].id;
							destPropertyXml.@dest_id = vl.interfaces.collection[i].id;
							destPropertyXml.@capacity = vl.capacity;
							linkXml.appendChild(destPropertyXml);
						}
					}
				}
			}
			
			if (vl.linkType == VirtualLink.TYPE_TUNNEL)
			{
				if(useInputRspecVersion < 1)
					linkXml.@link_type = "tunnel";
				else {
					var link_type:XML = <link_type />;
					link_type.@name = "gre-tunnel";
					linkXml.appendChild(link_type);
				}
			}
			
			for each (var currentVi:VirtualInterface in vl.interfaces.collection)
			{
				if(useInputRspecVersion >= 2 && currentVi.id == "control")
					continue;
				var interfaceRefXml:XML = <interface_ref />;
				if(useInputRspecVersion < 1) {
					interfaceRefXml.@virtual_node_id = currentVi.owner.clientId;
					if (vl.linkType == VirtualLink.TYPE_TUNNEL)
					{
						interfaceRefXml.@tunnel_ip = currentVi.ip;
						interfaceRefXml.@virtual_interface_id = "control";
					}
					else
						interfaceRefXml.@virtual_interface_id = currentVi.id;
				} else {
					interfaceRefXml.@client_id = currentVi.id;
				}
				
				linkXml.appendChild(interfaceRefXml);
			}
			
			if(vl.linkType == VirtualLink.TYPE_ION) {
				for each(s in vl.slivers.collection) {
					var componentHopIonXml:XML = <component_hop />;
					componentHopIonXml.@component_urn = IdnUrn.makeFrom(s.manager.Urn.authority, "link", "ion").full;
					interfaceRefXml = <interface_ref />;
					interfaceRefXml.@component_node_urn = IdnUrn.makeFrom(s.manager.Urn.authority, "node", "ion").full;
					interfaceRefXml.@component_interface_id = "eth0";
					componentHopIonXml.appendChild(interfaceRefXml);
					linkXml.appendChild(componentHopIonXml);
				}
			} else if(vl.linkType == VirtualLink.TYPE_GPENI) {
				for each(s in vl.slivers.collection) {
					var componentHopGpeniXml:XML = <component_hop />;
					componentHopGpeniXml.@component_urn = IdnUrn.makeFrom(s.manager.Urn.authority,
																			"link",
																			"gpeni").full;
					interfaceRefXml = <interface_ref />;
					interfaceRefXml.@component_node_urn = IdnUrn.makeFrom(s.manager.Urn.authority,
																			"node",
																			"gpeni").full;
					interfaceRefXml.@component_interface_id = "eth0";
					componentHopGpeniXml.appendChild(interfaceRefXml);
					linkXml.appendChild(componentHopGpeniXml);
				}
			}
			
			return linkXml;
		}
	}
}