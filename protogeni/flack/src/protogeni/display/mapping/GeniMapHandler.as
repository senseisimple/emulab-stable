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

package protogeni.display.mapping
{
	import com.google.maps.LatLng;
	import com.google.maps.LatLngBounds;
	
	import flash.events.Event;
	
	import mx.core.FlexGlobals;
	
	import protogeni.GeniEvent;
	import protogeni.resources.GeniManager;
	import protogeni.resources.PhysicalLink;
	import protogeni.resources.PhysicalLinkGroup;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.Slice;
	
	/**
	 * Handles everything for drawing GENI resources onto Google Maps
	 * 
	 * @author mstrum
	 * 
	 */
	public final class GeniMapHandler
	{
		public var map:GeniMap;
		
		public var nodeGroupMarkers:Vector.<GeniMapMarker> = new Vector.<GeniMapMarker>();
		private var linkMarkers:Vector.<GeniMapLink> = new Vector.<GeniMapLink>();
		private var nodeGroupClusterMarkers:Vector.<GeniMapMarker> = new Vector.<GeniMapMarker>();
		
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
			if(drawing) {
				drawing = false;
				FlexGlobals.topLevelApplication.stage.removeEventListener(Event.ENTER_FRAME, drawNext);
			}
			map.clearOverlays();
			nodeGroupMarkers = new Vector.<GeniMapMarker>();
			linkMarkers = new Vector.<GeniMapLink>();
			nodeGroupClusterMarkers = new Vector.<GeniMapMarker>();
		}
		
		public function redrawFromScratch():void
		{
			clearAll();
			for each(var gm:GeniManager in Main.geniHandler.GeniManagers) {
				this.changingManagers.push(gm);
			}
			this.drawMapNow();
		}
		
		// If nothing given, gives bounds for all resources
		public static function getBounds(a:Vector.<LatLng> = null):LatLngBounds
		{
			var coords:Vector.<LatLng>;
			if(a == null) {
				coords = new Vector.<LatLng>();
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
			
			if(Main.debugMode)
				trace("MAP...New Manager Added: " + event.changedObject.Hrn);
			this.changingManagers.push(event.changedObject);
			this.drawMap();
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
			myIndex = 0;
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
				var linkGroup:PhysicalLinkGroup = gm.Links.collection[myIndex];
				var add:Boolean = !linkGroup.IsSameSite();
				if(add) {
					for each(var v:PhysicalLink in linkGroup.collection)
					{
						if(v.linkTypes.indexOf("ipv4") > -1) {
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
				var nodeGroup:PhysicalNodeGroup = gm.Nodes.collection[myIndex];
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
			}
			
			changingManagers = changingManagers.slice(1);
			if(changingManagers.length>0) {
				myState = LINK_ADD;
				myIndex = 0;
				return;
			}
			
			myState = CLUSTER;
			myIndex = 0;
		}
		
		private var clustersToAdd:Vector.<Vector.<GeniMapMarker>>;
		public function doCluster():void {
			
			// Limit to user resources if selected
			// TODO: Add/Remove virtual links!
			var marker:GeniMapMarker;
			var slice:Slice = null;
			var link:GeniMapLink;
			if(this.userResourcesOnly) {
				if(this.selectedSlice != null
						&& this.selectedSlice.urn != null
						&& this.selectedSlice.urn.full.length>0)
					slice = selectedSlice;
				for each(marker in this.nodeGroupMarkers)
					marker.setUser(slice);
				for each(marker in this.nodeGroupClusterMarkers)
					marker.setUser(slice);
				for each(link in this.linkMarkers) {
					if(link.polyline.visible) {
						link.polyline.hide();
						link.label.visible = false;
					}
				}
				
				/*
				// Draw virtual links
				for each(var sliver:Sliver in selectedSlice.slivers)
				{
				if(!sliver.manager.Show)
				continue;
				for each(var vl:VirtualLink in sliver.links) {
				addVirtualLink(vl);
				}	    			
				}
				*/
			} else {
				for each(marker in this.nodeGroupMarkers)
					marker.setDefault();
				for each(marker in this.nodeGroupClusterMarkers)
					marker.setDefault();
				for each(link in this.linkMarkers) {
					if(!link.polyline.visible && link.group.GetManager().Show) {
						link.polyline.show()
						link.label.visible = true;
					} else if(link.polyline.visible && !link.group.GetManager().Show) {
						link.polyline.hide()
						link.label.visible = false;
					}
				}
			}
			
			// Cluster node groups close to each other
			var clusterer:Clusterer = new Clusterer(nodeGroupMarkers, map.getZoom(), 40);
			var clusteredMarkers:Vector.<Vector.<GeniMapMarker>> = clusterer.clusters;
			clustersToAdd = new Vector.<Vector.<GeniMapMarker>>();
			
			// Show group markers that should be shown now, hide those that shouldn't
			for each(var newArray:Vector.<GeniMapMarker> in clusteredMarkers) {
				var shouldShow:Boolean = newArray.length == 1;
				if(!shouldShow)
					clustersToAdd.push(newArray);
				for each(var newMarker:GeniMapMarker in newArray) {
					if(!newMarker.visible && shouldShow && newMarker.showGroups.collection.length > 0)
						newMarker.show();
					else if(newMarker.visible &&
							(newMarker.showGroups.collection.length == 0
								|| !shouldShow))
						newMarker.hide();
				}
			}
			
			var i:int;
			var j:int;
			var oldMarker:GeniMapMarker;
			var found:Boolean;
			var newMarkerArray:Vector.<GeniMapMarker>;
			
			// Hide markers no longer used
			for(i = 0; i < nodeGroupClusterMarkers.length; i++) {
				oldMarker = this.nodeGroupClusterMarkers[i] as GeniMapMarker;
				found = false;
				for(j = 0; j < clustersToAdd.length; j++) {
					newMarkerArray = clustersToAdd[j];
					if(newMarkerArray.length == oldMarker.cluster.length) {
						found = true;
						for each(var findMarker:GeniMapMarker in newMarkerArray) {
							if(oldMarker.cluster.indexOf(findMarker) == -1) {
								found = false;
								break;
							}
						}
						if(found) {
							clustersToAdd.splice(j, 1);	// remove markers which will stay from the create array
							break;
						}
					}
				}
				if(!found) {
					if(oldMarker.visible)
						oldMarker.hide();
				} else {
					if(oldMarker.visible && oldMarker.showGroups.collection.length == 0)
						oldMarker.hide();
					else if(!oldMarker.visible && oldMarker.showGroups.collection.length > 0)
						oldMarker.show();
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
				var cluster:Vector.<GeniMapMarker> = clustersToAdd[myIndex];
				var marker:GeniMapMarker = new GeniMapMarker(cluster);
				if(this.userResourcesOnly) {
					var slice:Slice = null;
					if(this.selectedSlice != null
							&& this.selectedSlice.urn != null
							&& this.selectedSlice.urn.full.length>0)
						slice = selectedSlice;
					marker.setUser(slice);
				}
				map.addOverlay(marker);
				if(marker.showGroups.collection.length == 0)
					marker.hide();
				nodeGroupClusterMarkers.push(marker);
				idx++;
				myIndex++;
				if(((new Date()).time - startTime.time) > 60) {
					if(Main.debugMode)
						trace("Clusters added:" + idx);
					return;
				}
			}
			clustersToAdd = null;
			
			myState = DONE;
		}
	}
}