package protogeni.resources
{
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Alert;
	
	import protogeni.Util;
	import protogeni.communication.Request;
	import protogeni.display.DisplayUtil;

	public class GeniManager
	{
		public static const STATUS_UNKOWN:int = 0;
		public static const STATUS_INPROGRESS:int = 1;
		public static const STATUS_VALID:int = 2;
		public static const STATUS_FAILED:int = 3;
		
		public static const TYPE_PROTOGENI:int = 0;
		public static const TYPE_PLANETLAB:int = 1;
		
		public static var processing:int = 0;
		public static var maxProcessing:int = 1;
		
		[Bindable]
		public var Url:String = "";
		
		[Bindable]
		public var Hrn:String = "";
		
		[Bindable]
		public var Urn:String = "";
		
		public var Authority:String = "";
		
		[Bindable]
		public var Version:int;
		
		[Bindable]
		public var errorMessage:String = "";
		public var errorDescription:String = "";
		
		[Bindable]
		public var Status:int = STATUS_UNKOWN;
		
		public var Rspec:XML = null;
		
		public var supportsIon:Boolean = false;
		public var supportsGpeni:Boolean = false;
		
		[Bindable]
		public var Show:Boolean = true;
		
		public var Nodes:PhysicalNodeGroupCollection;
		public var Links:PhysicalLinkGroupCollection = new PhysicalLinkGroupCollection();
		
		public var AllNodes:Vector.<PhysicalNode> = new Vector.<PhysicalNode>();
		
		public var colorIdx:int;
		
		public function AllNodesAsArray():Array {
			var allNodesArray:Array = new Array();
			for each (var elem:PhysicalNode in AllNodes) {
				allNodesArray.push(elem);
			}
			return allNodesArray;
		}
		public var AllLinks:Vector.<PhysicalLink> = new Vector.<PhysicalLink>();
		public function AllLinksAsArray():Array {
			var allLinksArray:Array = new Array();
			for each (var elem:PhysicalLink in AllLinks) {
				allLinksArray.push(elem);
			}
			return allLinksArray;
		}
		
		// For now set when RSPEC is parsed
		public var totalNodes:int = 0;
		public var availableNodes:int = 0;
		public var unavailableNodes:int = 0;
		public var percentageAvailable:int;
		
		public var rspecProcessor:RspecProcessorInterface;
		
		public var lastRequest:Request;
		
		public var type:int;
		
		public function GeniManager()
		{
			Nodes = new PhysicalNodeGroupCollection(this);
			colorIdx = DisplayUtil.getColorIdx();
		}
		
		public function VisitUrl():String
		{
			var hostPattern:RegExp = /^(http(s?):\/\/([^\/]+))(\/.*)?$/;
			var match:Object = hostPattern.exec(Url);
			if (match != null)
			{
				return match[1];
			} else
				return Url;
		}
		
		public function mightNeedSecurityException():Boolean
		{
			return errorMessage.search("#2048") > -1;
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
		
		public function clear():void
		{
			clearComponents();
			Rspec = null;
			Status = GeniManager.STATUS_UNKOWN;
			errorMessage = "";
			errorDescription = "";
		}
		
		public function clearComponents():void {
			Nodes = new PhysicalNodeGroupCollection(this);
			Links = new PhysicalLinkGroupCollection();
			AllNodes = new Vector.<PhysicalNode>();
			AllLinks = new Vector.<PhysicalLink>();
		}
		
		//---------------------------------------
		/*
		public function getGraphML():String {
			var added:Dictionary = new Dictionary();
			var randId:int = 0;
			
			// Start graph
			var nodes:String = "";
			var edges:String = "";
			
			var nodesToAdd:ArrayCollection = new ArrayCollection(this.AllNodesAsArray());
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
					if(added[connectedNode.id] != null || !nodesToAdd.contains(connectedNode))
						continue;
					edges += "<edge id=\"e" + (randId++) + "\" source=\"" + Util.getDotString(currentNode.name)  + "\" target=\"" + Util.getDotString(connectedNode.name) + "\"/>"
				}
				if(currentNode.subNodes != null && currentNode.subNodes.length > 0) {
					for each(var subNode:PhysicalNode in currentNode.subNodes) {
						edges += "<edge id=\"e" + (randId++) + "\" source=\"" + Util.getDotString(currentNode.name)  + "\" target=\"" + Util.getDotString(subNode.name) + "\"/>"
					}
				}
				added[currentNode.id] = currentNode;
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
					if(added[connectedNode.id] == null) {
						if(connectedNode.IsSwitch() && currentNode.IsSwitch())
							links += "\t" + Util.getDotString(currentNode.name) + " -- " + Util.getDotString(connectedNode.name) + " [style=bold, color=deepskyblue3, penwidth=60, len=0.2, weight=6];\n";
						else if(currentNode.subNodeOf == connectedNode || connectedNode.subNodeOf == currentNode)
							links += "\t" + Util.getDotString(currentNode.name) + " -- " + Util.getDotString(connectedNode.name) + " [style=dotted, len=0.1, weight=5, penwidth=2, color=palegreen1];\n";
						else
							links += "\t" + Util.getDotString(currentNode.name) + " -- " + Util.getDotString(connectedNode.name) + " [penwidth=8, len=0.3, weight=.8];\n";
					}
				}
				added[currentNode.id] = currentNode;
			}
			
			return dot + links + "}";
		}
		
		public var nodesToAdd:ArrayCollection;
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

			nodesToAdd = new ArrayCollection(this.AllNodesAsArray());
			locations = new ArrayCollection();
			switchReferences = new Dictionary();
			nodeReferences = new Dictionary();
			locationReferences = new Dictionary();
			
			// FIRST PASS
			// Process individual nodes
			for each(var pn:PhysicalNode in AllNodes) {
				var ref:int = pn.id.length;
				for(var i:int = 0; i < pn.id.length; i++) {
					ref += pn.id.charCodeAt(i)*(i+1);
				}
				nodeReferences[pn.id] = ref;
				if(pn.IsSwitch())
					switchReferences[pn.id] = ref + 537;
			}
			// Combine switches into nodes
			for each(var currentNode:PhysicalNode in AllNodes) {
				// Already added, probably a leaf node
				if(!nodesToAdd.contains(currentNode))
					continue;
				ref = nodeReferences[currentNode.id];
				
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
					var connectedNodes:Vector.<PhysicalNode> = new Vector.<PhysicalNode>();
					for each(var nac:PhysicalNode in a) {
						connectedNodes = Util.keepUniqueObjects(nac.GetNodes(), connectedNodes);
					}
					// Create the switch/switch-group ref
					ref = nodeReferences[currentNode.id];
					for each(nac in connectedNodes) {
						ref += nodeReferences[nac.id];
					}
					// group empty routers together
					if(ref == nodeReferences[currentNode.id] && levels > 0) {
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
						nodeReferences[nac.id] = ref;
					}
					if(locationReferences[ref] != null)
						Alert.show("Duplicate reference!!!!", "Problem",4, Main.Application());
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
						ref += switchReferences[connectedNode.id];
					else
						ref += nodeReferences[connectedNode.id];
				}
				if(locationReferences[ref] != null) {
					var otherLocation:Object = locationReferences[ref];
					otherLocation.list.addItem(currentNode);
					otherLocation.name = otherLocation.list.length + " Nodes";
					locationReferences[nodeReferences[currentNode.id]] = otherLocation;
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
						Alert.show("Duplicate reference!!!!", "Problem",4, Main.Application());
					locationReferences[ref] = newLocation;
					locationReferences[nodeReferences[currentNode.id]] = newLocation;
				}
			}
			
			// THIRD PASS
			// Replace links to nodes with links to locations
			for each(var location:Object in locations) {
				var linkedLocations:ArrayCollection = new ArrayCollection();
				var linkedLocationsNums:ArrayCollection = new ArrayCollection();
				for each(connectedNode in location.linked) {
					var linkedLocation:Object = locationReferences[nodeReferences[connectedNode.id]];
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
		*/
	}
}