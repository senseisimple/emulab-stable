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
		
		public var sliceNodes : Array;
		public var sliceNodesStatus : Array;
		
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
	      addResponse();
	      if (code == 0)
	      {
	        main.pgHandler.CurrentUser.credential = String(response.value);
	        postCall();
	      }
	      else
	      {
	        codeFailure();
	      }
	    }
	    
	    public function startResolveUser() : void
	    {
	      opName = "Looking up user";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName + "...\n");
	      op.reset(Geni.resolve);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("uuid", "66f3b32e-9666-11de-9be3-001143e453fe");
	      op.addField("type", "User");
	      op.call(completeResolveUser, failure);
	    }
	    
	    public function completeResolveUser(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	    	main.console.appendText("List Components complete...\n");
	      if (code == 0)
	      {
	      	postCall();
	      }
	      else
	      {
	        codeFailure();
	        main.console.appendText(op.getResponseXml());
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
	      op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc/ch");
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
	      	if(useCompression) {
	      		var decodor:Base64Decoder = new Base64Decoder();
		      	decodor.decode(response.value);
		      	var bytes:ByteArray = decodor.toByteArray();
		      	bytes.uncompress();
		      	var decodedRspec:String = bytes.toString();
		        Rspec = new XML(decodedRspec);
	      	} else
	      		Rspec = new XML(response.value);
	        main.pgHandler.processRspec(null);
	        
	        startSliceLookup();
	      }
	      else
	      {
	        codeFailure();
	        main.console.appendText(op.getResponseXml());
	      }
	    }
	    
	    public function startSliceLookup() : void
	    {
	      main.console.clear();
	      	      
	      opName = "Looking up slice";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.resolve);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("hrn", "mapslice");
	      op.addField("type", "Slice");
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
	      	main.pgHandler.Slices.addItem(slice);
	      	startSliceCredential();
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawMap();
	      }
	    }
	    
	    public function startSliceCredential() : void
	    {
	      main.console.clear();
	      	      
	      opName = "Acquiring slice credential";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.getCredential);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("uuid", slice.uuid);
	      op.addField("type", "Slice");
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
	      	startSliceStatus();
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawMap();
	      }
	    }
	    
	    public function startSliceStatus() : void
	    {
	      main.console.clear();
	      	      
	      opName = "Acquiring slice status";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.sliceStatus);
	      op.addField("credential", slice.credential);
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
	      	sliceNodes = new Array();
	      	sliceNodesStatus = new Array();
	      	// !!!IMPROTANT!!!
	      	// Iterates over the keys into the array instead of the actual values
	      	for (var urn : String in response.value.details) {
	      		sliceNodes.push(urn);
	      	}
	      	for each (var ready : String in response.value.details) {
	      		sliceNodesStatus.push(ready);
	      	}
	      	if(sliceNodes.length > 0)
	      		startResolveNodes();
	      	else
	      		postCall();
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawMap();
	      }
	    }
	    
	    public function startResolveNodes() : void
	    {
	      main.console.clear();
	      	      
	      opName = "Resolving " + sliceNodes.length + " more node(s)";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.resolveNode);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("uuid", sliceNodes.pop());
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
	      	main.pgHandler.addSliceNode(response.value.urn, sliceNodesStatus.pop(), slice);
	      	// GOT INfO ABOUT NODE
	      	if(this.sliceNodes.length > 0)
	      		startResolveNodes();
	      	else
	      		main.pgHandler.map.drawMap();
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawMap();
	      }
	    }
	}
}