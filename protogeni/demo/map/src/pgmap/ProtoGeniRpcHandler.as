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
	
		[Embed(source="demo-rspec-full.xml",mimeType="application/octet-stream")]
		public static const DemoSliceEmbeddedXml:Class;
		
		[Embed(source="delegatedcredential.xml",mimeType="application/octet-stream")]
		public static const DelegatedCredentialXml:Class;
	
		public var op : Operation;
		public var opName : String;
		
		public var main : pgmap;
		
		public var AfterCall:Function;
		
		public function ProtoGeniRpcHandler()
		{
			op = new Operation(null);
			opName = null;
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
	    	outputFailure(event, fault);
	    }
	    
	    private function outputFailure(event : ErrorEvent, fault : MethodFault) : void
	    {
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
			for each(var obj:Object in response.value)
			{
				var newCm:ComponentManager = new ComponentManager();
				newCm.Hrn = obj.hrn;
				newCm.Url = obj.url;
				main.pgHandler.ComponentManagers.addItem(newCm);
				//break;
			}
	      	main.console.appendText(op.getResponseXml());
	      	postCall();
	      }
	      else
	      {
	        codeFailure();
	        main.console.appendText(op.getResponseXml());
	      }
	    }
	    
	    private var currentIndex : int;
	    private var currentSecondaryIndex : int;
	    private var totalCalls:int = 0;
	    private var nextTotalCalls:int = 0;
	    private var callsMade:int = 0;
	    
	    public function startIndexedCall( after : Function ):void
	    {
	    	currentIndex = 0;
	    	currentSecondaryIndex = 0;
	    	callsMade = 0;
	    	nextTotalCalls = 0;
	    	after();
	    }
	    
	    private var currentCm:ComponentManager;
	    private var currentSlice:Slice;
	    public var currentSliver:Sliver;
	    
	    public function startResourceLookup() : void
	    {
	    	while(currentIndex < main.pgHandler.ComponentManagers.length
	    			&& (main.pgHandler.ComponentManagers[currentIndex] as ComponentManager).Rspec != null)
	    	{
	    		currentIndex++;
	    	}
	    	
	    	if(currentIndex == main.pgHandler.ComponentManagers.length)
	    	//if(currentIndex == 1)
	    	{
	    		main.chooseCMWindow.refreshList();
	    		//main.pgHandler.map.drawMap();
	    		//return;
	    		
	    		startResolveUser();
	    		return;
	    	}
	    	
	    	currentCm = main.pgHandler.ComponentManagers[currentIndex] as ComponentManager;
			currentCm.Status = ComponentManager.INPROGRESS;
			
			opName = "Getting " + currentCm.Hrn + " resources";
			main.setProgress(opName, Common.waitColor);
			main.startWaiting();
			main.console.appendText(opName + "...\n");
			op.reset(Geni.discoverResources);
			op.addField("credential", main.pgHandler.CurrentUser.credential);
			op.addField("compress", true);
			op.setExactUrl(currentCm.DiscoverResourcesUrl());
			op.call(completeResourceLookup, failResourceLookup);
	    }
	    
	    public function failResourceLookup(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.setProgress("Done", Common.failColor);
	    	main.stopWaiting();
	    	outputFailure(event, fault);
	    	var msg:String = event.toString();
	    	if(msg.search("#2048") > -1)
	        	currentCm.Message = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + currentCm.DiscoverResourcesUrl();
			else if(msg.search("#2032") > -1)
				currentCm.Message = "IO error, possibly due to the server being down";
			else if(msg.search("timed"))
				currentCm.Message = event.text;
			else
				currentCm.Message = msg;
			currentCm.Status = ComponentManager.FAILED;
			currentIndex++;
			main.chooseCMWindow.ResetStatus(currentCm);
			startResourceLookup();
	    }
	
	    public function completeResourceLookup(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	    	main.console.appendText("Resource lookup complete...\n");

	      if (code == 0)
	      {
	      	
      		var decodor:Base64Decoder = new Base64Decoder();
	      	decodor.decode(response.value);
	      	var bytes:ByteArray = decodor.toByteArray();
	      	bytes.uncompress();
	      	var decodedRspec:String = bytes.toString();
	      	
	      	//var decodedRspec:String = response.value;
	        currentCm.Rspec = new XML(decodedRspec);
	      	currentIndex++;
	      	currentCm.processRspec(startResourceLookup);
	      }
	      else
	      {
	        codeFailure();
	        currentCm.Status = ComponentManager.FAILED;
	        main.console.appendText(op.getResponseXml());
			main.chooseCMWindow.ResetStatus(currentCm);
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
	      //op.addField("uuid", main.pgHandler.CurrentUser.uuid);
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
	      	
	      	/*/ HACK ----------------------------
	      	currentCm = main.pgHandler.ComponentManagers[0];
	      	var ba:ByteArray = new DemoSliceEmbeddedXml() as ByteArray;
	      	var demoSlice : Slice = new Slice();
	      	demoSlice.creator = main.pgHandler.CurrentUser;
	      	demoSlice.hrn = "DEMO";
	      	demoSlice.uuid = "DEMO";
	      	demoSlice.urn = "DEMO";
	      	demoSlice.status = Slice.READY;
	      	main.pgHandler.CurrentUser.slices.addItem(demoSlice);

			var demoSliver : Sliver = new Sliver(demoSlice);
			demoSliver.componentManager = currentCm;
			demoSliver.status = Sliver.READY;
			demoSliver.rspec = new XML(ba.readUTFBytes(ba.length));
			demoSliver.parseRspec();
			demoSlice.slivers.addItem(demoSliver);
			
	      	main.pgHandler.map.drawAll();
	      	return;
	      	// END DEMO ---------------------------- /*/
	      	
	      	/* START DEMO
	      	currentSlice = new Slice();
	      	currentSlice.hrn = "gec6";
	      	
	      	var ba:ByteArray = new DelegatedCredentialXml() as ByteArray;
	      	currentSlice.credential = ba.readUTFBytes(ba.length);
	      	main.pgHandler.CurrentUser.slices.addItem(currentSlice);
	      	currentCm = main.pgHandler.ComponentManagers[0];
	      	startIndexedCall(startSliceStatus);
	      	return;
	    	
			  //opName = "Acquiring " + (((main.pgHandler.CurrentUser.slices.length - currentIndex) * main.pgHandler.ComponentManagers.length) + (currentSecondaryIndex - main.pgHandler.ComponentManagers.length)) + " more sliver credential(s)";
			  main.setProgress(opName, Common.waitColor);
			  main.startWaiting();
			  main.console.appendText(opName);
			  op.reset(Geni.getSliver);
			  op.addField("credential", currentSlice.credential);
			  op.setExactUrl(currentCm.Url);
			  op.call(completeGetSliver, failure);
			  addSend();
	      	return;
	      	END DEMO */
	      	
	      	var sliceHrns:Array = response.value.slices;
	      	if(sliceHrns != null && sliceHrns.length > 0) {
	      		for each(var sliceHrn:String in sliceHrns)
	      		{
	      			var userSlice:Slice = new Slice();
	      			userSlice.hrn = sliceHrn;
	      			main.pgHandler.CurrentUser.slices.addItem(userSlice);
	      		}
	      		main.fillCombobox();
	      		startIndexedCall(startSliceLookup);
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
	    	if(currentIndex >= main.pgHandler.CurrentUser.slices.length)
	    	{
	    		totalCalls = 0;
	    		startIndexedCall(startSliceCredential);
	    		return;
	    	}
	    	currentSlice = main.pgHandler.CurrentUser.slices[currentIndex] as Slice;
	      opName = "Looking up " + (main.pgHandler.CurrentUser.slices.length - currentIndex) + " more slice(s)";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.resolve);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("hrn", currentSlice.hrn);
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
	      	currentSlice.uuid = response.value.uuid;
	      	currentSlice.creator = main.pgHandler.CurrentUser;
	      	currentSlice.hrn = response.value.hrn;
	      	currentSlice.urn = response.value.urn;
	      	currentIndex++;
			
			startSliceLookup();
	      }
	      else
	      {
	        codeFailure();
	        main.pgHandler.map.drawAll();
	      }
	    }
	    
	    public function startSliceCredential() : void
	    {
	    	if(currentIndex == main.pgHandler.CurrentUser.slices.length)
	    	{
	    		//
	    		if(main.pgHandler.CurrentUser.hrn.search("mstrum") > -1)
	    		{
		    		currentSlice = new Slice();
		      		currentSlice.hrn = "gec6";
		    		currentSlice.creator = main.pgHandler.CurrentUser;
		    		currentSlice.status = "Ready";
		    		var ba:ByteArray = new DelegatedCredentialXml() as ByteArray;
			      	currentSlice.credential = ba.readUTFBytes(ba.length);
			      	main.pgHandler.CurrentUser.slices.addItem(currentSlice);
			      	totalCalls++;
			     }
		      	//
		      	
	    		for each(var s:Slice in main.pgHandler.CurrentUser.slices)
	    		{
	    			if(s.credential.length > 0)
	    				totalCalls++;
	    		}
	    		if(totalCalls > 0)
	    			startIndexedCall(startSliceStatus);
	    		else
	    			main.pgHandler.map.drawAll();
	    		return;
	    	}
	    	currentSlice = main.pgHandler.CurrentUser.slices[currentIndex] as Slice;
	      opName = "Acquiring " + (main.pgHandler.CurrentUser.slices.length - currentIndex) + " more slice credential(s)";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.getCredential);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("uuid", currentSlice.uuid);
	      op.addField("type", "Slice");
	      op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
		  op.call(completeSliceCredential, failSliceCredential);
	      addSend();
	    }
	    
	    public function failSliceCredential(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.setProgress("Done", Common.failColor);
	    	main.stopWaiting();
	    	outputFailure(event, fault);
	      	currentIndex++;
	      	startSliceCredential();
	    }
	    
	    public function completeSliceCredential(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	currentSlice.credential = String(response.value);
	      }
	      else
	      {
	        //codeFailure();
	        //main.pgHandler.map.drawAll();
	      }
	      	currentIndex++;
	      	startSliceCredential();
	    }
	    
	    public function startSliceStatus() : void
	    {
	    	 while(currentIndex < main.pgHandler.CurrentUser.slices.length
				  	&& (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0)
		  	{
			  	currentIndex++;
			  	currentSecondaryIndex = 0;
		  	}
				  	
	    	if(currentIndex >= main.pgHandler.CurrentUser.slices.length)
	    	{
	    		if(nextTotalCalls > 0)
	    		{
	    			totalCalls = nextTotalCalls;
	    			startIndexedCall(startGetSliver);
	    		}
	    		else
	    			main.pgHandler.map.drawAll();
	    		return;
	    	}

	    	currentSlice = main.pgHandler.CurrentUser.slices[currentIndex] as Slice;
	    	currentCm = main.pgHandler.ComponentManagers[currentSecondaryIndex] as ComponentManager;
	    	
	      opName = "Acquiring " + ((totalCalls * main.pgHandler.ComponentManagers.length) - (main.pgHandler.ComponentManagers.length * currentIndex + currentSecondaryIndex)) + " more slice status report(s)";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.sliceStatus);
	      op.addField("credential", currentSlice.credential);
	      op.setExactUrl(currentCm.Url);
		  op.call(completeSliceStatus, failSliceStatus);
	      addSend();
	    }
	    
	    public function failSliceStatus(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.setProgress("Done", Common.failColor);
	    	main.stopWaiting();
	    	outputFailure(event, fault);
			currentSecondaryIndex++;
			while(currentSecondaryIndex < main.pgHandler.ComponentManagers.length
	    		&& (main.pgHandler.ComponentManagers[currentSecondaryIndex] as ComponentManager).Status != ComponentManager.VALID)
	    		currentSecondaryIndex++;
		  if(currentSecondaryIndex == main.pgHandler.ComponentManagers.length)
		  {
		  	  currentSlice.DetectStatus();
			  currentIndex++;
			  while(currentIndex < main.pgHandler.CurrentUser.slices.length
			  	&& (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0)
			  	currentIndex++;
			  currentSecondaryIndex = 0;
		  }
		  startSliceStatus();
	    }
	    
	    public function completeSliceStatus(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	// Remember the sliver to try to get it and skip unneeded calls
	      	var newSliver:Sliver = new Sliver(currentSlice);
	      	newSliver.componentManager = currentCm;
	      	newSliver.sliceStatus = response.value.status;
	      	currentSlice.slivers.addItem(newSliver);
	      	nextTotalCalls++;
	      }
	      else
	      {
	        //codeFailure();
	        //main.pgHandler.map.drawAll();
	      }
	      
	      currentSecondaryIndex++;
	      while(currentSecondaryIndex < main.pgHandler.ComponentManagers.length
	    		&& (main.pgHandler.ComponentManagers[currentSecondaryIndex] as ComponentManager).Status != ComponentManager.VALID)
	    		currentSecondaryIndex++;
		  if(currentSecondaryIndex == main.pgHandler.ComponentManagers.length)
		  {
		  	  currentSlice.DetectStatus();
			  currentIndex++;
			  while(currentIndex < main.pgHandler.CurrentUser.slices.length
			  	&& (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0)
			  	currentIndex++;
			  currentSecondaryIndex = 0;
		  }
		  startSliceStatus();
	    }
	    
	    public function startGetSliver() : void
	    {
	    	while(currentIndex < main.pgHandler.CurrentUser.slices.length
				  	&& ((main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers.length == 0
				  		|| (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0))
		  	{
			  	currentIndex++;
			  	currentSecondaryIndex = 0;
		  	}
		  	
	    	if(currentIndex == main.pgHandler.CurrentUser.slices.length || totalCalls == 0)
	    	{
	    		if(nextTotalCalls > 0)
	    		{
	    			totalCalls = nextTotalCalls;
	    			startIndexedCall(startSliverStatus);
	    		}
	    		else
	    			main.pgHandler.map.drawAll();
	    		return;
	    	}

	    	currentSlice = main.pgHandler.CurrentUser.slices[currentIndex] as Slice;
	    	currentSliver = currentSlice.slivers[currentSecondaryIndex] as Sliver;
	    	
	      opName = "Acquiring " + (totalCalls - callsMade) + " more sliver credential(s)";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.getSliver);
	      op.addField("credential", currentSlice.credential);
	      op.setExactUrl(currentSliver.componentManager.Url);
		  op.call(completeGetSliver, failGetSliver);
		  callsMade++;
	      addSend();
	    }
	    
	    public function failGetSliver(event : ErrorEvent, fault : MethodFault) : void
	    {
			main.setProgress("Done", Common.failColor);
			main.stopWaiting();
			outputFailure(event, fault);
			currentSecondaryIndex++;
			while(currentSecondaryIndex < currentSlice.slivers.length
	    		&& currentSlice.credential.length == 0)
	    		currentSecondaryIndex++;
		  if(currentSecondaryIndex == currentSlice.slivers.length)
		  {
		  	  currentIndex++;
			  while(currentIndex < main.pgHandler.CurrentUser.slices.length
			  	&& ((main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers.length == 0
				  		|| (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0))
			  	currentIndex++;
			  currentSecondaryIndex = 0;
		  }
			startGetSliver();
   }
    
	    public function completeGetSliver(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	currentSliver.credential = String(response.value); 	
	      	nextTotalCalls++;
	      }
	      else
	      {
	        //codeFailure();
	        //main.pgHandler.map.drawAll();
	      }
	      
	      currentSecondaryIndex++;
	      while(currentSecondaryIndex < currentSlice.slivers.length
	    		&& currentSlice.credential.length == 0)
	    		currentSecondaryIndex++;
		  if(currentSecondaryIndex == currentSlice.slivers.length)
		  {
		  	  currentIndex++;
			  while(currentIndex < main.pgHandler.CurrentUser.slices.length
			  	&& ((main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers.length == 0
				  		|| (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0))
			  	currentIndex++;
			  currentSecondaryIndex = 0;
		  }
		  startGetSliver();
	    }
	    
	    public function startSliverStatus() : void
	    {
	    	while(currentIndex < main.pgHandler.CurrentUser.slices.length
				  	&& ((main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers.length == 0
				  		|| (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0))
		  	{
			  	currentIndex++;
			  	currentSecondaryIndex = 0;
		  	}
	    	while(currentIndex < main.pgHandler.CurrentUser.slices.length
	    		&& currentSecondaryIndex < (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers.length
	    		&& ((main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers[currentSecondaryIndex] as Sliver).credential == 0)
	    		currentSecondaryIndex++;
	    	
		  	
	    	if(currentIndex >= main.pgHandler.CurrentUser.slices.length || totalCalls == 0)
	    	{
	    		if(nextTotalCalls > 0)
	    		{
	    			totalCalls = nextTotalCalls;
	    			startIndexedCall(startSliverTicket);
	    		}
	    		else
	    			main.pgHandler.map.drawAll();
	    		return;
	    	}

	    	currentSlice = main.pgHandler.CurrentUser.slices[currentIndex] as Slice;
	    	currentSliver = currentSlice.slivers[currentSecondaryIndex] as Sliver;
	    	
	      opName = "Acquiring " + (totalCalls - callsMade) + " more sliver status report(s)";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.sliverStatus);
	      op.addField("credential", currentSliver.credential);
	      op.setExactUrl(currentSliver.componentManager.Url);
		  op.call(completeSliverStatus, failSliverStatus);
	      addSend();
	      callsMade++;
	    }
	    
	    public function failSliverStatus(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.setProgress("Done", Common.failColor);
	    	main.stopWaiting();
	    	outputFailure(event, fault);
	      currentSecondaryIndex++;
		    while(currentSecondaryIndex < currentSlice.slivers.length
	    		&& (currentSlice.credential.length == 0
	    			|| (currentSlice.slivers[currentSecondaryIndex] as Sliver).credential == 0))
	    		currentSecondaryIndex++;
		  if(currentSecondaryIndex == currentSlice.slivers.length)
		  {
		  	  currentIndex++;
			  while(currentIndex < main.pgHandler.CurrentUser.slices.length
			  	&& (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0)
			  	currentIndex++;
			  currentSecondaryIndex = 0;
		  }
	      	
			startSliverStatus();
	    }
	    
	    public function completeSliverStatus(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	currentSliver.status = response.value.status;
	      	nextTotalCalls++;
	      }
	      else
	      {
	        //codeFailure();
	        //main.pgHandler.map.drawAll();
	      }
	      
	      currentSecondaryIndex++;
	      while(currentSecondaryIndex < currentSlice.slivers.length
	    		&& (currentSlice.credential.length == 0
	    			|| (currentSlice.slivers[currentSecondaryIndex] as Sliver).credential == 0))
	    		currentSecondaryIndex++;
		  if(currentSecondaryIndex == currentSlice.slivers.length)
		  {
		  	  currentIndex++;
			  while(currentIndex < main.pgHandler.CurrentUser.slices.length
			  	&& (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0)
			  	currentIndex++;
			  currentSecondaryIndex = 0;
		  }
	      	
			startSliverStatus();
	    }
	    
	    public function startSliverTicket() : void
	    {
	    	while(currentIndex < main.pgHandler.CurrentUser.slices.length
				  	&& ((main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers.length == 0
				  		|| (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0))
		  	{
			  	currentIndex++;
			  	currentSecondaryIndex = 0;
		  	}
	    	while(currentIndex < main.pgHandler.CurrentUser.slices.length
	    		&& currentSecondaryIndex < (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers.length
	    		&& ((main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers[currentSecondaryIndex] as Sliver).credential == 0)
	    		currentSecondaryIndex++;

	    	if(currentIndex >= main.pgHandler.CurrentUser.slices.length || totalCalls == 0)
	    	{
	    		main.pgHandler.map.drawAll();
	    		return;
	    	}

	    	currentSlice = main.pgHandler.CurrentUser.slices[currentIndex] as Slice;
	    	currentSliver = currentSlice.slivers[currentSecondaryIndex] as Sliver;
	    	
	      opName = "Acquiring " + (totalCalls - callsMade) + " more sliver ticket(s)";
	      main.setProgress(opName, Common.waitColor);
	      main.startWaiting();
	      main.console.appendText(opName);
	      op.reset(Geni.sliverTicket);
	      op.addField("credential", currentSliver.credential);
	      op.setExactUrl(currentSliver.componentManager.Url);
		  op.call(completeSliverTicket, failSliverTicket);
	      addSend();
	    }
	    
	    public function failSliverTicket(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.setProgress("Done", Common.failColor);
	    	main.stopWaiting();
	    	outputFailure(event, fault);
	    	currentSecondaryIndex++;
		    while(currentSecondaryIndex < currentSlice.slivers.length
	    		&& (currentSlice.credential.length == 0
	    			|| (currentSlice.slivers[currentSecondaryIndex] as Sliver).status.length == 0))
	    		currentSecondaryIndex++;
		  if(currentSecondaryIndex == currentSlice.slivers.length)
		  {
		  	  currentIndex++;
			  while(currentIndex < main.pgHandler.CurrentUser.slices.length
				  	&& ((main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers.length == 0
				  		|| (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0))
			  	currentIndex++;
			  currentSecondaryIndex = 0;
		  }
	      	
			startSliverTicket();
	    }
	    
	    public function completeSliverTicket(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", Common.successColor);
	    	main.stopWaiting();
	      addResponse();
	      if (code == 0)
	      {
	      	currentSliver.rspec = new XML(response.value).descendants("rspec")[0];
	      	currentSliver.parseRspec();
	      }
	      else
	      {
	        //codeFailure();
	        //main.pgHandler.map.drawAll();
	      }
	      currentSecondaryIndex++;
		    while(currentSecondaryIndex < currentSlice.slivers.length
	    		&& (currentSlice.credential.length == 0
	    			|| (currentSlice.slivers[currentSecondaryIndex] as Sliver).credential == 0))
	    		currentSecondaryIndex++;
		  if(currentSecondaryIndex == currentSlice.slivers.length)
		  {
		  	  currentIndex++;
			  while(currentIndex < main.pgHandler.CurrentUser.slices.length
				  	&& ((main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers.length == 0
				  		|| (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0))
			  	currentIndex++;
			  currentSecondaryIndex = 0;
		  }
	      	
			startSliverTicket();
	    }
	}
}