package protogeni.display
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
	
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.PhysicalNodeGroupCollection;
	
	public class ProtogeniMapMarker extends Marker
	{
		public function ProtogeniMapMarker(o:Object)
		{
			var ll:LatLng;
			// Single
			if(o is PhysicalNodeGroup)
			{
				var g:PhysicalNodeGroup = o as PhysicalNodeGroup;
				ll = new LatLng(g.latitude, g.longitude);
			}
			// Cluster marker
			else if(o is Array)
			{
				var clusters:Array = o as Array;
				ll = (clusters[0] as ProtogeniMapMarker).getLatLng();
			}
			
			super(ll);
			nodeGroups = new PhysicalNodeGroupCollection();
			
			// Single marker
			if(o is PhysicalNodeGroup)
			{
				g = o as PhysicalNodeGroup;
				var groupInfo:PhysicalNodeGroupInfo = new PhysicalNodeGroupInfo();
				groupInfo.Load(g);
				
				if(g.city.length == 0)
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
									g.city = groupInfo.city;
								} catch (err:Error) { }
							}
						});
					
					geocoder.addEventListener(GeocodingEvent.GEOCODING_FAILURE,
						function(event:GeocodingEvent):void {
							//Main.log.appendMessage(
							//	new LogMessage("","Geocoding failed (" + event.status + " / " + event.eventPhase + ")","",true));
						});
					
					geocoder.reverseGeocode(new LatLng(g.latitude, g.longitude));
				} else {
					groupInfo.city = g.city;
				}
				
				addEventListener(MapMouseEvent.CLICK, function(e:Event):void {
					openInfoWindow(
						new InfoWindowOptions({
							customContent:groupInfo,
							customoffset: new Point(0, 10),
							width:125,
							height:160,
							drawDefaultFrame:true
						}));
				});
				
				this.setOptions(new MarkerOptions({
					icon:new PhysicalNodeGroupMarker(g.collection.length.toString(), this),
					//iconAllignment:MarkerOptions.ALIGN_RIGHT,
					iconOffset:new Point(-18, -18)
				}));
				
				nodeGroups.Add(g);
				info = groupInfo;
			}
			// Cluster marker
			else if(o is Array)
			{
				clusters = o as Array;
				var totalNodes:Number = 0;
				for each(var m:ProtogeniMapMarker in clusters) {
					totalNodes += m.nodeGroups.GetAll().length;
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
						icon:new PhysicalNodeGroupClusterMarker(totalNodes.toString(), this),
						//iconAllignment:MarkerOptions.ALIGN_RIGHT,
						iconOffset:new Point(-20, -20)
					}));
			}
		}

		public var nodeGroups:PhysicalNodeGroupCollection;
		public var info:DisplayObject;
	}
}