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
	import protogeni.display.DisplayUtil;
	
	// Acts as the component manager for physical nodes and links and slivers
	public class ComponentManager
	{
		public static var UNKOWN : int = 0;
	    public static var INPROGRESS : int = 1;
	    public static var VALID : int = 2;
	    public static var FAILED : int = 3;
		
		[Bindable]
		public var Url : String = "";
		
		[Bindable]
		public var Hrn : String = "";
		
		[Bindable]
		public var Urn : String = "";
		
		public var Version:int;
		
		[Bindable]
		public var Show : Boolean = true;
		
		public var errorMessage : String = "";
		public var errorDescription : String = "";
		
		public var Nodes:PhysicalNodeGroupCollection = new PhysicalNodeGroupCollection();
		public var Links:PhysicalLinkGroupCollection = new PhysicalLinkGroupCollection();
		public var AllNodes:ArrayCollection = new ArrayCollection();
		public var AllLinks:ArrayCollection = new ArrayCollection();
		
		[Bindable]
		public var Status : int = UNKOWN;
		
		public var main : protogeniflash;
		
		// For now set when RSPEC is parsed
		public var totalNodes:int = 0;
		public var availableNodes:int = 0;
		public var unavailableNodes:int = 0;
		public var percentageAvailable:int;
	    
		public function ComponentManager()
		{
			main = Main.Pgmap();
		}
		
		public function DiscoverResourcesUrl():String
		{
			return Url;
		}
		
		public function VisitUrl():String
		{
			return Url;
		}
		
		public function getAvailableNodes():ArrayCollection
		{
			var availNodes:ArrayCollection = new ArrayCollection();
			for each(var n:PhysicalNode in AllNodes)
			{
				if(n.available)
					availNodes.addItem(n);
			}
			return availNodes;
		}
		
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
	    public var Rspec:XML = null;
	    
	    public function mightNeedSecurityException():Boolean
	    {
	    	return errorMessage.search("#2048") > -1;
	    }
	    
	    public function clear():void
	    {
	    	Nodes = new PhysicalNodeGroupCollection();
			Links = new PhysicalLinkGroupCollection();
			AllNodes = new ArrayCollection();
			AllLinks = new ArrayCollection();
			Rspec = null;
			Status = ComponentManager.UNKOWN;
			errorMessage = "";
			errorDescription = "";
	    }
	    
	    public function processRspec(afterCompletion : Function):void {
			Main.log.setStatus("Parsing " + Hrn + " RSPEC", false);

			var ns:Namespace = Rspec.namespace();
			
			if (ns == null
				|| ns.uri == "http://www.protogeni.net/resources/rspec/0.1"
				|| ns.uri == "http://www.protogeni.net/resources/rspec/0.2")
			{
				Version = 1;
			}
			else if (ns.uri == "http://www.protogeni.net/resources/rspec/2")
			{
				Version = 2;
			}
			else
			{
				throw new Error("Unsupported ad rspec version: " + ns.uri);
			}

			var nodeName : QName = new QName(Rspec.namespace(), "node");
			nodes = Rspec.elements(nodeName);
			nodeName = new QName(Rspec.namespace(), "link");
			links = Rspec.elements(nodeName);

	    	myAfter = afterCompletion;
	    	myIndex = 0;
	    	myState = NODE_PARSE;
	    	interfaceDictionary = new Dictionary();
	    	nodeNameDictionary = new Dictionary();
	    	subnodeList = new ArrayCollection();
	    	main.stage.addEventListener(Event.ENTER_FRAME, parseNext);
	    }
	    
	    private function parseNext(event : Event) : void
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
				Main.log.setStatus("Parsing " + Hrn + " RSPEC Done",false);
	    		interfaceDictionary = null;
	    		nodeNameDictionary = null;
	    		subnodeList = null;
				this.totalNodes = this.AllNodes.length;
				this.availableNodes = this.getAvailableNodes().length;
				this.unavailableNodes = this.AllNodes.length - availableNodes;
				this.percentageAvailable = (this.availableNodes / this.totalNodes) * 100;
	      		Status = ComponentManager.VALID;
				Main.protogeniHandler.dispatchComponentManagerChanged(this);
				main.stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				myAfter();
			}
			else
			{
				Main.log.setStatus("Fail",true);
	    		interfaceDictionary = null;
	    		nodeNameDictionary = null;
	    		subnodeList = null;
				main.stage.removeEventListener(Event.ENTER_FRAME, parseNext);
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
				var nodeName : QName = new QName(Rspec.namespace(), "location");
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

				var ng:PhysicalNodeGroup = Nodes.GetByLocation(lat,lng);
				var tempString:String;
				if(ng == null) {
					tempString = "";
					if(temp != null)
						tempString = temp.@country;
					ng = new PhysicalNodeGroup(lat, lng, tempString, Nodes);
					Nodes.Add(ng);
				}

				var n:PhysicalNode = new PhysicalNode(ng);
				n.name = p.@component_name;
				switch(Version)
				{
					case 1:
						n.urn = p.@component_uuid;
						n.managerString = p.@component_manager_uuid;
						break;
					default:
						n.urn = p.@component_id;
						n.managerString = p.@component_manager_id;
				}
				
				n.manager = Main.protogeniHandler.ComponentManagers.getByUrn(n.managerString);

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
	        	AllNodes.addItem(n);
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
	        	
				var nodeName : QName = new QName(Rspec.namespace(), "interface_ref");
				var temps:XMLList = link.elements(nodeName);
	        	var interface1:String = temps[0].@component_interface_id
	        	//var ni1:NodeInterface = Nodes.GetInterfaceByID(interface1);
	        	var ni1:PhysicalNodeInterface = interfaceDictionary[interface1];
	        	if(ni1 != null) {
	        		var interface2:String = temps[1].@component_interface_id;
		        	//var ni2:NodeInterface = Nodes.GetInterfaceByID(interface2);
	        		var ni2:PhysicalNodeInterface = interfaceDictionary[interface2];
		        	if(ni2 != null) {
		        		var lg:PhysicalLinkGroup = Links.Get(ni1.owner.GetLatitude(), ni1.owner.GetLongitude(), ni2.owner.GetLatitude(), ni2.owner.GetLongitude());
		        		if(lg == null) {
		        			lg = new PhysicalLinkGroup(ni1.owner.GetLatitude(), ni1.owner.GetLongitude(), ni2.owner.GetLatitude(), ni2.owner.GetLongitude(), Links);
		        			Links.Add(lg);
		        		}
		        		var l:PhysicalLink = new PhysicalLink(lg);
		        		l.name = link.@component_name;
		        		l.managerString = link.@component_manager_uuid;
		        		l.manager = this;
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
	        	
				AllLinks.addItem(l);
	        }
	        myIndex += idx;
	    }
		
		public function getGraphML():String {
				var added:Dictionary = new Dictionary();
				var randId:int = 0;
				
				// Start graph
				var nodes:String = "";
				var edges:String = "";
				
				var nodesToAdd:ArrayCollection = new ArrayCollection(AllNodes.toArray());
				var nodeGroups:ArrayCollection = new ArrayCollection();
				
				// Add nodes and combine similar nodes together
				for each(var currentNode:PhysicalNode in AllNodes) {
					
					// Give nodes any special qualities, otherwise see if they need to be grouped
					if(currentNode.IsSwitch())
						nodes += "<node id=\"" + Util.getDotString(currentNode.name) + "\" age=\"10\" name=\"" + currentNode.name + "\" image=\"assets/entrepriseNetwork/switch.swf\"/>";
					else if(currentNode.subNodeOf != null)
						nodes += "<node id=\"" + Util.getDotString(currentNode.name) + "\" age=\"10\" name=\"" + currentNode.name + "\" image=\"assets/entrepriseNetwork/printer.swf\"/>";
					else if(currentNode.subNodes != null && currentNode.subNodes.length > 0)
						nodes += "<node id=\"" + Util.getDotString(currentNode.name) + "\" age=\"10\" name=\"" + currentNode.name + "\" image=\"assets/entrepriseNetwork/pc.swf\"/>";
					else {
						
						// Group simple nodes connected to same switches
						var connectedSwitches:ArrayCollection = currentNode.ConnectedSwitches();
						if(connectedSwitches.length > 0 && connectedSwitches.length == currentNode.GetNodes().length) {
							// Go through all the groups already made
							var addedNode:Boolean = false;
							for each(var switchCombination:Object in nodeGroups) {
								// See if all switches exist
								if(connectedSwitches.length == switchCombination.switches.length) {
									var found:Boolean = true;
									for each(var connectedSwitch:Object in connectedSwitches) {
										if(!switchCombination.switches.contains(connectedSwitch)) {
											found = false;
											break;
										}
									}
									if(found) {
										switchCombination.count++;
										nodesToAdd.removeItemAt(nodesToAdd.getItemIndex(currentNode));
										addedNode = true;
										break;
									}
								}
							}
							if(!addedNode) {
								var newGroup:Object = {switches: new ArrayCollection(connectedSwitches.toArray()), count: 1, name: Util.getDotString(currentNode.name), original: currentNode};
								nodeGroups.addItem(newGroup);
								nodesToAdd.removeItemAt(nodesToAdd.getItemIndex(currentNode));
							}
						}
					}
				}
				// Remove any groups with just 1 node
				for(var i:int = nodeGroups.length-1; i > -1; --i) {
					if(nodeGroups[i].count == 1) {
						nodesToAdd.addItem(nodeGroups[i].original);
						nodeGroups.removeItemAt(i);
					}
				}
				
				for each(currentNode in nodesToAdd) {
					// Add connections
					for each(var connectedNode:PhysicalNode in currentNode.GetNodes()) {
						if(added[connectedNode.urn] != null || !nodesToAdd.contains(connectedNode))
							continue;
						edges += "<edge id=\"e" + (randId++) + "\" source=\"" + Util.getDotString(currentNode.name)  + "\" target=\"" + Util.getDotString(connectedNode.name) + "\"/>"
					}
					if(currentNode.subNodes != null && currentNode.subNodes.length > 0) {
						for each(var subNode:PhysicalNode in currentNode.subNodes) {
							edges += "<edge id=\"e" + (randId++) + "\" source=\"" + Util.getDotString(currentNode.name)  + "\" target=\"" + Util.getDotString(subNode.name) + "\"/>"
						}
					}
					added[currentNode.urn] = currentNode;
				}
				
				// Build up node groups
				for each(var nodeGroup:Object in nodeGroups) {
					nodes += "<node id=\"" + Util.getDotString(nodeGroup.name) + "\" age=\"10\" name=\"" + nodeGroup.name + " (" + nodeGroup.count + ")\" image=\"assets/entrepriseNetwork/pccluster.swf\"/>";
					for each(connectedNode in nodeGroup.switches) {
						edges += "<edge id=\"e" + (randId++) + "\" source=\"" + Util.getDotString(connectedNode.name)  + "\" target=\"" + nodeGroup.name + "\"/>"
					}
				}
				
				return "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\""
							+ " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
							+ " xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">"
							+ "<graph id=\"" + this.Hrn + "\" edgedefault=\"undirected\""
							+ nodes + edges + "</graph></graphml>";
		}
		
		public function getGroupedGraphML():String
		{
			var edges:String = "";
			var nodes:String = "";
			
			createGraphGroups();
			
			// Output the nodes and links
			var addedLocationLinks:Dictionary = new Dictionary();
			for each(var location:Object in locations) {
				
				// output the node
				nodes += "<node id=\"" + location.ref + "\"";
				var n:String = Util.getDotString(location.list[0].name);
				if(location.isSwitch) {
					if(location.list.length > 1)
						n = location.list.length+" Switches";
					nodes += "name=\"" + n + "\" image=\"assets/entrepriseNetwork/switch.swf\"/>";
				} else {
					if(location.list.length > 1)
						nodes += " name=\"" + location.list.length+" Nodes\" image=\"assets/entrepriseNetwork/pccluster.swf\"/>";
					else
						nodes += " name=\"" + n + "\" image=\"assets/entrepriseNetwork/pc.swf\"/>";
				}
				
				// output the links
				for(var i:int = 0; i < location.linkedGroups.length; i++) {
					var connectedLocation:Object = location.linkedGroups[i];
					var linkedRef:int = location.ref + connectedLocation.ref;
					if(addedLocationLinks[linkedRef] == null) {
						edges += "<edge id=\"e" + location.ref.toString() + connectedLocation.ref.toString() + "\" source=\"" + location.ref + "\" target=\"" +  connectedLocation.ref + "\"/>"
						addedLocationLinks[linkedRef] = 1;
					}
				}
			}
			
			return "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\""
			+ " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
				+ " xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd\">"
				+ "<graph id=\"" + this.Hrn + "\" edgedefault=\"undirected\""
				+ nodes + edges + "</graph></graphml>";
		}

		public function getDotGraph():String {
			var added:Dictionary = new Dictionary();
			
			// Start graph
			var dot:String = "graph " + Util.getDotString(Hrn) + " {\n" +
				"\toverlap=scale;\n" + 
				"\tsize=\"10,10\";\n" +
				"\tfontsize=20;\n";
			var links:String = "";
			
			for each(var currentNode:PhysicalNode in AllNodes) {
				dot += "\t" + Util.getDotString(currentNode.name);
				if(currentNode.IsSwitch())
					dot += " [shape=box3d, style=filled, color=deepskyblue3, height=20, width=30];\n";
				else if(currentNode.subNodeOf != null)
					dot += " [style=dotted, color=palegreen, height=10, width=10];\n";
				else
					dot += " [style=filled, color=limegreen, height=10, width=10];\n";
				
				for each(var connectedNode:PhysicalNode in currentNode.GetNodes()) {
					if(added[connectedNode.urn] == null) {
						if(connectedNode.IsSwitch() && currentNode.IsSwitch())
							links += "\t" + Util.getDotString(currentNode.name) + " -- " + Util.getDotString(connectedNode.name) + " [style=bold, color=deepskyblue3, penwidth=60, len=0.2, weight=6];\n";
						else if(currentNode.subNodeOf == connectedNode || connectedNode.subNodeOf == currentNode)
							links += "\t" + Util.getDotString(currentNode.name) + " -- " + Util.getDotString(connectedNode.name) + " [style=dotted, len=0.1, weight=5, penwidth=2, color=palegreen1];\n";
						else
							links += "\t" + Util.getDotString(currentNode.name) + " -- " + Util.getDotString(connectedNode.name) + " [penwidth=8, len=0.3, weight=.8];\n";
					}
				}
				added[currentNode.urn] = currentNode;
			}
			
			return dot + links + "}";
		}
		
		public var nodesToAdd:ArrayCollection = new ArrayCollection(AllNodes.toArray());
		public var locations:ArrayCollection = new ArrayCollection();
		public var nodeReferences:Dictionary = new Dictionary();
		public var switchReferences:Dictionary = new Dictionary();
		public var locationReferences:Dictionary = new Dictionary();
		public var graphGroupsCreated:Boolean = false;
		
		// 0 = none
		// 1 = collapse leaf switches
		public function createGraphGroups(levels:int = 1):void
		{
			//if(graphGroupsCreated)
			//	return;
			nodesToAdd = new ArrayCollection(AllNodes.toArray());
			locations = new ArrayCollection();
			switchReferences = new Dictionary();
			nodeReferences = new Dictionary();
			locationReferences = new Dictionary();
			
			// FIRST PASS
			// Process individual nodes
			for each(var pn:PhysicalNode in AllNodes) {
				var ref:int = pn.urn.length;
				for(var i:int = 0; i < pn.urn.length; i++) {
					ref += pn.urn.charCodeAt(i)*(i+1);
				}
				nodeReferences[pn.urn] = ref;
				if(pn.IsSwitch())
					switchReferences[pn.urn] = ref + 537;
			}
			// Combine switches into nodes
			for each(var currentNode:PhysicalNode in AllNodes) {
				// Already added, probably a leaf node
				if(!nodesToAdd.contains(currentNode))
					continue;
				ref = nodeReferences[currentNode.urn];

				if(currentNode.IsSwitch()) {
					var a:ArrayCollection = new ArrayCollection();
					a.addItem(currentNode);
					
					// Collapse leaf switches
					// Get connected leaf switches
					if(levels > 0) {
						var isLeaf:Boolean = true;
						var foundOtherSwitch:Boolean = false;
						for each(var nextNode:PhysicalNode in currentNode.GetNodes()) {
							if(nextNode.IsSwitch()) {
								if(foundOtherSwitch)
									isLeaf = false;
								foundOtherSwitch = true;
								var found:Boolean = false;
								for each(var nextNextNode:PhysicalNode in nextNode.GetNodes()) {
									if(nextNextNode != currentNode && nextNextNode.IsSwitch()) {
										found = true;
										break;
									}
								}
								if(!found)
									a.addItem(nextNode);
							}
						}
						if(foundOtherSwitch && isLeaf)
							continue;
					}
					
					// Get the list of connected nodes
					var connectedNodes:ArrayCollection = new ArrayCollection();
					for each(var nac:PhysicalNode in a) {
						connectedNodes = Util.keepUniqueObjects(nac.GetNodes(), connectedNodes);
					}
					// Create the switch/switch-group ref
					ref = nodeReferences[currentNode.urn];
					for each(nac in connectedNodes) {
						ref += nodeReferences[nac.urn];
					}
					// group empty routers together
					if(ref == nodeReferences[currentNode.urn] && levels > 0) {
						ref = 1;
						for each(nac in nodesToAdd) {
							if(nac != currentNode && nac.IsSwitch() && nac.GetNodes().length == 0) {
								a.addItem(nac);
							}
						}
					}
					
					for each(nac in a) {
						i = connectedNodes.getItemIndex(nac);
						if(i > -1)
							connectedNodes.removeItemAt(i);
					}
					
					var newLocation:Object = {
						list:a,
						linked:connectedNodes.toArray(),
							ref:ref,
							isSwitch:true,
							name: a.length==1 ? currentNode.name : a.length + " Switches"
					};
					locations.addItem(newLocation);
					for each(nac in a) {
						nodesToAdd.removeItemAt(nodesToAdd.getItemIndex(nac));
						nodeReferences[nac.urn] = ref;
					}
					if(locationReferences[ref] != null)
						Alert.show("Duplicate reference!!!!", "Problem",4, Main.Pgmap());
					locationReferences[ref] = newLocation;
				}
			}
			
			// SECOND PASS
			// Process the nodes into locations
			for each(currentNode in nodesToAdd) {
				// Get a unique identifier which stands for the connected nodes
				ref = 0;
				for each(var connectedNode:PhysicalNode in currentNode.GetNodes()) {
					if(connectedNode.IsSwitch())
						ref += switchReferences[connectedNode.urn];
					else
						ref += nodeReferences[connectedNode.urn];
				}
				if(locationReferences[ref] != null) {
					var otherLocation:Object = locationReferences[ref];
					otherLocation.list.addItem(currentNode);
					otherLocation.name = otherLocation.list.length + " Nodes";
					locationReferences[nodeReferences[currentNode.urn]] = otherLocation;
				} else {
					newLocation = {
						list:new ArrayCollection([currentNode]),
						linked:currentNode.GetNodes().toArray(),
							ref:ref,
							isSwitch:false,
							isAggregate:false,
							name:currentNode.name
					};
					locations.addItem(newLocation);
					if(locationReferences[ref] != null)
						Alert.show("Duplicate reference!!!!", "Problem",4, Main.Pgmap());
					locationReferences[ref] = newLocation;
					locationReferences[nodeReferences[currentNode.urn]] = newLocation;
				}
			}
			
			// THIRD PASS
			// Replace links to nodes with links to locations
			for each(var location:Object in locations) {
				var linkedLocations:ArrayCollection = new ArrayCollection();
				var linkedLocationsNums:ArrayCollection = new ArrayCollection();
				for each(var connectedNode:PhysicalNode in location.linked) {
					var linkedLocation:Object = locationReferences[nodeReferences[connectedNode.urn]];
					if(!linkedLocations.contains(linkedLocation)) {
						linkedLocations.addItem(linkedLocation);
						linkedLocationsNums.addItem(1);
					} else {
						linkedLocationsNums[linkedLocations.getItemIndex(linkedLocation)]++;
					}
				}
				location.linkedGroups = linkedLocations;
				location.linkedGroupsNum = linkedLocationsNums;
			}
			
			graphGroupsCreated = true;
		}
		
		public function getDotGroupedGraph(addGraphWrapper:Boolean = true):String {
			var links:String = "";
			var dot:String = "";
			if(addGraphWrapper)
				dot = "graph " + Util.getDotString(Hrn) + " {\n" +
					"\toverlap=scale;\n" + 
					"\tsize=\"10,10\";\n" +
					"\tfontsize=20;\n" +
					"\tnode [fontsize=300];\n" +
					"\tedge [style=bold];\n";
			
			createGraphGroups();
			
			// Output the nodes and links
			var addedLocationLinks:Dictionary = new Dictionary();
			for each(var location:Object in locations) {
				
				// output the node
				dot += "\t" + location.ref;
				if(location.isSwitch) {
					dot += " [shape=box3d, style=filled, color=deepskyblue3, height=10, width=20, label=\""+Util.getDotString(location.name)+"\"];\n";
				} else {
					dot += " [style=filled, height="+.25*location.list.length+", width="+.375*location.list.length+", color=limegreen, label=\""+Util.getDotString(location.name)+"\"];\n";
				}
				
				// output the links
				for(var i:int = 0; i < location.linkedGroups.length; i++) {
					var connectedLocation:Object = location.linkedGroups[i];
					var linkedRef:int = location.ref + connectedLocation.ref;
					if(addedLocationLinks[linkedRef] == null) {
						links += "\t" + location.ref + " -- " + connectedLocation.ref;
						if(location.isSwitch && connectedLocation.isSwitch)
							links += " [style=bold, color=deepskyblue3, penwidth=60, len=0.2, weight=6];\n";
						else
							links += " [penwidth=8, len=0.3, weight="+0.8*location.linkedGroupsNum[i]+"];\n";
						addedLocationLinks[linkedRef] = 1;
					}
				}
			}
			
			dot += links;
			if(addGraphWrapper)
				dot += "}";
			return dot;
		}
	}
}