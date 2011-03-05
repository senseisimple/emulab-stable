package protogeni.resources
{
	import flash.events.Event;
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Alert;
	import mx.core.FlexGlobals;
	
	import protogeni.GeniEvent;
	import protogeni.Util;
	import protogeni.communication.CommunicationUtil;
	

	public class ProtogeniRspecProcessor implements RspecProcessorInterface
	{
		public function ProtogeniRspecProcessor(newGm:ProtogeniComponentManager)
		{
			gm = newGm;
		}
		
		private var gm:ProtogeniComponentManager;
		
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
		
		private var defaultNamespace:Namespace;
		
		private var detectedOutputRspecVersion:Number;
		public function processResourceRspec(afterCompletion : Function):void {
			gm.clearComponents();
			
			Main.Application().setStatus("Parsing " + gm.Hrn + " RSPEC", false);

			defaultNamespace = gm.Rspec.namespace();
			
			switch(defaultNamespace.uri) {
				case CommunicationUtil.rspec01Namespace:
					detectedOutputRspecVersion = 0.1;
					break;
				case CommunicationUtil.rspec02Namespace:
					detectedOutputRspecVersion = 0.2;
					break;
				case CommunicationUtil.rspec2Namespace:
					detectedOutputRspecVersion = 2;
					break;
				default:
					var msg:String = "RSPEC with the namespace '" +defaultNamespace.uri+ "' is not supported.";
					Alert.show(msg);
					LogHandler.appendMessage(new LogMessage(gm.Url, "Unsupported RSPEC", msg, true, LogMessage.TYPE_END));
					// FIXME: probably needs to error in a different way...
					gm.Status = GeniManager.STATUS_FAILED;
					afterCompletion();
					return;
			}
			
			if(gm.Rspec.@valid_until.length() == 1)
				gm.expires = Util.parseProtogeniDate(gm.Rspec.@valid_until);
			if(gm.Rspec.@expires.length() == 1)
				gm.expires = Util.parseProtogeniDate(gm.Rspec.@expires);
			if(gm.Rspec.@generated.length() == 1)
				gm.generated = Util.parseProtogeniDate(gm.Rspec.@generated);
			
			nodes = gm.Rspec.defaultNamespace::node;
			links = gm.Rspec.defaultNamespace::link;
			
			myAfter = afterCompletion;
			myIndex = 0;
			myState = NODE_PARSE;
			interfaceDictionary = new Dictionary();
			nodeNameDictionary = new Dictionary();
			subnodeList = new ArrayCollection();
			FlexGlobals.topLevelApplication.stage.addEventListener(Event.ENTER_FRAME, parseNext);
		}
		
		private function parseNext(event:Event) : void
		{
			if(!hasslot && GeniManager.processing > GeniManager.maxProcessing)
				return;
			if(!hasslot) {
				hasslot = true;
				GeniManager.processing++;
			}
			
			var startTime:Date = new Date();
			if (myState == NODE_PARSE)	    	
			{
				parseNextNode();
				if(Main.debugMode)
					trace("ParseN " + String((new Date()).time - startTime.time));
			}
			else if (myState == LINK_PARSE)
			{
				parseNextLink();
				if(Main.debugMode)
					trace("ParseL " + String((new Date()).time - startTime.time));
			}
			else if (myState == DONE)
			{
				GeniManager.processing--;
				Main.Application().setStatus("Parsing " + gm.Hrn + " RSPEC Done",false);
				interfaceDictionary = null;
				nodeNameDictionary = null;
				subnodeList = null;
				gm.totalNodes = gm.AllNodes.length;
				gm.availableNodes = gm.getAvailableNodes().length;
				gm.unavailableNodes = gm.AllNodes.length - gm.availableNodes;
				gm.percentageAvailable = (gm.availableNodes / gm.totalNodes) * 100;
				gm.Status = GeniManager.STATUS_VALID;
				Main.geniDispatcher.dispatchGeniManagerChanged(gm, GeniEvent.ACTION_POPULATED);
				Main.Application().stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				myAfter();
			}
			else
			{
				Main.Application().setStatus("Fail",true);
				interfaceDictionary = null;
				nodeNameDictionary = null;
				subnodeList = null;
				Main.Application().stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				Alert.show("Problem parsing RSPEC");
				// Throw exception
			}
		}
		
		private function parseNextNode():void {
			var startTime:Date = new Date();
			var idx:int = 0;
			
			while(myIndex < nodes.length()) {
				try {
					var p:XML = nodes[myIndex];
					
					// Get location info
					var lat:Number = -72.134678;
					var lng:Number = 170.332031;
					var locationXml:XML = p.defaultNamespace::location[0];
					if(Number(locationXml.@latitude) != 0 && Number(locationXml.@longitude) != 0)
					{
						lat = Number(locationXml.@latitude);
						lng = Number(locationXml.@longitude);
					}
					
					// Assign to a group based on location
					var ng:PhysicalNodeGroup = gm.Nodes.GetByLocation(lat,lng);
					if(ng == null) {
						ng = new PhysicalNodeGroup(lat, lng, locationXml.@country, gm.Nodes);
						gm.Nodes.Add(ng);
					}
					
					var n:PhysicalNode = new PhysicalNode(ng, gm);
					n.name = p.@component_name;
					switch(detectedOutputRspecVersion)
					{
						case 0.1:
						case 0.2:
							n.id = p.@component_uuid;
							break;
						case 2:
						default:
							n.id = p.@component_id;
							n.exclusive = p.@exclusive == "true";
					}
					
					for each(var ix:XML in p.children()) {
						if(ix.namespace() == defaultNamespace) {
							if(ix.localName() == "interface") {
								var i:PhysicalNodeInterface = new PhysicalNodeInterface(n);
								i.id = ix.@component_id;
								i.role = PhysicalNodeInterface.RoleIntFromString(ix.@role);
								i.publicIPv4 = ix.@public_ipv4;
								n.interfaces.Add(i);
								interfaceDictionary[i.id] = i;
							} else if(ix.localName() == "hardware_type") {
								n.hardwareTypes.push(ix.@name);
							} else if(ix.localName() == "sliver_type") {
								var newSliverType:SliverType = new SliverType(ix.@name);
								for each(var ixx:XML in ix.children()) {
									if(ixx.localName() == "disk_image")
										newSliverType.diskImages.push(new DiskImage(ixx.@name,
																					ixx.@os,
																					ixx.@version,
																					ixx.@description,
																					ixx.@default == "true"));
								}
								n.sliverTypes.push(newSliverType);
							} else if(ix.localName() == "available") {
								switch(detectedOutputRspecVersion)
								{
									case 0.1:
									case 0.2:
										n.available = ix.toString() == "true";
										break;
									case 2:
									default:
										n.available = ix.@now == "true";
								}
							} else if(ix.localName() == "relation") {
								var relationType:String = ix.@type;
								if(relationType == "subnode_of") {
									var pararentId:String = ix.@component_id;
									if(pararentId.length > 0)
										subnodeList.addItem({subNode:n, parentName:pararentId});
								}
							} else if(ix.localName() == "disk_image") {
								if(n.sliverTypes.length == 0)
									n.sliverTypes.push(new SliverType("N/A"));
								n.sliverTypes[0].diskImages.push(new DiskImage(ix.@name,
																				ix.@os,
																				ix.@version,
																				ix.@description,
																				ix.@default == "true"));
							}
							// Depreciated
							else if(ix.localName() == "exclusive") {
								var exString:String = ix.toString();
								n.exclusive = exString == "true";
							} else if(ix.localName() == "subnode_of") {
								var parentName:String = ix.toString();
								if(parentName.length > 0)
									subnodeList.addItem({subNode:n, parentName:parentName});
							} else if(ix.localName() == "node_type") {
								n.hardwareTypes.push(ix.@type_name);
							}
						}
					}
					
					n.rspec = p;
					ng.Add(n);
					nodeNameDictionary[n.id] = n;
					nodeNameDictionary[n.name] = n;
					gm.AllNodes.push(n);
				} catch(e:Error) {
					// skip if some problem
					if(Main.debugMode)
						trace("Skipped node which threw error in manager named " + gm.Hrn);
				}
				idx++;
				myIndex++;
				if(((new Date()).time - startTime.time) > 40) {
					if(Main.debugMode)
						trace("Nodes processed:" + idx);
					return;
				}
			}
			
			// Assign subnodes
			for each(var obj:Object in subnodeList)
			{
				var parentNode:PhysicalNode = nodeNameDictionary[obj.parentName];
				parentNode.subNodes.push(obj.subNode);
				obj.subNode.subNodeOf = parentNode;
			}
			
			myState = LINK_PARSE;
			myIndex = 0;
			return;
		}
		
		private function parseNextLink():void {
			var startTime:Date = new Date();
			var idx:int = 0;
			
			while(myIndex < links.length()) {
				try {
					var link:XML = links[myIndex];
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
						var referencedInterface:PhysicalNodeInterface = interfaceDictionary[interfaceId];
						if(referencedInterface != null)
							l.interfaceRefs.Add(interfaceDictionary[interfaceId]);
						else {
							// error
						}
					}
					
					var lg:PhysicalLinkGroup = gm.Links.Get(l.interfaceRefs.collection[0].owner.GetLatitude(), l.interfaceRefs.collection[0].owner.GetLongitude(), l.interfaceRefs.collection[1].owner.GetLatitude(), l.interfaceRefs.collection[1].owner.GetLongitude());
					if(lg == null) {
						lg = new PhysicalLinkGroup(l.interfaceRefs.collection[0].owner.GetLatitude(), l.interfaceRefs.collection[0].owner.GetLongitude(), l.interfaceRefs.collection[1].owner.GetLatitude(), l.interfaceRefs.collection[1].owner.GetLongitude(), gm.Links);
						gm.Links.Add(lg);
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
					l.manager = gm;
					
					for each(var ix:XML in link.children()) {
						if(ix.namespace() == defaultNamespace) {
							 if(ix.localName() == "property") {
								// FIXME: Hack since v1 just had link properties instead of dst & src
								l.latency = Number(ix.@latency);
								l.packetLoss = Number(ix.@packet_loss);
								l.bandwidth = Number(ix.@capacity);
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
								l.bandwidth = Number(ix);
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
					gm.AllLinks.push(l);
				} catch(e:Error) {
					// skip if some problem
					if(Main.debugMode)
						trace("Skipped link which threw error in manager named " + gm.Hrn);
				}
				idx++;
				myIndex++;
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
					case CommunicationUtil.rspec01Namespace:
						detectedInputRspecVersion = 0.1;
						break;
					case CommunicationUtil.rspec02Namespace:
						detectedInputRspecVersion = 0.2;
						break;
					case CommunicationUtil.rspec2Namespace:
						detectedInputRspecVersion = 2;
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
					if(detectedInputRspecVersion < 1)
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
					
					if(detectedInputRspecVersion < 1) {
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
					
					for each(var ix:XML in nodeXml.children()) {
						localName = ix.localName();
						// default namespace stuff
						if(ix.namespace() == defaultNamespace) {
							switch(localName) {
								case "interface":
									var virtualInterface:VirtualInterface = new VirtualInterface(virtualNode);
									if(detectedInputRspecVersion < 1) {
										virtualInterface.id = ix.@virtual_id;
									} else {
										virtualInterface.id = ix.@client_id;
										for each(var ipXml:XML in ix.children()) {
											if(ipXml.localName() == "ip") {
												virtualInterface.ip = ipXml.@address;
												virtualInterface.mask = ipXml.@mask;
												virtualInterface.type = ipXml.@type;
											}
										}
										if(virtualNode.physicalNode != null)
											virtualInterface.physicalNodeInterface = virtualNode.physicalNode.interfaces.GetByID(ix.@component_id, false);
									}
									virtualNode.interfaces.Add(virtualInterface);
									interfacesById[virtualInterface.id] = virtualInterface;
									break;
								case "disk_image":
									virtualNode.diskImage = ix.@name;
									break;
								case "sliver_type":
									virtualNode.sliverType = ix.@name;
									break;
								case "services":
									for each(var ixx:XML in ix.children()) {
									if(ixx.localName() == "login")
										virtualNode.loginServices.push(new LoginService(ixx.@authentication, ixx.@hostname, ixx.@port, ixx.@username));
									else if(ixx.localName() == "install")
										virtualNode.installServices.push(new InstallService(ixx.@url, ixx.@install_path, ixx.@file_type));
									else if(ixx.localName() == "execute")
										virtualNode.executeServices.push(new ExecuteService(ixx.@command, ixx.@shell));
								}
									break;
							}
						}
						// Extension stuff
						else
						{
							if(ix.localName() == "canvas_location" && (ix.namespace() as Namespace).prefix == "flack") {
								virtualNode.flackX = int(ix.@x);
								virtualNode.flackY = int(ix.@y);
							}
						}
					}
					
					virtualNode.rspec = nodeXml;
					s.nodes.addItem(virtualNode);
					nodesById[virtualNode.clientId] = virtualNode;
					if(virtualNode.physicalNode != null)
						virtualNode.physicalNode.virtualNodes.addItem(virtualNode);
				}
				
				for each(var vn:VirtualNode in s.nodes)
				{
					if(vn.physicalNode != null && vn.physicalNode.subNodeOf != null)
					{
						vn.superNode = s.nodes.getById(vn.physicalNode.subNodeOf.name);
						s.nodes.getById(vn.physicalNode.subNodeOf.name).subNodes.push(vn);
					}
				}
				
				for each(var linkXml:XML in s.rspec.defaultNamespace::link)
				{
					var virtualLink:VirtualLink = new VirtualLink();
					if(detectedInputRspecVersion < 1) {
						virtualLink.clientId = linkXml.@virtual_id;
						virtualLink.sliverId = linkXml.@sliver_urn;
						virtualLink.type = linkXml.@link_type;
					} else {
						virtualLink.clientId = linkXml.@client_id;
						virtualLink.sliverId = linkXml.@sliver_id;
						// vlantag?
					}
					
					for each(var viXml:XML in linkXml.children()) {
						if(virtualLink == null)
							break;
						localName = viXml.localName();
						// default namespace stuff
						if(viXml.namespace() == defaultNamespace) {
							switch(localName) {
								case "bandwidth":
									virtualLink.bandwidth = Number(viXml.toString());
									break;
								case "interface_ref":
									if(detectedInputRspecVersion < 1) {
										var vid:String = viXml.@virtual_interface_id;
										var nid:String = viXml.@virtual_node_id;
										var interfacedNode:VirtualNode = nodesById[nid];
										// Deal with outside node
										if(interfacedNode == null)
										{
											// Get outside node, don't add if not parsed in the other cm yet
											interfacedNode = s.slice.getVirtualNodeWithId(nid);
											if(interfacedNode == null || !(interfacedNode.slivers[0] as Sliver).created)
											{
												virtualLink = null;
												break;
											}
										}
										for each(var vi:VirtualInterface in interfacedNode.interfaces.collection)
										{
											if(vi.id == vid)
											{
												if(viXml.@tunnel_ip.length() == 1)
													vi.ip = viXml.@tunnel_ip;
												virtualLink.interfaces.addItem(vi);
												vi.virtualLinks.addItem(virtualLink);
												if(virtualLink.slivers.indexOf(interfacedNode.slivers[0]) == -1)
													virtualLink.slivers.push(interfacedNode.slivers[0]);
												break;
											}
										}
									} else {
										var niid:String = viXml.@client_id;
										var interfacedNodeInterface:VirtualInterface = interfacesById[niid];
										// Deal with outside node
										if(interfacedNodeInterface == null)
										{
											// Get outside node, don't add if not parsed in the other cm yet
											interfacedNodeInterface = s.slice.getVirtualInterfaceWithId(niid);
											if(interfacedNodeInterface == null || !interfacedNodeInterface.owner.slivers[0].created)
											{
												virtualLink = null;
												break;
											}
										}
										if(virtualLink != null) {
											virtualLink.interfaces.addItem(interfacedNodeInterface);
											interfacedNodeInterface.virtualLinks.addItem(virtualLink);
											if(virtualLink.slivers.indexOf(interfacedNodeInterface.owner.slivers[0]) == -1)
												virtualLink.slivers.push(interfacedNodeInterface.owner.slivers[0]);
										}
									}
									break;
								case "component_hop":
									// not a tunnel
									var testString:String = viXml.toXMLString();
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
					if(virtualLink.slivers.indexOf(s) == -1)
						continue;
					
					virtualLink.rspec = linkXml;
					
					// Make sure nodes and link are in all the slivers
					for each(var checkSliver:Sliver in virtualLink.slivers) {
						if(!checkSliver.links.contains(virtualLink))
							checkSliver.links.addItem(virtualLink);
						for each(var checkInterfaceExisting:VirtualInterface in virtualLink.interfaces) {
							if(!checkSliver.nodes.contains(checkInterfaceExisting.owner))
								checkSliver.nodes.addItem(checkInterfaceExisting.owner);
						}
					}
					
					for each(var checkInterface:VirtualInterface in virtualLink.interfaces) {
						// Deal with links between managers
						if(checkInterface.owner.manager != s.manager) {
							
							if(virtualLink.linkType == VirtualLink.TYPE_NORMAL)
								virtualLink.linkType = VirtualLink.TYPE_TUNNEL;
							
							Util.addIfNonexistingToArrayCollection(checkInterface.owner.slivers, s);
							for each(var checkOtherInterface:VirtualInterface in virtualLink.interfaces) {
								if(checkOtherInterface != checkInterface)
									Util.addIfNonexistingToArrayCollection(checkOtherInterface.owner.slivers, checkInterface.owner.slivers[0]);
							}
						}
					}
				}
				
				return;
		}
		
		public function generateSliverRspec(s:Sliver):XML
		{
			var requestRspec:XML = new XML("<?xml version=\"1.0\" encoding=\"UTF-8\"?><rspec type=\"request\" />");
			var defaultNamespace:Namespace;
			switch(gm.inputRspecVersion) {
				case 0.1:
					defaultNamespace = new Namespace(null, CommunicationUtil.rspec01Namespace);
					break;
				case 0.2:
					defaultNamespace = new Namespace(null, CommunicationUtil.rspec02Namespace);
					break;
				case 2:
					defaultNamespace = new Namespace(null, CommunicationUtil.rspec2Namespace);
					break;
			}
			requestRspec.setNamespace(defaultNamespace);
			var xsiNamespace:Namespace = new Namespace("xsi", "http://www.w3.org/2001/XMLSchema-instance");
			requestRspec.addNamespace(xsiNamespace);
			var schemaLocations:String;
			//var tunnelNamespace:Namespace;
			switch(gm.inputRspecVersion) {
				case 0.1:
					schemaLocations = CommunicationUtil.rspec01SchemaLocation;
					break;
				case 0.2:
					schemaLocations = CommunicationUtil.rspec02SchemaLocation;
					break;
				case 2:
					schemaLocations = CommunicationUtil.rspec2SchemaLocation;
					break;
			}
			requestRspec.@xsiNamespace::schemaLocation = schemaLocations;
			if(gm.inputRspecVersion >= 2) {
				requestRspec.addNamespace(CommunicationUtil.flackNamespace);
			}
			// FIXME: Need to add schemaLocation and namespaces if they were there before...
			// TOHERE

			for each(var vn:VirtualNode in s.nodes) {
				requestRspec.appendChild(generateNodeRspec(vn));
			}
			
			for each(var vl:VirtualLink in s.links) {
				requestRspec.appendChild(generateLinkRspec(vl));
			}
			
			return requestRspec;
		}
		
		public function generateNodeRspec(vn:VirtualNode):XML
		{
			var nodeXml:XML = <node />;
			if(gm.inputRspecVersion < 1) {
				nodeXml.@virtual_id = vn.clientId;
				nodeXml.@component_manager_uuid = vn.manager.Urn;
				nodeXml.@virtualization_type = vn.virtualizationType;
			} else {
				nodeXml.@client_id = vn.clientId;
				nodeXml.@component_manager_id = vn.manager.Urn;
			}
			
			if (vn.IsBound()) {
				if(gm.inputRspecVersion < 1) {
					nodeXml.@component_uuid = vn.physicalNode.id;
				} else {
					nodeXml.@component_id = vn.physicalNode.id;
				}
			}
			
			if (!vn.Exclusive)
			{
				if(gm.inputRspecVersion < 1) {
					nodeXml.@virtualization_subtype = vn.virtualizationSubtype;
					nodeXml.@exclusive = 0;
				} else {
					nodeXml.@exclusive = "false";
				}
			}
			else {
				if(gm.inputRspecVersion < 1)
					nodeXml.@exclusive = 1;
				else
					nodeXml.@exclusive = "true";
			}
			
			// Currently only pcs
			if(gm.inputRspecVersion < 1) {
				var nodeType:String = "pc";
				if (!vn.Exclusive)
					nodeType = "pcvm";
				var nodeTypeXml:XML = <node_type />;
				nodeTypeXml.@type_name = nodeType;
				nodeTypeXml.@type_slots = 1;
				nodeXml.appendChild(nodeTypeXml);
			} else {
				var sliverType:XML = <sliver_type />
				sliverType.@name = vn.sliverType;
				if(vn.diskImage.length > 0) {
					var sliverDiskImageXml:XML = <disk_image />;
					sliverDiskImageXml.@name = vn.diskImage;
					sliverType.appendChild(sliverDiskImageXml);
				}
				nodeXml.appendChild(sliverType);
			}
			
			// Services
			if(gm.inputRspecVersion < 1) {
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
			
			if(gm.inputRspecVersion < 2 && vn.diskImage.length > 0) {
				var diskImageXml:XML = <disk_image />;
				diskImageXml.@name = vn.diskImage;
				nodeXml.appendChild(diskImageXml);
			}
			
			if (gm.inputRspecVersion < 1 && vn.superNode != null)
				nodeXml.appendChild(XML("<subnode_of>" + vn.superNode.sliverId + "</subnode_of>"));
			
			for each (var current:VirtualInterface in vn.interfaces.collection)
			{
				if(gm.inputRspecVersion >= 2 && current.id == "control")
					continue;
				var interfaceXml:XML = <interface />;
				if(gm.inputRspecVersion < 1)
					interfaceXml.@virtual_id = current.id;
				else {
					interfaceXml.@client_id = current.id;
					for each(var currentLink:VirtualLink in current.virtualLinks) {
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
			
			if(gm.inputRspecVersion >= 2) {
				var flackXml:XML = <canvas_location />;
				flackXml.setNamespace(CommunicationUtil.flackNamespace);
				flackXml.@x = vn.flackX;
				flackXml.@y = vn.flackY;
				nodeXml.appendChild(flackXml);
			}
			
			return nodeXml;
		}
		
		public function generateLinkRspec(vl:VirtualLink):XML
		{
			var linkXml:XML =  <link />;
			
			if(gm.inputRspecVersion < 1)
				linkXml.@virtual_id = vl.clientId;
			else
				linkXml.@client_id = vl.clientId;
			
			if(gm.inputRspecVersion >= 2) {
				for each(var s:Sliver in vl.slivers) {
					var cmXml:XML = <component_manager />
					cmXml.@name = s.manager.Urn;
					linkXml.appendChild(cmXml);
				}
			}
			
			if (gm.inputRspecVersion < 1 && vl.linkType != VirtualLink.TYPE_TUNNEL)
				linkXml.appendChild(XML("<bandwidth>" + vl.bandwidth + "</bandwidth>"));
			
			if (vl.linkType == VirtualLink.TYPE_TUNNEL)
			{
				if(gm.inputRspecVersion < 1)
					linkXml.@link_type = "tunnel";
				else {
					var link_type:XML = <link_type />;
					link_type.@name = "gre-tunnel";
					linkXml.appendChild(link_type);
				}
			}
			
			for each (var currentVi:VirtualInterface in vl.interfaces)
			{
				if(gm.inputRspecVersion >= 2 && currentVi.id == "control")
					continue;
				var interfaceRefXml:XML = <interface_ref />;
				if(gm.inputRspecVersion < 1) {
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
				for each(s in vl.slivers) {
					var componentHopIonXml:XML = <component_hop />;
					componentHopIonXml.@component_urn = Util.makeUrn(s.manager.Authority, "link", "ion");
					interfaceRefXml = <interface_ref />;
					interfaceRefXml.@component_node_urn = Util.makeUrn(s.manager.Authority, "node", "ion");
					interfaceRefXml.@component_interface_id = "eth0";
					componentHopIonXml.appendChild(interfaceRefXml);
					linkXml.appendChild(componentHopIonXml);
				}
			} else if(vl.linkType == VirtualLink.TYPE_GPENI) {
				for each(s in vl.slivers) {
					var componentHopGpeniXml:XML = <component_hop />;
					componentHopGpeniXml.@component_urn = Util.makeUrn(s.manager.Authority, "link", "gpeni");
					interfaceRefXml = <interface_ref />;
					interfaceRefXml.@component_node_urn = Util.makeUrn(s.manager.Authority, "node", "gpeni");
					interfaceRefXml.@component_interface_id = "eth0";
					componentHopGpeniXml.appendChild(interfaceRefXml);
					linkXml.appendChild(componentHopGpeniXml);
				}
			}
			
			return linkXml;
		}
	}
}