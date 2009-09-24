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
	import com.mattism.http.xmlrpc.MethodFault;
	
	import flash.events.ErrorEvent;
    	
	public class ProtoGeniHandler
	{
		public var op : Operation;
		public var opName : String;
		public var credential : Credential;
		
		public var main : pgmap;
		
		public var ComponentManagerURL:String = "https://boss.emulab.net:443/protogeni/xmlrpc/cm";
		
		public var Rspec:XML = null;
		
		public var Nodes:NodeGroupCollection = new NodeGroupCollection();
		public var Links:LinkGroupCollection = new LinkGroupCollection();
		
		public function ProtoGeniHandler()
		{
			op = new Operation(null);
			opName = null;
			credential = null;
			credential = new Credential();
		}
		
		public function clear() : void
		{
			Nodes = new NodeGroupCollection();
			Links = new LinkGroupCollection();
			Rspec = null;
		}
		
		public function startCredential() : void
	    {
	      main.console.clear();
	      
	      clear();
	      
	      opName = "Acquiring credential";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.getCredential);
		  op.call(completeCredential, failure);
	      addSend();
	    }
	    
		public function completeCredential(code : Number, response : Object) : void
	    {
	      addResponse();
	      if (code == 0)
	      {
	        credential.base = String(response.value);
	        startSshLookup();
	      }
	      else
	      {
	        codeFailure();
	      }
	    }
	    
	    public function failure(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.openConsole();
	    	main.setProgress("Operation failed!", Common.failColor);
	    	main.stopWaiting();
	    	main.console.appendText("****fail****");
	      if (fault != null)
	      {
	        main.console.appendText("\nFAILURE fault: " + opName + ": "
	                                + fault.getFaultString());
	      }
	      else
	      {
	        main.console.appendText("\nFAILURE event: " + opName + ": "
	                                + event.toString());
	      }
	      main.console.appendText("\nURL: " + op.getUrl());
	    }
	
	    public function codeFailure() : void
	    {
	      main.console.appendText("Code Failure: " + opName);
	    }
	    
	    public function addSend() : void
	    {
	      main.console.appendText("\n-----------------------------------------\n");
	      main.console.appendText("SEND: " + opName + "\n");
	      main.console.appendText("URL: " + op.getUrl() + "\n");
	      main.console.appendText("-----------------------------------------\n\n");
	      main.console.appendText(op.getSendXml());
	//      clip.xmlText.scrollV = clip.xmlText.maxScrollV;
	    }
	    
	    public function addResponse() : void
	    {
	      main.console.appendText("\n-----------------------------------------\n");
	      main.console.appendText("RESPONSE: " + opName + "\n");
	      main.console.appendText("-----------------------------------------\n\n");
	      main.console.appendText(op.getResponseXml());
	    }
	    
	    public function startSshLookup() : void
	    {
	      opName = "Acquiring SSH Keys";
	      main.setProgress(opName, Common.waitColor);
	      main.console.appendText(opName);
	      op.reset(Geni.getKeys);
	      op.addField("credential", credential.base);
	      op.call(completeSshLookup, failure);
	      addSend();
	    }
	
	    public function completeSshLookup(code : Number, response : Object) : void
	    {
	      addResponse();
	      if (code == 0)
	      {
	        credential.ssh = response.value;
	        startResourceLookup();
	//        startUserLookup();
	      }
	      else
	      {
	        codeFailure();
	      }
	    }
	    
	    public function startResourceLookup() : void
	    {
	      opName = "Looking up resources";
	      main.setProgress(opName, Common.waitColor);
	      main.console.appendText(opName + "...\n");
	      op.reset(Geni.discoverResources);
	      op.addField("credential", credential.base);
	      op.setUrl(ComponentManagerURL);
	      op.call(completeResourceLookup, failure);
	    }
	
	    public function completeResourceLookup(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	    	main.console.appendText("Resource lookup complete...\n");
	      if (code == 0)
	      {
	        Rspec = new XML(response.value);
	        processRspec();
	      }
	      else
	      {
	        codeFailure();
	        main.console.appendText(op.getResponseXml());
	      }
	    }
	    
	    private function processRspec():void {
	    	namespace rsync01namespace = "http://www.protogeni.net/resources/rspec/0.1"; 
	        use namespace rsync01namespace; 
	        
	       // main.console.appendText(String(rspec.toXMLString()) + "\n");
	        main.console.appendText("Processing RSPEC...\n");
	        
	        // Process nodes
	        var locations:XMLList = Rspec.node.location;
	        main.console.appendText("Detected " + locations.length().toString() + " nodes with location info...\n");
	        
	        // Process nodes, combining same locations
	        for each(var location:XML in locations) {
	        	var lat:Number = Number(location.@latitude);
	        	var lng:Number = Number(location.@longitude);
	        	
	        	main.console.appendText("Found node at " + lat + ", " + lng + "\n");

	        	var ng:NodeGroup = Nodes.GetByLocation(lat,lng);
	        	if(ng == null) {
	        		var cnt:String = location.@country;
	        		ng = new NodeGroup(lat, lng, cnt, Nodes);
	        		Nodes.Add(ng);
	        	}
	        	
	        	var n:Node = new Node(ng);
	        	var p:XML = location.parent();
	        	n.name = p.@component_name;
	        	n.uuid = p.@component_uuid;
	        	n.available = Boolean(p.available);
	        	n.exclusive = Boolean(p.exclusive);
	        	n.manager = p.@component_manager_uuid;
	        	
	        	for each(var ix:XML in p.children()) {
	        		if(ix.localName() == "interface") {
	        			var i:NodeInterface = new NodeInterface(n);
	        			i.id = ix.@component_id;
	        			n.interfaces.Add(i);
	        		} else if(ix.localName() == "node_type") {
	        			var t:NodeType = new NodeType();
	        			t.name = ix.@type_name;
	        			t.slots = ix.@type_slots;
	        			t.isStatic = ix.@static;
	        			n.types.addItem(t);
	        		}
	        	}
	        	
	        	n.rspec = p.copy();
	        	main.console.appendText(" ... Name: " + n.name + "\n ... UUID: " + n.uuid +  "\n ... Available: " + n.available +  "\n ... Exclusive: " + n.exclusive + "\n");
	        	ng.Add(n);
	        }
	        
	        // Process links
	        var links:XMLList = Rspec.link;
	        for each(var link:XML in links) {
	        	var interfaces:XMLList = link.interface_ref;
	        	// 1
	        	var interface1:String = interfaces[0].@component_interface_id
	        	var ni1:NodeInterface = Nodes.GetInterfaceByID(interface1);
	        	if(ni1 != null) {
	        		var interface2:String = interfaces[1].@component_interface_id;
		        	var ni2:NodeInterface = Nodes.GetInterfaceByID(interface2);
		        	if(ni2 != null) {
		        		var lg:LinkGroup = Links.Get(ni1.owner.GetLatitude(), ni1.owner.GetLongitude(), ni2.owner.GetLatitude(), ni2.owner.GetLongitude());
		        		if(lg == null) {
		        			lg = new LinkGroup(ni1.owner.GetLatitude(), ni1.owner.GetLongitude(), ni2.owner.GetLatitude(), ni2.owner.GetLongitude(), Links);
		        			Links.Add(lg);
		        		}
		        		var l:Link = new Link(lg);
		        		l.name = link.@component_name;
		        		l.manager = link.@component_manager_uuid;
		        		l.uuid = link.@component_uuid;
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
	        }
	        
	        main.console.appendText("Found " + Links.collection.length + " visible links\n");
	        
			main.console.appendText("Detected " + links.length().toString() + " Links...\n");
	
			main.mapHandler.drawMap();
	    }
	}
}