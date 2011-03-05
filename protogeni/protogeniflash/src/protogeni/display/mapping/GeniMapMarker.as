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
	
	import mx.events.FlexEvent;
	
	import protogeni.display.DisplayUtil;
	import protogeni.resources.GeniManager;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.PhysicalNodeGroupCollection;
	
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
			var totalNodes:Number = 0;
			
			// Single marker
			if(o is PhysicalNodeGroup)
			{
				cluster = [o];
				var nodeGroup:PhysicalNodeGroup = o as PhysicalNodeGroup;
				totalNodes = nodeGroup.collection.length;
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
				
				addEventListener(MapMouseEvent.CLICK, function(e:Event):void {
					openInfoWindow(
						new InfoWindowOptions({
							customContent:groupInfo,
							customoffset: new Point(0, 10),
							width:180,
							height:130,
							drawDefaultFrame:true
						}));
				});
				
				this.setOptions(new MarkerOptions({
					icon:new PhysicalNodeGroupMarker(totalNodes.toString(), this, nodeGroup.owner.owner.type),
					//iconAllignment:MarkerOptions.ALIGN_RIGHT,
					iconOffset:new Point(-18, -18)
				}));
				
				nodeGroups.Add(nodeGroup);
				info = groupInfo;
			}
				// Cluster marker
			else if(o is Array)
			{
				cluster = o as Array;
				var type:int = (cluster[0] as GeniMapMarker).nodeGroups.GetType();
				for each(var m:GeniMapMarker in cluster) {
					totalNodes += m.nodeGroups.GetAll().length;
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
				
				addEventListener(MapMouseEvent.CLICK, function(e:Event):void {
					openInfoWindow(
						new InfoWindowOptions({
							customContent:clusterInfo,
							customoffset: new Point(0, 10),
							width:180,
							height:170,
							drawDefaultFrame:true
						}));
				});
				
				this.setOptions(new MarkerOptions({
					icon:new PhysicalNodeGroupClusterMarker(totalNodes.toString(), this, type),
					//iconAllignment:MarkerOptions.ALIGN_RIGHT,
					iconOffset:new Point(-20, -20)
				}));
			}
		}
		
		public var nodeGroups:PhysicalNodeGroupCollection;
		public var info:DisplayObject;
		public var added:Boolean = false;
		public var cluster:Array;
	}
}