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
		public function GeniMapMarker(o:Object)
		{
			var ll:LatLng;
			// Single
			if(o is PhysicalNodeGroup)
				ll = new LatLng(o.latitude, o.longitude);
				// Cluster marker
			else if(o is Array)
				ll = o[0].getLatLng();
			
			super(ll);
			nodeGroups = new PhysicalNodeGroupCollection(null);
			
			// Single marker
			if(o is PhysicalNodeGroup)
			{
				cluster = [o];
				var nodeGroup:PhysicalNodeGroup = o as PhysicalNodeGroup;
				var groupInfo:PhysicalNodeGroupInfo = new PhysicalNodeGroupInfo();
				groupInfo.Load(nodeGroup);
				
				if(nodeGroup.city.length == 0)
				{
					var geocoder:ClientGeocoder = new ClientGeocoder();
					geocoder.addEventListener(GeocodingEvent.GEOCODING_SUCCESS,
						function(event:GeocodingEvent):void {
							var placemarks:Array = event.response.placemarks;
							if (placemarks.length > 0) {
								try {
									var p:Placemark = event.response.placemarks[0] as Placemark;
									var fullAddress : String = p.address;
									var splitAddress : Array = fullAddress.split(',');
									if(splitAddress.length == 3)
										groupInfo.city = splitAddress[0];
									else 
										if(splitAddress.length == 4)
											groupInfo.city = splitAddress[1];
										else
											groupInfo.city = fullAddress;
									nodeGroup.city = groupInfo.city;
								} catch (err:Error) { }
							}
						});
					
					geocoder.addEventListener(GeocodingEvent.GEOCODING_FAILURE,
						function(event:GeocodingEvent):void {
							//Main.log.appendMessage(
							//	new LogMessage("","Geocoding failed (" + event.status + " / " + event.eventPhase + ")","",true));
						});
					
					geocoder.reverseGeocode(new LatLng(nodeGroup.latitude, nodeGroup.longitude));
				} else {
					groupInfo.city = nodeGroup.city;
				}
	
				infoWindow = groupInfo;
				
				nodeGroups.Add(nodeGroup);
				info = groupInfo;
			}
				// Cluster marker
			else if(o is Array)
			{
				cluster = o as Array;
				var type:int = (cluster[0] as GeniMapMarker).nodeGroups.GetType();
				for each(var m:GeniMapMarker in cluster) {
					if(type != m.nodeGroups.GetType())
						type = -1;
					this.nodeGroups.Add(m.nodeGroups.collection[0]);
				}
				
				var clusterInfo:PhysicalNodeGroupClusterInfo = new PhysicalNodeGroupClusterInfo();
				clusterInfo.addEventListener(FlexEvent.CREATION_COMPLETE,
					function loadNodeGroup(evt:FlexEvent):void {
						clusterInfo.Load(nodeGroups.collection);
						//clusterInfo.setZoomButton(bounds);
					});
				infoWindow = clusterInfo;
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
				DisplayUtil.viewNodeCollection(new ArrayCollection(showGroups.GetAll()));
			}
			
		}

		public function setDefault():void {
			var oldLength:int = showGroups.GetAll().length;
			
			// Set the show groups as the managers which are visible
			showGroups = new PhysicalNodeGroupCollection(null);
			for each(var testGroup:PhysicalNodeGroup in nodeGroups.collection) {
				var newTestGroup:PhysicalNodeGroup = new PhysicalNodeGroup(testGroup.latitude, testGroup.longitude, testGroup.country, showGroups);
				newTestGroup.city = testGroup.city;
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
					icon:new PhysicalNodeGroupMarker(this.showGroups.GetAll().length.toString(), this, showGroups.GetType()),
					//iconAllignment:MarkerOptions.ALIGN_RIGHT,
					iconOffset:new Point(-18, -18)
				}));
			} else if(showGroups.collection.length > 1) {
				this.setOptions(new MarkerOptions({
					icon:new PhysicalNodeGroupClusterMarker(this.showGroups.GetAll().length.toString(), this, showGroups.GetType()),
					//iconAllignment:MarkerOptions.ALIGN_RIGHT,
					iconOffset:new Point(-20, -20)
				}));
			}
			
		}
		
		// Either sets all nodes the user has or just from one slice
		public function setUser(slice:Slice = null):void {
			showGroups = new PhysicalNodeGroupCollection(null);
			for each(var group:PhysicalNodeGroup in nodeGroups.collection) {
				var newGroup:PhysicalNodeGroup = new PhysicalNodeGroup(group.latitude, group.longitude, group.country, showGroups);
				newGroup.city = group.city;
				for each(var node:PhysicalNode in group.collection) {
					if(slice == null) {
						if(node.virtualNodes.length > 0)
							newGroup.Add(node);
					} else {
						for each(var virtualNode:VirtualNode in node.virtualNodes) {
							if(virtualNode.slivers[0].slice == slice) {
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
					icon:new PhysicalNodeGroupMarker(showGroups.GetAll().length.toString(), this, showGroups.GetType()),
					//iconAllignment:MarkerOptions.ALIGN_RIGHT,
					iconOffset:new Point(-18, -18)
				}));
			} else if(showGroups.collection.length > 1) {
				this.setOptions(new MarkerOptions({
					icon:new PhysicalNodeGroupClusterMarker(showGroups.GetAll().length.toString(), this, showGroups.GetType()),
					//iconAllignment:MarkerOptions.ALIGN_RIGHT,
					iconOffset:new Point(-20, -20)
				}));
			}
		}
		
		public var infoWindow:UIComponent;
		public var nodeGroups:PhysicalNodeGroupCollection;
		public var info:DisplayObject;
		public var added:Boolean = false;
		public var cluster:Array;
		public var showGroups:PhysicalNodeGroupCollection = new PhysicalNodeGroupCollection(null);
	}
}