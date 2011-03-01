package protogeni.display.mapping
{
	import com.google.maps.LatLng;
	import com.google.maps.LatLngBounds;
	
	import flash.events.Event;
	import flash.utils.Timer;
	
	import mx.collections.ArrayCollection;
	import mx.core.FlexGlobals;
	
	import protogeni.GeniEvent;
	import protogeni.resources.GeniManager;
	import protogeni.resources.PhysicalLink;
	import protogeni.resources.PhysicalLinkGroup;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.Slice;

	public class GeniMapHandler2
	{
		public var map:GeniMap;
		
		public var markers:Array = [];
		private var links:Vector.<GeniMapLink> = new Vector.<GeniMapLink>();
		private var clusterMarkers:Vector.<GeniMapMarker>;
		private var linksToAdd:Vector.<GeniMapLink> = new Vector.<GeniMapLink>();
		private var markersToAdd:Vector.<GeniMapMarker> = new Vector.<GeniMapMarker>();
		
		public var userResourcesOnly:Boolean = false;
		public var selectedSlice:Slice = null;
		
		public function GeniMapHandler2(newMap:GeniMap)
		{
			map = newMap;
			Main.geniDispatcher.addEventListener(GeniEvent.GENIMANAGER_CHANGED, managerChanged);
		}
		
		public function destruct():void {
			Main.geniDispatcher.removeEventListener(GeniEvent.GENIMANAGER_CHANGED, managerChanged);
		}
		
		// If nothing given, gives bounds for all resources
		public static function getBounds(a:Array = null):LatLngBounds
		{
			var coords:Array;
			if(a == null) {
				coords = new Array();
				for each(var m:GeniMapMarker in Main.geniHandler.mapHandler.markers)
				coords.push(m.getLatLng());
			} else
				coords = a;
			
			if(coords.length == 0)
				return null;
			
			var s:Number = (coords[0] as LatLng).lat();
			var n:Number = (coords[0] as LatLng).lat();
			var w:Number = (coords[0] as LatLng).lng();
			var e:Number = (coords[0] as LatLng).lng();
			for each(var ll:LatLng in coords)
			{
				if(ll.lat() < s)
					s = ll.lat();
				else if(ll.lat() > n)
					n = ll.lat();
				
				if(ll.lng() < w)
					w = ll.lng();
				if(ll.lng() > e)
					e = ll.lng();
			}
			
			return new LatLngBounds(new LatLng(s,w), new LatLng(n,e));
		}
		
		public function zoomToPhysicalNode(n:PhysicalNode, zoom:Boolean = false):void
		{
			map.panTo(new LatLng(n.GetLatitude(), n.GetLongitude()));
		}
		
		// Create markers for managers only once
		public var changingManagers:ArrayCollection = new ArrayCollection();
		public function managerChanged(event:GeniEvent):void {
			if(event.action != GeniEvent.ACTION_POPULATED)
				return;

			changingManagers.addItem(event.changedObject);
			
			if(changingManagers.length == 1) {
				myStateManager = LINK_ADD;
				myIndexManager = 0;
				FlexGlobals.topLevelApplication.stage.addEventListener(Event.ENTER_FRAME, addNext);
			}
		}
		
		private static var LINK_ADD : int = 0;
		private static var NODE_ADD : int = 1;
		private static var CLUSTER : int = 2;
		private static var DONE : int = 3;
		
		private static var MAX_WORK_MANAGER : int = 15;
		
		private var myIndexManager:int;
		private var myStateManager:int;
		
		public function addNext(event:Event):void {
			
			var startTime:Date = new Date();
			
			if(myStateManager == LINK_ADD) {
				addNextLink();
				if(Main.debugMode)
					LogHandler.appendMessage(new LogMessage("", "MapAddL " + String((new Date()).time - startTime.time)));
			} else if(myStateManager == NODE_ADD) {
				addNextNode();
				if(Main.debugMode)
					LogHandler.appendMessage(new LogMessage("", "MapAddN " + String((new Date()).time - startTime.time)));
			} else if(myStateManager == DONE) {
				changingManagers.removeItemAt(0);
				if(this.changingManagers.length == 0) {
					Main.Application().stage.removeEventListener(Event.ENTER_FRAME, addNext);
					drawMap();
				} else {
					myIndexManager = 0;
					myStateManager = LINK_ADD;
				}
			}
		}
		
		public function addNextLink():void {
			var idx:int = 0;
			var gm:GeniManager = changingManagers.getItemAt(0) as GeniManager;
			
			while(myIndexManager < gm.Links.collection.length) {
				var linkGroup:PhysicalLinkGroup = gm.Links.collection.getItemAt(myIndexManager) as PhysicalLinkGroup;
				var add:Boolean = true;
				for each(var v:PhysicalLink in linkGroup.collection)
				{
					if(v.rspec.toXMLString().indexOf("ipv4") > -1)
					{
						add = false;
						break;
					}
				}
				if(add) {
					var gml:GeniMapLink = new GeniMapLink(linkGroup);
					linksToAdd.push(gml);
					//links.push(gml);
				}
				
				idx++
				if(idx == MAX_WORK_MANAGER)
					return;
				myIndexManager++;
			}
			
			myStateManager = NODE_ADD;
			myIndexManager = 0;
			return;
		}
		
		public function addNextNode():void {
			var idx:int = 0;
			var gm:GeniManager = changingManagers.getItemAt(0) as GeniManager;
			
			while(myIndexManager < gm.Nodes.collection.length) {
				var nodeGroup:PhysicalNodeGroup = gm.Nodes.collection.getItemAt(myIndexManager) as PhysicalNodeGroup;
				if(nodeGroup.collection.length > 0) {
					markersToAdd.push(new GeniMapMarker(nodeGroup));
					//markers.push(gmm);
				}
				idx++
				if(idx == MAX_WORK_MANAGER)
					return;
				myIndexManager++;
			}
			
			myStateManager = DONE;
			return;
		}
		
		public function drawAll():void {
			drawMap();
			Main.Application().fillCombobox();
		}
		
		public function drawMap(junk:* = null):void {
			drawMapNow();
		}
		
		private static var MAX_WORK_DRAW : int = 15;
		
		public var myIndexDraw:int;
		public var myStateDraw:int;
		public var drawing:Boolean = false;
		public var drawAfter:Boolean = false;
		
		// Create cluster nodes and show/hide components
		public function drawMapNow():void {
			//Main.log.appendMessage(new LogMessage("", "Drawing map"));
			
			if(!map.ready)
				return;
			
			if(drawing) {
				drawAfter = true;
				return;
			}
			
			drawing = true;
			myStateDraw = LINK_ADD;
			FlexGlobals.topLevelApplication.stage.addEventListener(Event.ENTER_FRAME, drawNext);
		}
		
		public function drawNext(event:Event):void {
			
			var startTime:Date = new Date();
			
			if(myStateDraw == LINK_ADD) {
				drawNextLink();
				if(Main.debugMode)
					LogHandler.appendMessage(new LogMessage("", "MapDrawL " + String((new Date()).time - startTime.time)));
			} else if(myStateDraw == NODE_ADD) {
				drawNextNode();
				if(Main.debugMode)
					LogHandler.appendMessage(new LogMessage("", "MapDrawN " + String((new Date()).time - startTime.time)));
			} else if(myStateDraw == CLUSTER) {
				drawCluster();
				if(Main.debugMode)
					LogHandler.appendMessage(new LogMessage("", "MapDrawC " + String((new Date()).time - startTime.time)));
			} else if(myStateDraw == DONE) {
				Main.Application().stage.removeEventListener(Event.ENTER_FRAME, drawNext);
				drawing = false;
				if(drawAfter) {
					drawAfter = false;
					drawMap();
				}
			}
		}
		
		public function drawNextLink():void {
			var idx:int = 0;
			
			while(linksToAdd.length > 0) {
				var drawLink:GeniMapLink = linksToAdd.pop();
				map.addOverlay(drawLink.polyline);
				map.addOverlay(drawLink.label);
				drawLink.added = true;
				links.push(drawLink);
				idx++;
				if(idx == MAX_WORK_DRAW)
					return;
			}
			
			myStateDraw = NODE_ADD;
			return;
		}
		
		public function drawNextNode():void {
			var idx:int = 0;
			
			while(markersToAdd.length > 0) {
				var drawMarker:GeniMapMarker = markersToAdd.pop();
				map.addOverlay(drawMarker)
				markers.push(drawMarker);
				drawMarker.added = true;
				idx++;
				if(idx == MAX_WORK_DRAW)
					return;
			}

			myStateDraw = DONE;
			myIndexDraw = 0;
			
			// Remove and do calculations
			if(clusterMarkers != null) {
				for each(var removeMarker:GeniMapMarker in clusterMarkers)
				map.removeOverlay(removeMarker);
			}
			clusterMarkers = new Vector.<GeniMapMarker>();
			
			// Cluster node groups close to each other
			var marker:GeniMapMarker;
			var clusterer:Clusterer = new Clusterer(markers, map.getZoom(), 35);
			clusteredMarkers = clusterer.clusters;
			
			/*
			if(userResourcesOnly && selectedSlice != null && selectedSlice.Status() != null) {
				// Draw virtual links
				for each(var sliver:Sliver in selectedSlice.slivers)
				{
					if(!sliver.manager.Show)
						continue;
					for each(var vl:VirtualLink in sliver.links) {
						addVirtualLink(vl);
					}	    			
				}
			}
			*/
		}
		
		private var clusteredMarkers:Array;
		public function drawCluster():void {
			var idx:int = 0;

			while(myIndexDraw < clusteredMarkers.length) {
				var cluster:Array = clusteredMarkers[myIndexDraw];
				if (cluster.length == 1) {
					cluster[0].show();
				} else {
					for each(var hideMarker:GeniMapMarker in cluster)
					hideMarker.hide();
					var marker:GeniMapMarker = new GeniMapMarker(cluster);
					map.addOverlay(marker);
					clusterMarkers.push(marker);
				}
				idx++;
				myIndexDraw++;
				if(idx == MAX_WORK_DRAW)
					return;
			}

			myStateDraw = DONE;
		}
	}
}