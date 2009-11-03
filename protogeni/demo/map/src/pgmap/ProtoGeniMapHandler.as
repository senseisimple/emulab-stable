package pgmap
{
	import com.google.maps.InfoWindowOptions;
	import com.google.maps.LatLng;
	import com.google.maps.LatLngBounds;
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
	import mx.managers.PopUpManager;
	
    	
	public class ProtoGeniMapHandler
	{
		public var main : pgmap;
		
		
		public function ProtoGeniMapHandler()
		{
		}
		
		private var markers:ArrayCollection;
		private var nodeGroupClusters:ArrayCollection;
		
		private function addMarker(g:NodeGroup):void
	    {
	    	// Create the group to be drawn
	    	var drawGroup:NodeGroup = new NodeGroup(g.latitude, g.longitude, g.country , main.pgHandler.Nodes);
	    	drawGroup.owner = g.owner;
	    	if(main.userResourcesOnly) {
	    		for each(var n:Node in g.collection) {
	    			if( n.slice != null &&
	    			      ((main.selectedSlice == null || main.selectedSlice.status != null) ||
	    			      (n.slice == main.selectedSlice)) )
	    			{
	    				drawGroup.Add(n);
	    			}
	    			
	    			// Figure out links
	    		}
	    	} else {
	    		drawGroup = g;
	    	}
	    	
	    	if(drawGroup.collection.length > 0) {
	    		var m:Marker = new Marker(
			      	new LatLng(drawGroup.latitude, drawGroup.longitude),
			      	new MarkerOptions({
			                  strokeStyle: new StrokeStyle({color: 0x092B9F}),
			                  fillStyle: new FillStyle({color: 0xD2E1F0, alpha: 1}),
			                  radius: 14,
			                  hasShadow: true,
			                  //tooltip: g.country,
			                  label: drawGroup.collection.length.toString()
			      	}));
	
		        var groupInfo:NodeGroupInfo = new NodeGroupInfo();
		        groupInfo.Load(drawGroup, main);
		        
		        if(drawGroup.city.length == 0)
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
					        		drawGroup.city = groupInfo.city;
					        	} catch (err:Error) { }
					        }
					      });
					        	
					  geocoder.addEventListener(GeocodingEvent.GEOCODING_FAILURE,
					        function(event:GeocodingEvent):void {
					        main.console.appendText("******************\n");
					        main.console.appendText("Geocoding failed!\n");
					        main.console.appendText(event.status + "\n"); // 500
					        main.console.appendText(event.eventPhase + "\n"); //2
					        });
		
					  geocoder.reverseGeocode(new LatLng(g.latitude, g.longitude));
		        } else {
		        	groupInfo.city = drawGroup.city;
		        }
		        m.addEventListener(MapMouseEvent.CLICK, function(e:Event):void {
		            m.openInfoWindow(
		            	new InfoWindowOptions({
		            		customContent:groupInfo,
		            		customoffset: new Point(0, 10),
		            		width:140,
		            		height:150,
		            		drawDefaultFrame:true
		            	}));
		        });
	
		  		main.map.addOverlay(m);
		  		markers.addItem({marker:m, nodeGroup:g});
	    	} else {
	    		// Draw an empty marker
	    		var nonodes:Marker = new Marker(
			      	new LatLng(drawGroup.latitude, drawGroup.longitude),
			      	new MarkerOptions({
			                  strokeStyle: new StrokeStyle({color: 0x666666}),
			                  fillStyle: new FillStyle({color: 0xCCCCCC, alpha: .8}),
			                  radius: 8,
			                  hasShadow: false
			      	}));
	
		        main.map.addOverlay(nonodes);
	    	}
	        
	    }
	    
	    private function addNodeGroupCluster(nodeGroups:ArrayCollection):void {
	    	var totalNodes:Number = 0;
	    	var l:LatLngBounds = new LatLngBounds();
	    	var upperLat:Number = nodeGroups[0].nodeGroup.latitude;
	    	var lowerLat:Number = nodeGroups[0].nodeGroup.latitude;
	    	var rightLong:Number = nodeGroups[0].nodeGroup.longitude;
	    	var leftLong:Number = nodeGroups[0].nodeGroup.longitude;
	    	var nodeGroupsOnly:ArrayCollection = new ArrayCollection();
	    	for each(var o:Object in nodeGroups) {
	    		// Check to see if the new group expends the cluster size
	    		if(o.nodeGroup.latitude > upperLat)
	    			upperLat = o.nodeGroup.latitude;
	    		else if(o.nodeGroup.latitude < lowerLat)
	    			lowerLat = o.nodeGroup.latitude;
	    		if(o.nodeGroup.longitude > rightLong)
	    			rightLong = o.nodeGroup.longitude;
	    		else if(o.nodeGroup.longitude < leftLong)
	    			leftLong = o.nodeGroup.longitude;

	    		totalNodes += o.nodeGroup.collection.length;
	    		nodeGroupsOnly.addItem(o.nodeGroup);
	    		o.marker.visible = false;
	    	}
	    	
	    	// Save the bounds of the cluster
	    	var bounds:LatLngBounds = new LatLngBounds(new LatLng(upperLat, leftLong), new LatLng(lowerLat, rightLong));
	    	
	    	var clusterInfo:NodeGroupClusterInfo = new NodeGroupClusterInfo();
	    	clusterInfo.addEventListener(FlexEvent.CREATION_COMPLETE,
	    		function loadNodeGroup(evt:FlexEvent):void {
	    			clusterInfo.Load(nodeGroupsOnly);
		    		clusterInfo.setZoomButton(bounds);
	    		});
		    

	    	var m:Marker = new Marker(
		      	bounds.getCenter(),
		      	new MarkerOptions({
		      			  icon:new iconLabelSprite(totalNodes.toString()),
		      			  iconAllignment:MarkerOptions.ALIGN_RIGHT,
		      			  iconOffset:new Point(-2, -2),
		                  hasShadow: true
		      	}));
		    
		    m.addEventListener(MapMouseEvent.CLICK, function(e:Event):void {
		            m.openInfoWindow(
		            	new InfoWindowOptions({
		            		customContent:clusterInfo,
		            		customoffset: new Point(0, 10),
		            		width:150,
		            		height:150,
		            		drawDefaultFrame:true
		            	}));
		        });
				  	
	  		main.map.addOverlay(m);
	    }
	    
	    public function addLink(lg:LinkGroup):void {
	    	// Create the group to be drawn
	    	var drawGroup:LinkGroup = lg;
	    	
	    	if(drawGroup.collection.length > 0 && !main.userResourcesOnly) {
	    		// Add line
				var polyline:Polyline = new Polyline([
					new LatLng(drawGroup.latitude1, drawGroup.longitude1),
					new LatLng(drawGroup.latitude2, drawGroup.longitude2)
					], new PolylineOptions({ strokeStyle: new StrokeStyle({
						color: 0xFF00FF,
						thickness: 4,
						alpha:1})
					}));
	
				main.map.addOverlay(polyline);
				
				// Add link marker
				var ll:LatLng = new LatLng((drawGroup.latitude1 + drawGroup.latitude2)/2, (drawGroup.longitude1 + drawGroup.longitude2)/2);
				
				var t:TooltipOverlay = new TooltipOverlay(ll, Common.kbsToString(drawGroup.TotalBandwidth()));
		  		t.addEventListener(MouseEvent.CLICK, function(e:Event):void {
		            e.stopImmediatePropagation();
		            main.pgHandler.map.viewLinkGroup(drawGroup)
		        });
		        
		  		main.map.addOverlay(t);
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
	
				main.map.addOverlay(blankline);
	    	}
	    }
	    
	    public function addPointLink(pl:PointLink):void {
	    	// Create the group to be drawn
	    	if(pl.slice != main.selectedSlice ||
	    		pl.node1.owner == pl.node2.owner )
	    		return;
	    	
    		// Add line
    		var c:Object = 0x66FF00;
    		if(pl.type != "tunnel")
    			c = 0xFF00FF;
			var polyline:Polyline = new Polyline([
				new LatLng(pl.node1.GetLatitude(), pl.node1.GetLongitude()),
				new LatLng(pl.node2.GetLatitude(), pl.node2.GetLongitude())
				], new PolylineOptions({ strokeStyle: new StrokeStyle({
					color: c,
					thickness: 4,
					alpha:1})
				}));

			main.map.addOverlay(polyline);
	    }
	    
	    public function drawAll():void {
	    	drawMap();
	    	main.fillCombobox();
	    }
	    
	    public function drawMap():void {
	    	main.map.closeInfoWindow();
	    	main.map.clearOverlays();
	    	
	    	main.setProgress("Drawing map",Common.waitColor);
	    	
	    	markers = new ArrayCollection();
	    	for each(var g:NodeGroup in main.pgHandler.Nodes.collection) {
	        	addMarker(g);
	        }
	        
	        nodeGroupClusters = new ArrayCollection();
	        var added:ArrayCollection = new ArrayCollection();
	        for each(var o:Object in markers) {
	        	if(!added.contains(o)) {
	        		var overlapping:ArrayCollection = new ArrayCollection();
		        	getOverlapping(o, overlapping);
		        	if(overlapping.length > 0) {
		        		added.addAll(overlapping);
		        		nodeGroupClusters.addItem(overlapping);
		        		addNodeGroupCluster(overlapping);
		        	}
	        	}
	        }
	        
	        var drawSlice:Boolean = main.userResourcesOnly && main.selectedSlice != null && main.selectedSlice.status != null;
	        if(drawSlice) {
	        	for each(var drawGroup:LinkGroup in main.pgHandler.Links.collection) {
	        		// Add line
					var blankline:Polyline = new Polyline([
						new LatLng(drawGroup.latitude1, drawGroup.longitude1),
						new LatLng(drawGroup.latitude2, drawGroup.longitude2)
						], new PolylineOptions({ strokeStyle: new StrokeStyle({
							color: 0x666666,
							thickness: 3,
							alpha:.8})
						}));
		
					main.map.addOverlay(blankline);
	        	}
	        	for each(var pl:PointLink in main.selectedSlice.Links) {
		        	addPointLink(pl);
		        }
	        } else {
	        	for each(var l:LinkGroup in main.pgHandler.Links.collection) {
		        	if(!l.IsSameSite()) {
		        		addLink(l);
		        	}
		        }
	        }
	        
	        main.setProgress("Done", Common.successColor);
	    }
	    
	    public function getOverlapping(o:Object, added:ArrayCollection):void {
	    	var m:Marker = o.marker;
	        var d:DisplayObject = m.foreground;
        	for each(var o2:Object in markers) {
        		if(o2 != o && !added.contains(o2)) {
		        	var m2:Marker = o2.marker;
		        	var d2:DisplayObject = m2.foreground;
	        		if(d.hitTestObject(d2)) {
	        			added.addItem(o2);
	        			getOverlapping(o2, added);
	        		}
        		}
	        }
	    }
	    
	    public function viewNodeGroup(group:NodeGroup):void {
	    	var ngWindow:NodeGroupAdvancedWindow = new NodeGroupAdvancedWindow();
	    	ngWindow.main = main;
	    	PopUpManager.addPopUp(ngWindow, main, false);
       		PopUpManager.centerPopUp(ngWindow);
       		ngWindow.loadGroup(group);
	    }
	    
	    public function viewLinkGroup(group:LinkGroup):void {
	    	var lgWindow:LinkGroupAdvancedWindow = new LinkGroupAdvancedWindow();
	    	lgWindow.main = main;
	    	PopUpManager.addPopUp(lgWindow, main, false);
       		PopUpManager.centerPopUp(lgWindow);
       		lgWindow.loadGroup(group);
       		
	    }
	}
}