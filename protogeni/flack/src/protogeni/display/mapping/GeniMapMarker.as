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
	import com.google.maps.InfoWindowOptions;
	import com.google.maps.LatLng;
	import com.google.maps.MapMouseEvent;
	import com.google.maps.overlays.Marker;
	import com.google.maps.overlays.MarkerOptions;
	import com.google.maps.services.ClientGeocoder;
	import com.google.maps.services.GeocodingEvent;
	import com.google.maps.services.Placemark;
	
	import flash.display.DisplayObject;
	import flash.events.Event;
	import flash.geom.Point;
	
	import mx.collections.ArrayCollection;
	import mx.core.UIComponent;
	import mx.events.FlexEvent;
	
	import protogeni.display.DisplayUtil;
	import protogeni.resources.GeniManager;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.PhysicalNodeGroupCollection;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
	import protogeni.resources.VirtualNode;
	
	public class GeniMapMarker extends Marker
	{
		public function GeniMapMarker(o:*)
		{
			if(o is Vector.<GeniMapMarker>)
				this.cluster = o as Vector.<GeniMapMarker>;
			
			var ll:LatLng;
			// Single
			if(o is PhysicalNodeGroup)
				ll = new LatLng(o.latitude, o.longitude);
				// Cluster marker
			else if(cluster != null)
				ll = cluster[0].getLatLng();
			
			super(ll);
			nodeGroups = new PhysicalNodeGroupCollection(null);
			
			// Single marker
			if(o is PhysicalNodeGroup)
			{
				cluster = new Vector.<GeniMapMarker>();
				cluster.push(this);
				this.nodeGroups.Add(o);
			}
			// Cluster marker
			else if(cluster != null)
			{
				var type:int = cluster[0].nodeGroups.GetType();
				for each(var m:GeniMapMarker in cluster) {
					if(type != m.nodeGroups.GetType())
						type = -1;
					for each(var nodeGroup:PhysicalNodeGroup in m.nodeGroups.collection)
						this.nodeGroups.Add(nodeGroup);
				}
				
				
			}
			
			setDefault();
			addEventListener(MapMouseEvent.CLICK, clicked);
		}
		
		public function clicked(e:Event):void {
			if(showGroups.GetAll().length == nodeGroups.GetAll().length) {
				this.openInfoWindow(
					new InfoWindowOptions({
						customContent:infoWindow,
						customoffset: new Point(0, 10),
						width:infoWindow.width,
						height:infoWindow.height,
						drawDefaultFrame:true
					}));
			} else {
				var newCollection:ArrayCollection = new ArrayCollection();
				for each(var node:PhysicalNode in showGroups.GetAll())
					newCollection.addItem(node);
				DisplayUtil.viewNodeCollection(newCollection);
			}
			
		}

		public function setDefault():void {
			var oldLength:int = showGroups.GetAll().length;
			
			// Set the show groups as the managers which are visible
			showGroups = new PhysicalNodeGroupCollection(null);
			for each(var testGroup:PhysicalNodeGroup in nodeGroups.collection) {
				var newTestGroup:PhysicalNodeGroup = new PhysicalNodeGroup(testGroup.latitude, testGroup.longitude, testGroup.country, showGroups, testGroup);
				for each(var testNode:PhysicalNode in testGroup.collection) {
					if(testNode.manager.Show)
						newTestGroup.Add(testNode);
				}
				if(newTestGroup.collection.length > 0)
					showGroups.Add(newTestGroup);
			}
			
			// Already shown correctly
			if(showGroups.GetAll().length == oldLength)
				return;
			
			if(showGroups.collection.length == 1) {
				this.setOptions(new MarkerOptions({
					icon:new PhysicalNodeGroupMarker(this),
					//iconAllignment:MarkerOptions.ALIGN_RIGHT,
					iconOffset:new Point(-18, -18)
				}));
				
				var groupInfo:PhysicalNodeGroupInfo = new PhysicalNodeGroupInfo();
				groupInfo.addEventListener(FlexEvent.CREATION_COMPLETE,
					function loadNodeGroup(evt:FlexEvent):void {
						groupInfo.Load(showGroups.collection[0]);
						//clusterInfo.setZoomButton(bounds);
					});
				infoWindow = groupInfo;
				
			} else if(showGroups.collection.length > 1) {
				this.setOptions(new MarkerOptions({
					icon:new PhysicalNodeGroupMarker(this),
					//icon:new PhysicalNodeGroupClusterMarker(this.showGroups.GetAll().length.toString(), this, showGroups.GetType()),
					//iconAllignment:MarkerOptions.ALIGN_RIGHT,
					iconOffset:new Point(-20, -20)
				}));
				
				var clusterInfo:PhysicalNodeGroupClusterInfo = new PhysicalNodeGroupClusterInfo();
				clusterInfo.addEventListener(FlexEvent.CREATION_COMPLETE,
					function loadNodeGroup(evt:FlexEvent):void {
						clusterInfo.Load(showGroups.collection);
						//clusterInfo.setZoomButton(bounds);
					});
				infoWindow = clusterInfo;
			}
			
		}
		
		// Either sets all nodes the user has or just from one slice
		public function setUser(slice:Slice = null):void {
			showGroups = new PhysicalNodeGroupCollection(null);
			for each(var group:PhysicalNodeGroup in nodeGroups.collection) {
				var newGroup:PhysicalNodeGroup = new PhysicalNodeGroup(group.latitude, group.longitude, group.country, showGroups, group);
				for each(var node:PhysicalNode in group.collection) {
					if(slice == null) {
						if(node.virtualNodes.length > 0)
							newGroup.Add(node);
					} else {
						for each(var virtualNode:VirtualNode in node.virtualNodes.collection) {
							if(virtualNode.sliver.slice == slice) {
								newGroup.Add(node);
								break;
							}
						}
					}
				}
				if(newGroup.collection.length > 0)
					showGroups.Add(newGroup);
			}

			if(showGroups.collection.length == 1) {
				this.setOptions(new MarkerOptions({
					icon:new PhysicalNodeGroupMarker(this),
					//iconAllignment:MarkerOptions.ALIGN_RIGHT,
					iconOffset:new Point(-18, -18)
				}));
			} else if(showGroups.collection.length > 1) {
				this.setOptions(new MarkerOptions({
					icon:new PhysicalNodeGroupMarker(this),
					//icon:new PhysicalNodeGroupClusterMarker(showGroups.GetAll().length.toString(), this, showGroups.GetType()),
					//iconAllignment:MarkerOptions.ALIGN_RIGHT,
					iconOffset:new Point(-20, -20)
				}));
			}
		}
		
		public var infoWindow:UIComponent;
		public var nodeGroups:PhysicalNodeGroupCollection;
		public var info:DisplayObject;
		public var added:Boolean = false;
		public var cluster:Vector.<GeniMapMarker>;
		public var showGroups:PhysicalNodeGroupCollection = new PhysicalNodeGroupCollection(null);
	}
}