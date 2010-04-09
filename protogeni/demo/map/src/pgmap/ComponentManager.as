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
	import mx.controls.Alert;
	
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
		
		public var Urn : String = "";
		
		[Bindable]
		public var Show : Boolean = false;
		
		public var errorMessage : String = "";
		public var errorDescription : String = "";
		
		public var Nodes:PhysicalNodeGroupCollection = new PhysicalNodeGroupCollection();
		public var Links:PhysicalLinkGroupCollection = new PhysicalLinkGroupCollection();
		public var AllNodes:ArrayCollection = new ArrayCollection();
		
		[Bindable]
		public var Status : int = UNKOWN;
		
		public var main : pgmap;
	    
		public function ComponentManager()
		{
			main = Common.Main();
		}
		
		public function DiscoverResourcesUrl():String
		{
			if(Hrn == "wail.cm" || Hrn == "cmulab.cm" || Hrn == "ukgeni.cm")
				return Url + "v2";
			else
				return Url;
		}
		
		public function VisitUrl():String
		{
			return Url.substr(0, Url.length-3);
		}
		
		private static var NODE_PARSE : int = 0;
	    private static var LINK_PARSE : int = 1;
	    private static var DONE : int = 2;
	    
	    private static var MAX_WORK : int = 60;
	    
	    private var myAfter:Function;
	    private var myIndex:int;
	    private var myState:int = NODE_PARSE;
	    private var locations:XMLList;
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
			Rspec = null;
			Status = ComponentManager.UNKOWN;
			errorMessage = "";
			errorDescription = "";
	    }
	    
	    public function processRspec(afterCompletion : Function):void {
	    	main.setProgress("Parsing " + Hrn + " RSPEC", Common.waitColor);
	    	main.startWaiting();
	    	
	    	namespace rsync01namespace = "http://www.protogeni.net/resources/rspec/0.1"; 
	        use namespace rsync01namespace; 

	    	myAfter = afterCompletion;
	    	myIndex = 0;
	    	myState = NODE_PARSE;
	    	locations = Rspec.node.location;
	    	links = Rspec.link;
	    	interfaceDictionary = new Dictionary();
	    	nodeNameDictionary = new Dictionary();
	    	subnodeList = new ArrayCollection();
	    	main.stage.addEventListener(Event.ENTER_FRAME, parseNext);
	    }
	    
	    private function parseNext(event : Event) : void
	    {
			if (myState == NODE_PARSE)	    	
			{
	    		//main.setProgress("Parsing " + (locations.length() - myIndex) + " more nodes", Common.waitColor);
				parseNextNode();
			}
			else if (myState == LINK_PARSE)
			{
	    		//main.setProgress("Parsing " + (links.length() - myIndex) + " more links", Common.waitColor);
				parseNextLink();
			}
			else if (myState == DONE)
			{
				main.setProgress("Done", Common.successColor);
	    		main.stopWaiting();
	    		interfaceDictionary = null;
	    		nodeNameDictionary = null;
	    		subnodeList = null;
	      		Status = ComponentManager.VALID;
	      		main.chooseCMWindow.ResetStatus(this);
	      		main.pgHandler.map.drawMap();
				main.stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				myAfter();
			}
			else
			{
				main.setProgress("Fail", Common.failColor);
	    		main.stopWaiting();
	    		interfaceDictionary = null;
	    		nodeNameDictionary = null;
	    		subnodeList = null;
				main.stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				Alert.show("Problem parsing RSPEC");
				// Throw exception
			}
	    }
	    
	    private function parseNextNode():void {
	    	namespace rsync01namespace = "http://www.protogeni.net/resources/rspec/0.1"; 
	        use namespace rsync01namespace; 
	        
	        var idx:int;
	        for(idx = 0; idx < MAX_WORK; idx++) {
	        	var fullIdx:int = myIndex + idx;
	        	//main.console.appendText("idx:" + idx.toString() + " full:" + fullIdx.toString());
	        	if(fullIdx == locations.length()) {
	        		
	        		// Assign subnodes
	        		for each(var obj:Object in subnodeList)
	        		{
	        			var parentNode:PhysicalNode = nodeNameDictionary[obj.parentName];
	        			parentNode.subNodes.addItem(obj.subNode);
	        			obj.subNode.subNodeOf = parentNode;
	        		}
	        		
	        		myState = LINK_PARSE;
	        		myIndex = 0;
	        		//main.console.appendText("...finished nodes\n");
	        		return;
	        	}
	        	//main.console.appendText("parsing...");
	        	//main.console.appendText(fullIdx.toString());
	        	var location:XML = locations[fullIdx];
	        	
	        	var lat:Number = Number(location.@latitude);
	        	var lng:Number = Number(location.@longitude);
	        	if(lat == 0 && lng == 0)
	        	{
	        		lat = 39.017625;
	        		lng = 125.763899;
	        	}
	        	
	        	var ng:PhysicalNodeGroup = Nodes.GetByLocation(lat,lng);
	        	if(ng == null) {
	        		var cnt:String = location.@country;
	        		ng = new PhysicalNodeGroup(lat, lng, cnt, Nodes);
	        		Nodes.Add(ng);
	        	}
	        	
	        	var n:PhysicalNode = new PhysicalNode(ng);
	        	var p:XML = location.parent();
	        	n.name = p.@component_name;
	        	n.urn = p.@component_uuid;
	        	n.available = p.available == "true";
	        	n.exclusive = p.exclusive == "true";
	        	n.managerString = p.@component_manager_uuid;
	        	n.manager = this;
	        	
	        	var parentName:String = p.subnode_of;
	        	if(parentName.length > 0)
	        		subnodeList.addItem({subNode:n, parentName:parentName});
	        	
	        	for each(var ix:XML in p.children()) {
	        		if(ix.localName() == "interface") {
	        			var i:PhysicalNodeInterface = new PhysicalNodeInterface(n);
	        			i.id = ix.@component_id;
	        			n.interfaces.Add(i);
	        			interfaceDictionary[i.id] = i;
	        		} else if(ix.localName() == "node_type") {
	        			var t:NodeType = new NodeType();
	        			t.name = ix.@type_name;
	        			t.slots = ix.@type_slots;
	        			t.isStatic = ix.@static;
	        			n.types.addItem(t);
	        		}
	        	}
	        	
	        	n.rspec = p.copy();
	        	ng.Add(n);
	        	nodeNameDictionary[n.urn] = n;
	        	nodeNameDictionary[n.name] = n;
	        	AllNodes.addItem(n);
	        	
	        	//main.console.appendText("done\n");
	        }
	        myIndex += idx;
	    }
	    
	    private function parseNextLink():void {
	    	namespace rsync01namespace = "http://www.protogeni.net/resources/rspec/0.1"; 
	        use namespace rsync01namespace; 
	        
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
	        	
	        	var interfaces:XMLList = link.interface_ref;
	        	var interface1:String = interfaces[0].@component_interface_id
	        	//var ni1:NodeInterface = Nodes.GetInterfaceByID(interface1);
	        	var ni1:PhysicalNodeInterface = interfaceDictionary[interface1];
	        	if(ni1 != null) {
	        		var interface2:String = interfaces[1].@component_interface_id;
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
		        		l.bandwidth = link.bandwidth;
		        		l.latency = link.latency;
		        		l.packetLoss = link.packet_loss;
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
	        	
	        	//main.console.appendText("done\n");
	        }
	        myIndex += idx;
	    }

		public function getDotGraph():String {
			var added:Dictionary = new Dictionary();
			
			// Start graph
			var dot:String = "graph " + Common.getDotString(Hrn) + " {\n" +
				"\toverlap=scale;\n" + 
				"\tsize=\"10,10\";\n" +
				"\tfontsize=20;\n" +
				"\tnode [fontsize=300];\n" +
				"\tedge [style=bold];\n";
			
			var nodesToAdd:ArrayCollection = new ArrayCollection(AllNodes.toArray());
			var nodeGroups:ArrayCollection = new ArrayCollection();

			// Add nodes and combine similar nodes together
			for each(var currentNode:PhysicalNode in AllNodes) {
				// Give nodes any special qualities, otherwise see if they need to be grouped
				if(currentNode.IsSwitch())
					dot += "\t" + Common.getDotString(currentNode.name) + " [shape=box3d, style=filled, color=deepskyblue3, height=20, width=30];\n";
				else if(currentNode.subNodeOf != null)
					dot += "\t" + Common.getDotString(currentNode.name) + " [style=dotted, color=palegreen];\n";
				else if(currentNode.subNodes != null && currentNode.subNodes.length > 0)
					dot += "\t" + Common.getDotString(currentNode.name) + " [style=filled, color=palegreen];\n";
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
							var newGroup:Object = {switches: new ArrayCollection(connectedSwitches.toArray()), count: 1, name: Common.getDotString(currentNode.name), original: currentNode};
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
					if(connectedNode.IsSwitch() && currentNode.IsSwitch())
						dot += "\t" + Common.getDotString(currentNode.name) + " -- " + Common.getDotString(connectedNode.name) + " [style=bold, color=deepskyblue3, penwidth=60, len=0.2, weight=6, width=10, height=10];\n";
					else
						dot += "\t" + Common.getDotString(currentNode.name) + " -- " + Common.getDotString(connectedNode.name) + " [penwidth=8, len=0.3, weight=.8];\n";
				}
				if(currentNode.subNodes != null && currentNode.subNodes.length > 0) {
					for each(var subNode:PhysicalNode in currentNode.subNodes) {
						dot += "\t" + Common.getDotString(currentNode.name) + " -- " + Common.getDotString(subNode.name) + " [style=dotted, len=0.1, weight=5, penwidth=2, color=palegreen1];\n";
					}
				}
				added[currentNode.urn] = currentNode;
			}
			
			// Build up node groups
			for each(var nodeGroup:Object in nodeGroups) {
				dot += "\t" + nodeGroup.name + " [style=filled, height="+.25*nodeGroup.count+", width="+.375*nodeGroup.count+", color=limegreen, label=\""+nodeGroup.count+" Nodes\"];\n";
				for each(connectedNode in nodeGroup.switches) {
					dot += "\t" + nodeGroup.name + " -- " + Common.getDotString(connectedNode.name) + " [style=bold, color=limegreen, penwidth=26, len=0.35, weight=2];\n";
				}
			}
			
			return dot + "}";
		}

	}
}