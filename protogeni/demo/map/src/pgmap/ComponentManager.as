package pgmap
{
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Alert;
	
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
		public var Show : Boolean = false;
		
		public var Message : String = "";
		
		public var Nodes:PhysicalNodeGroupCollection = new PhysicalNodeGroupCollection();
		public var Links:PhysicalLinkGroupCollection = new PhysicalLinkGroupCollection();
		
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
	    
	    public function clear():void
	    {
	    	Nodes = new PhysicalNodeGroupCollection();
			Links = new PhysicalLinkGroupCollection();
			Rspec = null;
			Status = ComponentManager.UNKOWN;
			Message = "";
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

	}
}