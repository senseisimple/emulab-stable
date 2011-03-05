package protogeni.display.mapping
{
	import com.google.maps.LatLng;
	import com.google.maps.LatLngBounds;
	
	import flash.events.Event;
	import flash.utils.Timer;
	
	import mx.collections.ArrayCollection;
	import mx.core.FlexGlobals;
	
	import protogeni.GeniEvent;
	import protogeni.Util;
	import protogeni.resources.GeniManager;
	import protogeni.resources.PhysicalLink;
	import protogeni.resources.PhysicalLinkGroup;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.Slice;
	import protogeni.resources.VirtualNode;

	public class GeniMapHandler
	{
		public var map:GeniMap;
		
		public var nodeGroupMarkers:Array = [];
		private var linkMarkers:Vector.<GeniMapLink> = new Vector.<GeniMapLink>();
		private var nodeGroupClusterMarkers:ArrayCollection = new ArrayCollection();
		
		public var userResourcesOnly:Boolean = false;
		public var selectedSlice:Slice = null;
		
		public function GeniMapHandler(newMap:GeniMap)
		{
			map = newMap;
			Main.geniDispatcher.addEventListener(GeniEvent.GENIMANAGER_CHANGED, managerChanged);
		}
		
		public function destruct():void {
			clearAll();
			Main.geniDispatcher.removeEventListener(GeniEvent.GENIMANAGER_CHANGED, managerChanged);
		}
		
		public function clearAll():void {
			map.clearOverlays();
			nodeGroupMarkers = [];
			linkMarkers = new Vector.<GeniMapLink>();
			nodeGroupClusterMarkers = new ArrayCollection();
		}
		
		// If nothing given, gives bounds for all resources
		public static function getBounds(a:Array = null):LatLngBounds
		{
			var coords:Array;
			if(a == null) {
				coords = new Array();
				for each(var m:GeniMapMarker in Main.geniHandler.mapHandler.nodeGroupMarkers)
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
		
		// Push any managers to be drawn
		public var changingManagers:Vector.<GeniManager> = new Vector.<GeniManager>();
		public function managerChanged(event:GeniEvent):void {
			if(event.action != GeniEvent.ACTION_POPULATED)
				return;

			this.changingManagers.push(event.changedObject);
			this.drawMap();
		}
		
		public function drawAll():void {
			drawMap();
			Main.Application().fillCombobox();
		}
		
		public function drawMap(junk:* = null):void {
			drawMapNow();
		}
		
		private static var LINK_ADD : int = 0;
		private static var NODE_ADD : int = 1;
		private static var CLUSTER : int = 2;
		private static var CLUSTER_ADD : int = 3;
		private static var DONE : int = 4;
		
		private static var MAX_WORK : int = 10;// 15;
		
		public var myIndex:int;
		public var myState:int;
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
			if(changingManagers.length > 0 )
				myState = LINK_ADD;
			else
				myState = CLUSTER;
			FlexGlobals.topLevelApplication.stage.addEventListener(Event.ENTER_FRAME, drawNext);
		}
		
		public function drawNext(event:Event):void {
			
			var startTime:Date = new Date();
			
			if(myState == LINK_ADD) {
				drawNextLink();
				if(Main.debugMode)
					trace("MapDrawL " + String((new Date()).time - startTime.time));
			} else if(myState == NODE_ADD) {
				drawNextNode();
				if(Main.debugMode)
					trace("MapDrawN " + String((new Date()).time - startTime.time));
			} else if(myState == CLUSTER) {
				doCluster();
				if(Main.debugMode)
					trace("MapDrawC " + String((new Date()).time - startTime.time));
			} else if(myState == CLUSTER_ADD) {
				drawCluster();
				if(Main.debugMode)
					trace("MapDrawCa " + String((new Date()).time - startTime.time));
			} else if(myState == DONE) {
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
			var gm:GeniManager = changingManagers[0];
			
			var startTime:Date = new Date();
			
			while(myIndex < gm.Links.collection.length) {
				var linkGroup:PhysicalLinkGroup = gm.Links.collection.getItemAt(myIndex) as PhysicalLinkGroup;
				var add:Boolean = !linkGroup.IsSameSite();
				if(add) {
					for each(var v:PhysicalLink in linkGroup.collection)
					{
						if(v.rspec.toXMLString().indexOf("ipv4") > -1)
						{
							add = false;
							break;
						}
					}
				}
				
				if(add) {
					var gml:GeniMapLink = new GeniMapLink(linkGroup);
					linkMarkers.push(gml);
					map.addOverlay(gml.polyline);
					map.addOverlay(gml.label);
					gml.added = true;
				}
				
				idx++
				myIndex++;
				if(((new Date()).time - startTime.time) > 60) {
					if(Main.debugMode)
						trace("Links added:" + idx);
					return;
				}
				//if(idx == MAX_WORK)
				//	return;
			}
			
			myState = NODE_ADD;
			myIndex = 0;
			return;
		}
		
		public function drawNextNode():void {
			
			var startTime:Date = new Date();
			
			var idx:int = 0;
			var gm:GeniManager = changingManagers[0];
			
			while(myIndex < gm.Nodes.collection.length) {
				var nodeGroup:PhysicalNodeGroup = gm.Nodes.collection.getItemAt(myIndex) as PhysicalNodeGroup;
				if(nodeGroup.collection.length > 0) {
					var gmm:GeniMapMarker = new GeniMapMarker(nodeGroup);
					map.addOverlay(gmm);
					nodeGroupMarkers.push(gmm);
					gmm.added = true;
				}
				idx++
				myIndex++;
				if(((new Date()).time - startTime.time) > 60) {
					if(Main.debugMode)
						trace("Nodes added:" + idx);
					return;
				}
				//if(idx == MAX_WORK)
				//	return;
			}
			
			changingManagers = changingManagers.slice(1);
			if(changingManagers.length>0) {
				myState = LINK_ADD;
				myIndex = 0;
				return;
			}
			
			myState = CLUSTER;
			myIndex = 0;
			
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
		
		private var clustersToAdd:ArrayCollection;
		public function doCluster():void {
			
			// Cluster node groups close to each other
			var clusterer:Clusterer = new Clusterer(nodeGroupMarkers, map.getZoom(), 40);
			var clusteredMarkers:Array = clusterer.clusters;
			clustersToAdd = new ArrayCollection();
			
			// Show group markers that should be shown now, hide those that shouldn't
			for each(var newArray:Array in clusteredMarkers) {
				var shouldShow:Boolean = newArray.length == 1;
				if(!shouldShow)
					clustersToAdd.addItem(newArray);
				for each(var newMarker:GeniMapMarker in newArray) {
					if(shouldShow && !newMarker.visible)
						newMarker.show();
					else if(!shouldShow && newMarker.visible)
						newMarker.hide();
				}
			}
			
			// Remove markers no longer used
			for(var i:int = 0; i < nodeGroupClusterMarkers.length; i++) {
				var oldMarker:GeniMapMarker = this.nodeGroupClusterMarkers.getItemAt(i) as GeniMapMarker;
				var found:Boolean = false;
				for(var j:int = 0; j < clustersToAdd.length; j++) {
					var newMarkerArray:Array = clustersToAdd.getItemAt(j) as Array;
					if(Util.haveSame(newMarkerArray, oldMarker.cluster)) {
						found = true;
						clustersToAdd.removeItemAt(j);	// remove markers which will stay from the create array
						break;
					}
				}
				if(!found) {
					this.map.removeOverlay(oldMarker);
					this.nodeGroupClusterMarkers.removeItemAt(i);
					i--;
				}
			}
			
			if(clusteredMarkers == null)
				myState = DONE;
			else {
				myIndex = 0;
				myState = CLUSTER_ADD;
			}
		}
		
		
		public function drawCluster():void {
			var idx:int = 0;
			
			var startTime:Date = new Date();

			while(myIndex < clustersToAdd.length) {
				var cluster:Array = clustersToAdd.getItemAt(myIndex) as Array;
				var marker:GeniMapMarker = new GeniMapMarker(cluster);
				map.addOverlay(marker);
				nodeGroupClusterMarkers.addItem(marker);
				idx++;
				myIndex++;
				if(((new Date()).time - startTime.time) > 60) {
					if(Main.debugMode)
						trace("Clusters added:" + idx);
					return;
				}
				//if(idx == MAX_WORK)
				//	return;
			}
			clustersToAdd = null;

			myState = DONE;
		}
	}
}