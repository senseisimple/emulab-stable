package protogeni.resources
{
	import flash.events.Event;
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Alert;
	

	public class ProtogeniAdParser implements AdParserInterface
	{
		public function ProtogeniAdParser(newGm:GeniManager)
		{
			gm = newGm;
		}
		
		private var gm:GeniManager;
		
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
		private var rspecVersion:int;
		
		public function processRspec(afterCompletion : Function):void {
			Main.log.setStatus("Parsing " + gm.Hrn + " RSPEC", false);
			
			var ns:Namespace = gm.Rspec.namespace();
			
			if (ns == null
				|| ns.uri == "http://www.protogeni.net/resources/rspec/0.1"
				|| ns.uri == "http://www.protogeni.net/resources/rspec/0.2")
			{
				rspecVersion = 1;
			}
			else if (ns.uri == "http://www.protogeni.net/resources/rspec/2")
			{
				rspecVersion = 2;
			}
			else
			{
				throw new Error("Unsupported ad rspec version: " + ns.uri);
			}
			
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
				Main.log.setStatus("Parsing " + gm.Hrn + " RSPEC Done",false);
				interfaceDictionary = null;
				nodeNameDictionary = null;
				subnodeList = null;
				gm.totalNodes = gm.AllNodes.length;
				gm.availableNodes = gm.getAvailableNodes().length;
				gm.unavailableNodes = gm.AllNodes.length - gm.availableNodes;
				gm.percentageAvailable = (gm.availableNodes / gm.totalNodes) * 100;
				gm.Status = GeniManager.VALID;
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
				
				var n:PhysicalNode = new PhysicalNode(ng);
				n.name = p.@component_name;
				switch(rspecVersion)
				{
					case 1:
						n.urn = p.@component_uuid;
						n.managerString = p.@component_manager_uuid;
						break;
					default:
						n.urn = p.@component_id;
						n.managerString = p.@component_manager_id;
				}
				
				n.manager = Main.geniHandler.GeniManagers.getByUrn(n.managerString);
				
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
	}
}