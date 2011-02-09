package protogeni.resources
{
	import flash.events.Event;
	import flash.utils.Dictionary;
	import flash.xml.XMLNode;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Alert;
	

	public class PlanetLabAdParser implements AdParserInterface
	{
		public function PlanetLabAdParser(newGm:PlanetLabAggregateManager)
		{
			gm = newGm;
		}
		
		private var gm:PlanetLabAggregateManager;
		
		private static var PARSE : int = 0;
		private static var DONE : int = 1;
		
		private static var MAX_WORK : int = 60;
		
		private var myAfter:Function;
		private var myIndex:int;
		private var myState:int = PARSE;
		private var sites:XMLList;
		
		public function processRspec(afterCompletion : Function):void {
			Main.log.setStatus("Parsing " + gm.Hrn + " RSPEC", false);
			
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
			if (myState == PARSE)	    	
			{
				parseNextNode();
			}
			else if (myState == DONE)
			{
				Main.log.setStatus("Parsing " + gm.Hrn + " RSPEC Done",false);
				gm.totalNodes = gm.AllNodes.length;
				gm.availableNodes = gm.totalNodes;
				gm.unavailableNodes = 0;
				gm.percentageAvailable = 100;
				gm.Status = GeniManager.VALID;
				Main.geniDispatcher.dispatchGeniManagerChanged(gm);
				Main.Application().stage.removeEventListener(Event.ENTER_FRAME, parseNext);
				myAfter();
			}
			else
			{
				Main.log.setStatus("Fail",true);
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
				gm.sites.addItem(site);
				for each(var p:XML in s.child("node")) {
					var node:PhysicalNode = new PhysicalNode(null);
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
							default:
						}
					}
					node.rspec = p.copy();
					node.manager = gm;
					node.available = true;
					node.exclusive = false;
					processedNodes++;
					gm.AllNodes.addItem(node);
					site.nodes.addItem(node);
				}
				myIndex++;
			}
		}
	}
}