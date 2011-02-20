package protogeni.resources
{
	import flash.events.Event;
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Alert;
	
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
		
		private static var MAX_WORK : int = 60;
		
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
		
		public function processResourceRspec(afterCompletion : Function):void {
			Main.log.setStatus("Parsing " + gm.Hrn + " RSPEC", false);

			var nodeName:QName = new QName(gm.Rspec.namespace(), "node");
			nodes = gm.Rspec.elements(nodeName);
			nodeName = new QName(gm.Rspec.namespace(), "link");
			links = gm.Rspec.elements(nodeName);
			
			myAfter = afterCompletion;
			myIndex = 0;
			myState = NODE_PARSE;
			interfaceDictionary = new Dictionary();
			nodeNameDictionary = new Dictionary();
			subnodeList = new ArrayCollection();
			Main.Application().stage.addEventListener(Event.ENTER_FRAME, parseNext);
		}
		
		private function parseNext(event:Event) : void
		{
			if(!hasslot && GeniManager.processing > GeniManager.maxProcessing)
				return;
			if(!hasslot) {
				hasslot = true;
				GeniManager.processing++;
			}
			if (myState == NODE_PARSE)	    	
			{
				parseNextNode();
			}
			else if (myState == LINK_PARSE)
			{
				parseNextLink();
			}
			else if (myState == DONE)
			{
				GeniManager.processing--;
				Main.log.setStatus("Parsing " + gm.Hrn + " RSPEC Done",false);
				interfaceDictionary = null;
				nodeNameDictionary = null;
				subnodeList = null;
				gm.totalNodes = gm.AllNodes.length;
				gm.availableNodes = gm.getAvailableNodes().length;
				gm.unavailableNodes = gm.AllNodes.length - gm.availableNodes;
				gm.percentageAvailable = (gm.availableNodes / gm.totalNodes) * 100;
				gm.Status = GeniManager.STATUS_VALID;
				Main.geniDispatcher.dispatchGeniManagerChanged(gm);
				Main.Application().stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				myAfter();
			}
			else
			{
				Main.log.setStatus("Fail",true);
				interfaceDictionary = null;
				nodeNameDictionary = null;
				subnodeList = null;
				Main.Application().stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				Alert.show("Problem parsing RSPEC");
				// Throw exception
			}
		}
		
		private function parseNextNode():void {
			var idx:int;
			for(idx = 0; idx < MAX_WORK; idx++) {
				var fullIdx:int = myIndex + idx;
				if(fullIdx == nodes.length()) {
					
					// Assign subnodes
					for each(var obj:Object in subnodeList)
					{
						var parentNode:PhysicalNode = nodeNameDictionary[obj.parentName];
						parentNode.subNodes.addItem(obj.subNode);
						obj.subNode.subNodeOf = parentNode;
					}
					
					myState = LINK_PARSE;
					myIndex = 0;
					return;
				}
				
				var p:XML = nodes[fullIdx];
				
				// Get location info
				var nodeName : QName = new QName(gm.Rspec.namespace(), "location");
				var temps:XMLList = p.elements(nodeName);
				var temp:XML = temps[0];
				var lat:Number = -72.134678;
				var lng:Number = 170.332031;
				if(temps.length() > 0)
				{
					if(Number(temp.@latitude) != 0 && Number(temp.@longitude) != 0)
					{
						lat = Number(temp.@latitude);
						lng = Number(temp.@longitude);
					}
				}
				
				var ng:PhysicalNodeGroup = gm.Nodes.GetByLocation(lat,lng);
				var tempString:String;
				if(ng == null) {
					tempString = "";
					if(temp != null)
						tempString = temp.@country;
					ng = new PhysicalNodeGroup(lat, lng, tempString, gm.Nodes);
					gm.Nodes.Add(ng);
				}
				
				var n:PhysicalNode = new PhysicalNode(ng, gm);
				n.name = p.@component_name;
				switch(gm.outputRspecVersion)
				{
					case 0.1:
						n.urn = p.@component_uuid;
						n.managerString = p.@component_manager_uuid;
						break;
					case 2:
					default:
						n.urn = p.@component_id;
						n.managerString = p.@component_manager_id;
				}
				
				for each(var ix:XML in p.children()) {
					if(ix.localName() == "interface") {
						var i:PhysicalNodeInterface = new PhysicalNodeInterface(n);
						i.id = ix.@component_id;
						tempString = ix.@role;
						i.role = PhysicalNodeInterface.RoleIntFromString(tempString);
						n.interfaces.Add(i);
						interfaceDictionary[i.id] = i;
					} else if(ix.localName() == "node_type") {
						var t:NodeType = new NodeType();
						t.name = ix.@type_name;
						t.slots = ix.@type_slots;
						t.isStatic = ix.@static;
						// isBgpMux = true?
						// upstreamAs = value of key->upstream_as in field
						n.types.addItem(t);
					} else if(ix.localName() == "available") {
						var availString:String = ix.toString();
						n.available = availString == "true";
					} else if(ix.localName() == "exclusive") {
						var exString:String = ix.toString();
						n.exclusive = exString == "true";
					} else if(ix.localName() == "subnode_of") {
						var parentName:String = ix.toString();
						if(parentName.length > 0)
							subnodeList.addItem({subNode:n, parentName:parentName});
					} else if(ix.localName() == "disk_image") {
						var newDiskImage:DiskImage = new DiskImage();
						newDiskImage.name = ix.@name;
						newDiskImage.os = ix.@os;
						newDiskImage.description = ix.@description;
						newDiskImage.version = ix.@version;
						n.diskImages.addItem(newDiskImage);
					}
				}
				
				n.rspec = p.copy();
				ng.Add(n);
				nodeNameDictionary[n.urn] = n;
				nodeNameDictionary[n.name] = n;
				gm.AllNodes.addItem(n);
			}
			myIndex += idx;
		}
		
		private function parseNextLink():void {
			var idx:int;
			for(idx = 0; idx < MAX_WORK; idx++) {
				var fullIdx:int = myIndex + idx;
				//main.console.appendText("idx:" + idx.toString() + " full:" + fullIdx.toString());
				if(fullIdx == links.length()) {
					myState = DONE;
					//main.console.appendText("...finished links\n");
					return;
				}
				//main.console.appendText("parsing...");
				var link:XML = links[fullIdx];
				
				var nodeName : QName = new QName(gm.Rspec.namespace(), "interface_ref");
				var temps:XMLList = link.elements(nodeName);
				var interface1:String = temps[0].@component_interface_id
				//var ni1:NodeInterface = Nodes.GetInterfaceByID(interface1);
				var ni1:PhysicalNodeInterface = interfaceDictionary[interface1];
				if(ni1 != null) {
					var interface2:String = temps[1].@component_interface_id;
					//var ni2:NodeInterface = Nodes.GetInterfaceByID(interface2);
					var ni2:PhysicalNodeInterface = interfaceDictionary[interface2];
					if(ni2 != null) {
						var lg:PhysicalLinkGroup = gm.Links.Get(ni1.owner.GetLatitude(), ni1.owner.GetLongitude(), ni2.owner.GetLatitude(), ni2.owner.GetLongitude());
						if(lg == null) {
							lg = new PhysicalLinkGroup(ni1.owner.GetLatitude(), ni1.owner.GetLongitude(), ni2.owner.GetLatitude(), ni2.owner.GetLongitude(), gm.Links);
							gm.Links.Add(lg);
						}
						var l:PhysicalLink = new PhysicalLink(lg);
						l.name = link.@component_name;
						l.managerString = link.@component_manager_uuid;
						l.manager = gm;
						l.urn = link.@component_uuid;
						l.interface1 = ni1;
						l.interface2 = ni2;
						
						for each(var ix:XML in link.children()) {
							if(ix.localName() == "bandwidth") {
								l.bandwidth = Number(ix);
							} else if(ix.localName() == "latency") {
								l.latency = Number(ix);
							} else if(ix.localName() == "packet_loss") {
								l.packetLoss = Number(ix);
							}
						}
						
						l.rspec = link.copy();
						
						for each(var tx:XML in link.link_type) {
							var s:String = tx.@type_name;
							l.types.addItem(s);
						}
						
						lg.Add(l);
						ni1.links.addItem(l);
						ni2.links.addItem(l);
						if(lg.IsSameSite()) {
							ni1.owner.owner.links = lg;
						}
					}
				}
				
				gm.AllLinks.addItem(l);
			}
			myIndex += idx;
		}
		
		public function processSliverRspec(s:Sliver):void
		{
			var xmlns:Namespace = s.rspec.namespace();
			var rspecVersion:int = 1;
			if(xmlns.uri == CommunicationUtil.rspec2Namespace) {
				rspecVersion = 2;
			}
			
			s.validUntil = Util.parseProtogeniDate(s.rspec.@valid_until);

			s.nodes = new VirtualNodeCollection();
			s.links = new VirtualLinkCollection();
			
			var nodesById:Dictionary = new Dictionary();
			var interfacesById:Dictionary = new Dictionary();

			var linksXml : ArrayCollection = new ArrayCollection();
			var nodesXml : ArrayCollection = new ArrayCollection();
			for each(var component:XML in s.rspec.children())
			{
				if(component.localName() == "link")
					linksXml.addItem(component);
				else if(component.localName() == "node")
					nodesXml.addItem(component);
			}
			
			for each(var nodeXml:XML in nodesXml)
			{
				var cmNode:PhysicalNode;
				if(rspecVersion == 1)
					cmNode = s.manager.Nodes.GetByUrn(nodeXml.@component_urn);
				else
					cmNode = s.manager.Nodes.GetByUrn(nodeXml.@component_id);
				if(cmNode != null)
				{
					var virtualNode:VirtualNode = new VirtualNode(s);
					if(rspecVersion == 1) {
						virtualNode.setToPhysicalNode(s.manager.Nodes.GetByUrn(nodeXml.@component_urn));
						virtualNode.id = nodeXml.@virtual_id;
						virtualNode.manager = Main.geniHandler.GeniManagers.getByUrn(nodeXml.@component_manager_urn);
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
					} else {
						virtualNode.setToPhysicalNode(s.manager.Nodes.GetByUrn(nodeXml.@component_id));
						virtualNode.id = nodeXml.@client_id;
						virtualNode.isShared = nodeXml.@exclusive == "true";
						virtualNode.manager = Main.geniHandler.GeniManagers.getByUrn(nodeXml.@component_manager_id);
						virtualNode.urn = nodeXml.@sliver_id;
					}
					
					for each(var ix:XML in nodeXml.children()) {
						if(ix.localName() == "interface") {
							var virtualInterface:VirtualInterface = new VirtualInterface(virtualNode);
							if(rspecVersion == 1) {
								virtualInterface.id = ix.@virtual_id;
							} else {
								virtualInterface.id = ix.@client_id;
								virtualInterface.physicalNodeInterface = virtualNode.physicalNode.interfaces.GetByID(ix.@component_id, false);
							}
							virtualNode.interfaces.Add(virtualInterface);
							interfacesById[virtualInterface.id] = virtualInterface;
						} else if(ix.localName() == "disk_image") {
							virtualNode.diskImage = ix.@name;
						} else if(ix.localName() == "sliver_type") {
							// don't do anything now
						} else if(ix.localName() == "services") {
							for each(var ixx:XML in ix.children()) {
								if(ix.localName() == "login") {
									virtualNode.hostname = ixx.@hostname;
									virtualNode.sshdport = ixx.@port;
								}
							}
						}
					}
					
					virtualNode.rspec = nodeXml.copy();
					s.nodes.addItem(virtualNode);
					nodesById[virtualNode.id] = virtualNode;
					virtualNode.physicalNode.virtualNodes.addItem(virtualNode);
				}
				// Don't add outside nodes ... do that if found when parsing links ...
			}
			
			for each(var vn:VirtualNode in s.nodes)
			{
				if(vn.physicalNode.subNodeOf != null)
				{
					vn.superNode = s.nodes.getById(vn.physicalNode.subNodeOf.name);
					s.nodes.getById(vn.physicalNode.subNodeOf.name).subNodes.push(vn);
				}
			}
			
			for each(var linkXml:XML in linksXml)
			{
				var virtualLink:VirtualLink = new VirtualLink(s);
				if(rspecVersion == 1) {
					virtualLink.id = linkXml.@virtual_id;
					virtualLink.type = linkXml.@link_type;
				} else {
					virtualLink.id = linkXml.@client_id;
					virtualLink.urn = linkXml.@sliver_id;
					// vlantag?
				}
				
				for each(var viXml:XML in linkXml.children()) {
					if(viXml.localName() == "bandwidth")
						virtualLink.bandwidth = viXml.toString();
					if(viXml.localName() == "interface_ref") {
						if(rspecVersion == 1) {
							var vid:String = viXml.@virtual_interface_id;
							var nid:String = viXml.@virtual_node_id;
							var interfacedNode:VirtualNode = nodesById[nid];
							// Deal with outside node
							if(interfacedNode == null)
							{
								// Get outside node, don't add if not parsed in the other cm yet
								interfacedNode = s.slice.getVirtualNodeWithId(nid);
								if(interfacedNode == null)
								{
									virtualLink = null;
									break;
								}
							}
							for each(var vi:VirtualInterface in interfacedNode.interfaces.collection)
							{
								if(vi.id == vid)
								{
									virtualLink.interfaces.addItem(vi);
									vi.virtualLinks.addItem(virtualLink);
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
								if(interfacedNode == null)
								{
									virtualLink = null;
									break;
								}
							}
							virtualLink.interfaces.addItem(interfacedNodeInterface);
							interfacedNodeInterface.virtualLinks.addItem(virtualLink);
						}
					}
				}
				
				if(virtualLink == null)
					continue;
				
				virtualLink.rspec = linkXml.copy();
				virtualLink.firstNode = (virtualLink.interfaces[0] as VirtualInterface).virtualNode;
				virtualLink.secondNode = (virtualLink.interfaces[1] as VirtualInterface).virtualNode;
				
				// Deal with tunnel
				if(virtualLink.firstNode.slivers[0] != s)
				{
					Util.addIfNonexistingToArrayCollection(virtualLink.firstNode.slivers, s);
					Util.addIfNonexistingToArrayCollection(virtualLink.secondNode.slivers, virtualLink.firstNode.slivers[0]);
					virtualLink.firstNode.slivers[0].links.addItem(virtualLink);
				} else if(virtualLink.secondNode.slivers[0] != s)
				{
					Util.addIfNonexistingToArrayCollection(virtualLink.secondNode.slivers, s);
					Util.addIfNonexistingToArrayCollection(virtualLink.firstNode.slivers, virtualLink.secondNode.slivers[0]);
					virtualLink.secondNode.slivers[0].links.addItem(virtualLink);
				}
				
				s.links.addItem(virtualLink);
			}
		}
		
		public function generateSliverRspec(s:Sliver):XML
		{
			var requestRspec:XML;
			switch(gm.inputRspecMinVersion) {
				case 0.1:
					requestRspec = new XML("<?xml version=\"1.0\" encoding=\"UTF-8\"?> "
						+ "<rspec "
						+ "xmlns=\""+CommunicationUtil.rspec02Namespace+"\" "
						+ "type=\"request\" />");
					break;
				case 2:
				default:
					var schemaLocations:String = "http://www.protogeni.net/resources/rspec/2 http://www.protogeni.net/resources/rspec/2/request.xsd";
					var moreXmlns:String = "";
					for each(var tvl:VirtualLink in s.links) {
						if(tvl.isTunnel()) {
							schemaLocations += " http://www.protogeni.net/resources/rspec/ext/gre-tunnel/1 http://www.protogeni.net/resources/rspec/ext/gre-tunnel/1/request.xsd";
							moreXmlns = "xmlns:tun=\"http://www.protogeni.net/resources/rspec/ext/gre-tunnel/1\" "
							break;
						}
					}
					requestRspec = new XML("<?xml version=\"1.0\" encoding=\"UTF-8\"?> "
						+ "<rspec "
						+ "xmlns=\""+CommunicationUtil.rspec2Namespace+"\" "
						+ moreXmlns
						+ "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
						+ "xsi:schemaLocation=\"" + schemaLocations + "\" "
						+ "type=\"request\" />");
					break;
			}
			
			for each(var vn:VirtualNode in s.nodes) {
				var nodeXml : XML = <node />;
				
				if (!vn.isVirtual) {
					if(gm.inputRspecMinVersion == 0.1)
						nodeXml.@component_uuid = vn.physicalNode.urn;
					else {
						nodeXml.@component_id = vn.physicalNode.urn;
					}
				}
					
				if(gm.inputRspecMinVersion == 0.1) {
					nodeXml.@virtual_id = vn.id;
					nodeXml.@component_manager_uuid = vn.manager.Urn;
					nodeXml.@virtualization_type = vn.virtualizationType;
				} else {
					nodeXml.@client_id = vn.id;
					nodeXml.@component_manager_id = vn.manager.Urn;
				}
				
				if (vn.isShared)
				{
					if(gm.inputRspecMinVersion == 0.1) {
						nodeXml.@virtualization_subtype = vn.virtualizationSubtype;
						nodeXml.@exclusive = 0;
					} else {
						nodeXml.@exclusive = "false";
					}
				}
				else {
					if(gm.inputRspecMinVersion == 0.1)
						nodeXml.@exclusive = 1;
					else
						nodeXml.@exclusive = "true";
				}
				
				// Currently only pcs
				if(gm.inputRspecMinVersion == 0.1) {
					var nodeType:String = "pc";
					if (vn.isShared)
						nodeType = "pcvm";
					var nodeTypeXml:XML = <node_type />;
					nodeTypeXml.@type_name = nodeType;
					nodeTypeXml.@type_slots = 1;
					nodeXml.appendChild(nodeTypeXml);
				} else {
					var sliverType:XML = <sliver_type />
						sliverType.@name = "raw-pc";
						nodeXml.appendChild(sliverType);
				}
				
				if(vn.startupCommand.length > 0)
					nodeXml.@startup_command = vn.startupCommand;
				
				if(vn.tarfiles.length > 0)
					nodeXml.@tarfiles = vn.tarfiles;
				
				if(vn.diskImage.length > 0)
				{
					var diskImageXml:XML = <disk_image />;
					diskImageXml.@name = vn.diskImage;
					nodeXml.appendChild(diskImageXml);
				}
				
				if (gm.inputRspecMinVersion == 0.1 && vn.superNode != null)
					nodeXml.appendChild(XML("<subnode_of>" + vn.superNode.urn + "</subnode_of>"));
				
				for each (var current:VirtualInterface in vn.interfaces.collection)
				{
					if(gm.inputRspecMinVersion >= 2 && current.id == "control")
						continue;
					var interfaceXml:XML = <interface />;
					if(gm.inputRspecMinVersion == 0.1)
						interfaceXml.@virtual_id = current.id;
					else {
						interfaceXml.@client_id = current.id;
						for each(var currentLink:VirtualLink in current.virtualLinks) {
							if(currentLink.isTunnel()) {
								var tunnelXml:XML = <interface_ip />;
								for each(var xn:Namespace in requestRspec.namespaceDeclarations()) {
									tunnelXml.addNamespace(xn);
									if(xn.prefix == "tun")
										tunnelXml.setNamespace(xn);
								}
								tunnelXml.@address = current.ip;
								tunnelXml.@netmask = "255.255.255.0";
								interfaceXml.appendChild(tunnelXml);
							}
						}
					}
					nodeXml.appendChild(interfaceXml);
				}
				
				requestRspec.appendChild(nodeXml);
			}
			
			for each(var vl:VirtualLink in s.links) {
				var linkXml : XML = <link />;
				
				if(gm.inputRspecMinVersion == 0.1)
					linkXml.@virtual_id = vl.id;
				else
					linkXml.@client_id = vl.id;
				
				if (gm.inputRspecMinVersion == 0.1 && !vl.isTunnel())
					linkXml.appendChild(XML("<bandwidth>" + vl.bandwidth + "</bandwidth>"));
				
				if (vl.isTunnel())
				{
					if(gm.inputRspecMinVersion == 0.1)
						linkXml.@link_type = "tunnel";
					else {
						var link_type:XML = <link_type />;
						link_type.@name = "gre-tunnel";
						linkXml.appendChild(link_type);
					}
				}
				else {
					if(gm.inputRspecMinVersion == 0.1)
						linkXml.@link_type = "ethernet";
				}
				
				for each (var currentVi:VirtualInterface in vl.interfaces)
				{
					if(gm.inputRspecMinVersion >= 2 && currentVi.id == "control")
						continue;
					var interfaceRefXml:XML = <interface_ref />;
					if(gm.inputRspecMinVersion == 0.1) {
						interfaceRefXml.@virtual_node_id = currentVi.virtualNode.id;
						if (vl.isTunnel())
						{
							interfaceRefXml.@tunnel_ip = currentVi.ip;
							interfaceRefXml.@virtual_interface_id = "control";
						}
						else
						{
							interfaceRefXml.@virtual_interface_id = currentVi.id;
						}
					} else {
						interfaceRefXml.@client_id = currentVi.id;
					}
					
					linkXml.appendChild(interfaceRefXml);
				}
				
				requestRspec.appendChild(linkXml);
			}
			
			return requestRspec;
		}
	}
}