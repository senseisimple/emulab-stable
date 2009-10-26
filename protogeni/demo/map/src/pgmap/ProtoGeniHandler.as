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
	import flash.events.Event;
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Alert;
	import mx.managers.PopUpManager;
	
	public class ProtoGeniHandler
	{
		public var main : pgmap;
		
		[Bindable]
		public var rpc : ProtoGeniRpcHandler;
		
		[Bindable]
		public var map : ProtoGeniMapHandler;
		
		[Bindable]
		public var CurrentUser:User = new User();
		
		public var Nodes:NodeGroupCollection = new NodeGroupCollection();
		public var Links:LinkGroupCollection = new LinkGroupCollection();
		
		public var Slices:ArrayCollection = new ArrayCollection();
		
		public function ProtoGeniHandler(m:pgmap)
		{
			rpc = new ProtoGeniRpcHandler();
			map = new ProtoGeniMapHandler();
			rpc.main = m;
			map.main = m;
			main = m;
		}
		
		public function clear() : void
		{
			Nodes = new NodeGroupCollection();
			Links = new LinkGroupCollection();
			Slices = new ArrayCollection();
		}
		
		public function getCredential(afterCompletion : Function):void {
			rpc.AfterCall = afterCompletion;
			rpc.startCredential();
		}
		
		public function guarenteeCredential(afterCompletion : Function):void {
			if(CurrentUser.credential != null)
				afterCompletion();
			else
				getCredential(afterCompletion);
		}
		
		public function getComponents(afterCompletion : Function):void {
			rpc.AfterCall = afterCompletion;
			rpc.startListComponents();
		}
		
		public function getResourcesAndSlices():void {
			clear();
			rpc.startResourceLookup();
		}
		
		public function showResolveNode(n:Node):void {
			rpc.startResolveNode(n);
		}
		
		public function showResolveNodeResult():void {
			var rspecView:XmlWindow = new XmlWindow();
			PopUpManager.addPopUp(rspecView, main, false);
   			PopUpManager.centerPopUp(rspecView);
   			//rspecView.loadXml(node.rspec);
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
	    
	    public function processRspec(afterCompletion : Function):void {
	    	main.setProgress("Parsing RSPEC", Common.waitColor);
	    	main.startWaiting();
	    	
	    	namespace rsync01namespace = "http://www.protogeni.net/resources/rspec/0.1"; 
	        use namespace rsync01namespace; 
	        
	    	myAfter = afterCompletion;
	    	myIndex = 0;
	    	myState = NODE_PARSE;
	    	locations = rpc.Rspec.node.location;
	    	links = rpc.Rspec.link;
	    	interfaceDictionary = new Dictionary();
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
				Common.Main().stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				myAfter();
			}
			else
			{
				main.setProgress("Fail", Common.failColor);
	    		main.stopWaiting();
	    		interfaceDictionary = null;
				Common.Main().stage.removeEventListener(Event.ENTER_FRAME, parseNext);
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
	        		myState = LINK_PARSE;
	        		myIndex = 0;
	        		//main.console.appendText("...finished nodes\n");
	        		return;
	        	}
	        	//main.console.appendText("parsing...");
	        	main.console.appendText(fullIdx.toString());
	        	var location:XML = locations[fullIdx];
	        	
	        	var lat:Number = Number(location.@latitude);
	        	var lng:Number = Number(location.@longitude);
	        	
	        	var ng:NodeGroup = Nodes.GetByLocation(lat,lng);
	        	if(ng == null) {
	        		var cnt:String = location.@country;
	        		ng = new NodeGroup(lat, lng, cnt, Nodes);
	        		Nodes.Add(ng);
	        	}
	        	
	        	var n:Node = new Node(ng);
	        	var p:XML = location.parent();
	        	n.name = p.@component_name;
	        	n.urn = p.@component_uuid;
	        	n.available = p.available == "true";
	        	n.exclusive = p.exclusive == "true";
	        	n.manager = p.@component_manager_uuid;
	        	
	        	for each(var ix:XML in p.children()) {
	        		if(ix.localName() == "interface") {
	        			var i:NodeInterface = new NodeInterface(n);
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
	        	var ni1:NodeInterface = interfaceDictionary[interface1];
	        	if(ni1 != null) {
	        		var interface2:String = interfaces[1].@component_interface_id;
		        	//var ni2:NodeInterface = Nodes.GetInterfaceByID(interface2);
	        		var ni2:NodeInterface = interfaceDictionary[interface2];
		        	if(ni2 != null) {
		        		var lg:LinkGroup = Links.Get(ni1.owner.GetLatitude(), ni1.owner.GetLongitude(), ni2.owner.GetLatitude(), ni2.owner.GetLongitude());
		        		if(lg == null) {
		        			lg = new LinkGroup(ni1.owner.GetLatitude(), ni1.owner.GetLongitude(), ni2.owner.GetLatitude(), ni2.owner.GetLongitude(), Links);
		        			Links.Add(lg);
		        		}
		        		var l:Link = new Link(lg);
		        		l.name = link.@component_name;
		        		l.manager = link.@component_manager_uuid;
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
	}
}