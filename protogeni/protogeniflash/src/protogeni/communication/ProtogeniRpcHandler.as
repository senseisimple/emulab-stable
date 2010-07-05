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
 
 package protogeni.communication
{
	import com.mattism.http.xmlrpc.MethodFault;
	
	import flash.events.ErrorEvent;
	import flash.events.SecurityErrorEvent;
	import flash.utils.ByteArray;
	
	import mx.controls.Alert;
	import mx.utils.Base64Decoder;
	
	import protogeni.Util;
	import protogeni.communication.Operation;
	import protogeni.display.DisplayUtil;
	import protogeni.resources.ComponentManager;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
    
    // Handles all the XML-RPC calls
	public class ProtogeniRpcHandler
	{
		public function ProtogeniRpcHandler()
		{
		}
		
		public var queue:RequestQueue = new RequestQueue();
		public var working:Boolean = false;
		public var forceStop:Boolean = false;
		
		// Run everything from the very beginning
		public function startInitiationSequence():void
		{
			pushRequest(new RequestGetCredential());
			pushRequest(new RequestGetKeys());
			/*
			loadComponentManagers??
			queue.push(new RequestUserResolve());
			queue.push(new RequestSliceResolve());
			queue.push(new RequestSliceCredential());
			queue.push(new RequestSliverGet());
			queue.push(new RequestSliverStatus());
			queue.push(new RequestSliverResolve());*/
		}
		
		public function loadComponentManagers():void
		{
			// clear??
			//queue.push(new RequestListComponents());
			//queue.push(new RequestDiscoverResources());
			// all
		}
		
		public function pushRequest(newRequest : Request, forceStart:Boolean = true) : void
		{
			if (newRequest != null)
			{
				queue.push(newRequest);
				if (!working && forceStart)
				{
					start();
				}
			}
		}
		
		public function start() : void
		{
			if (!queue.isEmpty())
			{
				var front:Request = queue.front();
				var op:Operation = front.start();
				op.call(complete, failure);
				Main.log.setStatus(front.name, true, false);
				Main.log.appendMessage(new LogMessage(op.getUrl(), "Send (" + front.name + ")", op.getSendXml(), false, LogMessage.TYPE_START));
				working = true;
			}
			else
			{
				working = false;
			}
			Main.protogeniHandler.dispatchQueueChanged();
		}
		
		private function failure(event : ErrorEvent, fault : MethodFault) : void
		{
			// Get and give general info for the failure
			var failMessage:String = "";
			var msg : String = "";
			if (fault != null)
			{
				msg = fault.getFaultString();
				failMessage += "\nFAILURE fault: " + queue.front().name + ": " + msg;
			}
			else
			{
				msg = event.toString();
				failMessage += "\nFAILURE event: " + queue.front().name + ": " + msg;
				if(msg.search("#2048") > -1)
					failMessage += "\nStream error, possibly due to server error";
				else if(msg.search("#2032") > -1)
					failMessage += "\nIO Error, possibly due to server problems or you have no SSL certificate";
			}
			failMessage += "\nURL: " + queue.front().op.getUrl();
			Main.log.appendMessage(new LogMessage(queue.front().op.getUrl(), "Failure", failMessage, true, LogMessage.TYPE_END));
			Main.log.open();
			Main.log.setStatus(queue.front().name + " failed!", false, true);

			// Find out what to do next
			var next : Request = queue.front().fail(event, fault);
			queue.front().cleanup();
			queue.pop();
			if (next != null)
			{
				queue.push(next);
			}
			
			if(!forceStop)
				start();
			else
				forceStop = false;
		}
		
		private function complete(code : Number, response : Object) : void
		{
			try
			{
				// Output completed
				if(code != CommunicationUtil.GENIRESPONSE_SUCCESS)
				{
					Main.log.setStatus(queue.front().name + " done", false, true);
					Main.log.appendMessage(new LogMessage(queue.front().op.getUrl(), CommunicationUtil.GeniresponseToString(code), queue.front().op.getResponseXml(), true, LogMessage.TYPE_END));
				} else {
					Main.log.setStatus(queue.front().name + " done", false, false);
					Main.log.appendMessage(new LogMessage(queue.front().op.getUrl(), "Response (" + queue.front().name + ")", queue.front().op.getResponseXml(), false, LogMessage.TYPE_END));
				}

				var next : Request = queue.front().complete(code, response);
				if (next != null)
				{
					queue.push(next);
				}
				queue.front().cleanup();
				queue.pop();
				
			}
			catch (e : Error)
			{
				codeFailure(queue.front().name, "Error caught in RPC-Handler Complete", e);
			}
			
			if(!forceStop)
				start();
			else
				forceStop = false;
		}
		
		public function codeFailure(name:String, detail:String = "", e:Error = null) : void
		{
			Main.log.open();
			if(e != null)
				Main.log.appendMessage(new LogMessage("", "Code Failure: " + name,detail + "\n\n" + e.toString() + "\n\n" + e.getStackTrace(),true,LogMessage.TYPE_END));
			else
				Main.log.appendMessage(new LogMessage("", "Code Failure: " + name,detail,true,LogMessage.TYPE_END));
			forceStop = true;
		}
	    
		/*
	    public function startListComponents() : void
	    {
	      opName = "Looking up components";
	      main.setProgress(opName, DisplayUtil.waitColor);
	      main.startWaiting();
	      //main.console.appendText(opName + "...\n");
	      op.reset(CommunicationUtil.listComponents);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      //op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
	      op.call(completeListComponents, failure);
		  addSend();
	    }
	    
	    public function completeListComponents(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", DisplayUtil.successColor);
	    	main.stopWaiting();
	    	//main.console.appendText("List Components complete...\n");
			addResponse(code);
	      if (code == GENIRESPONSE_SUCCESS)
	      {
			for each(var obj:Object in response.value)
			{
				var newCm:ComponentManager = new ComponentManager();
				newCm.Hrn = obj.hrn;
				newCm.Url = obj.url;
				main.console.groups.push(newCm.Url);
				main.console.groupNames.push(newCm.Hrn);
				newCm.Urn = obj.urn;
				main.pgHandler.ComponentManagers.addItem(newCm);
				//break;
			}
	      	postCall();
	      }
	      else
	      {
	        codeFailure();
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
	    	
	    	if(currentIndex >= main.pgHandler.ComponentManagers.length)
	    	{
				main.chooseCMWindow.refreshList();
	    		startResolveUser();

	    		return;
	    	}
	    	
	    	currentCm = main.pgHandler.ComponentManagers[currentIndex] as ComponentManager;
			currentCm.Status = ComponentManager.INPROGRESS;
			
			opName = "Getting " + currentCm.Hrn + " resources";
			main.setProgress(opName, DisplayUtil.waitColor);
			main.startWaiting();
			//main.console.appendText(opName + "...\n");
			op.reset(CommunicationUtil.discoverResources);
			op.addField("credentials", new Array(main.pgHandler.CurrentUser.credential));
			op.addField("compress", true);
			op.setExactUrl(currentCm.DiscoverResourcesUrl());
			op.call(completeResourceLookup, failResourceLookup);
			addSend();
	    }
	    
	    public function failResourceLookup(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.setProgress("Done", DisplayUtil.failColor);
	    	main.stopWaiting();
	    	outputFailure(event, fault);
	    	currentCm.errorMessage = event.toString();
	    	currentCm.errorDescription = "";
	    	if(currentCm.errorMessage.search("#2048") > -1)
	        	currentCm.errorDescription = "Stream error, possibly due to server error.  Another possible error might be that you haven't added an exception for:\n" + currentCm.DiscoverResourcesUrl();
			else if(currentCm.errorMessage.search("#2032") > -1)
				currentCm.errorDescription = "IO error, possibly due to the server being down";
			else if(currentCm.errorMessage.search("timed"))
				currentCm.errorDescription = event.text;
			currentCm.Status = ComponentManager.FAILED;
			currentIndex++;
			main.chooseCMWindow.ResetStatus(currentCm);
			startResourceLookup();
	    }
	
	    public function completeResourceLookup(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", DisplayUtil.successColor);
	    	main.stopWaiting();
	    	//main.console.appendText("Resource lookup complete...\n");
	      addResponse(code);

	      if (code == GENIRESPONSE_SUCCESS)
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
	      } else {
		      	main.setProgress("Done", DisplayUtil.failColor);
		    	main.stopWaiting();
		    	currentCm.errorMessage = response.output;
	    		currentCm.errorDescription = GeniresponseToString(code) + ": " + currentCm.errorMessage;
				currentCm.Status = ComponentManager.FAILED;
				currentIndex++;
				main.chooseCMWindow.ResetStatus(currentCm);
				startResourceLookup();
	      }
	    }
	    
	    public function startResolveUser() : void
	    {
	      main.pgHandler.CurrentUser.slices.removeAll();
	      opName = "Resolving user";
	      main.setProgress(opName, DisplayUtil.waitColor);
	      main.startWaiting();
	      //main.console.appendText(opName + "...\n");
	      op.reset(CommunicationUtil.resolve);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("hrn", main.pgHandler.CurrentUser.urn);
	      op.addField("type", "User");
	      op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
	      op.call(completeResolveUser, failure);
		  addSend();
	    }
	    
	    public function completeResolveUser(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", DisplayUtil.successColor);
	    	main.stopWaiting();
	    	//main.console.appendText("Resolve user complete...\n");
			addResponse(code);
	      if (code == GENIRESPONSE_SUCCESS)
	      {
	      	main.pgHandler.CurrentUser.uid = response.value.uid;
	      	main.pgHandler.CurrentUser.hrn = response.value.hrn;
	      	main.pgHandler.CurrentUser.uuid = response.value.uuid;
	      	main.pgHandler.CurrentUser.email = response.value.email;
	      	main.pgHandler.CurrentUser.name = response.value.name;
	      	
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
	      main.setProgress(opName, DisplayUtil.waitColor);
	      main.startWaiting();
	      //main.console.appendText(opName);
	      op.reset(CommunicationUtil.resolve);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("hrn", currentSlice.hrn);
	      op.addField("type", "Slice");
	      op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
		  op.call(completeSliceLookup, failure);
	      addSend();
	    }
	    
	    public function completeSliceLookup(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", DisplayUtil.successColor);
	    	main.stopWaiting();
	      addResponse(code);
	      if (code == GENIRESPONSE_SUCCESS)
	      {
	      	currentSlice.uuid = response.value.uuid;
	      	currentSlice.creator = main.pgHandler.CurrentUser;
	      	currentSlice.hrn = response.value.hrn;
	      	currentSlice.urn = response.value.urn;
	      	for each(var sliverCm:String in response.value.component_managers) {
	      		var newSliver:Sliver = new Sliver(currentSlice);
		      	newSliver.componentManager = main.pgHandler.getCm(sliverCm);
		      	currentSlice.slivers.addItem(newSliver);
	      	}
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
	    		for each(var s:Slice in main.pgHandler.CurrentUser.slices)
	    		{
	    			if(s.credential.length > 0)
	    				totalCalls++;
	    		}
	    		if(totalCalls > 0) {
	    			totalCalls = 0;
	    			for each(var slice:Slice in main.pgHandler.CurrentUser.slices)
		    		{
		    			totalCalls += slice.slivers.length;
		    		}
		    		startIndexedCall(startGetSliver); // Was SliceStatus at V1
	    		}
	    		else
	    			main.pgHandler.map.drawAll();
	    		return;
	    	}
	    	currentSlice = main.pgHandler.CurrentUser.slices[currentIndex] as Slice;
	      opName = "Acquiring " + (main.pgHandler.CurrentUser.slices.length - currentIndex) + " more slice credential(s)";
	      main.setProgress(opName, DisplayUtil.waitColor);
	      main.startWaiting();
	      //main.console.appendText(opName);
	      op.reset(CommunicationUtil.getCredential);
	      op.addField("credential", main.pgHandler.CurrentUser.credential);
	      op.addField("uuid", currentSlice.uuid);
	      op.addField("type", "Slice");
	      op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
		  op.call(completeSliceCredential, failSliceCredential);
	      addSend();
	    }
	    
	    public function failSliceCredential(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.setProgress("Done", DisplayUtil.failColor);
	    	main.stopWaiting();
	    	outputFailure(event, fault);
	      	currentIndex++;
	      	startSliceCredential();
	    }
	    
	    public function completeSliceCredential(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", DisplayUtil.successColor);
	    	main.stopWaiting();
	      addResponse(code);
	      if (code == GENIRESPONSE_SUCCESS)
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
	    
	    public function startGetSliver() : void
	    {
	    	while(currentIndex < main.pgHandler.CurrentUser.slices.length
				  	&& (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0)
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
	      main.setProgress(opName, DisplayUtil.waitColor);
	      main.startWaiting();
	      //main.console.appendText(opName);
	      op.reset(CommunicationUtil.getSliver);
	      op.addField("slice_urn", currentSlice.urn);
	      op.addField("credentials", new Array(currentSlice.credential));
	      op.setExactUrl(currentSliver.componentManager.Url);
		  op.call(completeGetSliver, failGetSliver);
		  callsMade++;
	      addSend();
	    }
	    
	    public function failGetSliver(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.setProgress("Done", DisplayUtil.failColor);
	    	main.stopWaiting();
	    	outputFailure(event, fault);
			currentSecondaryIndex++;
			while(currentSecondaryIndex < currentSlice.slivers.length
	    		&& (currentSlice.slivers[currentSecondaryIndex] as Sliver).componentManager.Status != ComponentManager.VALID)
	    		currentSecondaryIndex++;
		  if(currentSecondaryIndex == (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers.length)
		  {
		  	  currentIndex++;
			  while(currentIndex < main.pgHandler.CurrentUser.slices.length
			  	&& (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0)
			  	currentIndex++;
			  currentSecondaryIndex = 0;
		  }
		  startGetSliver();
   }
    
	    public function completeGetSliver(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", DisplayUtil.successColor);
	    	main.stopWaiting();
	      addResponse(code);
	      if (code == GENIRESPONSE_SUCCESS)
	      {
	      	currentSliver.credential = String(response.value);
	      	var cred:XML = new XML(response.value);
	      	currentSliver.urn = cred.credential.target_urn;
	      	nextTotalCalls++;
	      }
	      else if(code == GENIRESPONSE_SEARCHFAILED)
	      {
	      	// NO SLICE FOUND HERE
	      }
	      else
	      {
	        //codeFailure();
	        //main.pgHandler.map.drawAll();
	      }
	      
		  currentSecondaryIndex++;
	      while(currentSecondaryIndex < currentSlice.slivers.length
	    		&& (currentSlice.slivers[currentSecondaryIndex] as Sliver).componentManager.Status != ComponentManager.VALID)
	    		currentSecondaryIndex++;
		  if(currentSecondaryIndex == (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).slivers.length)
		  {
		  	  //currentSlice.DetectStatus();
			  currentIndex++;
			  while(currentIndex < main.pgHandler.CurrentUser.slices.length
			  	&& (main.pgHandler.CurrentUser.slices[currentIndex] as Slice).credential.length == 0)
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
	    			startIndexedCall(startResolveSliver);
	    		}
	    		else
	    			main.pgHandler.map.drawAll();
	    		return;
	    	}

	    	currentSlice = main.pgHandler.CurrentUser.slices[currentIndex] as Slice;
	    	currentSliver = currentSlice.slivers[currentSecondaryIndex] as Sliver;
	    	
	      opName = "Acquiring " + (totalCalls - callsMade) + " more sliver status report(s)";
	      main.setProgress(opName, DisplayUtil.waitColor);
	      main.startWaiting();
	      //main.console.appendText(opName);
	      op.reset(CommunicationUtil.sliverStatus);
	      op.addField("slice_urn", currentSlice.urn);
	      op.addField("credentials", new Array(currentSlice.credential));
	      op.setExactUrl(currentSliver.componentManager.Url);
		  op.call(completeSliverStatus, failSliverStatus);
	      addSend();
	      callsMade++;
	    }
	    
	    public function failSliverStatus(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.setProgress("Done", DisplayUtil.failColor);
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
	    	main.setProgress("Done", DisplayUtil.successColor);
	    	main.stopWaiting();
	      addResponse(code);
	      if (code == GENIRESPONSE_SUCCESS)
	      {
	      	currentSliver.status = response.value.status;
	      	currentSliver.state = response.value.state;
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
	    
	    public function startResolveSliver() : void
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
	    		main.fillCombobox();
	    		main.pgHandler.map.drawAll();
	    		return;
	    	}

	    	currentSlice = main.pgHandler.CurrentUser.slices[currentIndex] as Slice;
	    	currentSliver = currentSlice.slivers[currentSecondaryIndex] as Sliver;
	    	
	      opName = "Acquiring " + (totalCalls - callsMade) + " more sliver rspec(s)";
	      main.setProgress(opName, DisplayUtil.waitColor);
	      main.startWaiting();
	      //main.console.appendText(opName);
	      op.reset(CommunicationUtil.resolveResource);
	      op.addField("urn", currentSliver.urn);
	      op.addField("credentials", new Array(currentSliver.credential));
	      op.setExactUrl(currentSliver.componentManager.Url);
		  op.call(completeResolveSliver, failResolveSliver);
	      addSend();
	      callsMade++;
	    }
	    
	    public function failResolveSliver(event : ErrorEvent, fault : MethodFault) : void
	    {
	    	main.setProgress("Done", DisplayUtil.failColor);
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
	      	
			startResolveSliver();
	    }
	    
	    public function completeResolveSliver(code : Number, response : Object) : void
	    {
	    	main.setProgress("Done", DisplayUtil.successColor);
	    	main.stopWaiting();
	      addResponse(code);
	      if (code == GENIRESPONSE_SUCCESS)
	      {
	      	//currentSliver.rspec = new XML(response.value).descendants("rspec")[0];
	      	currentSliver.rspec = new XML(response.value.manifest);
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
	      	
			startResolveSliver();
	    }
		
		// CREATE SLICE STUFF
		public function startSlice(name:String) : void
		{
			currentSlice = new Slice();
			currentSlice.hrn = name;
			currentSlice.urn = Util.makeUrn(CommunicationUtil.defaultAuthority, "slice", name);
			currentSlice.creator = main.pgHandler.CurrentUser;
			AfterCall = startSliceResolve;
			startSshLookup();
		}
		
		public function startSliceResolve() : void
		{
			opName = "Resolving slice";
			main.setProgress(opName, DisplayUtil.waitColor);
			main.startWaiting();
			op.reset(CommunicationUtil.resolve);
			op.addField("credential", main.pgHandler.CurrentUser.credential);
			op.addField("hrn", currentSlice.hrn);
			op.addField("type", "Slice");
			//op.setUrl("https://boss.emulab.net:443/protogeni/xmlrpc");
			op.call(completeSliceResolve, failure);
			addSend();
		}
		
		public function completeSliceResolve(code : Number, response : Object) : void
		{
			main.setProgress("Done", DisplayUtil.successColor);
			main.stopWaiting();
			addResponse(code);
			if (code == GENIRESPONSE_SUCCESS)
			{
				Alert.show("Slice '" + currentSlice.hrn + "' already exists");
				main.setProgress("Done", DisplayUtil.failColor);
				main.stopWaiting();
			}
			else
			{
				startSliceDelete();
			}
		}
		
		public function startSliceDelete() : void
		{
			opName = "Deleting existing slice";
			main.setProgress(opName, DisplayUtil.waitColor);
			main.startWaiting();
			op.reset(CommunicationUtil.remove);
			op.addField("credential", main.pgHandler.CurrentUser.credential);
			op.addField("hrn", currentSlice.urn);
			op.addField("type", "Slice");
			op.call(completeSliceDelete, failure);
			addSend();
		}
		
		public function completeSliceDelete(code : Number, response : Object) : void
		{
			main.setProgress("Done", DisplayUtil.successColor);
			main.stopWaiting();
			addResponse(code);
			if (code == GENIRESPONSE_SUCCESS || code == GENIRESPONSE_SEARCHFAILED)
			{
				startSliceCreate();
			}
			else
			{
				codeFailure();
			}
		}
		
		public function startSliceCreate() : void
		{
			opName = "Creating new slice";
			main.setProgress(opName, DisplayUtil.waitColor);
			main.startWaiting();
			op.reset(CommunicationUtil.register);
			op.addField("credential", main.pgHandler.CurrentUser.credential);
			op.addField("hrn", currentSlice.urn);
			op.addField("type", "Slice");
			//      op.addField("userbindings", new Array(user.uuid));
			op.call(completeSliceCreate, failure);
			addSend();
		}
		
		public function completeSliceCreate(code : Number, response : Object) : void
		{
			main.setProgress("Done", DisplayUtil.successColor);
			main.stopWaiting();
			addResponse(code);
			if (code == GENIRESPONSE_SUCCESS)
			{
				currentSlice.credential = String(response.value);
				main.pgHandler.CurrentUser.slices.addItem(currentSlice)
				DisplayUtil.viewSlice(currentSlice);
			}
			else
			{
				codeFailure();
				main.openConsole();
			}
		}
		*/
	}
}