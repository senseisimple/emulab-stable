package protogeni.display
{
	import com.google.maps.InfoWindowOptions;
	import com.google.maps.LatLng;
	import com.google.maps.LatLngBounds;
	import com.google.maps.Map;
	import com.google.maps.MapMouseEvent;
	import com.google.maps.overlays.Marker;
	import com.google.maps.overlays.MarkerOptions;
	import com.google.maps.overlays.Polyline;
	import com.google.maps.overlays.PolylineOptions;
	import com.google.maps.services.ClientGeocoder;
	import com.google.maps.services.GeocodingEvent;
	import com.google.maps.services.Placemark;
	import com.google.maps.styles.FillStyle;
	import com.google.maps.styles.StrokeStyle;
	
	import flash.display.DisplayObject;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import flash.geom.Point;
	
	import mx.collections.ArrayCollection;
	import mx.events.FlexEvent;
	
	import protogeni.Util;
	import protogeni.resources.ComponentManager;
	import protogeni.resources.PhysicalLink;
	import protogeni.resources.PhysicalLinkGroup;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.Slice;
	import protogeni.resources.Sliver;
	import protogeni.resources.VirtualInterface;
	import protogeni.resources.VirtualLink;
	import protogeni.resources.VirtualNode;
	
    // Handles adding all the ProtoGENI info to the Google Map component
	public class ProtogeniMapHandler
	{
		public var map:ProtogeniMap;
		
		public function ProtogeniMapHandler()
		{
		}
		
		private var markers:Array;
		private var attachedMarkers:Array;
		private var clusterer:Clusterer;
		private var linkLineOverlays:ArrayCollection;
		private var linkLabelOverlays:ArrayCollection;

		private var nodeGroupClusters:ArrayCollection;		
		
		public var userResourcesOnly:Boolean = false;
		public var selectedSlice:Slice = null;
		
		public static function getBounds(a:Array):LatLngBounds
		{
			var s:Number = (a[0] as LatLng).lat();
			var n:Number = (a[0] as LatLng).lat();
			var w:Number = (a[0] as LatLng).lng();
			var e:Number = (a[0] as LatLng).lng();
			for each(var ll:LatLng in a)
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
		
		public function zoomToPhysicalNode(n:PhysicalNode):void
		{
			map.panTo(new LatLng(n.GetLatitude(), n.GetLongitude()));
		}
		
		private function addNodeGroupMarker(g:PhysicalNodeGroup):void
	    {
	    	// Create the group to be drawn
	    	var drawGroup:PhysicalNodeGroup = new PhysicalNodeGroup(g.latitude, g.longitude, g.country, g.owner);
	    	if(userResourcesOnly) {
	    		for each(var n:PhysicalNode in g.collection) {
	    			for each(var vn:VirtualNode in n.virtualNodes)
	    			{
	    				if(vn.slivers[0].slice == selectedSlice)
	    				{
	    					drawGroup.Add(n);
	    					break;
	    				}
	    			}
	    		}
	    	} else {
	    		drawGroup = g;
	    	}
	    	
	    	if(drawGroup.collection.length > 0) {
	    		markers.push(new ProtogeniMapMarker(g));
	    	} else {
	    		// Draw an empty marker
	    		var nonodes:Marker = new Marker(
			      	new LatLng(g.latitude, g.longitude),
			      	new MarkerOptions({
			                  strokeStyle: new StrokeStyle({color: 0x666666}),
			                  fillStyle: new FillStyle({color: 0xCCCCCC, alpha: .8}),
			                  radius: 8,
			                  hasShadow: false
			      	}));
	
		        map.addOverlay(nonodes);
	    	}
	    }
	    
	    public function addPhysicalLink(lg:PhysicalLinkGroup):void {
	    	// Create the group to be drawn
	    	var drawGroup:PhysicalLinkGroup = lg;
			for each(var v:PhysicalLink in drawGroup.collection)
			{
				if(v.rspec.toXMLString().indexOf("ipv4") > -1)
				{
					//Main.log.appendText("Skipped");
					return;
				}
			}
	    	
	    	if(drawGroup.collection.length > 0 && !userResourcesOnly) {
	    		// Add line
				var polyline:Polyline = new Polyline([
					new LatLng(drawGroup.latitude1, drawGroup.longitude1),
					new LatLng(drawGroup.latitude2, drawGroup.longitude2)
					], new PolylineOptions({ strokeStyle: new StrokeStyle({
						color: DisplayUtil.linkBorderColor,
						thickness: 4,
						alpha:1})
					}));
	
				map.addOverlay(polyline);
				linkLineOverlays.addItem(polyline);
				
				// Add link marker
				var ll:LatLng = new LatLng((drawGroup.latitude1 + drawGroup.latitude2)/2, (drawGroup.longitude1 + drawGroup.longitude2)/2);
				
				var t:TooltipOverlay = new TooltipOverlay(ll, Util.kbsToString(drawGroup.TotalBandwidth()), DisplayUtil.linkBorderColor, DisplayUtil.linkColor);
		  		t.addEventListener(MouseEvent.CLICK, function(e:Event):void {
		            e.stopImmediatePropagation();
		            DisplayUtil.viewPhysicalLinkGroup(drawGroup)
		        });
		        
		  		map.addOverlay(t);
				linkLabelOverlays.addItem(t);
	    	} else {
	    		// Add line
				var blankline:Polyline = new Polyline([
					new LatLng(drawGroup.latitude1, drawGroup.longitude1),
					new LatLng(drawGroup.latitude2, drawGroup.longitude2)
					], new PolylineOptions({ strokeStyle: new StrokeStyle({
						color: 0x666666,
						thickness: 3,
						alpha:.8})
					}));

				map.addOverlay(blankline);
	    	}
	    }
	    
	    public function addVirtualLink(pl:VirtualLink):void {
    		// Add line
    		var backColor:Object = DisplayUtil.linkColor;
    		var borderColor:Object = DisplayUtil.linkBorderColor;
    		if(pl.type == "tunnel")
    		{
    			backColor = DisplayUtil.tunnelColor;
    			borderColor = DisplayUtil.tunnelBorderColor;
    		}
    		
    		var current:int = 0;
    		var node1:PhysicalNode = (pl.interfaces[pl.interfaces.length - 1] as VirtualInterface).virtualNode.physicalNode;
    		while(current != pl.interfaces.length - 1)
    		{
    			var node2:PhysicalNode = (pl.interfaces[current] as VirtualInterface).virtualNode.physicalNode;
    			
				if(node1.owner == node2.owner)
				{
					node1 = node2;
					current++;
					if(current == pl.interfaces.length)
    				current = 0;
					continue;
				}
					
    			var firstll:LatLng = new LatLng(node1.GetLatitude(), node1.GetLongitude());
	    		var secondll:LatLng = new LatLng(node2.GetLatitude(), node2.GetLongitude());
				
				var polyline:Polyline = new Polyline([
					firstll,
					secondll
					], new PolylineOptions({ strokeStyle: new StrokeStyle({
						color: borderColor,
						thickness: 4,
						alpha:1})
					}));
	
				map.addOverlay(polyline);
				linkLineOverlays.addItem(polyline);
					
				// Add point link marker
				var ll:LatLng = new LatLng((firstll.lat() + secondll.lat())/2, (firstll.lng() + secondll.lng())/2);
				
				var t:TooltipOverlay = new TooltipOverlay(ll, Util.kbsToString(pl.bandwidth), borderColor, backColor);
		  		t.addEventListener(MouseEvent.CLICK, function(e:Event):void {
		            e.stopImmediatePropagation();
		            DisplayUtil.viewVirtualLink(pl)
		        });

		  		map.addOverlay(t);
				linkLabelOverlays.addItem(t);
				
				node1 = node2;
				current++;
				if(current == pl.interfaces.length)
    				current = 0;
    		}
	    }
	    
	    public function drawAll():void {
	    	drawMap();
	    	Main.Pgmap().fillCombobox();
	    }
	    
	    public function drawMap(junk:* = null):void {
			//Main.log.setStatus("Drawing map", true, false);
	    	
	    	map.closeInfoWindow();
	    	map.clearOverlays();

	    	linkLabelOverlays = new ArrayCollection();
	    	linkLineOverlays = new ArrayCollection();
	        markers = [];
			
	    	// Draw physical components
	    	for each(var cm:ComponentManager in Main.protogeniHandler.ComponentManagers)
	    	{
	    		if(!cm.Show)
	    			continue;
	    		
	    		// Links
		        for each(var l:PhysicalLinkGroup in cm.Links.collection) {
			        	if(!l.IsSameSite()) {
			        		addPhysicalLink(l);
			        	}
			       }
		        
		        // Nodes
		    	for each(var g:PhysicalNodeGroup in cm.Nodes.collection) {
		        	addNodeGroupMarker(g);
		        }
	    	}
	    	
	    	if(userResourcesOnly && selectedSlice != null && selectedSlice.Status() != null) {
	    		// Draw virtual links
	    		for each(var sliver:Sliver in selectedSlice.slivers)
	    		{
	    			if(!sliver.componentManager.Show)
	    				continue;
	    			for each(var vl:VirtualLink in sliver.links) {
			        	addVirtualLink(vl);
			        }	    			
	    		}
	        }

			// Cluster node groups close to each other
			var marker:ProtogeniMapMarker;
			clusterer = new Clusterer(markers, map.getZoom(), 35);
			attachedMarkers = [];
			var clusteredMarkers:Array = clusterer.clusters;
			
			for each (var cluster:Array in clusteredMarkers) {
				if (cluster.length == 1) {
					// there is only a single marker in this cluster
					marker = cluster[0];
				} else {
					marker = new ProtogeniMapMarker(cluster);
				}
				map.addOverlay(marker);
				attachedMarkers.push(marker);
			}
	    }
	}
}