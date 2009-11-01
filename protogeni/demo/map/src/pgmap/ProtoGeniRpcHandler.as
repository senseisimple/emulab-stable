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
	import flash.utils.ByteArray;
	
	import mx.utils.Base64Decoder;
    	
	public class ProtoGeniRpcHandler
	{
		public var op : Operation;
		public var opName : String;
		public var slice : Slice;
		
		[Bindable]
		public var useCompression : Boolean = true;
		
		public var sliceUrns : Array;
		public var sliceNodes : Array;
		public var sliceNodesStatus : Array;
		
		public var allSlices : Array;
		public var slicesLeft : Array;
		public var allSlivers : Array;
		public var sliversLeft : Array;
		
		public var main : pgmap;
		
		public var ComponentManagerURL:String = "https://boss.emulab.net:443/protogeni/xmlrpc/cm";
		
		public var Rspec:XML = null;
		
		[Bindable]
		public var Components:Array = null;
		
		public var AfterCall:Function;
		
		public function ProtoGeniRpcHandler()
		{
			op = new Operation(null);
			opName = null;
		}
		
		public function clearAll() : void
		{
			Components = null;
			Rspec = null;
		}
		
		public function prepareSlices():void {
			slicesLeft = new Array();
			for each(var s:Slice in allSlices) {
				slicesLeft.push(s);
			}
		}
		
		public function prepareSlivers():void {
			sliversLeft = new Array();
			for each(var s:Slice in allSlices) {
				if(s.status == "ready")
					sliversLeft.push(s);
			}
		}
	    
	    public function postCall() : void {
	    		main.console.appendText("Seeing if there are any other method to call...\n");
	    	if(AfterCall != null) {
	    		main.console.appendText("Doing a post call...\n");
	        	AfterCall();
	        }
	    }
	    
	    public function failure(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.openConsole();
	    	main.setProgress("Operation failed!", Common.failColor);
	    	main.stopWaiting();
	    	main.console.appendText("****fail****");
	    	var msg : String = "";
	      if (fault != null)
	      {
	      	msg = fault.getFaultString();
	        main.console.appendText("\nFAILURE fault: " + opName + ": "
	                                + msg);
	      }
	      else
	      {
	      	msg = event.toString();
	        main.console.appendText("\nFAILURE event: " + opName + ": "
	                                + msg);
	        if(msg.search("#2048") > -1)
	        	main.console.appendText("\nStream error, possibly due to server error");
	        else if(msg.search("#2032") > -1)
	        	main.console.appendText("\nIO Error, possibly due to server problems or you have no SSL certificate");
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
	    
	    public function startCredential() : void
	    {
	      main.console.clear();
	      	      
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
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	    	main.console.appendText("Acquiring credential complete...\n");
	      addResponse();
	      if (code == 0)
	      {
	      	main.pgHandler.CurrentUser.credential = String(response.value);
	      	var cred:XML = new XML(response.value);
	      	main.pgHandler.CurrentUser.uuid = cred.credential.owner_urn;
	        postCall();
	      }
	      else
	      {
	        codeFailure();
	      }
	    }
	    
	    public function startListComponents() : void
	    {
	      opName = "Looking up components";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName + "...\n");
	      op.reset(Geni.listComponents);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
	      op.call(completeListComponents, failure);
	    }
	    
	    public function completeListComponents(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	    	main.console.appendText("List Components complete...\n");
	      if (code == 0)
	      {
	      	Components = response.value;
	      	main.console.appendText(op.getResponseXml());
	      	postCall();
	      }
	      else
	      {
	        codeFailure();
	        main.console.appendText(op.getResponseXml());
	      }
	    }
	    
	    public function startResourceLookup() : void
	    {
	      opName = "Looking up resources";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName + "...\n");
	      op.reset(Geni.discoverResources);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      if(useCompression)
	      	op.addField("compress", true);
	      op.setExactUrl(ComponentManagerURL);
	      op.call(completeResourceLookup, failure);
	    }
	
	    public function completeResourceLookup(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	    	main.console.appendText("Resource lookup complete...\n");

	      if (code == 0)
	      {
	      	if(useCompression) {
	      		var decodor:Base64Decoder = new Base64Decoder();
		      	decodor.decode(response.value);
		      	var bytes:ByteArray = decodor.toByteArray();
		      	bytes.uncompress();
		      	var decodedRspec:String = bytes.toString();
		        Rspec = new XML(decodedRspec);
	      	} else
	      		Rspec = new XML(response.value);
	      	
	      	//var d1:Date = new Date();
	        main.pgHandler.processRspec(startResolveUser);
	        //var d2:Date = new Date();
	        //Alert.show((d2.time - d1.time).toString());
	      }
	      else
	      {
	        codeFailure();
	        main.console.appendText(op.getResponseXml());
	      }
	    }
	    
	    public function startResolveUser() : void
	    {
	      main.pgHandler.CurrentUser.slices.removeAll();
	      opName = "Resolving user";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName + "...\n");
	      op.reset(Geni.resolve);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("uuid", main.pgHandler.CurrentUser.uuid);
	      op.addField("type", "User");
	      op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
	      op.call(completeResolveUser, failure);
	    }
	    
	    public function completeResolveUser(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	    	main.console.appendText("Resolve user complete...\n");
	      if (code == 0)
	      {
	      	main.pgHandler.CurrentUser.uid = response.value.uid;
	      	main.pgHandler.CurrentUser.hrn = response.value.hrn;
	      	main.pgHandler.CurrentUser.uuid = response.value.uuid;
	      	main.pgHandler.CurrentUser.email = response.value.email;
	      	main.pgHandler.CurrentUser.name = response.value.name;
	      	
	      	sliceUrns = response.value.slices;
	      	if(sliceUrns != null && sliceUrns.length > 0) {
	      		allSlices = new Array();
	      		startSliceLookup();
	      	}
	      	else
	      		main.pgHandler.map.drawAll();
	      }
	      else
	      {
	        codeFailure();
	        main.console.appendText(op.getResponseXml());
	      }
	    }
	    
	    public function startSliceLookup() : void
	    {
	      opName = "Looking up " + sliceUrns.length + " more slice(s)";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.resolve);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("hrn", sliceUrns.pop());
	      op.addField("type", "Slice");
	      op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
		  op.call(completeSliceLookup, failure);
	      addSend();
	    }
	    
	    public function completeSliceLookup(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	slice = new Slice();
	      	slice.uuid = response.value.uuid;
	      	slice.creator = main.pgHandler.CurrentUser;
	      	slice.hrn = response.value.hrn;
	      	
	      	allSlices.push(slice);
	      	main.pgHandler.CurrentUser.slices.addItem(slice);

			if(sliceUrns.length > 0)
				startSliceLookup();
			else {
				prepareSlices();
	      		startSliceCredential();
			}
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawAll();
	      }
	    }
	    
	    public function startSliceCredential() : void
	    {
	      opName = "Acquiring " + slicesLeft.length + " more slice credential(s)";
	      slice = slicesLeft.pop();
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.getCredential);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("uuid", slice.uuid);
	      op.addField("type", "Slice");
	      op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
		  op.call(completeSliceCredential, failure);
	      addSend();
	    }
	    
	    public function completeSliceCredential(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	slice.credential = String(response.value);
	      	
	      	if(slicesLeft.length > 0)
				startSliceCredential();
			else {
				prepareSlices();
				startSliceStatus();
			}
	      		
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawAll();
	      }
	    }
	    
	    public function startSliceStatus() : void
	    {
	      opName = "Acquiring " + slicesLeft.length + " more slice status report(s)";
	      slice = slicesLeft.pop();
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.sliceStatus);
	      op.addField("credential", slice.credential);
	      op.setUrl(ComponentManagerURL);
		  op.call(completeSliceStatus, failure);
	      addSend();
	    }
	    
	    public function completeSliceStatus(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	slice.status = response.value.status;
	      	
	      	/*
	      	
	      	sliceNodes = new Array();
	      	sliceNodesStatus = new Array();
	      	// !!!IMPROTANT!!!
	      	// Iterates over the keys into the array instead of the actual values
	      	for (var urn : String in response.value.detailsNew) {
	      		sliceNodes.push(urn);
	      	}
	      	for each (var ready : String in response.value.detailsNew) {
	      		sliceNodesStatus.push(ready);
	      	}
	      	if(sliceNodes.length > 0)
	      		startResolveNodes();
	      	else
	      		postCall();
	      	*/
	      }
	      else
	      {
	        //codeFailure();
	        //main.pgHandler.map.drawAll();
	      }
	      
	      if(slicesLeft.length > 0)
	      		startSliceStatus();
	      	else {
	      		prepareSlivers();
	      		if(sliversLeft.length == 0) {
	      			main.pgHandler.map.drawAll();
	      		} else {
	      			startGetSliver();
	      		}
	      	}
	    }
	    
	    public function startGetSliver() : void
	    {
	      opName = "Acquiring " + sliversLeft.length + " more sliver credential(s)";
	      slice = sliversLeft.pop();
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.getSliver);
	      op.addField("credential", slice.credential);
	      op.setUrl(ComponentManagerURL);
		  op.call(completeGetSliver, failure);
	      addSend();
	    }
	    
	    public function completeGetSliver(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	slice.sliverCredential = String(response.value);
	      	
	      	if(sliversLeft.length > 0)
	      		startGetSliver();
	      	else {
	      		prepareSlivers();
	      		startSliverStatus();
	      	}
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawAll();
	      }
	    }
	    
	    public function startSliverStatus() : void
	    {
	      opName = "Acquiring " + sliversLeft.length + " more sliver status report(s)";
	      slice = sliversLeft.pop();
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.sliverStatus);
	      op.addField("credential", slice.sliverCredential);
	      op.setUrl(ComponentManagerURL);
		  op.call(completeSliverStatus, failure);
	      addSend();
	    }
	    
	    public function completeSliverStatus(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	slice.sliverStatus = response.value.status;
	      	
	      	if(sliversLeft.length > 0)
	      		startSliverStatus();
	      	else {
	      		prepareSlivers();
	      		startSliverTicket();
	      	}
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawAll();
	      }
	    }
	    
	    public function startSliverTicket() : void
	    {
	      opName = "Acquiring " + sliversLeft.length + " more sliver ticket(s)";
	      slice = sliversLeft.pop();
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.sliverTicket);
	      op.addField("credential", slice.sliverCredential);
	      op.setUrl(ComponentManagerURL);
		  op.call(completeSliverTicket, failure);
	      addSend();
	    }
	    
	    public function completeSliverTicket(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	var sliverXml:XML = new XML(response.value);
	      	var sliverRspec:XMLList = sliverXml.descendants("rspec").children();
	      	if(sliverRspec.length() > 0) {
	      		for each(var component:XML in sliverRspec) {
	      			if(component.localName() == "node") {
	      				var node:Node = main.pgHandler.Nodes.GetByUrn(component.component_urn);
	      				node.slice = slice;
	      				node.sliverUrn = component.sliver_urn;
	      				node.sliverUuid = component.sliver_uuid;
	      				node.sliverRspec = component;
	      				slice.Nodes.addItem(node);
	      			} else if(component.localName() == "link") {
	      				var nodeNames:XMLList = component.descendants("virtual_node_id");
	      				var link:PointLink = new PointLink();
	      				link.node1 = main.pgHandler.Nodes.GetByName(nodeNames[0]);
	      				link.node2 = main.pgHandler.Nodes.GetByName(nodeNames[1]);
	      				link.type = component.@link_type;
	      				link.slice = slice;
	      				slice.Links.addItem(link);
	      			}
	      		}
	      	}
	      	
	      	if(sliversLeft.length > 0)
	      		startSliverTicket();
	      	else
	      		main.pgHandler.map.drawAll();
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawAll();
	      }
	    }
	    
	    /* ************ NOT IN USE CURRENTLY ************ */
	    
	    public function startResolveNodes() : void
	    {
	      opName = "Resolving " + sliceNodes.length + " more node(s)";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.resolveNode);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("urn", sliceNodes.pop());
	      op.addField("type", "Node");
		  op.call(completeResolveNodes, failure);
	      addSend();
	    }
	    
	    public function completeResolveNodes(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	//main.pgHandler.addSliceNode(response.value.urn, sliceNodesStatus.pop(), slice);
	      	// GOT INfO ABOUT NODE
	      	if(this.sliceNodes.length > 0)
	      		startResolveNodes();
	      	else
	      		main.pgHandler.map.drawAll();
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawAll();
	      }
	    }
	    
	    public function startResolveNode(n:Node) : void
	    {
	      opName = "Resolving Node: " + n.urn + "\n";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.resolveNode);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("urn", n.urn);
	      op.addField("type", "Node");
		  op.call(completeResolveNode, failure);
	      addSend();
	    }
	    
	    public function completeResolveNode(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	main.pgHandler.showResolveNodeResult();
	      	//main.pgHandler.addSliceNode(response.value.urn, sliceNodesStatus.pop(), slice);
	      	// GOT INfO ABOUT NODE
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawAll();
	      }
	    }
	}
}