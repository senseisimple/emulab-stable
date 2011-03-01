package protogeni.resources
{
	import flash.events.Event;
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Alert;
	
	import protogeni.GeniEvent;
	import protogeni.communication.RequestSitesLocation;
	

	public class PlanetlabRspecProcessor implements RspecProcessorInterface
	{
		public function PlanetlabRspecProcessor(newGm:PlanetlabAggregateManager)
		{
			gm = newGm;
		}
		
		private var gm:PlanetlabAggregateManager;
		
		private static var PARSE : int = 0;
		private static var DONE : int = 1;
		
		private static var MAX_WORK : int = 60;
		
		private var myAfter:Function;
		private var myIndex:int;
		private var myState:int = PARSE;
		private var sites:XMLList;
		private var hasslot:Boolean = false;
		
		public function processResourceRspec(afterCompletion : Function):void {
			gm.clearComponents();
			
			Main.Application().setStatus("Parsing " + gm.Hrn + " RSPEC", false);
			
			if (gm.Rspec.@type != "SFA")
			{
				throw new Error("Unsupported ad rspec type: " + gm.Rspec.@type);
			}
			
			var networkXml:XMLList = gm.Rspec.children();
			gm.networkName = networkXml[0].@name;
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
					LogHandler.appendMessage(new LogMessage(gm.Url, "ParseN " + String((new Date()).time - startTime.time)));
			}
			else if (myState == DONE)
			{
				GeniManager.processing--;
				Main.Application().setStatus("Parsing " + gm.Hrn + " RSPEC Done",false);
				gm.totalNodes = gm.AllNodes.length;
				gm.availableNodes = gm.totalNodes;
				gm.unavailableNodes = 0;
				gm.percentageAvailable = 100;
				gm.Status = GeniManager.STATUS_VALID;
				Main.geniDispatcher.dispatchGeniManagerChanged(gm); // not 'populated' until sites are resolved
				Main.Application().stage.removeEventListener(Event.ENTER_FRAME, parseNext);

				var r:RequestSitesLocation = new RequestSitesLocation(gm);
				r.forceNext = true;
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
			var processedNodes:int = 0;
			if(myIndex == 90)
				myIndex = 90;
			while(processedNodes < MAX_WORK) {
				if(myIndex == sites.length()) {
					myState = DONE;
					return;
				}
				var s:XML = sites[myIndex];
				var site:Site = new Site();
				site.id = s.@id;
				site.name = s.child("name")[0].toString();
				site.hrn = gm.networkName + "." + site.id;
				gm.sites.addItem(site);
				for each(var p:XML in s.child("node")) {
					var node:PhysicalNode = new PhysicalNode(null, gm);
					node.name = p.@id;
					for each(var tx:XML in p.children()) {
						switch(tx.localName()) {
							case "bw_limit":
								node.bw_limitKbps = int(tx.toString());
								switch(tx.@units) {
									case "kbps":
									default:
										break;
								}
								break;
							case "hostname":
								node.hostname = tx.toString();
								break;
							case "urn":
								node.urn = tx.toString();
								break;
							default:
						}
					}
					node.rspec = p.copy();
					node.tag = site;
					node.available = true;
					node.exclusive = false;
					processedNodes++;
					gm.AllNodes.push(node);
					site.nodes.addItem(node);
				}
				myIndex++;
			}
		}
		
		public function processSliverRspec(s:Sliver):void
		{
		}
		
		public function generateSliverRspec(s:Sliver):XML
		{
			var requestRspec:XML = new XML("<?xml version=\"1.0\" encoding=\"UTF-8\"?> "
				+ "<RSpec type=\"SFA\" />");
			var networkXml:XML = new XML("<network name=\""+gm.networkName+"\" />");
			
			var requestSites:ArrayCollection = new ArrayCollection();
			var requestSitesDic:Dictionary = new Dictionary();
			for each(var vn:VirtualNode in s.nodes) {
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