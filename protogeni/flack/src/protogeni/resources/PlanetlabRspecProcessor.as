/* GENIPUBLIC-COPYRIGHT
* Copyright (c) 2008-2011 University of Utah and the Flux Group.
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
	import flash.events.Event;
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Alert;
	
	import protogeni.communication.RequestSitesLocation;
	
	public class PlanetlabRspecProcessor implements RspecProcessorInterface
	{
		private var manager:PlanetlabAggregateManager;
		public function PlanetlabRspecProcessor(newGm:PlanetlabAggregateManager)
		{
			this.manager = newGm;
		}
		
		private static var PARSE : int = 0;
		private static var DONE : int = 1;
		
		private static var MAX_WORK : int = 60;
		
		private var myAfter:Function;
		private var myIndex:int;
		private var myState:int = PARSE;
		private var sites:XMLList;
		private var hasslot:Boolean = false;
		
		public function processResourceRspec(afterCompletion : Function):void {
			manager.clearComponents();
			
			Main.Application().setStatus("Parsing " + manager.Hrn + " RSPEC", false);
			
			if (manager.Rspec.@type != "SFA")
			{
				throw new Error("Unsupported ad rspec type: " + manager.Rspec.@type);
			}
			
			var networkXml:XMLList = manager.Rspec.children();
			manager.networkName = networkXml[0].@name;
			sites = networkXml[0].children();
			var d:int = sites.length();
			
			myAfter = afterCompletion;
			myIndex = 0;
			myState = PARSE;
			
			Main.Application().stage.addEventListener(Event.ENTER_FRAME, parseNext);
		}
		
		private function parseNext(event:Event) : void
		{
			if(!hasslot && GeniManager.processing > GeniManager.maxProcessing)
				return;
			if(!hasslot) {
				hasslot = true;
				GeniManager.processing++;
			}
			
			var startTime:Date = new Date();
			if (myState == PARSE)	    	
			{
				parseNextNode();
				if(Main.debugMode)
					LogHandler.appendMessage(new LogMessage(manager.Url, "ParseN " + String((new Date()).time - startTime.time)));
			}
			else if (myState == DONE)
			{
				GeniManager.processing--;
				Main.Application().setStatus("Parsing " + manager.Hrn + " RSPEC Done",false);
				manager.totalNodes = manager.AllNodes.length;
				manager.availableNodes = manager.totalNodes;
				manager.unavailableNodes = 0;
				manager.percentageAvailable = 100;
				manager.Status = GeniManager.STATUS_VALID;
				Main.geniDispatcher.dispatchGeniManagerChanged(manager); // not 'populated' until sites are resolved
				Main.Application().stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				
				var r:RequestSitesLocation = new RequestSitesLocation(manager);
				r.forceNext = true;
				//var r:RequestResolvePl = new RequestResolvePl(gm);
				//r.forceNext = true;
				Main.geniHandler.requestHandler.pushRequest(r);
				
				myAfter();
			}
			else
			{
				Main.Application().setStatus("Fail",true);
				Main.Application().stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				Alert.show("Problem parsing RSPEC");
				// Throw exception
			}
		}
		
		private function parseNextNode():void {
			var idx:int = 0;
			var startTime:Date = new Date();
			
			while(myIndex < sites.length()) {
				var s:XML = sites[myIndex];
				var site:Site = new Site();
				site.id = s.@id;
				site.name = s.child("name")[0].toString();
				site.hrn = manager.networkName + "." + site.id;
				manager.sites.addItem(site);
				for each(var p:XML in s.child("node")) {
					var node:PhysicalNode = new PhysicalNode(null, manager);
					node.name = p.@id;
					for each(var tx:XML in p.children()) {
						switch(tx.localName()) {
							case "urn":
								node.id = tx.toString();
								break;
							default:
						}
					}
					node.rspec = p.copy();
					node.tag = site;
					node.available = true;
					node.exclusive = false;
					idx++;
					manager.AllNodes.push(node);
					site.nodes.push(node);
				}
				myIndex++;
				if(((new Date()).time - startTime.time) > 40) {
					if(Main.debugMode)
						trace("Links processed:" + idx);
					return;
				}
			}
			
			myState = DONE;
			return;
		}
		
		public function processSliverRspec(s:Sliver):void
		{
		}
		
		public function generateSliverRspec(s:Sliver):XML
		{
			var requestRspec:XML = new XML("<?xml version=\"1.0\" encoding=\"UTF-8\"?><RSpec type=\"SFA\" />");
			var networkXml:XML = new XML("<network name=\""+manager.networkName+"\" />");
			
			var requestSites:ArrayCollection = new ArrayCollection();
			var requestSitesDic:Dictionary = new Dictionary();
			for each(var vn:VirtualNode in s.nodes.collection) {
				var newRequestSite:Object = requestSitesDic[vn.physicalNode.tag.id];
				if(newRequestSite == null) {
					newRequestSite = new Object();
					newRequestSite.nodes = new ArrayCollection();
					newRequestSite.site = vn.physicalNode.tag;
					requestSites.addItem(newRequestSite);
					requestSitesDic[vn.physicalNode.tag.id] = newRequestSite;
				}
				newRequestSite.nodes.addItem(vn);
			}
			for each(var requestSite:Object in requestSites) {
				var siteXml:XML = new XML("<site id=\""+requestSite.site.id+"\"><name>"+requestSite.site.name+"</name></site>");
				for each(var addvn:VirtualNode in requestSite.nodes) {
					var nodeXml:XML = addvn.physicalNode.rspec.copy();
					nodeXml.appendChild(<sliver/>);
					siteXml.appendChild(nodeXml);
				} 
				networkXml.appendChild(siteXml);
			}
			
			requestRspec.appendChild(networkXml);
			return requestRspec;
		}
	}
}